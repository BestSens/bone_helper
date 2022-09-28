/*
 * jsonHelper.hpp
 *
 *  Created on: 13.10.2016
 *      Author: Jan Schöppach
 */

#ifndef JSONHELPER_HPP_
#define JSONHELPER_HPP_

#include <syslog.h>

#include <string>

#include "nlohmann/json.hpp"

namespace bestsens {
	[[deprecated("use json::update instead")]]
	inline auto merge_json(const nlohmann::json &a, const nlohmann::json &b) -> nlohmann::json {
		try {
			nlohmann::json result = a.flatten();
			nlohmann::json tmp = b.flatten();

			for (auto it = tmp.begin(); it != tmp.end(); ++it) {
				result[it.key()] = it.value();
			}

			return result.unflatten();
		} catch (const nlohmann::json::exception& ia) {
			syslog(LOG_WARNING, "error merging json: %s", ia.what());
		}

		nlohmann::json return_value = a;
		if (!return_value.is_object() && !return_value.is_array()) {
			return_value = b;
		}

		return return_value;
	}

	// Modified JSON Merge Patch (RFC 7396 - section 2)
	// to combine two json objects arithmetically
	//
	// e. g. from = { "data": 1 } and to = { "data": 2 } with operation ADD
	// leads to { "data": 3 }
	template <typename Function>
	static auto arithmetic_merge_json(const nlohmann::json& from, nlohmann::json to, Function operation)
		-> nlohmann::json {
		if (from.is_structured()) {
			if (from.type() != to.type()) {
				return to;	// for type incompatibility just return old value
			}

			size_t i{0};
			for (auto it = from.cbegin(); it != from.cend(); ++it, ++i) {
				if (from.is_object()) {
					if (!it.value().is_null()) {
						to[it.key()] = arithmetic_merge_json(it.value(), to.at(it.key()), operation);
					}
				} else {
					if (i >= to.size()) {
						break;
					}

					to[i] = arithmetic_merge_json(it.value(), to.at(i), operation);
				}
			}

			return to;
		}

		// combine from and to if they are numbers
		if (from.is_number() && to.is_number()) {
			if (from.is_number_integer() && to.is_number_integer()) {
				const auto a = from.get<long>();
				const auto b = to.get<long>();

				return nlohmann::json(operation(a, b));
			} else {
				const auto a = from.get<double>();
				const auto b = to.get<double>();

				return nlohmann::json(operation(a, b));
			}
		}

		return to;	// from is neither a number nor an object, return old value
	}

	inline auto arithmetic_add_json(const nlohmann::json& from, const nlohmann::json& to) -> nlohmann::json {
		return arithmetic_merge_json(from, to, [](auto a, auto b) { return a + b; });
	}

	inline auto arithmetic_sub_json(const nlohmann::json& from, const nlohmann::json& to) -> nlohmann::json {
		return arithmetic_merge_json(from, to, [](auto a, auto b) { return a - b; });
	}

	inline auto arithmetic_mul_json(const nlohmann::json& from, const nlohmann::json& to) -> nlohmann::json {
		return arithmetic_merge_json(from, to, [](auto a, auto b) { return a * b; });
	}

	inline auto arithmetic_div_json(const nlohmann::json& from, const nlohmann::json& to) -> nlohmann::json {
		return arithmetic_merge_json(from, to, [](double a, double b) {
			if (b == 0) {
				return a;
			}
			
			try {
				return a / b;
			} catch (...) {}
			return a;
		});
	}

	template <typename keytype, typename T>
	auto value_ig_type(const nlohmann::json& input, const keytype& key, const T& default_value) -> T {
		try {
			return input.value(key, default_value);
		} catch (const nlohmann::json::type_error& e) {
			return default_value;
		}
	}

	template <typename keytype>
	auto is_json_number(const nlohmann::json& input, const keytype& key) -> bool {
		return (input.contains(key) && input.at(key).is_number());
	}

	template <typename keytype>
	auto is_json_array(const nlohmann::json& input, const keytype& key) -> bool {
		return (input.contains(key) && input.at(key).is_array());
	}

	template <typename keytype>
	auto is_json_string(const nlohmann::json& input, const keytype& key) -> bool {
		return (input.contains(key) && input.at(key).is_string());
	}

	template <typename keytype>
	auto is_json_bool(const nlohmann::json& input, const keytype& key) -> bool {
		return (input.contains(key) && input.at(key).is_boolean());
	}

	template <typename keytype>
	auto is_json_object(const nlohmann::json& input, const keytype& key) -> bool {
		return (input.contains(key) && input.at(key).is_object());
	}

	template <typename keytype>
	auto is_json_node(const nlohmann::json& input, const keytype& key) -> bool {
		return (input.contains(key) && !input.at(key).is_null());
	}

	template <typename keytype>
	auto checkedUpdateFromJSON(const nlohmann::json& j, const keytype& name, int& value) -> bool {
		if (is_json_number(j, name)) {
			value = j.at(name).template get<int>();
		} else {
			return false;
		}

		return true;
	}

	template <typename keytype>
	auto checkedUpdateFromJSON(const nlohmann::json& j, const keytype& name, int& value, const int default_value)
		-> bool {
		if (!checkedUpdateFromJSON(j, name, value)) {
			value = default_value;
			return false;
		}

		return true;
	}

	template <typename keytype>
	auto checkedUpdateFromJSON(const nlohmann::json& j, const keytype& name, double& value) -> bool {
		if (is_json_number(j, name)) {
			value = j.at(name).template get<double>();
		} else {
			return false;
		}

		return true;
	}

	template <typename keytype>
	auto checkedUpdateFromJSON(const nlohmann::json& j, const keytype& name, double& value, const double default_value)
		-> bool {
		if (!checkedUpdateFromJSON(j, name, value)) {
			value = default_value;
			return false;
		}

		return true;
	}

	template <typename keytype>
	auto checkedUpdateFromJSON(const nlohmann::json& j, const keytype& name, bool& value) -> bool {
		if (is_json_bool(j, name)) {
			value = j.at(name).template get<bool>();
		} else {
			return false;
		}

		return true;
	}

	template <typename keytype>
	auto checkedUpdateFromJSON(const nlohmann::json& j, const keytype& name, bool& value, const bool default_value)
		-> bool {
		if (!checkedUpdateFromJSON(j, name, value)) {
			value = default_value;
			return false;
		}

		return true;
	}

	template <typename keytype>
	auto checkedUpdateFromJSON(const nlohmann::json& j, const keytype& name, std::string& value) -> bool {
		if (is_json_string(j, name)) {
			value = j.at(name).template get<std::string>();
		} else {
			return false;
		}

		return true;
	}

	template <typename keytype>
	auto checkedUpdateFromJSON(const nlohmann::json& j, const keytype& name, std::string& value,
									  const std::string& default_value) -> bool {
		if (!checkedUpdateFromJSON(j, name, value)) {
			value = default_value;
			return false;
		}

		return true;
	}

	inline auto get_filtered_values(const nlohmann::json& j, const std::vector<std::string>& filter) -> nlohmann::json {
		if (filter.empty()) {
			return j;
		}

		nlohmann::json result;

		for (const auto& e : j.items()) {
			const auto it = std::find(filter.cbegin(), filter.cend(), e.key());

			if (it != filter.end()) {
				result[e.key()] = e.value();
			}
		}

		return result;
	}
} //namespace bestsens

#endif
