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
#include "bone_helper/fsHelper.hpp"

#ifdef ENABLE_SYSTEMD_STATUS
#include <systemd/sd-daemon.h>
#include <systemd/sd-journal.h>
#endif

namespace bestsens {
	auto strerror_s(int errnum) -> std::string;
	
	namespace system_helper {
		auto getUID() -> unsigned int;
		auto getUID(const std::string &user_name) -> unsigned int;
		auto getGID() -> unsigned int;
		auto getGID(const std::string &group_name) -> unsigned int;
		auto dropPriviledges() -> void;

		auto pipeSystemCommand(const std::string& command) -> std::vector<std::string>;

		auto daemonize() -> void;

		auto memcpy_swap_bo(void *dest, const void *src, std::size_t count) -> void;
		auto memcpy_be(void *dest, const void *src, std::size_t count) -> void;
		auto memcpy_le(void *dest, const void *src, std::size_t count) -> void;

		constexpr auto *deleteFilesRecursive = fs::deleteFilesRecursive;
		constexpr auto *getDirectoriesUnsorted = fs::getDirectoriesUnsorted;
		constexpr auto *getDirectories = fs::getDirectories;
		constexpr auto *readDirectory = fs::readDirectory;
		constexpr auto *readDirectoryUnsorted = fs::readDirectoryUnsorted;
		constexpr auto *readDirectoryNatural = fs::readDirectoryNatural;

		namespace systemd {
			auto ready() -> void;
			auto watchdog() -> void;
			auto status(const std::string &status) -> void;
			auto error(int errno) -> void;

			class MultiWatchdog {
			public:
				MultiWatchdog();
				MultiWatchdog(const MultiWatchdog &) = default;
				MultiWatchdog(MultiWatchdog &&) = delete;
				auto operator=(const MultiWatchdog &) -> MultiWatchdog & = default;
				auto operator=(MultiWatchdog &&) -> MultiWatchdog & = delete;
				~MultiWatchdog();

				auto enable() -> void;
				auto disable() -> void;
				auto trigger() -> void;
			private:
				int own_entry{0};
				static inline std::vector<int*> watchdog_list{}; //NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
				static inline std::mutex list_mtx{}; //NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
			};
		} // namespace systemd
	} // namespace system_helper
} // namespace bestsens

#endif /* SYSTEM_HELPER_HPP_ */
