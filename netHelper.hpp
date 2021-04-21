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
#include <mutex>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <openssl/sha.h>

#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

using json = nlohmann::json;

namespace bestsens {
	class netHelper {
	public:
		netHelper(std::string conn_target, std::string conn_port, bool use_msgpack = false) : conn_target(conn_target), conn_port(conn_port), user_level(0), use_msgpack(use_msgpack) {
			std::lock_guard<std::mutex> lock(this->sock_mtx);
			
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
				spdlog::critical("socket");
			}

			this->set_timeout(this->timeout);
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
		int send_command(std::string command, json& response, json payload = {}, int api_version = 0);

		int set_timeout(const int timeout);

		int get_sockfd();

		static std::string sha512(std::string input);
		static unsigned int getLastRawPosition(const unsigned char * str);
		static unsigned int getLastRawPosition(const char * str);

		int recv(void * buffer, size_t read_size);
	private:
		int sockfd;
		int connected = 0;
		int timeout = 10;
		struct addrinfo remote;
		struct addrinfo * res;

		std::mutex sock_mtx;

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

	inline int netHelper::get_sockfd() {
		return this->sockfd;
	}

	inline unsigned int netHelper::getLastRawPosition(const unsigned char * str) {
		unsigned int last_position;

		try {
			last_position = (static_cast<uint8_t>(str[0]) << 24) + (static_cast<uint8_t>(str[1]) << 16) + (static_cast<uint8_t>(str[2]) << 8) + (static_cast<uint8_t>(str[3]));
		} catch(...) {
			last_position = 0;
		}

		return last_position;
	}

	inline unsigned int netHelper::getLastRawPosition(const char * str) {
		return getLastRawPosition(reinterpret_cast<const unsigned char*>(str));
	}

	inline std::string netHelper::sha512(std::string input) {
		unsigned char hash[SHA512_DIGEST_LENGTH];
		char hash_hex[SHA512_DIGEST_LENGTH*2+1];

		char* input_str = const_cast<char*>(input.c_str());

		SHA512(reinterpret_cast<unsigned char*>(input_str), input.length(), hash);

		for(int i=0; i<SHA512_DIGEST_LENGTH; i++) {
			sprintf(hash_hex + i*2, "%02x", hash[i]);
		}

		return std::string(hash_hex);
	}

	inline int netHelper::login(std::string user_name, std::string password, bool use_hash) {
		/*
		 * request token
		 */
		json token_response;

		this->send_command("request_token", token_response, nullptr);

		if(!token_response.at("payload").at("token").is_string()) {
			spdlog::error("token request failed");
			return 0;
		}

		std::string token = token_response.at("payload").at("token").get<std::string>();
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

		try {
			if(login_response.at("payload").at("error").is_string())
				syslog(LOG_ERR, "login failed: %s", login_response.at("payload").at("error").get<std::string>().c_str());
		} catch(...) {}

		this->user_level = login_response.at("payload").at("user_level").get<int>();

		return this->user_level;
	}

	inline int netHelper::is_logged_in() {
		return this->user_level;
	}

	inline int netHelper::set_timeout(const int timeout) {
		this->timeout = timeout;

		struct timeval tv;
		tv.tv_sec = timeout;
		tv.tv_usec = 0;
		return setsockopt(this->sockfd, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char *>(&tv), sizeof tv);
	}

