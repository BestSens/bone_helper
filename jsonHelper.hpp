/*
 * jsonHelper.hpp
 *
 *  Created on: 13.10.2016
 *      Author: Jan Schöppach
 */

#ifndef JSONHELPER_HPP_
#define JSONHELPER_HPP_

#include "../json/src/json.hpp"

namespace bestsens {
    inline bool is_json_number(const json& input, std::string key) {
        return (input != NULL && input.count(key) > 0 && input.at(key).is_number());
    }

    inline bool is_json_array(const json& input, std::string key) {
        return (input != NULL && input.count(key) > 0 && input.at(key).is_array());
    }

    inline bool is_json_string(const json& input, std::string key) {
        return (input != NULL && input.count(key) > 0 && input.at(key).is_string());
    }

    inline bool is_json_object(const json& input, std::string key) {
        return (input != NULL && input.count(key) > 0 && input.at(key).is_object());
    }

    inline bool is_json_node(const json& input, std::string key) {
        return (input != NULL && input.count(key) > 0 && !input.at(key).is_null());
    }
}

#endif
