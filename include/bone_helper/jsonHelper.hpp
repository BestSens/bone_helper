/*
 * jsonHelper.hpp
 *
 *  Created on: 13.10.2016
 *      Author: Jan Schöppach
 */

#ifndef JSONHELPER_HPP_
#define JSONHELPER_HPP_

#include <syslog.h>
#include "nlohmann/json.hpp"

#define JSON_ARITHMETIC_ADD [](double a, double b){ return a + b; }
#define JSON_ARITHMETIC_SUB [](double a, double b){ return a - b; }
#define JSON_ARITHMETIC_MUL [](double a, double b){ return a * b; }
#define JSON_ARITHMETIC_DIV [](double a, double b){ try { return a / b; } catch(...){} return a; }

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

	// Modified JSON Merge Patch (RFC 7396 - section 2)
	// to combine two json objects arithmetically
	//
	// e. g. from = { "data": 1 } and to = { "data": 2 } with operation ADD
	// leads to { "data": 3 }
	template<typename Function>
	static json arithmetic_merge_json(const json& from, json to, Function operation)
	{
		if(from.is_structured())
		{
			if(from.type() != to.type())
			{
				return to; // for type incompatibility just return old value
			}
			
			int i = 0;
			for(json::const_iterator it = from.begin(); it != from.end(); ++it, ++i)
			{
				if(from.is_object())
				{
					if(!it.value().is_null())
					{
						to[it.key()] = arithmetic_merge_json(it.value(), to[it.key()], operation);
					}
				}
				else // is array
				{
					if(i >= static_cast<int>(to.size()))
						break;
					
					to[i] = arithmetic_merge_json(it.value(), to[i], operation);
				}
			}
			return to;
		}
		
		// combine from and to if they are numbers
		if(from.is_number() && to.is_number())
		{
			double a = from.get<double>();
			double b = to.get<double>();

			return json(operation(a, b));
		}
		return to; // from is neither a number nor an object, return old value
	}
	
	inline json arithmetic_add_json(const json& from, const json& to)
	{
		return arithmetic_merge_json(from, to, JSON_ARITHMETIC_ADD);
	}
	
	inline json arithmetic_sub_json(const json& from, const json& to)
	{
		return arithmetic_merge_json(from, to, JSON_ARITHMETIC_SUB);
	}
	
	inline json arithmetic_mul_json(const json& from, const json& to)
	{
		return arithmetic_merge_json(from, to, JSON_ARITHMETIC_MUL);
	}
	
	inline json arithmetic_div_json(const json& from, const json& to)
	{
		return arithmetic_merge_json(from, to, JSON_ARITHMETIC_DIV);
	}

	template <typename T>
	inline T value_ig_type(const json& input, const std::string& key, const T& default_value) {
		try {
			return input.value(key, default_value);
		} catch(const json::type_error& e) {
			return default_value;
		}
	}

	inline bool is_json_number(const json& input, const std::string& key) {
		return (input.contains(key) && input.at(key).is_number());
	}

	inline bool is_json_array(const json& input, const std::string& key) {
		return (input.contains(key) && input.at(key).is_array());
	}

	inline bool is_json_string(const json& input, const std::string& key) {
		return (input.contains(key) && input.at(key).is_string());
	}

	inline bool is_json_bool(const json& input, const std::string& key) {
		return (input.contains(key) && input.at(key).is_boolean());
	}

	inline bool is_json_object(const json& input, const std::string& key) {
		return (input.contains(key) && input.at(key).is_object());
	}

	inline bool is_json_node(const json& input, const std::string& key) {
		return (input.contains(key) && !input.at(key).is_null());
	}

	inline bool checkedUpdateFromJSON(const json& j, const std::string& name, int& value) {
		if(is_json_number(j, name))
			value = j.at(name).get<int>();
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
			value = j.at(name).get<double>();
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
			value = j.at(name).get<bool>();
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
			value = j.at(name).get<std::string>();
		else
			return false;

		return true;
	}

	inline bool checkedUpdateFromJSON(const json& j, const std::string& name, std::string& value,
									  const std::string& default_value) {
		if (!checkedUpdateFromJSON(j, name, value)) {
			value = default_value;
			return false;
		}

		return true;
	}

	inline json get_filtered_values(const json& j, const std::vector<std::string>& filter) {
		if(filter.size() == 0)
			return j;

		json result;

		for(auto& e : j.items()) {
			auto it = std::find(filter.begin(), filter.end(), e.key());

			if(it != filter.end())
				result[e.key()] = e.value();
		}

		return result;
	}
} //namespace bestsens

#endif
