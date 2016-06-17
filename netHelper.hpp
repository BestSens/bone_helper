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
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <syslog.h>

#include "../json/src/json.hpp"

namespace bestsens {
    class netHelper {
    public:
		netHelper(std::string conn_target, std::string conn_port);
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
		int send_command(std::string command, nlohmann::json * response, const nlohmann::json * payload);
	};

	int netHelper::get_sockfd() {
		return this->sockfd;
	}

	int jsonNetHelper::send_command(std::string command, nlohmann::json * response = NULL, const nlohmann::json * payload = NULL) {
		nlohmann::json temp = {{"command", command}};

		if(payload)
			temp["payload"] = *payload;

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
		char len_buffer[8];

		if(this->recv(&len_buffer, 8) == 8) {
			data_len = strtoul(len_buffer, NULL, 16);
		}

		/*
		 * receive actual data and parse
		 */
		char * str = (char*)malloc(data_len+1);
		int t = this->recv(str, data_len);

		if(t > 0) {
			str[t] = '\0';

			if(response) {
				*response = nlohmann::json::parse(str);

				if(response->empty()) {
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
		this->conn_target = conn_target;
		this->conn_port = conn_port;

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
			return -1;

		/*
		 * connect to socket
		 */
		if(::connect(this->sockfd, this->res->ai_addr, this->res->ai_addrlen) == -1) {
			syslog(LOG_CRIT, "error connecting to %s:%s", this->conn_target.c_str(), this->conn_port.c_str());
			return -1;
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
