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
		netHelper(std::string conn_target, std::string conn_port, bool use_msgpack = false, bool silent = false);
		~netHelper() noexcept;

		auto connect() -> int;
		void disconnect() noexcept;
		auto login(const std::string& user_name, const std::string& password, bool use_hash = 1) -> int;

		auto is_logged_in() const -> int;

		auto is_connected() const -> bool;

		auto send(const std::string& data) const -> int;
		auto send(const char * data) const -> int;
		auto send(const std::vector<uint8_t>& data) const -> int;
		auto send_command(const std::string& command, nlohmann::json& response, const nlohmann::json& payload = {},
						  int api_version = 0) -> int;

		auto set_timeout(const int timeout) -> int;

		auto get_sockfd() const -> int;

		auto get_mutex() -> std::mutex&;

		static auto sha512(const std::string& input) -> std::string;
		static auto getLastRawPosition(const unsigned char * str) -> unsigned int;
		static auto getLastRawPosition(const char * str) -> unsigned int;

		auto recv(void * buffer, size_t read_size) const -> int;
	private:
		int sockfd{-1};
		bool connected{false};
		int timeout{10};
		struct addrinfo remote{};
		struct addrinfo * res{nullptr};

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
