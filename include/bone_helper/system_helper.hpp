/*
 * system_helper.hpp
 *
 *  Created on: 16.08.2017
 *	  Author: Jan Schöppach
 */

#ifndef SYSTEM_HELPER_HPP_
#define SYSTEM_HELPER_HPP_

#include <mutex>
#include <string>
#include <vector>

#ifdef ENABLE_SYSTEMD_STATUS
#include <systemd/sd-daemon.h>
#include <systemd/sd-journal.h>
#else
#include <syslog.h>
#endif

namespace bestsens {
	namespace system_helper {
		void daemonize();

		void memcpy_swap_bo(void *dest, const void *src, std::size_t count);
		void memcpy_be(void *dest, const void *src, std::size_t count);
		void memcpy_le(void *dest, const void *src, std::size_t count);

		auto readDirectory(const std::string &directory_location, const std::string &start_string,
						   const std::string &extension, bool full_path) -> std::vector<std::string>;

		namespace systemd {
			void ready();
			void watchdog();
			void status(const std::string &status);
			void error(int errno);

			class MultiWatchdog {
			public:
				MultiWatchdog();
				~MultiWatchdog();
				void trigger();
			private:
				int own_entry{0};
				static std::vector<int*> watchdog_list;
				static std::mutex list_mtx;
			};
		} // namespace systemd

		class LogManager {
		public:
			LogManager() {
				this->setEcho(this->enable_echo);
			};
			explicit LogManager(std::string process_name) : process_name{std::move(process_name)} {
				this->setEcho(this->enable_echo);
			};

			void setMaxLogLevel(int max_log_level);
			void setEcho(bool enable_echo);

			void write(int priority, const std::string& message);
			void write(const std::string& message);
			void write(int priority, const char *fmt, ...);
			void write(int priority, const char *fmt, va_list ap);
			void write(const char *fmt, ...);

			void auditlog(const char *fmt, ...);
		private:
			int max_log_level{LOG_INFO};
			const int default_log_level{LOG_INFO};
			bool enable_echo{true};
			std::string process_name;

			std::mutex mutex;
		};
	} // namespace system_helper
} // namespace bestsens

#endif /* SYSTEM_HELPER_HPP_ */
