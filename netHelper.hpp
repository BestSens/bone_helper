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
		netHelper(std::string conn_target, std::string conn_port) : conn_target(conn_target), conn_port(conn_port);
		~netHelper();

		int connect();
		int disconnect();

		int is_connected();

		int send(std::string data);
		int send(const char * data);

		int get_sockfd();

		int recv(void * buffer, size_t read_size);
    private:
		int sockfd;
		int connected = 0;
		struct addrinfo remote;
		struct addrinfo * res;

		std::string conn_target;
		std::string conn_port;
    };

	class jsonNetHelper : public netHelper {
	public:
        using netHelper::netHelper;

        jsonNetHelper(std::string &conn_target, std::string &conn_port) : netHelper(conn_target, conn_port), user_level(0) {}

		int send_command(std::string command, json& response, json payload);


        int login(std::string user_name, std::string hashed_password);
        int is_logged_in();
    private:
        std::string user_name;
        int user_level;
	};

	int netHelper::get_sockfd() {
		return this->sockfd;
	}

    int jsonNetHelper::login(std::string &user_name, std::string &hashed_password) {
        /*
         * request token
         */
        json token_response;

        this->send_command("request_token", token_response, NULL);

        if(!token_response["payload"]["token"].is_string()) {
            syslog(LOG_ERR, "token request failed");
            return 0;
        }

        std::string token = token_response["payload"]["token"];

        /*
         * sign token
         */
        std::string concat = hashed_password + token;

        unsigned char hash[SHA512_DIGEST_LENGTH];
        char login_token[SHA512_DIGEST_LENGTH*2+1] = "";

        SHA512((unsigned char*)concat.c_str(), concat.length(), hash);

        for(int i=0; i<SHA512_DIGEST_LENGTH; i++) {
            sprintf(login_token + i*2, "%02x", hash[i]);
        }

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

    int jsonNetHelper::is_logged_in() {
        return this->user_level;
    }

	int jsonNetHelper::send_command(std::string command, json& response, json payload = {}) {
		json temp = {{"command", command}};

		if(payload.is_object())
			temp["payload"] = payload;

		std::stringstream data_stream;
		data_stream << temp << "\r\n";

		/*
		 * send data to server
		 */
		this->send(data_stream.str());

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
		char * str = (char*)malloc(data_len+1);
		int t = this->recv(str, data_len);

		if(t > 0) {
			str[t] = '\0';

			if(&response != NULL) {
				response = json::parse(str);

				if(response.empty()) {
					syslog(LOG_ERR, "Error");
					free(str);
					return 0;
				}
			}
		}

		free(str);

		return 1;
	}

	netHelper::netHelper(std::string conn_target, std::string conn_port) {
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

	int netHelper::send(std::string data) {
		if(!this->connected)
			return -1;

		return ::send(this->sockfd, data.c_str(), data.length(), 0);
	}

	int netHelper::send(const char * data) {
		return this->send(std::string(data));
	}

	int netHelper::recv(void * buffer, size_t read_size) {
		if(!this->connected)
			return -1;

		return ::recv(this->sockfd, buffer, read_size, 0);
	}
}

#endif /* NETHELPER_HPP_ */
