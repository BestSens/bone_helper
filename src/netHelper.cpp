/*
 * netHelper.cpp
 *
 *  Created on: 14.01.2022
 *      Author: Jan Schöppach
 */

#include "bone_helper/netHelper.hpp"

#include <fcntl.h>
#include <netdb.h>
#include <openssl/sha.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>
#include <exception>
#include <mutex>
#include <stdexcept>
#include <string>

#include "bone_helper/jsonHelper.hpp"
#include "bone_helper/system_helper.hpp"
#include "boost/asio.hpp"
#include "boost/asio/ip/address.hpp"
#include "boost/asio/ssl.hpp"
#include "boost/asio/ssl/context.hpp"
#include "boost/asio/ssl/stream.hpp"
#include "boost/asio/ssl/verify_mode.hpp"
#include "fmt/format.h"
#include "fmt/ranges.h"
#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

namespace bestsens {
	using json = nlohmann::json;
	using boost::asio::ip::tcp;

	netHelper_base::netHelper_base(std::string conn_target, std::string conn_port, bool use_msgpack, bool silent)
		: conn_target(std::move(conn_target)),
		  conn_port(std::move(conn_port)),
		  use_msgpack(use_msgpack),
		  silent(silent) {
		const auto address = boost::asio::ip::address::from_string(this->conn_target);
		this->is_ipv6 = address.is_v6();
		this->set_timeout_ms(this->timeout);
	}

	// netHelper_base::netHelper_base(netHelper_base&& src) noexcept : use_ssl(src.use_ssl) {
	// 	const std::lock_guard<std::mutex> lock_src(src.sock_mtx);

	// 	std::swap(this->sockfd, src.sockfd);
	// 	std::swap(this->connected, src.connected);
	// 	std::swap(this->timeout, src.timeout);
	// 	std::swap(this->remote, src.remote);
	// 	std::swap(this->res, src.res);
	// 	std::swap(this->user_name, src.user_name);
	// 	std::swap(this->conn_target, src.conn_target);
	// 	std::swap(this->conn_port, src.conn_port);
	// 	std::swap(this->user_level, src.user_level);
	// 	std::swap(this->use_msgpack, src.use_msgpack);
	// 	std::swap(this->silent, src.silent);

	// 	std::swap(this->s, src.s);
	// 	std::swap(this->io_service, src.io_service);
	// }

	auto netHelper_base::get_mutex() -> std::mutex& {
		return this->sock_mtx;
	}

	auto netHelper_base::getLastRawPosition(const unsigned char* str) -> unsigned int {
		const auto last_position = [&]() -> unsigned int {
			try {
				return (static_cast<uint8_t>(str[0]) << 24u) + (static_cast<uint8_t>(str[1]) << 16u) +
					   (static_cast<uint8_t>(str[2]) << 8u) + (static_cast<uint8_t>(str[3]));
			} catch (...) {}

			return 0;
		}();

		return last_position;
	}

	auto netHelper_base::getLastRawPosition(const char* str) -> unsigned int {
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
		return getLastRawPosition(reinterpret_cast<const unsigned char*>(str));
	}

	auto netHelper_base::sha512(const std::string& input) -> std::string {
		std::array<unsigned char, SHA512_DIGEST_LENGTH> hash{};

		std::vector<char> hash_hex{};
		hash_hex.reserve(static_cast<long>(SHA512_DIGEST_LENGTH) * 2);

		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
		SHA512(reinterpret_cast<const unsigned char*>(input.c_str()), input.length(), hash.data());

		for (const auto& e : hash) {
			fmt::format_to(std::back_inserter(hash_hex), "{:02x}", e);
		}

		return {hash_hex.begin(), hash_hex.end()};
	}

	auto netHelper_base::login(const std::string& user_name, const std::string& password, bool use_hash) -> int {
		/*
		 * request token
		 */
		json token_response;

		this->send_command("request_token", token_response, nullptr);

		if (!token_response.at("payload").at("token").is_string()) {
			if (!this->silent) spdlog::error("token request failed");
			return 0;
		}

		const auto token = token_response.at("payload").at("token").get<std::string>();
		const auto hashed_password = [&]() -> std::string {
			if (use_hash)
				return password;

			return this->sha512(password);
		}();

		/*
		 * sign token
		 */
		const auto concat = hashed_password + token;
		const auto login_token = this->sha512(concat);

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
			if (login_response.at("payload").at("error").is_string()) {
				if (!this->silent) {
					spdlog::error("login failed: {}", login_response.at("payload").at("error").get<std::string>());
				}
			}
		} catch (...) {}

