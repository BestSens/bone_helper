/*
 * system_helper.cpp
 *
 *  Created on: 15.10.2021
 *      Author: Jan Schöppach
 */

#include "bone_helper/system_helper.hpp"

#include <grp.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <bit>
#include <cstdarg>
#include <cstring>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

#include "boost/asio.hpp"
#include "boost/process.hpp"

#ifdef ENABLE_SYSTEMD_STATUS
#include <systemd/sd-daemon.h>
#include <systemd/sd-journal.h>
#endif

namespace bestsens {
	auto strerror_s(int errnum) -> std::string {
		std::array<char, 256> buffer{};
#ifndef __GLIBC__
		strerror_r(errnum, buffer.data(), buffer.size());
		return {buffer.data()};
#else
		return {strerror_r(errnum, buffer.data(), buffer.size())};
#endif
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

		auto pipeSystemCommand(const std::string &command) -> std::vector<std::string> {
			namespace bp = boost::process;

			const auto new_command = command + " 2>&1";	 // redirect stderr to stdout
			const std::vector<std::string> args = {"-c", new_command};

			boost::asio::io_service ios;
			bp::ipstream is;

			const bp::child c(bp::search_path("sh"), args, bp::std_in.close(), bp::std_out > is, ios);
			ios.run();

			std::vector<std::string> lines{};
			std::string line;

			while (std::getline(is, line)) {
				lines.push_back(line + "\n");
			}

			return lines;
		}

		void daemonize() {
			/* Fork off the parent process */
			const auto pid = fork();
			if (pid < 0) {
				exit(EXIT_FAILURE);	 // NOLINT(concurrency-mt-unsafe)
			}

			/* If we got a good PID, then
			   we can exit the parent process. */
			if (pid > 0) {
				exit(EXIT_SUCCESS);	 // NOLINT(concurrency-mt-unsafe)
			}

			/* Change the file mode mask */
			umask(0);

			/* Create a new SID for the child process */
			const auto sid = setsid();
			if (sid < 0) {
				exit(EXIT_FAILURE);	 // NOLINT(concurrency-mt-unsafe)
			}

			/* Change the current working directory */
			if ((chdir("/")) < 0) {
				exit(EXIT_FAILURE);	 // NOLINT(concurrency-mt-unsafe)
			}

			/* Close out the standard file descriptors */
			close(STDIN_FILENO);
			close(STDOUT_FILENO);
			close(STDERR_FILENO);
		}

		auto memcpy_swap_bo(void *dest, const void *src, std::size_t count) -> void {
			char *dest_char = reinterpret_cast<char *>(dest); //NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
			const char *src_char = reinterpret_cast<const char *>(src); //NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)

			for (std::size_t i = 0; i < count; i++) {
				std::memcpy(dest_char + i, src_char + (count - i - 1), 1);
			}
		}

		auto memcpy_be(void *dest, const void *src, std::size_t count) -> void {
			static_assert(std::endian::native == std::endian::big || std::endian::native == std::endian::little,
						  "mixed endian is not supported");

			if constexpr (std::endian::native == std::endian::big) {
				std::memcpy(dest, src, count);
			} else {
				memcpy_swap_bo(dest, src, count);
			}
		}

		auto memcpy_le(void *dest, const void *src, std::size_t count) -> void {
			static_assert(std::endian::native == std::endian::big || std::endian::native == std::endian::little,
						  "mixed endian is not supported");

			if constexpr (std::endian::native == std::endian::little) {
				std::memcpy(dest, src, count);
			} else {
				memcpy_swap_bo(dest, src, count);
			}
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

			void MultiWatchdog::enable() {
				const std::lock_guard<std::mutex> lock(MultiWatchdog::list_mtx);
				MultiWatchdog::watchdog_list.push_back(&this->own_entry);
			}

			void MultiWatchdog::disable() {
				const std::lock_guard<std::mutex> lock(MultiWatchdog::list_mtx);

				auto it = std::find(MultiWatchdog::watchdog_list.begin(), MultiWatchdog::watchdog_list.end(),
									&this->own_entry);

				if (it != MultiWatchdog::watchdog_list.end()) {
					MultiWatchdog::watchdog_list.erase(it);
				}
			}

			MultiWatchdog::MultiWatchdog() {
				this->enable();
			}

			MultiWatchdog::~MultiWatchdog() {
				this->disable();
			}

			void MultiWatchdog::trigger() {
				const std::lock_guard<std::mutex> lock(MultiWatchdog::list_mtx);
				this->own_entry = 1;

				if (std::any_of(MultiWatchdog::watchdog_list.begin(), MultiWatchdog::watchdog_list.end(),
								[](const int *e) { return *e == 0; })) {
					return;
				}

				watchdog();

				for (auto &e : MultiWatchdog::watchdog_list) {
					if (*e != -1) {
						*e = 0;
					}
				}
			}
		}  // namespace systemd
	}  // namespace system_helper
}  // namespace bestsens