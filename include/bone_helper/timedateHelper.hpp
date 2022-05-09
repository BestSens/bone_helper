/*
 * timedateHelper.hpp
 *
 *  Created on: 23.02.2017
 *	  Author: Jan Schöppach
 */

#ifndef TIMEDATEHELPER_HPP_
#define TIMEDATEHELPER_HPP_

#include <sys/types.h>
#include <unistd.h>

#include <array>
#include <cstring>
#include <ctime>
#include <iostream>
#include <memory>
#include <mutex>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#ifdef ENABLE_SYSTEMD_DBUS
#include <systemd/sd-bus.h>
#endif

namespace bestsens {
	namespace timedateHelper {
		struct timedateinfo_t {
			std::string date;
			std::string timezone;
			std::string timezone_offset;
			std::string timeservers;
			bool timesync_enabled;
		};

		inline std::vector<std::string> pipeSystemCommand(const std::string& command) {
			std::string cmd = command + " 2>&1";
			std::array<char, 128> line;
			std::vector<std::string> lines;
			std::shared_ptr<FILE> pipe(popen(cmd.c_str(), "r"), pclose);

			if(!pipe) throw std::runtime_error("popen() failed!");

			while(fgets(line.data(), 128, pipe.get()) != NULL)
			{
				lines.push_back(std::string(line.data()));
			}

			if(ferror(pipe.get()))
			{
				throw std::runtime_error(strerror(errno));
			}

			return lines;
		}

		inline std::string getDate() {
			std::time_t rawtime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
			std::tm tm = *std::localtime(&rawtime);

			char mbstr[20];
			std::strftime(mbstr, sizeof(mbstr), "%F %T", &tm);

			return std::string(mbstr);
		}

		inline std::string getTimezoneOffset() {
			std::time_t rawtime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
			std::tm tm = *std::localtime(&rawtime);

			char mbstr[6];
			std::strftime(mbstr, sizeof(mbstr), "%z", &tm);

			return std::string(mbstr);
		}

		inline timedateinfo_t getTimeDateInfo() {
			timedateinfo_t ti;

			for(auto input : pipeSystemCommand("timedatectl status")) {
				std::regex tzrgx("Time zone:\\s*([a-zA-Z]+\\/[a-zA-Z]+)");
				std::regex tsrgx("Network time on:\\s*(yes|no)");
				std::smatch match;

				if(std::regex_search(input, match, tzrgx)) {
					if(match.ready() && match.size() == 2)
						ti.timezone = match[1];
				} else if(std::regex_search(input, match, tsrgx)) {
					if(match.ready() && match.size() == 2) {
						ti.timesync_enabled = match[1].compare("yes") == 0;
					}
				}
			}

			ti.date = getDate();
			ti.timezone_offset = getTimezoneOffset();
			return ti;
		}

		inline void setDate(const std::string& date) {
			if (geteuid() == 0) { 
				const std::time_t rawtime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
				std::tm tm = *std::localtime(&rawtime);

				if (strptime(date.c_str(), "%F %T", &tm) == NULL) {
					std::string error = std::string("could not set time: error parsing string");
					throw std::runtime_error(error);
				}

				const std::time_t newtime = std::mktime(&tm);
				const struct timespec newtime_ts = {newtime, 0};

				if (clock_settime(CLOCK_REALTIME, &newtime_ts) != 0) {
					std::string error = std::string("could not set time: ") + strerror(errno);
					throw std::runtime_error(error);
				}
			} else { // no root priviledges, try old sudo -n approach
				std::string cmd = std::string("sudo -n timedatectl set-time \"") + date + std::string("\"");

				auto lines = pipeSystemCommand(cmd);

				if (lines.size() > 0) {
					std::string error = std::string("could not set time: ") + lines[0];
					throw std::runtime_error(error);
				}
			}
		}

		inline void setTimezone(const std::string& timezone) {
			std::string cmd = std::string("sudo -n timedatectl set-timezone \"") + timezone + std::string("\"");

			auto lines = pipeSystemCommand(cmd);

			if(lines.size() > 0) {
				std::string error = std::string("could not set timezone: ") + lines[0];
				throw std::runtime_error(error);
			}
		}

		inline std::string getTimezone() {
			for(auto input : pipeSystemCommand("timedatectl status")) {
				std::regex r("Time zone:\\s*([a-zA-Z]+\\/[a-zA-Z]+)");
				std::smatch match;

				if(std::regex_search(input, match, r))
					if(match.ready() && match.size() == 2)
						return match[1];
			}

			throw std::runtime_error("could not get timezone");

			return "";
		}

