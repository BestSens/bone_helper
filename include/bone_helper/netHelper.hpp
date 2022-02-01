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

#ifndef BONE_HELPER_NO_SSL
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/error.h"
#include "mbedtls/platform.h"
#include "mbedtls/ssl.h"
#endif /* BONE_HELPER_NO_SSL */

#include "nlohmann/json.hpp"

namespace bestsens {
	class netHelper {
	public:
		netHelper(std::string conn_target, std::string conn_port, bool use_msgpack = false, bool silent = false,
				  bool use_ssl = false);
		~netHelper() noexcept;

		auto connect() -> int;
		void disconnect() noexcept;
		auto login(const std::string& user_name, const std::string& password, bool use_hash = 1) -> int;

		auto is_logged_in() const -> int;

		auto is_connected() const -> bool;

		auto send(const std::string& data) -> int;
		auto send(const char * data) -> int;
		auto send(const std::vector<uint8_t>& data) -> int;
		auto send_command(const std::string& command, nlohmann::json& response, const nlohmann::json& payload = {},
						  int api_version = 0) -> int;

		auto set_timeout(const int timeout) -> int;

		auto get_sockfd() const -> int;

		auto get_mutex() -> std::mutex&;

		static auto sha512(const std::string& input) -> std::string;
		static auto getLastRawPosition(const unsigned char * str) -> unsigned int;
		static auto getLastRawPosition(const char * str) -> unsigned int;

		auto recv(void * buffer, size_t read_size) -> int;
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
		const bool use_ssl{false};

#ifndef BONE_HELPER_NO_SSL
		void initSSL();
		void doSSLHandshake();

		mbedtls_entropy_context entropy;
		mbedtls_ctr_drbg_context ctr_drbg;
		// mbedtls_x509_crt cacert;
		mbedtls_ssl_context ssl;
		mbedtls_ssl_config ssl_conf;
#endif /* BONE_HELPER_NO_SSL */

		auto ssl_send_wrapper(const char* buffer, size_t len) -> int;
		auto ssl_recv_wrapper(char* buffer, size_t amount) -> int;

		static auto recv_cb(void *ctx, unsigned char *buf, size_t len) -> int;
		static auto send_cb(void *ctx, const unsigned char *buf, size_t len) -> int;
	};

	class jsonNetHelper : public netHelper {
	public:
		using netHelper::netHelper;
	};
} // namespace bestsens

#endif /* NETHELPER_HPP_ */
