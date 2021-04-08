/*
 * system_helper.hpp
 *
 *  Created on: 16.08.2017
 *	  Author: Jan Schöppach
 */

#ifndef SYSTEM_HELPER_HPP_
#define SYSTEM_HELPER_HPP_

#include <string>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <mutex>
#include <cstdarg>
#include <cstring>
#include <dirent.h>
#include <unistd.h>
#include <endian.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef ENABLE_SYSTEMD_STATUS
#include <systemd/sd-daemon.h>
#include <systemd/sd-journal.h>
#else
#include <syslog.h>
#endif


namespace bestsens {
	namespace system_helper {
		inline void daemonize() {
			/* Our process ID and Session ID */
			pid_t pid, sid;

			/* Fork off the parent process */
			pid = fork();
			if (pid < 0) {
				exit(EXIT_FAILURE);
			}
			/* If we got a good PID, then
			   we can exit the parent process. */
			if (pid > 0) {
				exit(EXIT_SUCCESS);
			}

			/* Change the file mode mask */
			umask(0);

			/* Open any logs here */

			/* Create a new SID for the child process */
			sid = setsid();
			if (sid < 0) {
				/* Log the failure */
				exit(EXIT_FAILURE);
			}

			/* Change the current working directory */
			if ((chdir("/")) < 0) {
				/* Log the failure */
				exit(EXIT_FAILURE);
			}

			/* Close out the standard file descriptors */
			close(STDIN_FILENO);
			close(STDOUT_FILENO);
			close(STDERR_FILENO);
		}

		inline void memcpy_swap_bo(void * dest, const void * src, std::size_t count) {
			char * dest_char = reinterpret_cast<char *>(dest);
			const char * src_char = reinterpret_cast<const char *>(src);

			for(std::size_t i = 0; i < count; i++)
				std::memcpy(dest_char + i, src_char + (count - i - 1), 1);
		}

		inline void memcpy_be(void * dest, const void * src, std::size_t count) {
#if __BYTE_ORDER == __BIG_ENDIAN
			std::memcpy(dest, src, count);
#elif __BYTE_ORDER == __LITTLE_ENDIAN
			memcpy_swap_bo(dest, src, count);
#endif
		}

		inline void memcpy_le(void * dest, const void * src, std::size_t count) {
#if __BYTE_ORDER == __LITTLE_ENDIAN
			std::memcpy(dest, src, count);
#elif __BYTE_ORDER == __BIG_ENDIAN
			memcpy_swap_bo(dest, src, count);
#endif
		}

		/*
		* © 2009 http://www.mdawson.net/misc/readDirectory.php
		* TODO: make custom implementation to avoid potential copyright problems
		*/
		inline std::vector<std::string> readDirectory(const std::string &directoryLocation, const std::string &start_string, const std::string &extension, bool full_path = true) {
			std::vector<std::string> result;
			std::string lcExtension(extension);
			std::transform(lcExtension.begin(), lcExtension.end(), lcExtension.begin(), ::tolower);

			DIR *dir;
			struct dirent *ent;

			if((dir = opendir(directoryLocation.c_str())) == NULL)
				throw std::runtime_error("error opening directory");

			while((ent = readdir(dir)) != NULL) {
				std::string entry(ent->d_name);
				std::string lcEntry(entry);

				std::transform(lcEntry.begin(), lcEntry.end(), lcEntry.begin(), ::tolower);

				size_t pos = lcEntry.rfind(lcExtension);
				size_t pos2 = lcEntry.find(start_string);
				if(pos != std::string::npos && pos == lcEntry.length() - lcExtension.length() && pos2 == 0) {
					if(full_path)
						result.push_back(directoryLocation + "/" +entry);
					else
						result.push_back(entry);
				}
			}

			if(closedir(dir) != 0) {
				throw std::runtime_error("error closing directory");
			}

			std::sort(result.begin(), result.end());

			return result;
		}

		namespace systemd {
			inline void ready() {
				#ifdef ENABLE_SYSTEMD_STATUS
				sd_notify(0, "READY=1");
				#endif
			}

			inline void watchdog() {
				#ifdef ENABLE_SYSTEMD_STATUS
				sd_notify(0, "WATCHDOG=1");
				#endif
			}

