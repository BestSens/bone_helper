/*
 * netHelper.hpp
 *
 *  Created on: 15.06.2016
 *      Author: Jan Schöppach
 */

#ifndef NETHELPER_HPP_
#define NETHELPER_HPP_

#include <sstream>
#include <cstring>
#include <netdb.h>
#include <syslog.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <openssl/sha.h>

#include "../json/src/json.hpp"

using json = nlohmann::json;

namespace bestsens {
	class netHelper {
	public:
		netHelper(std::string conn_target, std::string conn_port, bool use_msgpack = false) : conn_target(conn_target), conn_port(conn_port), user_level(0), use_msgpack(use_msgpack) {
			/*
			 * socket configuration
			 */
			memset(&(this->remote), 0, sizeof (this->remote));
			this->remote.ai_family = AF_UNSPEC;
			this->remote.ai_socktype = SOCK_STREAM;

			getaddrinfo(this->conn_target.c_str(), this->conn_port.c_str(), &(this->remote), &(this->res));

			/*
			 * open socket
			 */
			if((this->sockfd = socket(this->res->ai_family, this->res->ai_socktype, this->res->ai_protocol)) == -1) {
				syslog(LOG_CRIT, "socket");
			}
		};
		~netHelper();

		int connect();
		int disconnect();
		int login(std::string user_name, std::string password, bool use_hash = 1);

		int is_logged_in();

		int is_connected();

		int send(std::string data);
		int send(const char * data);
		int send(const std::vector<uint8_t>& data);
		int send_command(std::string command, json& response, json payload);

		int set_timeout(const int timeout);

		int get_sockfd();

		static std::string sha512(std::string input);
		static unsigned int getLastRawPosition(const unsigned char * str);

		int recv(void * buffer, size_t read_size);
	private:
		int sockfd;
		int connected = 0;
		struct addrinfo remote;
		struct addrinfo * res;

		std::string user_name;
		std::string conn_target;
		std::string conn_port;

		int user_level;
		bool use_msgpack = false;
	};

	class jsonNetHelper : public netHelper {
	public:
		using netHelper::netHelper;
	};

	int netHelper::get_sockfd() {
		return this->sockfd;
	}

	inline unsigned int netHelper::getLastRawPosition(const unsigned char * str) {
		unsigned int last_position;

		try {
			last_position = ((uint8_t)str[0] << 24) + ((uint8_t)str[1] << 16) + ((uint8_t)str[2] << 8) + ((uint8_t)str[3]);
		} catch(...) {
			last_position = 0;
		}

		return last_position;
	}

	inline std::string netHelper::sha512(std::string input) {
		unsigned char hash[SHA512_DIGEST_LENGTH];
		char hash_hex[SHA512_DIGEST_LENGTH*2+1];

		SHA512((unsigned char*)input.c_str(), input.length(), hash);

		for(int i=0; i<SHA512_DIGEST_LENGTH; i++) {
			sprintf(hash_hex + i*2, "%02x", hash[i]);
		}

		return std::string(hash_hex);
	}

	int netHelper::login(std::string user_name, std::string password, bool use_hash) {
		/*
		 * request token
		 */
		json token_response;

		this->send_command("request_token", token_response, nullptr);

		if(!token_response["payload"]["token"].is_string()) {
			syslog(LOG_ERR, "token request failed");
			return 0;
		}

		std::string token = token_response["payload"]["token"];
		std::string hashed_password;

		if(use_hash)
			hashed_password = password;
		else
			hashed_password = this->sha512(password);

		/*
		 * sign token
		 */
		std::string concat = hashed_password + token;
		std::string login_token = this->sha512(concat);

		/*
		 * do login
		 */
		json payload = {
			{"signed_token", login_token},
			{"username", user_name}
		};
		json login_response;

		this->send_command("auth", login_response, payload);

		if(login_response["payload"]["error"].is_string())
			syslog(LOG_ERR, "login failed: %s", login_response["payload"]["error"].get<std::string>().c_str());

		this->user_level = login_response["payload"]["user_level"];

		return this->user_level;
	}

