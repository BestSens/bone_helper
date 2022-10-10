/*
 * netHelper.cpp
 *
 *  Created on: 14.01.2022
 *      Author: Jan Schöppach
 */

#include "bone_helper/netHelper.hpp"

#include <fcntl.h>
#include <netdb.h>
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
#include "fmt/format.h"
#include "fmt/ranges.h"
#ifndef BONE_HELPER_NO_SSL
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/error.h"
#include "mbedtls/platform.h"
#include "mbedtls/ssl.h"
#endif /* BONE_HELPER_NO_SSL */
#include "mbedtls/sha512.h"
#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

namespace bestsens {
	using json = nlohmann::json;

	netHelper::netHelper(std::string conn_target, std::string conn_port, bool use_msgpack, bool silent, bool use_ssl)
		: conn_target(std::move(conn_target)),
		  conn_port(std::move(conn_port)),
		  use_msgpack(use_msgpack),
		  silent(silent),
		  use_ssl(use_ssl) {
#ifdef BONE_HELPER_NO_SSL
		if (use_ssl)
			throw std::runtime_error("SSL requested but disabled on compile");
#endif /* BONE_HELPER_NO_SSL */

		const std::lock_guard<std::mutex> lock(this->sock_mtx);
		
		/*
		 * socket configuration
		 */
		memset(&(this->remote), 0, sizeof (this->remote));
		this->remote.ai_family = AF_UNSPEC;
		this->remote.ai_socktype = SOCK_STREAM;

		const auto retval =
			getaddrinfo(this->conn_target.c_str(), this->conn_port.c_str(), &(this->remote), &(this->res));

		if (retval != 0) throw std::runtime_error(fmt::format("error getting addrinfo: {}", gai_strerror(retval)));

#ifndef BONE_HELPER_NO_SSL
		if (use_ssl)
			this->initSSL();
#endif /* BONE_HELPER_NO_SSL */

		/*
		 * open socket
		 */
		this->sockfd = socket(this->res->ai_family, this->res->ai_socktype, this->res->ai_protocol);
		if (this->sockfd == -1) {
			throw std::runtime_error(fmt::format("error opening socket: {}", this->sockfd));
		}

		this->set_timeout(this->timeout);
	}

	netHelper::netHelper(netHelper&& src) noexcept : use_ssl(src.use_ssl) {
		const std::lock_guard<std::mutex> lock_src(src.sock_mtx);

		std::swap(this->sockfd, src.sockfd);
		std::swap(this->connected, src.connected);
		std::swap(this->timeout, src.timeout);
		std::swap(this->remote, src.remote);
		std::swap(this->res, src.res);
		std::swap(this->user_name, src.user_name);
		std::swap(this->conn_target, src.conn_target);
		std::swap(this->conn_port, src.conn_port);
		std::swap(this->user_level, src.user_level);
		std::swap(this->use_msgpack, src.use_msgpack);
		std::swap(this->silent, src.silent);

#ifndef BONE_HELPER_NO_SSL
		std::swap(this->entropy, src.entropy);
		std::swap(this->ctr_drbg, src.ctr_drbg);
		std::swap(this->ssl, src.ssl);
		std::swap(this->ssl_conf, src.ssl_conf);
#endif
	}

