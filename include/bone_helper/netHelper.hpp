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
#include <sys/time.h>
#include <sys/types.h>

#include <mutex>
#include <string>

#include "boost/asio.hpp"
#include "boost/asio/ip/tcp.hpp"
#include "boost/asio/ssl.hpp"
#include "boost/asio/ssl/context.hpp"
#include "boost/asio/ssl/stream.hpp"
#include "nlohmann/json.hpp"

namespace bestsens {
	class netHelper_base {
	public:
		netHelper_base(std::string conn_target, std::string conn_port, bool use_msgpack = false, bool silent = false);
		virtual ~netHelper_base() noexcept = default;

		netHelper_base(const netHelper_base&) = delete;
		// netHelper(netHelper&& src) noexcept;

		auto login(const std::string& user_name, const std::string& password, bool use_hash = true) -> int;

		auto is_logged_in() const -> int;

		auto is_connected() const -> bool;

		auto send(const char * data) -> int;
		auto send(const std::vector<uint8_t>& data) -> int;
		auto send_command(const std::string& command, nlohmann::json& response, const nlohmann::json& payload = {},
						  int api_version = 0) -> int;

		auto getCommandReturnPayload(const std::string& command, const nlohmann::json& payload = {}, int api_version = 0)
			-> nlohmann::json;

		auto set_timeout(int timeout) -> int;

		auto get_mutex() -> std::mutex&;

		static auto sha512(const std::string& input) -> std::string;
		static auto getLastRawPosition(const unsigned char * str) -> unsigned int;
		static auto getLastRawPosition(const char * str) -> unsigned int;

		virtual auto connect() -> int;
		virtual auto disconnect() noexcept -> void;
		virtual auto set_timeout_ms(int timeout_ms) -> int;
		virtual auto send(const std::string& data) -> int;
		virtual auto recv(void * buffer, size_t read_size) -> int;
	protected:
		bool connected{false};
		int timeout{10000};
		struct addrinfo remote{};

		bool is_ipv6{false};

		std::mutex sock_mtx{};

		std::string user_name;
		std::string conn_target;
		std::string conn_port;

		int user_level{0};
		bool use_msgpack{false};
		bool silent{false};

		boost::asio::io_service io_service;
	};

	class netHelper : public netHelper_base {
	public:
		netHelper(std::string conn_target, std::string conn_port, bool use_msgpack = false, bool silent = false);
		~netHelper() noexcept override = default;
		netHelper(const netHelper&) = delete;
		netHelper(netHelper&& src) noexcept = delete;

		auto connect() -> int override;
		auto disconnect() noexcept -> void override;

		auto set_timeout_ms(int timeout_ms) -> int override;

		auto send(const std::string& data) -> int override;
		auto recv(void * buffer, size_t read_size) -> int override;
	private:
		boost::asio::ip::tcp::socket s;
	};

	class netHelperSSL : public netHelper_base {
	public:
		netHelperSSL(std::string conn_target, std::string conn_port, bool use_msgpack = false, bool silent = false);
		~netHelperSSL() noexcept override = default;
		netHelperSSL(const netHelperSSL&) = delete;
		netHelperSSL(netHelperSSL&& src) noexcept = delete;

		auto connect() -> int override;
		auto disconnect() noexcept -> void override;

		auto set_timeout_ms(int timeout_ms) -> int override;

		auto send(const std::string& data) -> int override;
		auto recv(void * buffer, size_t read_size) -> int override;
	private:
		boost::asio::ssl::context ssl_ctx;
		boost::asio::ssl::stream<boost::asio::ip::tcp::socket> s;
	};
} // namespace bestsens

#endif /* NETHELPER_HPP_ */