		this->user_level = login_response.at("payload").at("user_level").get<int>();

		return this->user_level;
	}

	auto netHelper_base::is_logged_in() const -> int {
		return this->user_level;
	}

	/*!
		@brief	sets socket timeout
		@param	timeout: timeout in s
		@return	Returns 0 on success, -1 for errors.
	*/
	[[deprecated]] auto netHelper_base::set_timeout(int timeout) -> int {
		return this->set_timeout_ms(timeout * 1000);
	}

	/*!
		@brief	sets socket timeout
		@param	timeout_ms: timeout in ms
		@return	Returns 0 on success, -1 for errors.
	*/
	auto netHelper_base::set_timeout_ms(int /*timeout_ms*/) -> int {
		return 0;
	}

	auto netHelper_base::send_command(const std::string& command, json& response, const json& payload, int api_version) -> int {
		json temp = {{"command", command}};

		if (payload.is_object()) {
			temp["payload"] = payload;
		}

		if (api_version > 0) {
			temp["api"] = api_version;
		}

		std::lock_guard<std::mutex> lock(this->sock_mtx);

		/*
		 * send data to server
		 */
		if (!this->use_msgpack) {
			this->send(fmt::format("{}\r\n", temp.dump()));
		} else {
			std::vector<uint8_t> v_msgpack = json::to_msgpack(temp);
			v_msgpack.push_back('\r');
			v_msgpack.push_back('\n');

			this->send(v_msgpack);
		}

		/*
		 * receive data length
		 */
		unsigned long data_len = 0;
		std::array<char, 9> len_buffer{};

		if (this->recv(len_buffer.data(), 8) == 8) {
			len_buffer[8] = '\0';
			data_len = strtoul(static_cast<char*>(len_buffer.data()), nullptr, 16);
		}

		/*
		 * receive actual data and parse
		 */
		std::vector<uint8_t> str(data_len+1);

		const auto t = this->recv(str.data(), data_len);

		if (t > 0 && static_cast<unsigned long>(t) == data_len) {
			str[t] = '\0';

			try {
				if (!this->use_msgpack)
					response = json::parse(str);
				else {
					str.erase(str.begin() + t);
					response = json::from_msgpack(str);
				}

				if (response.empty()) {
					if (!this->silent) spdlog::error("Error");
				} else {
					return 1;
				}
			} catch(const json::exception& ia) {
				if (!this->silent) {
					spdlog::error("{}", ia.what());
					spdlog::error("input string: \"{:c}\"", fmt::join(str, ""));
				}
			}
		} else {
			if (!this->silent) {
				spdlog::critical("could not receive all data");
				spdlog::critical("input string: \"{:c}\"", fmt::join(str, ""));
			}

			throw std::runtime_error("could not receive all data");
		}

		return 0;
	}

	auto netHelper_base::getCommandReturnPayload(const std::string& command, const json& payload, int api_version) -> json {
		json j;
		const auto retval = this->send_command(command, j, payload, api_version);

		if (retval == 0) {
			throw std::runtime_error("unknown error");
		}

		if (!is_json_object(j, "payload")) {
			return {};
		}

		auto &return_payload = j.at("payload");

		if (is_json_string(return_payload, "error")) {
			throw std::runtime_error(return_payload.at("error").get<std::string>());
		}

		return return_payload;
	}

	auto netHelper_base::is_connected() const -> bool {
		return this->connected;
	}

	/*!
		@brief	connects socket
		@return	Returns 0 on success, != 0 for errors.
	*/
	auto netHelper_base::connect() -> int {
		return 0;
	}

	void netHelper_base::disconnect() noexcept {}

	auto netHelper_base::send(const char * data) -> int {
		return this->send(std::string(data));
	}

	auto netHelper_base::send(const std::vector<uint8_t>& data) -> int {
		const std::string s(data.begin(), data.end());
		return this->send(s);
	}

	auto netHelper_base::send(const std::string&  /*data*/) -> int {
		return 0;
	}

	auto netHelper_base::recv(void *  /*buffer*/, size_t  /*read_size*/) -> int {
		return 0;
	}

	netHelper::netHelper(std::string conn_target, std::string conn_port, bool use_msgpack, bool silent) 
		: s(tcp::socket(this->io_service)),
		netHelper_base(std::move(conn_target), std::move(conn_port), use_msgpack, silent)
	{}
	
	/*!
		@brief	connects socket
		@return	Returns 0 on success, != 0 for errors.
	*/
	auto netHelper::connect() -> int {
		if (this->connected) {
			return 1;
		}

		tcp::resolver resolver(io_service);
		tcp::resolver::query query(this->is_ipv6 ? tcp::v6() : tcp::v4(), this->conn_target, this->conn_port);
		tcp::resolver::iterator iterator = resolver.resolve(query);

		boost::asio::connect(this->s, iterator);

		this->connected = true;

		return 0;
	}

	void netHelper::disconnect() noexcept {
		if (!this->connected) {
			return;
		}

		std::lock_guard<std::mutex> lock(this->sock_mtx);

		this->s.shutdown(boost::asio::ip::tcp::socket::shutdown_both);

		this->connected = false;
	}

	auto netHelper::send(const std::string& data) -> int {
		if (!this->connected) {
			return -1;
		}

		const auto size = boost::asio::write(this->s, boost::asio::buffer(data, data.size()));
		return static_cast<int>(size);
	}

	auto netHelper::recv(void * buffer, size_t read_size) -> int {
		if (!this->connected) {
			return -1;
		}

		const auto reply_length = boost::asio::read(this->s, boost::asio::buffer(buffer, read_size));
		return static_cast<int>(reply_length);
	}

	/*!
		@brief	sets socket timeout
		@param	timeout_ms: timeout in ms
		@return	Returns 0 on success, -1 for errors.
	*/
	auto netHelper::set_timeout_ms(int timeout_ms) -> int {
		this->timeout = timeout_ms;
		this->s.set_option(boost::asio::detail::socket_option::integer<SOL_SOCKET, SO_RCVTIMEO>{timeout_ms});
		return 0;
	}

	netHelperSSL::netHelperSSL(std::string conn_target, std::string conn_port, bool use_msgpack, bool silent)
		: ssl_ctx(boost::asio::ssl::context(boost::asio::ssl::context::sslv23)),
		  s(boost::asio::ssl::stream<tcp::socket>(this->io_service, this->ssl_ctx)),
		  netHelper_base(std::move(conn_target), std::move(conn_port), use_msgpack, silent) {
		this->s.set_verify_mode(boost::asio::ssl::verify_none);
	}

	/*!
		@brief	sets socket timeout
		@param	timeout_ms: timeout in ms
		@return	Returns 0 on success, -1 for errors.
	*/
	auto netHelperSSL::set_timeout_ms(int timeout_ms) -> int {
		this->timeout = timeout_ms;
		this->s.lowest_layer().set_option(
			boost::asio::detail::socket_option::integer<SOL_SOCKET, SO_RCVTIMEO>{timeout_ms});
		return 0;
	}

	/*!
		@brief	connects socket
		@return	Returns 0 on success, != 0 for errors.
	*/
	auto netHelperSSL::connect() -> int {
		if (this->connected) {
			return 1;
		}

		tcp::resolver resolver(io_service);
		tcp::resolver::query query(this->is_ipv6 ? tcp::v6() : tcp::v4(), this->conn_target, this->conn_port);
		tcp::resolver::iterator iterator = resolver.resolve(query);

		boost::asio::connect(this->s.lowest_layer(), iterator);
		this->s.handshake(boost::asio::ssl::stream<tcp::socket>::client);

		this->connected = true;

		return 0;
	}

	void netHelperSSL::disconnect() noexcept {
		if (!this->connected) {
			return;
		}

		std::lock_guard<std::mutex> lock(this->sock_mtx);

		this->s.lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both);

		this->connected = false;
	}

	auto netHelperSSL::send(const std::string& data) -> int {
		if (!this->connected) {
			return -1;
		}

		const auto size = boost::asio::write(this->s, boost::asio::buffer(data, data.size()));
		return static_cast<int>(size);
	}

	auto netHelperSSL::recv(void * buffer, size_t read_size) -> int {
		if (!this->connected) {
			return -1;
		}

		const auto reply_length = boost::asio::read(this->s, boost::asio::buffer(buffer, read_size));
		return static_cast<int>(reply_length);
	}
}