	netHelper::~netHelper() noexcept {
		this->disconnect();

#ifndef BONE_HELPER_NO_SSL
		if (this->use_ssl) {
			mbedtls_entropy_free(&this->entropy);
			mbedtls_ctr_drbg_free(&this->ctr_drbg);
			// mbedtls_x509_crt_free(&this->cacert);
			mbedtls_ssl_free(&this->ssl);
			mbedtls_ssl_config_free(&this->ssl_conf);
		}
#endif /* BONE_HELPER_NO_SSL */

		freeaddrinfo(this->res);
	}

#ifndef BONE_HELPER_NO_SSL
	void netHelper::initSSL() {
		int ret{0};

		mbedtls_entropy_init(&this->entropy);
		mbedtls_ctr_drbg_init(&this->ctr_drbg);
		// mbedtls_x509_crt_init(&this->cacert);
		mbedtls_ssl_init(&this->ssl);
		mbedtls_ssl_config_init(&this->ssl_conf);

		constexpr auto personalisation = "bestsens-netHelper-personalize-string";

		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
		const auto *const personalisation_unsigned_char = reinterpret_cast<const unsigned char*>(personalisation);

		if ((ret = mbedtls_ctr_drbg_seed(&this->ctr_drbg, mbedtls_entropy_func, &this->entropy,
										 personalisation_unsigned_char, 36)) != 0)
			throw std::runtime_error(fmt::format("mbedtls_crt_drbg_init: 0x{:X}", -ret));

		// if ((ret = mbedtls_x509_crt_parse(&this->cacert, (const unsigned char*)SSL_CA_PEM, sizeof(SSL_CA_PEM))) != 0)
		// 	throw std::runtime_error(fmt::format("mbedtls_x509_crt_parse: 0x{:X}", -ret));

		if ((ret = mbedtls_ssl_config_defaults(&this->ssl_conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM,
											   MBEDTLS_SSL_PRESET_DEFAULT)) != 0)
			throw std::runtime_error(fmt::format("mbedtls_ssl_config_defaults: 0x{:X}", -ret));

		// TODO: SSL certificates cannot be determined valid for BeMoS controllers as there is no CA certificate
		mbedtls_ssl_conf_authmode(&this->ssl_conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
		// mbedtls_ssl_conf_ca_chain(&this->ssl_conf, &this->cacert, nullptr);
		mbedtls_ssl_conf_rng(&this->ssl_conf, mbedtls_ctr_drbg_random, &this->ctr_drbg);

		if ((ret = mbedtls_ssl_setup(&this->ssl, &this->ssl_conf)) != 0)
			throw std::runtime_error(fmt::format("mbedtls_ssl_setup: 0x{:X}", -ret));

		mbedtls_ssl_set_hostname(&this->ssl, this->conn_target.c_str());
		mbedtls_ssl_set_bio(&this->ssl, static_cast<void*>(&this->sockfd), sendCb, recvCb, nullptr);
	}

	void netHelper::doSSLHandshake() {
		int ret{0};

		spdlog::debug("Starting the TLS handshake...");
		ret = mbedtls_ssl_handshake(&this->ssl);
		if (ret < 0) {
			if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
				throw std::runtime_error(fmt::format("mbedtls_ssl_handshake: 0x{:X}", -ret));
		}

		std::array<char, 1024> buffer{};

		/* It also means the handshake is done, time to print info */
		spdlog::debug("TLS connection to {} established", this->conn_target);

		mbedtls_x509_crt_info(buffer.data(), buffer.size(), "\r    ", mbedtls_ssl_get_peer_cert(&this->ssl));
		spdlog::debug("Server certificate:\r\n{}", buffer.data());

		const auto flags = mbedtls_ssl_get_verify_result(&this->ssl);
		if (flags != 0) {
		    mbedtls_x509_crt_verify_info(buffer.data(), buffer.size(), "\r  ! ", flags);
		    spdlog::warn("Certificate verification failed:\r\n{}", buffer.data());
		} else {
			spdlog::debug("Certificate verification passed");
		}
	}
#endif /* BONE_HELPER_NO_SSL */

	auto netHelper::get_sockfd() const -> int {
		return this->sockfd;
	}

	auto netHelper::get_mutex() -> std::mutex& {
		return this->sock_mtx;
	}

	auto netHelper::getLastRawPosition(const unsigned char* str) -> unsigned int {
		const auto last_position = [&]() -> unsigned int {
			try {
				return (static_cast<uint8_t>(str[0]) << 24u) + (static_cast<uint8_t>(str[1]) << 16u) +
					   (static_cast<uint8_t>(str[2]) << 8u) + (static_cast<uint8_t>(str[3]));
			} catch (...) {}

			return 0;
		}();

		return last_position;
	}

	auto netHelper::getLastRawPosition(const char* str) -> unsigned int {
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
		return getLastRawPosition(reinterpret_cast<const unsigned char*>(str));
	}

	auto netHelper::sha512(const std::string& input) -> std::string {
		std::array<unsigned char, 64> output_buffer{};
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
		mbedtls_sha512(reinterpret_cast<const unsigned char*>(input.c_str()), input.size(), output_buffer.data(), 0);
		return fmt::format("{:02x}", fmt::join(output_buffer, ""));
	}

	auto netHelper::login(const std::string& user_name, const std::string& password, bool use_hash) -> int {
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
				if (!this->silent)
					spdlog::error("login failed: {}", login_response.at("payload").at("error").get<std::string>());
			}
		} catch (...) {}

		this->user_level = login_response.at("payload").at("user_level").get<int>();

