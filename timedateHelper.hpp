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
			for(auto input : pipeSystemCommand("timedatectl status")) {
				std::regex r("Local.*([0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2})");
				std::smatch match;

				if(std::regex_search(input, match, r))
					if(match.ready() && match.size() == 2)
						return match[1];
			}

			throw std::runtime_error("could not get current time");

			return "";
		}

		void setDate(const std::string& date) {
			std::string cmd = std::string("sudo -n timedatectl set-time \"") + date + std::string("\"");

			auto lines = pipeSystemCommand(cmd);

			if(lines.size() > 0) {
				std::string error = std::string("could not set time: ") + lines[0];
				throw std::runtime_error(error);
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

		void setTimesync(bool timesync_enabled) {
			std::string cmd = std::string("sudo -n timedatectl set-ntp ") + (timesync_enabled ? "true" : "false");

			auto lines = pipeSystemCommand(cmd);

			if(lines.size() > 0) {
				std::string error = std::string("could not set timeync: ") + lines[0];
				throw std::runtime_error(error);
			}
		}

		bool getTimesync() {
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

			return "";
		}
	} // namespace timedateHelper
} // namespace bestsens

#endif /* TIMEDATEHELPER_HPP_ */