			inline void status(__attribute__((unused)) const std::string& status) {
				#ifdef ENABLE_SYSTEMD_STATUS
				sd_notifyf(0, "STATUS=%s", status.c_str());
				#endif
			}

			inline void error(__attribute__((unused)) int errno) {
				#ifdef ENABLE_SYSTEMD_STATUS
				sd_notifyf(0, "STATUS=%s\nERRNO=%d", strerror(errno), errno);
				#endif
			}

			class MultiWatchdog {
			public:
				MultiWatchdog();
				~MultiWatchdog();
				void trigger();
			private:
				int own_entry = 0;
				static std::vector<int*> watchdog_list;
				static std::mutex list_mtx;
			};

			inline MultiWatchdog::MultiWatchdog() {
				std::lock_guard<std::mutex> lock(list_mtx);
				watchdog_list.push_back(&own_entry);
			}

			inline MultiWatchdog::~MultiWatchdog() {
				std::lock_guard<std::mutex> lock(list_mtx);

				auto it = std::find(watchdog_list.begin(), watchdog_list.end(), &own_entry);

				if(it != watchdog_list.end())
					watchdog_list.erase(it);
			}

			inline void MultiWatchdog::trigger() {
				std::lock_guard<std::mutex> lock(list_mtx);
				own_entry = 1;

				for(const auto &e : watchdog_list) 
					if(*e == 0) return;

				watchdog();

				for(auto &e : watchdog_list)
					if(*e != -1) *e = 0;
			}
		} // namespace systemd

		class LogManager {
		public:
			explicit LogManager(const char* process_name);
			LogManager() : LogManager("") {};

			void setMaxLogLevel(int max_log_level);
			void setEcho(bool enable_echo);

			void write(int priority, const std::string& message);
			void write(const std::string& message);
			void write(int priority, const char *fmt, ...);
			void write(int priority, const char *fmt, va_list ap);
			void write(const char *fmt, ...);

			void auditlog(const char *fmt, ...);
		private:
			int max_log_level = LOG_INFO;
			const int default_log_level = LOG_INFO;
			bool enable_echo = true;
			std::string process_name;

			std::mutex mutex;
		};

		inline LogManager::LogManager(const char* process_name) {
			this->process_name = std::string(process_name);
			this->setEcho(this->enable_echo);
		}

		inline void LogManager::setMaxLogLevel(int max_log_level) {
			this->max_log_level = max_log_level;

			setlogmask(LOG_UPTO(max_log_level));
		}

		inline void LogManager::setEcho(bool enable_echo) {
			this->enable_echo = enable_echo;

			closelog();
			if(!this->enable_echo)
				openlog(this->process_name.c_str(), LOG_NDELAY | LOG_PID, LOG_DAEMON);
			else
				openlog(this->process_name.c_str(), LOG_CONS | LOG_PERROR | LOG_NDELAY | LOG_PID, LOG_DAEMON);
		}

		inline void LogManager::write(const std::string& message) {
			return this->write(this->default_log_level, message);
		}

		inline void LogManager::write(const char *fmt, ...) {
			va_list ap;
			va_start(ap, fmt);
			this->write(this->default_log_level, fmt, ap);
			va_end(ap);
		}

		inline void LogManager::write(int priority, const std::string& message) {
			return this->write(priority, "%s", message.c_str());
		}

		inline void LogManager::write(int priority, const char *fmt, ...) {
			va_list ap;
			va_start(ap, fmt);
			this->write(priority, fmt, ap);
			va_end(ap);
		}

		inline void LogManager::write(int priority, const char *fmt, va_list ap) {
			if(priority > this->max_log_level)
				return;

			this->mutex.lock();
			#ifdef ENABLE_SYSTEMD_STATUS
				if(this->enable_echo) {
					vfprintf(stdout, fmt, ap);
					if(fmt[std::strlen(fmt)-1] != '\n')
						fprintf(stdout, "\n");
				}

				sd_journal_printv(priority, fmt, ap);
			#else
				vsyslog(priority, fmt, ap);
			#endif
			this->mutex.unlock();
		}

		inline void LogManager::auditlog(const char *fmt, ...) {
			va_list ap;
			va_start(ap, fmt);
			this->write(LOG_INFO, fmt, ap);
			va_end(ap);
		}
	} // namespace system_helper
} // namespace bestsens

#endif /* SYSTEM_HELPER_HPP_ */
