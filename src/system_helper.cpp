/*
 * system_helper.cpp
 *
 *  Created on: 15.10.2021
 *      Author: Jan Schöppach
 */

#include "bone_helper/system_helper.hpp"

#include <endian.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <cstdarg>
#include <cstring>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef ENABLE_SYSTEMD_STATUS
#include <systemd/sd-daemon.h>
#include <systemd/sd-journal.h>
#else
#include <syslog.h>
#endif

#include "bone_helper/stdlib_backports.hpp"
#include "bone_helper/strnatcmp.hpp"
#include "tinydir.h"

namespace bestsens {
	namespace system_helper {
		void daemonize() {
			/* Fork off the parent process */
			pid_t pid = fork();
			if (pid < 0) exit(EXIT_FAILURE); // NOLINT(concurrency-mt-unsafe)

			/* If we got a good PID, then
			   we can exit the parent process. */
			if (pid > 0) exit(EXIT_SUCCESS); // NOLINT(concurrency-mt-unsafe)

			/* Change the file mode mask */
			umask(0);

			/* Create a new SID for the child process */
			pid_t sid = setsid();
			if (sid < 0) exit(EXIT_FAILURE); // NOLINT(concurrency-mt-unsafe)

			/* Change the current working directory */
			if ((chdir("/")) < 0) exit(EXIT_FAILURE); // NOLINT(concurrency-mt-unsafe)

			/* Close out the standard file descriptors */
			close(STDIN_FILENO);
			close(STDOUT_FILENO);
			close(STDERR_FILENO);
		}

		void memcpy_swap_bo(void *dest, const void *src, std::size_t count) {
			char *dest_char = reinterpret_cast<char *>(dest);
			const char *src_char = reinterpret_cast<const char *>(src);

			for (std::size_t i = 0; i < count; i++) std::memcpy(dest_char + i, src_char + (count - i - 1), 1);
		}

		void memcpy_be(void *dest, const void *src, std::size_t count) {
#if __BYTE_ORDER == __BIG_ENDIAN
			std::memcpy(dest, src, count);
#elif __BYTE_ORDER == __LITTLE_ENDIAN
			memcpy_swap_bo(dest, src, count);
#endif
		}

		void memcpy_le(void *dest, const void *src, std::size_t count) {
#if __BYTE_ORDER == __LITTLE_ENDIAN
			std::memcpy(dest, src, count);
#elif __BYTE_ORDER == __BIG_ENDIAN
			memcpy_swap_bo(dest, src, count);
#endif
		}

		auto readDirectoryUnsorted(const std::string &directory_location, const std::string &start_string,
								   const std::string &extension, bool full_path = true) -> std::vector<std::string> {

			std::vector<std::string> result;

			std::string lc_extension(extension);
			std::transform(lc_extension.begin(), lc_extension.end(), lc_extension.begin(), ::tolower);

			tinydir_dir dir;

			if (tinydir_open(&dir, directory_location.c_str()) != 0)
				throw std::runtime_error("error opening directory");

			try {
				while (dir.has_next) {
					tinydir_file file;

					tinydir_readfile(&dir, &file);
					tinydir_next(&dir);

					if (file.is_dir)
						continue;

					const std::string entry(file.name);
					std::string lc_entry(entry);
					std::transform(lc_entry.begin(), lc_entry.end(), lc_entry.begin(), ::tolower);

					if (backports::startsWith(lc_entry, start_string) && backports::endsWith(lc_entry, lc_extension)) {
						if (full_path)
							result.push_back(directory_location + "/" + entry);
						else
							result.push_back(entry);
					}
				}
			} catch (...) {}

			tinydir_close(&dir);

			return result;
		}

		auto readDirectory(const std::string &directory_location, const std::string &start_string,
						   const std::string &extension, bool full_path = true) -> std::vector<std::string> {

			auto result = readDirectoryUnsorted(directory_location, start_string, extension, full_path);
			std::sort(result.begin(), result.end());

			return result;
		}

