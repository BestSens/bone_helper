/*
 * system_helper.hpp
 *
 *  Created on: 16.08.2017
 *      Author: Jan Schöppach
 */

#ifndef SYSTEM_HELPER_HPP_
#define SYSTEM_HELPER_HPP_

#include <unistd.h>
#include <string>
#include <vector>
#include <dirent.h>

#ifdef ENABLE_SYSTEMD_STATUS
#include <systemd/sd-daemon.h>
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

        /*
        * © 2009 http://www.mdawson.net/misc/readDirectory.php
        * TODO: make custom implementation to avoid potential copyright problems
        */
        inline std::vector<std::string> readDirectory(const std::string &directoryLocation, const std::string &start_string, const std::string &extension) {
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
        			result.push_back(directoryLocation + "/" +entry);
        		}
        	}

        	if(closedir(dir) != 0) {
        		throw std::runtime_error("error closing directory");
        	}

            std::sort(result.begin(), result.end());

        	return result;
        }
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
    }
}

#endif /* SYSTEM_HELPER_HPP_ */