	inline int netHelper::send_command(std::string command, json& response, json payload, int api_version) {
		json temp = {{"command", command}};

		if(payload.is_object())
			temp["payload"] = payload;

        if(api_version > 0)
            temp["api"] = api_version;

        std::lock_guard<std::mutex> lock(this->sock_mtx);

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
					else {
						str.erase(str.begin() + t);
						response = json::from_msgpack(str);
					}

					if(response.empty())
						spdlog::error("Error");
					else
						return 1;
				}
				catch(const json::exception& ia) {
					spdlog::error("{}", ia.what());
					spdlog::error("input string: \"{}\"", str.data());
				}
			}
		} else {
			spdlog::critical("could not receive all data");
			spdlog::critical("input string: \"{}\"", str.data());
			throw std::runtime_error("could not receive all data");
		}

		return 0;
	}

	inline netHelper::~netHelper() {
		this->disconnect();
		freeaddrinfo(this->res);
	}

	inline int netHelper::is_connected() {
		return this->connected;
	}

	inline int netHelper::connect() {
		if(this->connected)
			return 1;

		// Set non-blocking 
		long arg;
		if((arg = fcntl(this->sockfd, F_GETFL, NULL)) < 0)
			throw std::runtime_error("Error fcntl(..., F_GETFL)");

		arg |= O_NONBLOCK; 
		
		if(fcntl(this->sockfd, F_SETFL, arg) < 0)
			throw std::runtime_error("Error fcntl(..., F_GETFL)");

		/*
		 * connect to socket
		 */

		std::lock_guard<std::mutex> lock(this->sock_mtx);

		int res = ::connect(this->sockfd, this->res->ai_addr, this->res->ai_addrlen);
		fd_set myset;
		int valopt;
		struct timeval tv;

		if(res < 0) {
			if (errno == EINPROGRESS) {
				do {
					tv.tv_sec = this->timeout;
					tv.tv_usec = 0;
					FD_ZERO(&myset); 
					FD_SET(this->sockfd, &myset);
					res = select(this->sockfd+1, NULL, &myset, NULL, &tv);

					if(res < 0 && errno != EINTR) {
						spdlog::critical("Error connecting {} - {}", errno, strerror(errno));
						return 1; 
					} else if (res > 0) {
						socklen_t lon = sizeof(int);
						if(getsockopt(this->sockfd, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon) < 0) { 
							spdlog::critical("Error in getsockopt() {} - {}", errno, strerror(errno)); 
							return 1; 
						}

						if(valopt) { 
							spdlog::critical("Error in delayed connection() {} - {}", valopt, strerror(valopt)); 
							return 1;  
						}

						break;
					} else {
						spdlog::critical("Timeout in select() - Cancelling!"); 
						return 1;
					}
				} while(1);
			}
		} else {
			spdlog::critical("error connecting to {}:{}", this->conn_target, this->conn_port);
			return 1;
		}

		// Set to blocking mode again... 
		if((arg = fcntl(this->sockfd, F_GETFL, NULL)) < 0)
			throw std::runtime_error("Error fcntl(..., F_GETFL)");

		arg &= (~O_NONBLOCK); 

		if(fcntl(this->sockfd, F_SETFL, arg) < 0)
			throw std::runtime_error("Error fcntl(..., F_GETFL)");

		// I hope that is all 

		this->connected = 1;

		return 0;
	}

	inline int netHelper::disconnect() {
		if(!this->connected)
			return -1;

		std::lock_guard<std::mutex> lock(this->sock_mtx);

		if(close(this->sockfd))
			throw std::runtime_error("error closing socket");

		this->connected = 0;

		return 0;
	}

	inline int netHelper::send(const char * data) {
		return this->send(std::string(data));
	}

	inline int netHelper::send(const std::vector<uint8_t>& data) {
		std::string s = std::string(data.begin(), data.end());
		return this->send(s);
	}

	inline int netHelper::send(std::string data) {
		if(!this->connected)
			return -1;

		/*
		 * send data untill buffer is empty
		 * or requested data len is reached
		 */
		unsigned int t = 0;
		int error_counter = 0;
		while(t < data.length()) {
			int count = ::send(this->sockfd, data.c_str() + t, data.length() - t, 0);

			if(count <= 0 && error_counter++ > 10) {
				if(error_counter++ > 10) {
					spdlog::critical("error sending data: {} ({})", errno, strerror(errno));
					break;
				}
			} else {
				error_counter = 0;
				t += count;
			}
		}

		return static_cast<int>(t);
	}

	inline int netHelper::recv(void * buffer, size_t read_size) {
		if(!this->connected)
			return -1;

		/*
		 * receive data untill buffer is empty
		 * or requested data len is reached
		 */
		unsigned int t = 0;
		int error_counter = 0;
		while(t < read_size) {
			int count = ::recv(this->sockfd, reinterpret_cast<char *>(buffer) + t, read_size - t, 0);

			if(count <= 0) {
				if(error_counter++ > 10) {
					spdlog::critical("error receiving data: {} ({})", errno, strerror(errno));
					break;
				}
			} else {
				error_counter = 0;
				t += count;
			}
		}

		return static_cast<int>(t);
	}
} // namespace bestsens

#endif /* NETHELPER_HPP_ */