		return this->user_level;
	}

	auto netHelper::is_logged_in() const -> int {
		return this->user_level;
	}

	auto netHelper::getTimevalStruct() const -> timeval {
		timeval tv{};
		tv.tv_sec = this->timeout / 1000;
		tv.tv_usec = (this->timeout % 1000) * 1000;

		return tv;
	}

	/*!
		@brief	sets socket timeout
		@param	timeout: timeout in s
		@return	Returns 0 on success, -1 for errors.
	*/
	[[deprecated]] auto netHelper::set_timeout(long timeout) -> int {
		return this->set_timeout_ms(timeout * 1000);
	}

	/*!
		@brief	sets socket timeout
		@param	timeout_ms: timeout in ms
		@return	Returns 0 on success, -1 for errors.
	*/
	auto netHelper::set_timeout_ms(long timeout_ms) -> int {
		this->timeout = timeout_ms;

		const auto tv = this->getTimevalStruct();

		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
		return setsockopt(this->sockfd, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&tv), sizeof tv);
	}

	auto netHelper::send_command(const std::string& command, json& response, const json& payload, int api_version) -> int {
		json temp = {{"command", command}};

		if (payload.is_object())
			temp["payload"] = payload;

		if (api_version > 0)
			temp["api"] = api_version;

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
					spdlog::error("input string: \"{}\"", str.data());
				}
			}
		} else {
			if (!this->silent) {
				spdlog::critical("could not receive all data");
				spdlog::critical("input string: \"{}\"", str.data());
			}

			throw std::runtime_error("could not receive all data");
		}

		return 0;
	}

	auto netHelper::getCommandReturnPayload(const std::string& command, const json& payload, int api_version) -> json {
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

	auto netHelper::is_connected() const -> bool {
		return this->connected;
	}

	/*!
		@brief	connects socket
		@return	Returns 0 on success, != 0 for errors.
	*/
	auto netHelper::connect() -> int {
		if (this->connected)
			return 1;

		// Set non-blocking 
		long arg{0};
		if ((arg = fcntl(this->sockfd, F_GETFL, nullptr)) < 0) // NOLINT(hicpp-vararg)
			throw std::runtime_error("Error fcntl(..., F_GETFL)");

		arg |= O_NONBLOCK;  // NOLINT(hicpp-signed-bitwise)
		
		if (fcntl(this->sockfd, F_SETFL, arg) < 0) // NOLINT(hicpp-vararg)
			throw std::runtime_error("Error fcntl(..., F_GETFL)");

		/*
		 * connect to socket
		 */

		std::lock_guard<std::mutex> lock(this->sock_mtx);

		auto res = ::connect(this->sockfd, this->res->ai_addr, this->res->ai_addrlen);
		fd_set myset;
		int valopt{0};

		if (res < 0) {
			if (errno == EINPROGRESS) {
				do {
					auto tv = this->getTimevalStruct();
					FD_ZERO(&myset); // NOLINT(readability-isolate-declaration)
					FD_SET(this->sockfd, &myset); // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
					res = select(this->sockfd+1, nullptr, &myset, nullptr, &tv);

					if (res < 0 && errno != EINTR) {
						if (!this->silent)
							spdlog::critical("Error connecting {} - {}", errno, bestsens::strerror_s(errno));
						return 1;
					} else if (res > 0) {
						socklen_t lon = sizeof(int);
						if (getsockopt(this->sockfd, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon) < 0) {
							if (!this->silent)
								spdlog::critical("Error in getsockopt() {} - {}", errno, bestsens::strerror_s(errno));
							return 1; 
						}

						if (valopt != 0) {
							if (!this->silent)
								spdlog::critical("Error in delayed connection() {} - {}", valopt, bestsens::strerror_s(valopt));
							return 1;  
						}

						break;
					} else {
						if (!this->silent) spdlog::critical("Timeout in select() - Cancelling!"); 
						return 1;
					}
				} while(true);
			}
		} else {
			if (!this->silent) spdlog::critical("error connecting to {}:{}", this->conn_target, this->conn_port);
			return 1;
		}

		if ((arg = fcntl(this->sockfd, F_GETFL, NULL)) < 0) // NOLINT(hicpp-vararg)
			throw std::runtime_error("Error fcntl(..., F_GETFL)");

		arg &= (~O_NONBLOCK); // NOLINT(hicpp-signed-bitwise)

		if (fcntl(this->sockfd, F_SETFL, arg) < 0) // NOLINT(hicpp-vararg)
			throw std::runtime_error("Error fcntl(..., F_GETFL)");

#ifndef BONE_HELPER_NO_SSL
		if (this->use_ssl)
			this->doSSLHandshake();
#endif /* BONE_HELPER_NO_SSL */

		this->connected = true;

		return 0;
	}

	void netHelper::disconnect() noexcept {
		if (!this->connected)
			return;

		std::lock_guard<std::mutex> lock(this->sock_mtx);

		close(this->sockfd);

		this->connected = false;
	}

	auto netHelper::sendCb(void *ctx, const unsigned char *buf, size_t len) -> int {
		auto sockfd = *static_cast<int*>(ctx);

		const auto ret = ::send(sockfd, buf, len, 0);
		return static_cast<int>(ret);
	}

	auto netHelper::sslSendWrapper(const char* buffer, size_t len) -> int {
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
		const auto *const buffer_unsigned = reinterpret_cast<const unsigned char*>(buffer);

		if (!this->use_ssl) {
			const auto ret = sendCb(&this->sockfd, buffer_unsigned, len);

			if (ret < 0) {
				throw std::runtime_error(fmt::format("send error: 0x{:X}", -ret));
			}

			return ret;
		}
			
#ifndef BONE_HELPER_NO_SSL
		const auto ret = mbedtls_ssl_write(&this->ssl, buffer_unsigned, len);
		if (ret < 0) {
			if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
				throw std::runtime_error(fmt::format("mbedtls_ssl_write: 0x{:X}", -ret));

			return 0;
		}

		return ret;
#else
		return EXIT_FAILURE;
#endif /* BONE_HELPER_NO_SSL */
	}

	auto netHelper::recvCb(void *ctx, unsigned char *buf, size_t len) -> int {
		auto sockfd = *static_cast<int*>(ctx);

		const auto ret = ::recv(sockfd, buf, len, 0);
		return static_cast<int>(ret);
	}

	auto netHelper::sslRecvWrapper(char* buffer, size_t amount) -> int {
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
		auto* buffer_unsigned = reinterpret_cast<unsigned char*>(buffer);

		if (!this->use_ssl) {
			const auto ret = recvCb(&this->sockfd, buffer_unsigned, amount);

			if (ret < 0) {
				throw std::runtime_error(fmt::format("send error: 0x{:X}", -ret));
			}

			return ret;
		}

#ifndef BONE_HELPER_NO_SSL
		const auto ret = mbedtls_ssl_read(&this->ssl, buffer_unsigned, amount);
		if (ret < 0) {
			if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
				throw std::runtime_error(fmt::format("mbedtls_ssl_read: 0x{:X}", -ret));

			return 0;
		}

		buffer[ret] = 0;

		return ret;
#else
		return EXIT_FAILURE;
#endif /* BONE_HELPER_NO_SSL */
	}

	auto netHelper::send(const char * data) -> int {
		return this->send(std::string(data));
	}

	auto netHelper::send(const std::vector<uint8_t>& data) -> int {
		const std::string s(data.begin(), data.end());
		return this->send(s);
	}

	auto netHelper::send(const std::string& data) -> int {
		if (!this->connected)
			return -1;

		/*
		 * send data until buffer is empty
		 * or requested data len is reached
		 */
		unsigned int t = 0;
		int error_counter = 0;
		while (t < data.length()) {
			const auto count = this->sslSendWrapper(data.c_str() + t, data.length() - t);

			if (count <= 0 && error_counter++ > 10) {
				if (error_counter++ > 10) {
					if (!this->silent)
						spdlog::critical("error sending data: {} ({})", errno, bestsens::strerror_s(errno));
					break;
				}
			} else {
				error_counter = 0;
				t += count;
			}
		}

		return static_cast<int>(t);
	}

	auto netHelper::recv(void * buffer, size_t read_size) -> int {
		if (!this->connected)
			return -1;

		/*
		 * receive data untill buffer is empty
		 * or requested data len is reached
		 */
		unsigned int t = 0;
		int error_counter = 0;
		while (t < read_size) {
			const auto count = this->sslRecvWrapper(static_cast<char *>(buffer) + t, read_size - t);

			if (count <= 0) {
				if (error_counter++ > 10) {
					if (!this->silent)
						spdlog::critical("error receiving data: {} ({})", errno, bestsens::strerror_s(errno));
					break;
				}
			} else {
				error_counter = 0;
				t += count;
			}
		}

		return static_cast<int>(t);
	}
}