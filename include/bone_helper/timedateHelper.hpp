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

#include "bone_helper/system_helper.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "fmt/chrono.h"

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

		inline auto getDate() -> std::string {
			const boost::posix_time::ptime date{boost::posix_time::second_clock::local_time()};
			return fmt::format("{:%F %T}", boost::posix_time::to_tm(date));
		}

		inline auto getTimezoneOffset() -> std::string {
			const boost::posix_time::ptime date{boost::posix_time::second_clock::local_time()};
			return fmt::format("{:%z}", boost::posix_time::to_tm(date));
		}

		inline auto getTimeDateInfo() -> timedateinfo_t {
			timedateinfo_t ti{};

			for (const auto& input : system_helper::pipeSystemCommand("timedatectl status")) {
				const std::regex tzrgx("Time zone:\\s*([a-zA-Z]+\\/[a-zA-Z]+)");
				const std::regex tsrgx("Network time on:\\s*(yes|no)");
				std::smatch match;

				if (std::regex_search(input, match, tzrgx)) {
					if (match.ready() && match.size() == 2) {
						ti.timezone = match[1];
					}
				} else if (std::regex_search(input, match, tsrgx)) {
					if (match.ready() && match.size() == 2) {
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
				const boost::posix_time::ptime current_date{boost::posix_time::second_clock::local_time()};
				std::tm tm = boost::posix_time::to_tm(current_date);

				if (strptime(date.c_str(), "%F %T", &tm) == nullptr) {
					throw std::runtime_error("could not set time: error parsing string");
				}

				const std::time_t newtime = std::mktime(&tm);
				const struct timespec newtime_ts = {newtime, 0};

				if (clock_settime(CLOCK_REALTIME, &newtime_ts) != 0) {
					throw std::runtime_error(fmt::format("could not set time: {}", strerror_s(errno)));
				}
			} else { // no root priviledges, try old sudo -n approach
				const auto cmd = std::string("sudo -n timedatectl set-time \"") + date + std::string("\"");
				const auto lines = system_helper::pipeSystemCommand(cmd);

				if (!lines.empty()) {
					throw std::runtime_error(fmt::format("could not set time: {}", lines[0]));
				}
			}
		}

		inline void setTimezone(const std::string& timezone) {
			const auto cmd = fmt::format("sudo -n timedatectl set-timezone \"{}\"", timezone);

			auto lines = system_helper::pipeSystemCommand(cmd);

			if (!lines.empty()) {
				throw std::runtime_error(fmt::format("could not set time: {}", lines[0]));
			}
		}

		inline auto getTimezone() -> std::string {
			for (const auto& input : system_helper::pipeSystemCommand("timedatectl status")) {
				const std::regex r("Time zone:\\s*([a-zA-Z]+\\/[a-zA-Z]+)");
				std::smatch match;

				if (std::regex_search(input, match, r)) {
					if (match.ready() && match.size() == 2) {
						return match[1];
					}
				}
			}

			throw std::runtime_error("could not get timezone");
		}

		inline void setTimesync(bool timesync_enabled) {
			const auto cmd = fmt::format("sudo -n timedatectl set-ntp {}", timesync_enabled ? "true" : "false");

			auto lines = system_helper::pipeSystemCommand(cmd);

			if (!lines.empty()) {
				throw std::runtime_error(fmt::format("could not set time: {}", lines[0]));
			}
		}

		inline auto getTimesync() -> bool {
			for (const auto& input : system_helper::pipeSystemCommand("timedatectl status")) {
				const std::regex r("Network time on:\\s*(yes|no)");
				std::smatch match;

				if (std::regex_search(input, match, r)) {
					if (match.ready() && match.size() == 2) {
						return match[1].compare("yes") == 0;
					}
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