		inline void setTimesync(bool timesync_enabled) {
			std::string cmd = std::string("sudo -n timedatectl set-ntp ") + (timesync_enabled ? "true" : "false");

			auto lines = pipeSystemCommand(cmd);

			if(lines.size() > 0) {
				std::string error = std::string("could not set timeync: ") + lines[0];
				throw std::runtime_error(error);
			}
		}

		inline bool getTimesync() {
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

			throw std::runtime_error("could not get timesync status");
		}

#ifdef ENABLE_SYSTEMD_DBUS
		inline void setTimezone(sd_bus * bus, const std::string& timezone) {
			if(geteuid() == 0) {
				sd_bus_message * msg = NULL;
				sd_bus_error error = SD_BUS_ERROR_NULL;

				try {
					int r = sd_bus_call_method(bus,
							"org.freedesktop.timedate1",
							"/org/freedesktop/timedate1",
							"org.freedesktop.timedate1",
							"SetTimezone",
							&error,
							&msg,
							"sb",
							timezone.c_str(),
							false);

					if(r < 0) {
						throw std::runtime_error(std::string(error.message));
					}
				} catch(const std::exception& e) {
					sd_bus_error_free(&error);
					free(msg);

					throw;
				}

				sd_bus_error_free(&error);
				free(msg);
			} else {
				setTimezone(timezone);
			}
		}

		inline void setTimesync(sd_bus * bus, bool timesync_enabled) {
			if(geteuid() == 0) {
				sd_bus_message * msg = NULL;
				sd_bus_error error = SD_BUS_ERROR_NULL;

				try {
					int r = sd_bus_call_method(bus,
							"org.freedesktop.timedate1",
							"/org/freedesktop/timedate1",
							"org.freedesktop.timedate1",
							"SetNTP",
							&error,
							&msg,
							"bb",
							timesync_enabled,
							false);

					if(r < 0) {
						throw std::runtime_error(std::string(error.message));
					}
				} catch(const std::exception& e) {
					sd_bus_error_free(&error);
					free(msg);

					throw;
				}

				sd_bus_error_free(&error);
				free(msg);
			} else {
				setTimesync(timesync_enabled);
			}
		}


		inline std::string getTimezone(sd_bus * bus) {
			char * msg = 0;
			sd_bus_error error = SD_BUS_ERROR_NULL;
			std::string timezone = "";
			
			try {
				int r = sd_bus_get_property_string(bus,
						"org.freedesktop.timedate1",
						"/org/freedesktop/timedate1",
						"org.freedesktop.timedate1",
						"Timezone",
						&error,
						&msg);

				if(r < 0) {
					throw std::runtime_error(std::string(error.message));
				}

				timezone = std::string(msg);
			} catch(const std::exception& e) {
				sd_bus_error_free(&error);
				free(msg);

				throw;
			}

			sd_bus_error_free(&error);
			free(msg);

			return timezone;
		}

		inline bool getTimesync(sd_bus * bus) {
			sd_bus_message * msg = NULL;
			sd_bus_error error = SD_BUS_ERROR_NULL;
			int timesync;
			
			try {
				int r = sd_bus_get_property(bus,
						"org.freedesktop.timedate1",
						"/org/freedesktop/timedate1",
						"org.freedesktop.timedate1",
						"NTP",
						&error,
						&msg,
						"b");

				if(r < 0) {
					throw std::runtime_error(std::string(error.message));
				}

				r = sd_bus_message_read(msg, "b", &timesync);
				if(r < 0) {
					std::string err = std::string("error getting property: ") + std::string(strerror(-r));
					throw std::runtime_error(err);
				}
			} catch(const std::exception& e) {
				sd_bus_error_free(&error);
				sd_bus_message_unref(msg);
				msg = NULL;

				throw;
			}

			sd_bus_error_free(&error);
			sd_bus_message_unref(msg);
			msg = NULL;
			return timesync == 1;
		}

		inline timedateinfo_t getTimeDateInfo(sd_bus * bus) {
			timedateinfo_t ti{};
			ti.timezone = getTimezone(bus);
			ti.timesync = getTimesync(bus);
			ti.date = getDate();
			ti.timezone_offset = getTimezoneOffset();
			return ti;
		}
#endif
	} // namespace timedateHelper
} // namespace bestsens

#endif /* TIMEDATEHELPER_HPP_ */
