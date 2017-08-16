/*
 * system_helper.hpp
 *
 *  Created on: 16.08.2017
 *      Author: Jan Schöppach
 */

#ifndef SYSTEM_HELPER_HPP_
#define SYSTEM_HELPER_HPP_

#include <unistd.h>

namespace bestsens {
    namespace system_helper {
        void daemonize() {
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
    }
}

#endif /* SYSTEM_HELPER_HPP_ */
