/*
 * stdlib_backports.cpp
 *
 *  Created on: 17.09.2021
 *      Author: Jan Schöppach
 */

#include "bone_helper/stdlib_backports.hpp"

#include <string>

namespace backports {
    auto endsWith(const std::string& str, const std::string& suffix) -> bool {
        return str.size() >= suffix.size() && 0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
    }

    auto startsWith(const std::string& str, const std::string& prefix) -> bool {
        return str.size() >= prefix.size() && 0 == str.compare(0, prefix.size(), prefix);
    }

    auto endsWith(const std::string& str, const char* suffix, unsigned suffix_len) -> bool {
        return str.size() >= suffix_len && 0 == str.compare(str.size() - suffix_len, suffix_len, suffix, suffix_len);
    }

    auto endsWith(const std::string& str, const char* suffix) -> bool {
        return endsWith(str, suffix, std::string::traits_type::length(suffix));
    }

    auto startsWith(const std::string& str, const char* prefix, unsigned prefix_len) -> bool {
        return str.size() >= prefix_len && 0 == str.compare(0, prefix_len, prefix, prefix_len);
    }

    auto startsWith(const std::string& str, const char* prefix) -> bool {
        return startsWith(str, prefix, std::string::traits_type::length(prefix));
    }
}