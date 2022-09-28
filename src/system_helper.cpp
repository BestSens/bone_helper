/*
 * system_helper.cpp
 *
 *  Created on: 15.10.2021
 *      Author: Jan Schöppach
 */

#include "bone_helper/system_helper.hpp"

#include <endian.h>
#include <grp.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <array>
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
	auto strerror_s(int errnum) -> std::string {
		std::array<char, 256> buffer{};
		return {strerror_r(errnum, buffer.data(), buffer.size())};
	}

	namespace system_helper {
		auto getUID() -> unsigned int {
			return getuid();
		}

		auto getUID(const std::string &user_name) -> unsigned int {
			struct passwd *pwd = getpwnam(user_name.c_str());  // NOLINT (concurrency-mt-unsafe)

			if (pwd == nullptr) {
				throw std::runtime_error("error getting uid: " + strerror_s(errno));
			}

			return static_cast<unsigned int>(pwd->pw_uid);
		}

		auto getGID() -> unsigned int {
			return getgid();
		}

		auto getGID(const std::string &group_name) -> unsigned int {
			struct group *grp = getgrnam(group_name.c_str());  // NOLINT (concurrency-mt-unsafe)

			if (grp == nullptr) {
				throw std::runtime_error("error getting gid: " + strerror_s(errno));
			}

			return static_cast<unsigned int>(grp->gr_gid);
		}

		auto dropPriviledges() -> void {
			const auto userid = getUID("bemos");
			const auto groupid = getGID("bemos_users");

			if (setgid(groupid) != 0) {
				throw std::runtime_error("setgid: Unable to drop group privileges: " + strerror_s(errno));
			}

			if (setuid(userid) != 0) {
				throw std::runtime_error("setuid: Unable to drop user privileges: " + strerror_s(errno));
			}

			if (setuid(0) != -1) {
				throw std::runtime_error("managed to regain root privileges");
			}
		}

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

		auto deleteFilesRecursive(const std::string &directory_location) -> void {
			tinydir_dir dir;

			if (tinydir_open(&dir, directory_location.c_str()) != 0) {
				throw std::runtime_error("error opening directory " + directory_location);
			}

			while (dir.has_next != 0) {
				tinydir_file file;

				tinydir_readfile(&dir, &file);
				tinydir_next(&dir);

				const std::string current_filename(static_cast<char*>(file.name));

				/*
				 * this check is mandatory to not traverse upwards in folder structure!
				 */
				if (current_filename == "." || current_filename == "..") {
					continue;
				}

				const auto full_path = directory_location + "/" + current_filename;

				if (file.is_dir != 0) {
					deleteFilesRecursive(full_path);
				}

				/*
				 * we only want to delete regular files or folders, no symlinks or stuff
				 * we don't know what happens if we just delete them
				 */
				if (file.is_dir == 0 && file.is_reg == 0) {
					throw std::runtime_error("cannot delete " + full_path + ": not a regular file");
				}

				if (std::remove(full_path.c_str()) != 0) {
					throw std::runtime_error("error deleting file " + full_path + ": " +
											 strerror_s(errno));  // NOLINT(concurrency-mt-unsafe)
				}
			}

			if (std::remove(directory_location.c_str()) != 0) {
				throw std::runtime_error("error deleting file " + directory_location + ": " +
										 strerror_s(errno));  // NOLINT(concurrency-mt-unsafe)
			}
		}

		auto getDirectoriesUnsorted(const std::string &directory_location, bool recursive) -> std::vector<std::string> {
			std::vector<std::string> result;

			tinydir_dir dir;

			if (tinydir_open(&dir, directory_location.c_str()) != 0) {
				throw std::runtime_error("error opening directory " + directory_location);
			}

			try {
				while (dir.has_next != 0) {
					tinydir_file file;

					tinydir_readfile(&dir, &file);
					tinydir_next(&dir);

					if (file.is_dir == 0) {
						continue;
					}

					const std::string entry(static_cast<char*>(file.name));

					if (entry != "." && entry != "..") {
						const auto new_directory = directory_location + "/" + entry;
						result.push_back(new_directory);

						if (recursive) {
							const auto new_result = getDirectories(new_directory, recursive);
							result.insert(std::end(result), std::begin(new_result), std::end(new_result));
						}
					}
				}
			} catch (...) {}

			tinydir_close(&dir);
	
			return result;
		}

		auto getDirectories(const std::string &directory_location, bool recursive) -> std::vector<std::string> {
			auto result = getDirectoriesUnsorted(directory_location, recursive);
			std::sort(result.begin(), result.end(), compareNat);

			return result;
		}

		auto readDirectoryUnsorted(const std::string &directory_location, const std::string &start_string,
								   const std::string &extension, bool full_path = true) -> std::vector<std::string> {

			std::vector<std::string> result;

			std::string lc_extension(extension);
			std::transform(lc_extension.begin(), lc_extension.end(), lc_extension.begin(), ::tolower);

			tinydir_dir dir;

			if (tinydir_open(&dir, directory_location.c_str()) != 0) {
				throw std::runtime_error("error opening directory" + directory_location);
			}

			try {
				while (dir.has_next != 0) {
					tinydir_file file;

					tinydir_readfile(&dir, &file);
					tinydir_next(&dir);

					if (file.is_dir != 0) {
						continue;
					}

					const std::string entry(static_cast<char*>(file.name));
					std::string lc_entry(entry);
					std::transform(lc_entry.begin(), lc_entry.end(), lc_entry.begin(), ::tolower);

					if (backports::startsWith(entry, start_string) && backports::endsWith(lc_entry, lc_extension)) {
						if (full_path) {
							result.push_back(directory_location + "/" + entry);
						} else {
							result.push_back(entry);
						}
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
				sd_notifyf(0, "STATUS=%s\nERRNO=%d", bestsens::strerror_s(errno).c_str(), errno);
#endif
			}

			std::mutex MultiWatchdog::list_mtx;
			std::vector<int*> MultiWatchdog::watchdog_list;

			void MultiWatchdog::enable() {
				std::lock_guard<std::mutex> lock(MultiWatchdog::list_mtx);
				MultiWatchdog::watchdog_list.push_back(&this->own_entry);
			}

			void MultiWatchdog::disable() {
				std::lock_guard<std::mutex> lock(MultiWatchdog::list_mtx);

				auto it = std::find(MultiWatchdog::watchdog_list.begin(), MultiWatchdog::watchdog_list.end(), &this->own_entry);

				if (it != MultiWatchdog::watchdog_list.end()) MultiWatchdog::watchdog_list.erase(it);
			}

			MultiWatchdog::MultiWatchdog() {
				this->enable();
			}

			MultiWatchdog::~MultiWatchdog() {
				this->disable();
			}

			void MultiWatchdog::trigger() {
				std::lock_guard<std::mutex> lock(MultiWatchdog::list_mtx);
				this->own_entry = 1;

				if (std::any_of(MultiWatchdog::watchdog_list.begin(), MultiWatchdog::watchdog_list.end(),
								[](int *e) { return *e == 0; })) {
					return;
				}

				watchdog();

				for (auto &e : MultiWatchdog::watchdog_list)
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