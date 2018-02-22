/*
 * jsonHelper.hpp
 *
 *  Created on: 13.10.2016
 *      Author: Jan Schöppach
 */

#ifndef JSONHELPER_HPP_
#define JSONHELPER_HPP_

#include <syslog.h>
#include "../json/single_include/nlohmann/json.hpp"

using json = nlohmann::json;

namespace bestsens {
	[[deprecated("use json::update instead")]]
	inline json merge_json(const json &a, const json &b) {
		try {
			json result = a.flatten();
			json tmp = b.flatten();

			for(json::iterator it = tmp.begin(); it != tmp.end(); ++it)
				result[it.key()] = it.value();

			return result.unflatten();
		}
		catch(const json::exception& ia) {
			syslog(LOG_WARNING, "error merging json: %s", ia.what());
		}

		json return_value = a;
		if(!return_value.is_object() && !return_value.is_array())
			return_value = b;

		return return_value;
	}

	inline bool is_json_number(const json& input, std::string key) {
		return (input != NULL && input.count(key) > 0 && input.at(key).is_number());
	}

	inline bool is_json_array(const json& input, std::string key) {
		return (input != NULL && input.count(key) > 0 && input.at(key).is_array());
	}

	inline bool is_json_string(const json& input, std::string key) {
		return (input != NULL && input.count(key) > 0 && input.at(key).is_string());
	}

	inline bool is_json_bool(const json& input, std::string key) {
		return (input != NULL && input.count(key) > 0 && input.at(key).is_boolean());
	}

	inline bool is_json_object(const json& input, std::string key) {
		return (input != NULL && input.count(key) > 0 && input.at(key).is_object());
	}

	inline bool is_json_node(const json& input, std::string key) {
		return (input != NULL && input.count(key) > 0 && !input.at(key).is_null());
	}

	inline bool checkedUpdateFromJSON(const json& j, const std::string& name, int& value) {
		if(is_json_number(j, name))
			value = j[name];
		else
			return false;

		return true;
	}

	inline bool checkedUpdateFromJSON(const json& j, const std::string& name, int& value, const int default_value) {
		if(!checkedUpdateFromJSON(j, name, value)) {
			value = default_value;
			return false;
		}

		return true;
	}

	inline bool checkedUpdateFromJSON(const json& j, const std::string& name, double& value) {
		if(is_json_number(j, name))
			value = j[name];
		else
			return false;

		return true;
	}

	inline bool checkedUpdateFromJSON(const json& j, const std::string& name, double& value, const double default_value) {
		if(!checkedUpdateFromJSON(j, name, value)) {
			value = default_value;
			return false;
		}

		return true;
	}

	inline bool checkedUpdateFromJSON(const json& j, const std::string& name, bool& value) {
		if(is_json_bool(j, name))
			value = j[name];
		else
			return false;

		return true;
	}

	inline bool checkedUpdateFromJSON(const json& j, const std::string& name, bool& value, const bool default_value) {
		if(!checkedUpdateFromJSON(j, name, value)) {
			value = default_value;
			return false;
		}

		return true;
	}

	inline bool checkedUpdateFromJSON(const json& j, const std::string& name, std::string& value) {
		if(is_json_string(j, name))
			value = j[name];
		else
			return false;

		return true;
	}

	inline bool checkedUpdateFromJSON(const json& j, const std::string& name, std::string& value, const std::string default_value) {
		if(!checkedUpdateFromJSON(j, name, value)) {
			value = default_value;
			return false;
		}

		return true;
	}
} //namespace bestsens

#endif