	int netHelper::is_logged_in() {
		return this->user_level;
	}

	int netHelper::set_timeout(const int timeout) {
		struct timeval tv;
		tv.tv_sec = timeout;
		tv.tv_usec = 0;
		return setsockopt(this->sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
	}

	int netHelper::send_command(std::string command, json& response, json payload = {}) {
		json temp = {{"command", command}};

		if(payload.is_object())
			temp["payload"] = payload;

		/*
		 * send data to server
		 */
		if(!this->use_msgpack) {
			std::stringstream data_stream;
			data_stream << temp << "\r\n";

			this->send(data_stream.str());
		} else {
			std::vector<uint8_t> v_msgpack = json::to_msgpack(temp);
			v_msgpack.push_back('\r');
			v_msgpack.push_back('\n');

			this->send(v_msgpack);
		}

		/*
		 * receive data length
		 */
		int data_len = 0;
		char len_buffer[9];

		if(this->recv(&len_buffer, 8) == 8) {
			len_buffer[8] = '\0';
			data_len = strtoul(len_buffer, NULL, 16);
		}

		/*
		 * receive actual data and parse
		 */
		std::vector<uint8_t> str(data_len+1);

		int t = this->recv(str.data(), data_len);

		if(t > 0 && t == data_len) {
			str[t] = '\0';

			if(response != NULL) {
				try{
					if(!this->use_msgpack)
						response = json::parse(str);
					else
						response = json::from_msgpack(str);

					if(response.empty())
						syslog(LOG_ERR, "Error");
					else
						return 1;
				}
				catch(const std::invalid_argument& ia) {
					syslog(LOG_ERR, "%s", ia.what());
					syslog(LOG_ERR, "input string: \"%s\"", str.data());
				}
			}
		} else {
			syslog(LOG_CRIT, "could not receive all data");
			syslog(LOG_CRIT, "input string: \"%s\"", str.data());
			throw std::runtime_error("could not receive all data");
		}

		return 0;
	}

	netHelper::~netHelper() {
		this->disconnect();
		freeaddrinfo(this->res);
	}

	int netHelper::is_connected() {
		return this->connected;
	}

	int netHelper::connect() {
		if(this->connected)
			return 1;

		/*
		 * connect to socket
		 */
		if(::connect(this->sockfd, this->res->ai_addr, this->res->ai_addrlen) == -1) {
			syslog(LOG_CRIT, "error connecting to %s:%s", this->conn_target.c_str(), this->conn_port.c_str());
			return 1;
		}

		this->connected = 1;

		return 0;
	}

	int netHelper::disconnect() {
		if(!this->connected)
			return -1;

		this->connected = 0;

		return 0;
	}

	int netHelper::send(const char * data) {
		return this->send(std::string(data));
	}

	int netHelper::send(const std::vector<uint8_t>& data) {
		std::string s = std::string(data.begin(), data.end());
		return this->send(s);
	}

	int netHelper::send(std::string data) {
		if(!this->connected)
			return -1;

		/*
		 * send data untill buffer is empty
		 * or requested data len is reached
		 */
		unsigned int t = 0;
		while(t < data.length()) {
			int count = ::send(this->sockfd, (const char*)data.c_str() + t, data.length() - t, 0);

			if(count == 0)
				break;

			t += count;
		}

		return (int)t;
	}

	int netHelper::recv(void * buffer, size_t read_size) {
		if(!this->connected)
			return -1;

		/*
		 * receive data untill buffer is empty
		 * or requested data len is reached
		 */
		unsigned int t = 0;
		while(t < read_size) {
			int count = ::recv(this->sockfd, (char *)buffer + t, read_size - t, 0);

			if(count == 0)
				break;

			t += count;
		}

		return (int)t;
	}
}

#endif /* NETHELPER_HPP_ */
