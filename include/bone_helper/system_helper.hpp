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

		void daemonize();

		void memcpy_swap_bo(void *dest, const void *src, std::size_t count);
		void memcpy_be(void *dest, const void *src, std::size_t count);
		void memcpy_le(void *dest, const void *src, std::size_t count);

		constexpr auto *deleteFilesRecursive = fs::deleteFilesRecursive;
		constexpr auto *getDirectoriesUnsorted = fs::getDirectoriesUnsorted;
		constexpr auto *getDirectories = fs::getDirectories;
		constexpr auto *readDirectory = fs::readDirectory;
		constexpr auto *readDirectoryUnsorted = fs::readDirectoryUnsorted;
		constexpr auto *readDirectoryNatural = fs::readDirectoryNatural;

		namespace systemd {
			void ready();
			void watchdog();
			void status(const std::string &status);
			void error(int errno);

			class MultiWatchdog {
			public:
				MultiWatchdog();
				~MultiWatchdog();
				void enable();
				void disable();
				void trigger();
			private:
				int own_entry{0};
				static std::vector<int*> watchdog_list;
				static std::mutex list_mtx;
			};
		} // namespace systemd
	} // namespace system_helper
} // namespace bestsens

#endif /* SYSTEM_HELPER_HPP_ */