		auto readDirectoryNatural(const std::string &directory_location, const std::string &start_string,
						   const std::string &extension, bool full_path = true) -> std::vector<std::string> {

			auto result = readDirectoryUnsorted(directory_location, start_string, extension, full_path);
			std::sort(result.begin(), result.end(), compareNat);

			return result;
		}

		namespace systemd {
			void ready() {
#ifdef ENABLE_SYSTEMD_STATUS
				sd_notify(0, "READY=1");
#endif
			}

			void watchdog() {
#ifdef ENABLE_SYSTEMD_STATUS
				sd_notify(0, "WATCHDOG=1");
#endif
			}

			void status(__attribute__((unused)) const std::string &status) {
#ifdef ENABLE_SYSTEMD_STATUS
				sd_notifyf(0, "STATUS=%s", status.c_str());
#endif
			}

			void error(__attribute__((unused)) int errno) {
#ifdef ENABLE_SYSTEMD_STATUS
				sd_notifyf(0, "STATUS=%s\nERRNO=%d", strerror(errno), errno);
#endif
			}

			std::mutex MultiWatchdog::list_mtx;
			std::vector<int*> MultiWatchdog::watchdog_list;

			MultiWatchdog::MultiWatchdog() {
				std::lock_guard<std::mutex> lock(list_mtx);
				watchdog_list.push_back(&this->own_entry);
			}

			MultiWatchdog::~MultiWatchdog() {
				std::lock_guard<std::mutex> lock(list_mtx);

				auto it = std::find(watchdog_list.begin(), watchdog_list.end(), &this->own_entry);

				if (it != watchdog_list.end()) watchdog_list.erase(it);
			}

			void MultiWatchdog::trigger() {
				std::lock_guard<std::mutex> lock(list_mtx);
				this->own_entry = 1;

				for (const auto &e : watchdog_list)
					if (*e == 0) return;

				watchdog();

				for (auto &e : watchdog_list)
					if (*e != -1) *e = 0;
			}
		}  // namespace systemd

		void LogManager::setMaxLogLevel(int max_log_level) {
			this->max_log_level = max_log_level;

			setlogmask(LOG_UPTO(max_log_level));
		}

		void LogManager::setEcho(bool enable_echo) {
			this->enable_echo = enable_echo;

			closelog();
			if (!this->enable_echo)
				openlog(this->process_name.c_str(), LOG_NDELAY | LOG_PID, LOG_DAEMON);
			else
				openlog(this->process_name.c_str(), LOG_CONS | LOG_PERROR | LOG_NDELAY | LOG_PID, LOG_DAEMON);
		}

		void LogManager::write(const std::string &message) {
			return this->write(this->default_log_level, message);
		}

		void LogManager::write(const char *fmt, ...) {
			va_list ap;
			va_start(ap, fmt);
			this->write(this->default_log_level, fmt, ap);
			va_end(ap);
		}

		void LogManager::write(int priority, const std::string &message) {
			return this->write(priority, "%s", message.c_str());
		}

		void LogManager::write(int priority, const char *fmt, ...) {
			va_list ap;
			va_start(ap, fmt);
			this->write(priority, fmt, ap);
			va_end(ap);
		}

		void LogManager::write(int priority, const char *fmt, va_list ap) {
			if (priority > this->max_log_level) return;

			this->mutex.lock();
#ifdef ENABLE_SYSTEMD_STATUS
			if (this->enable_echo) {
				vfprintf(stdout, fmt, ap);
				if (fmt[std::strlen(fmt) - 1] != '\n') fprintf(stdout, "\n");
			}

			sd_journal_printv(priority, fmt, ap);
#else
			vsyslog(priority, fmt, ap);
#endif
			this->mutex.unlock();
		}

		void LogManager::auditlog(const char *fmt, ...) {
			va_list ap;
			va_start(ap, fmt);
			this->write(LOG_INFO, fmt, ap);
			va_end(ap);
		}
	}  // namespace system_helper
}  // namespace bestsens