/*
 * netHelper.hpp
 *
 *  Created on: 15.06.2016
 *      Author: Jan Schöppach
 */

#ifndef NETHELPER_HPP_
#define NETHELPER_HPP_

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <mutex>
#include <string>

#include "nlohmann/json.hpp"

namespace bestsens {
	class netHelper {
	public:
		netHelper(const std::string& conn_target, const std::string& conn_port, bool use_msgpack = false,
				  bool silent = false);
		~netHelper();

		auto connect() -> int;
		auto disconnect() -> int;
		auto login(const std::string& user_name, const std::string& password, bool use_hash = 1) -> int;

		auto is_logged_in() -> int;

		auto is_connected() -> int;

		auto send(const std::string& data) -> int;
		auto send(const char * data) -> int;
		auto send(const std::vector<uint8_t>& data) -> int;
		auto send_command(const std::string& command, nlohmann::json& response, nlohmann::json payload = {},
						  int api_version = 0) -> int;

		auto set_timeout(const int timeout) -> int;

		auto get_sockfd() -> int;

		auto get_mutex() -> std::mutex&;

		static auto sha512(const std::string& input) -> std::string;
		static auto getLastRawPosition(const unsigned char * str) -> unsigned int;
		static auto getLastRawPosition(const char * str) -> unsigned int;

		auto recv(void * buffer, size_t read_size) -> int;
	private:
		int sockfd{-1};
		int connected{0};
		int timeout{10};
		struct addrinfo remote;
		struct addrinfo * res;

		std::mutex sock_mtx;

		std::string user_name;
		std::string conn_target;
		std::string conn_port;

		int user_level{0};
		bool use_msgpack{false};
		bool silent{false};
	};

	class jsonNetHelper : public netHelper {
	public:
		using netHelper::netHelper;
	};
} // namespace bestsens

#endif /* NETHELPER_HPP_ */
