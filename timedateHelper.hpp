/*
 * timedateHelper.hpp
 *
 *  Created on: 23.02.2017
 *	  Author: Jan Schöppach
 */

#ifndef TIMEDATEHELPER_HPP_
#define TIMEDATEHELPER_HPP_

#include <string>
#include <vector>
#include <sstream>
#include <memory>
#include <regex>
#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <memory>

#ifdef ENABLE_SYSTEMD_STATUS
#include <systemd/sd-bus.h>
#endif

namespace bestsens {
	namespace timedateHelper {
		std::vector<std::string> pipeSystemCommand(const std::string& command) {
			std::string cmd = command + " 2>&1";
			std::array<char, 128> line;
			std::vector<std::string> lines;
			std::shared_ptr<FILE> pipe(popen(cmd.c_str(), "r"), pclose);

			if(!pipe) throw std::runtime_error("popen() failed!");

			while(!feof(pipe.get())) {
				if(fgets(line.data(), 128, pipe.get()) != NULL)
					lines.push_back(std::string(line.data()));
			}

			return lines;
		}

		/*
		 * SOURCE: http://stackoverflow.com/a/2275160/481329
		 */
		std::vector<std::string> split(std::string const &input) {
			std::istringstream buffer(input);
			std::vector<std::string> ret((std::istream_iterator<std::string>(buffer)), std::istream_iterator<std::string>());
			return ret;
		}

		bool hasEnding(std::string const &fullString, std::string const &ending) {
			if (fullString.length() >= ending.length()) {
				return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
			} else {
				return false;
			}
		}

		std::string getDate() {
			std::time_t rawtime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
			std::tm tm = *std::localtime(&rawtime);

			char mbstr[20];
			std::strftime(mbstr, sizeof(mbstr), "%F %T", &tm);

			return std::string(mbstr);
		}

		void setDate(const std::string& date) {
			if(geteuid() == 0) { 
				std::time_t rawtime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
				std::tm tm = *std::localtime(&rawtime);

				if(strptime(date.c_str(), "%F %T", &tm) == NULL) {
					std::string error = std::string("could not set time: error parsing string");
					throw std::runtime_error(error);
				}

				std::time_t newtime = std::mktime(&tm); 
				if(stime(&newtime) != 0) {
					std::string error = std::string("could not set time: ") + strerror(errno);
					throw std::runtime_error(error);
				}
			} else { // no root priviledges, try old sudo -n approach
				std::string cmd = std::string("sudo -n timedatectl set-time \"") + date + std::string("\"");

				auto lines = pipeSystemCommand(cmd);

				if(lines.size() > 0) {
					std::string error = std::string("could not set time: ") + lines[0];
					throw std::runtime_error(error);
				}
			}
		}

		void setTimezone(const std::string& timezone) {
			std::string cmd = std::string("sudo -n timedatectl set-timezone \"") + timezone + std::string("\"");

			auto lines = pipeSystemCommand(cmd);

			if(lines.size() > 0) {
				std::string error = std::string("could not set timezone: ") + lines[0];
				throw std::runtime_error(error);
			}
		}

		std::string getTimezone() {
#ifdef ENABLE_SYSTEMD_STATUS
			std::string timezone = "";

			sd_bus_error error = SD_BUS_ERROR_NULL;
			sd_bus * bus = NULL;
			char * msg = 0;
			
			try {
				int r;

				r = sd_bus_open_system(&bus);
				if(r < 0) 
					throw std::runtime_error("failed to connect to system bus");

				r = sd_bus_get_property_string(bus,
						"org.freedesktop.timedate1",
						"/org/freedesktop/timedate1",
						"org.freedesktop.timedate1",
						"Timezone",
						&error,
						&msg);

				if(r < 0) {
					std::string err = std::string("error getting property: ", error.message);
					throw std::runtime_error(err);
				}

				timezone = std::string(msg);
			} catch(const std::exception& e) {
				sd_bus_error_free(&error);
				sd_bus_unref(bus);
				free(msg);

				throw;
			}

			sd_bus_error_free(&error);
			sd_bus_unref(bus);
			free(msg);

			return timezone;
#else
			std::time_t rawtime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
			std::tm tm = *std::localtime(&rawtime);

			char mbstr[10];
			std::strftime(mbstr, sizeof(mbstr), "%Z", &tm);

			return std::string(mbstr);
#endif
		}

		std::string getTimezoneOffset() {
			std::time_t rawtime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
			std::tm tm = *std::localtime(&rawtime);

			char mbstr[6];
			std::strftime(mbstr, sizeof(mbstr), "%z", &tm);

			return std::string(mbstr);
		}

		void setTimesync(bool timesync_enabled) {
			std::string cmd = std::string("sudo -n timedatectl set-ntp ") + (timesync_enabled ? "true" : "false");

			auto lines = pipeSystemCommand(cmd);

			if(lines.size() > 0) {
				std::string error = std::string("could not set timeync: ") + lines[0];
				throw std::runtime_error(error);
			}
		}

		bool getTimesync() {
#ifdef ENABLE_SYSTEMD_STATUS
			sd_bus_error error = SD_BUS_ERROR_NULL;
			sd_bus * bus = NULL;
			sd_bus_message * msg = NULL;
			int timesync;
			
			try {
				int r;

				r = sd_bus_open_system(&bus);
				if(r < 0) 
					throw std::runtime_error("failed to connect to system bus");

				r = sd_bus_get_property(bus,
						"org.freedesktop.timedate1",
						"/org/freedesktop/timedate1",
						"org.freedesktop.timedate1",
						"NTP",
						&error,
						&msg,
						"b");

				if(r < 0) {
					std::string err = std::string("error getting property: ", error.message);
					throw std::runtime_error(err);
				}

				r = sd_bus_message_read(msg, "b", &timesync);

				if(r < 0)
					throw std::runtime_error("error getting property");
			} catch(const std::exception& e) {
				sd_bus_error_free(&error);
				sd_bus_unref(bus);
				sd_bus_message_unref(msg);
				msg = NULL;

				throw;
			}

			sd_bus_error_free(&error);
			sd_bus_unref(bus);
			sd_bus_message_unref(msg);
			msg = NULL;

			return timesync == 1;
#else
			for(auto input : pipeSystemCommand("timedatectl status")) {
				std::regex r("Network time on:\\s*(yes|no)");
				std::smatch match;

				if(std::regex_search(input, match, r))
					if(match.ready() && match.size() == 2) {
						if(match[1].compare("yes") == 0)
							return true;
						else
							return false;
					}
			}
#endif
			throw std::runtime_error("could not get timesync status");
			return false;
		}
	} // namespace timedateHelper
} // namespace bestsens

#endif /* TIMEDATEHELPER_HPP_ */
