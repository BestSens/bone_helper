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
#include "boost/asio/read.hpp"
#include "boost/asio/ssl.hpp"
#include "boost/asio/ssl/context.hpp"
#include "boost/asio/ssl/stream.hpp"
#include "boost/asio/ssl/verify_mode.hpp"
#include "boost/date_time/posix_time/posix_time_duration.hpp"
#include "boost/optional.hpp"
#include "fmt/format.h"
#include "fmt/ranges.h"
#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

namespace bestsens {
	using json = nlohmann::json;

	namespace detail {
		using boost::asio::ip::tcp;

		netHelper_base::netHelper_base(std::string conn_target, std::string conn_port, bool use_msgpack, bool silent)
			: conn_target(std::move(conn_target)),
			conn_port(std::move(conn_port)),
			use_msgpack(use_msgpack),
			silent(silent) {
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
		// 	std::swap(this->io_context, src.io_context);
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
			const json payload = {
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
			@param	timeout_ms: timeout in ms
			@return	Returns 0 on success, -1 for errors.
		*/
		auto netHelper_base::set_timeout_ms(unsigned int timeout_ms) -> void {
			this->timeout = timeout_ms;
		}

		auto netHelper_base::send_command(const std::string& command, json& response, const json& payload, int api_version) -> int {
			json temp = {{"command", command}};

			if (payload.is_object()) {
				temp["payload"] = payload;
			}

			if (api_version > 0) {
				temp["api"] = api_version;
			}

			const std::lock_guard<std::mutex> lock(this->sock_mtx);

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

		void netHelper_base::disconnect() {}

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

		netHelperTCP::netHelperTCP(std::string conn_target, std::string conn_port, bool use_msgpack, bool silent) 
			: s(tcp::socket(this->io_context)),
			netHelper_base(std::move(conn_target), std::move(conn_port), use_msgpack, silent)
		{}
		
		/*!
			@brief	connects socket
			@return	Returns 0 on success, != 0 for errors.
		*/
		auto netHelperTCP::connect() -> int {
			if (this->connected) {
				return 1;
			}

			tcp::resolver resolver(io_context);
			const tcp::resolver::query query(this->conn_target, this->conn_port);
			const tcp::resolver::iterator iterator = resolver.resolve(query);

			boost::optional<boost::system::error_code> timer_result;
			boost::asio::deadline_timer timer(this->io_context);
			timer.expires_from_now(boost::posix_time::milliseconds(this->timeout));
			timer.async_wait([&timer_result](const boost::system::error_code& error) { timer_result.reset(error); });

			boost::optional<boost::system::error_code> result;
			boost::asio::async_connect(this->s, iterator,
									[this, &result](const boost::system::error_code& error,
													const tcp::resolver::iterator& /*endpoint*/) { result.reset(error); });

			this->io_context.reset();
			while (this->io_context.run_one() != 0u) {
				if (result) {
					timer.cancel();
				} else if (timer_result) {
					this->s.cancel();
				}
			}

			if (*result) {
				throw boost::system::system_error(*result);
			}

			this->connected = true;

			return 0;
		}

		void netHelperTCP::disconnect() {
			if (!this->connected) {
				return;
			}

			const std::lock_guard<std::mutex> lock(this->sock_mtx);

			this->s.shutdown(boost::asio::ip::tcp::socket::shutdown_both);

			this->connected = false;
		}

		auto netHelperTCP::send(const std::string& data) -> int {
			if (!this->connected) {
				return -1;
			}

			boost::optional<boost::system::error_code> timer_result;
			boost::asio::deadline_timer timer(this->io_context);
			timer.expires_from_now(boost::posix_time::milliseconds(this->timeout));
			timer.async_wait([&timer_result](const boost::system::error_code& error) { timer_result.reset(error); });

			size_t size{0};

			boost::optional<boost::system::error_code> result;
			boost::asio::async_write(this->s, boost::asio::buffer(data, data.size()),
									[&result, &size](const boost::system::error_code& error, size_t length) {
										result.reset(error);
										size = length;
									});

			this->io_context.reset();
			while (this->io_context.run_one() != 0u) {
				if (result) {
					timer.cancel();
				} else if (timer_result) {
					this->s.cancel();
				}
			}

			if (*result) {
				throw boost::system::system_error(*result);
			}
			
			return static_cast<int>(size);
		}

		auto netHelperTCP::recv(void * buffer, size_t read_size) -> int {
			if (!this->connected) {
				return -1;
			}

			boost::optional<boost::system::error_code> timer_result;
			boost::asio::deadline_timer timer(this->io_context);
			timer.expires_from_now(boost::posix_time::milliseconds(this->timeout));
			timer.async_wait([&timer_result](const boost::system::error_code& error) { timer_result.reset(error); });

			size_t size{0};

			boost::optional<boost::system::error_code> result;
			boost::asio::async_read(this->s, boost::asio::buffer(buffer, read_size),
									[&result, &size](const boost::system::error_code& error, size_t length) {
										result.reset(error);
										size = length;
									});

			this->io_context.reset();
			while (this->io_context.run_one() != 0u) {
				if (result) {
					timer.cancel();
				} else if (timer_result) {
					this->s.cancel();
				}
			}

			if (*result) {
				throw boost::system::system_error(*result);
			}

			return static_cast<int>(size);
		}

		netHelperSSL::netHelperSSL(std::string conn_target, std::string conn_port, bool use_msgpack, bool silent)
			: ssl_ctx(boost::asio::ssl::context(boost::asio::ssl::context::sslv23)),
			s(boost::asio::ssl::stream<tcp::socket>(this->io_context, this->ssl_ctx)),
			netHelper_base(std::move(conn_target), std::move(conn_port), use_msgpack, silent) {
			this->s.set_verify_mode(boost::asio::ssl::verify_none);
		}

		/*!
			@brief	connects socket
			@return	Returns 0 on success, != 0 for errors.
		*/
		auto netHelperSSL::connect() -> int {
			if (this->connected) {
				return 1;
			}

			tcp::resolver resolver(io_context);
			const tcp::resolver::query query(this->conn_target, this->conn_port);
			const tcp::resolver::iterator iterator = resolver.resolve(query);

			boost::optional<boost::system::error_code> timer_result;
			boost::asio::deadline_timer timer(this->io_context);
			timer.expires_from_now(boost::posix_time::milliseconds(this->timeout));
			timer.async_wait([&timer_result](const boost::system::error_code& error) { timer_result.reset(error); });

			boost::optional<boost::system::error_code> result;
			boost::asio::async_connect(this->s.lowest_layer(), iterator,
									[this, &result](const boost::system::error_code& error,
													const tcp::resolver::iterator& /*endpoint*/) { result.reset(error); });

			this->io_context.reset();
			while (this->io_context.run_one() != 0u) {
				if (result) {
					timer.cancel();
				} else if (timer_result) {
					this->s.lowest_layer().cancel();
				}
			}

			if (*result) {
				throw boost::system::system_error(*result);
			}

			this->handshake();

			this->connected = true;

			return 0;
		}

		auto netHelperSSL::handshake() -> void {
			boost::optional<boost::system::error_code> timer_result;
			boost::asio::deadline_timer timer(this->io_context);
			timer.expires_from_now(boost::posix_time::milliseconds(this->timeout));
			timer.async_wait([&timer_result](const boost::system::error_code& error) { timer_result.reset(error); });

			boost::optional<boost::system::error_code> result;
			this->s.async_handshake(boost::asio::ssl::stream_base::client,
									[this, &result](const boost::system::error_code& error) { result.reset(error); });

			this->io_context.reset();
			while (this->io_context.run_one() != 0u) {
				if (result) {
					timer.cancel();
				} else if (timer_result) {
					this->s.lowest_layer().cancel();
				}
			}

			if (*result) {
				throw boost::system::system_error(*result);
			}
		}

		void netHelperSSL::disconnect() {
			if (!this->connected) {
				return;
			}

			const std::lock_guard<std::mutex> lock(this->sock_mtx);

			this->s.lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both);

			this->connected = false;
		}

		auto netHelperSSL::send(const std::string& data) -> int {
			if (!this->connected) {
				return -1;
			}

			boost::optional<boost::system::error_code> timer_result;
			boost::asio::deadline_timer timer(this->io_context);
			timer.expires_from_now(boost::posix_time::milliseconds(this->timeout));
			timer.async_wait([&timer_result](const boost::system::error_code& error) { timer_result.reset(error); });

			size_t size{0};

			boost::optional<boost::system::error_code> result;
			boost::asio::async_write(this->s, boost::asio::buffer(data, data.size()),
									[&result, &size](const boost::system::error_code& error, size_t length) {
										result.reset(error);
										size = length;
									});

			this->io_context.reset();
			while (this->io_context.run_one() != 0u) {
				if (result) {
					timer.cancel();
				} else if (timer_result) {
					this->s.lowest_layer().cancel();
				}
			}

			if (*result) {
				throw boost::system::system_error(*result);
			}

			return static_cast<int>(size);
		}

		auto netHelperSSL::recv(void * buffer, size_t read_size) -> int {
			if (!this->connected) {
				return -1;
			}

			boost::optional<boost::system::error_code> timer_result;
			boost::asio::deadline_timer timer(this->io_context);
			timer.expires_from_now(boost::posix_time::milliseconds(this->timeout));
			timer.async_wait([&timer_result](const boost::system::error_code& error) { timer_result.reset(error); });

			size_t size{0};

			boost::optional<boost::system::error_code> result;
			boost::asio::async_read(this->s, boost::asio::buffer(buffer, read_size),
									[&result, &size](const boost::system::error_code& error, size_t length) {
										result.reset(error);
										size = length;
									});

			this->io_context.reset();
			while (this->io_context.run_one() != 0u) {
				if (result) {
					timer.cancel();
				} else if (timer_result) {
					this->s.lowest_layer().cancel();
				}
			}

			if (*result) {
				throw boost::system::system_error(*result);
			}

			return static_cast<int>(size);
		}
	}  // namespace detail

	netHelper::netHelper(std::string conn_target, std::string conn_port, bool use_msgpack, bool silent, bool use_ssl) {
		if (use_ssl) {
			this->ptr = std::make_unique<detail::netHelperSSL>(std::move(conn_target), std::move(conn_port),
															   use_msgpack, silent);
		} else {
			this->ptr = std::make_unique<detail::netHelperTCP>(std::move(conn_target), std::move(conn_port),
															   use_msgpack, silent);
		}
	}

	auto netHelper::login(const std::string& user_name, const std::string& password, bool use_hash) -> int {
		return this->ptr->login(user_name, password, use_hash);
	};

	auto netHelper::is_logged_in() const -> int {
		return this->ptr->is_logged_in();
	};

	auto netHelper::is_connected() const -> bool {
		return this->ptr->is_connected();
	}

	auto netHelper::send(const char* data) -> int {
		return this->send(data);
	}

	auto netHelper::send(const std::vector<uint8_t>& data) -> int {
		return this->send(data);
	}

	auto netHelper::send_command(const std::string& command, json& response, const json& payload, int api_version)
		-> int {
		return this->ptr->send_command(command, response, payload, api_version);
	}

	auto netHelper::getCommandReturnPayload(const std::string& command, const json& payload, int api_version) -> json {
		return this->ptr->getCommandReturnPayload(command, payload, api_version);
	}

	[[deprecated]] auto netHelper::set_timeout(unsigned int timeout) -> void {
		return this->ptr->set_timeout_ms(timeout * 1000);
	}

	auto netHelper::set_timeout_ms(unsigned int timeout_ms) -> void {
		return this->ptr->set_timeout_ms(timeout_ms);
	}

	auto netHelper::get_mutex() -> std::mutex& {
		return this->ptr->get_mutex();
	}

	auto netHelper::sha512(const std::string& input) -> std::string {
		return detail::netHelper_base::sha512(input);
	}

	auto netHelper::getLastRawPosition(const unsigned char* str) -> unsigned int {
		return detail::netHelper_base::getLastRawPosition(str);
	}

	auto netHelper::getLastRawPosition(const char* str) -> unsigned int {
		return detail::netHelper_base::getLastRawPosition(str);
	}

	auto netHelper::connect() -> int {
		return this->ptr->connect();
	}

	auto netHelper::disconnect() noexcept -> void {
		return this->ptr->disconnect();
	}

	auto netHelper::send(const std::string& data) -> int {
		return this->ptr->send(data);
	}

	auto netHelper::recv(void* buffer, size_t read_size) -> int {
		return this->ptr->recv(buffer, read_size);
	}
}