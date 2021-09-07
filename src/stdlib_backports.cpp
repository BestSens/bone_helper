/*
 * stdlib_backports.cpp
 *
 *  Created on: 17.09.2021
 *      Author: Jan Schöppach
 */

#include "bone_helper/stdlib_backports.hpp"

#include <string>

namespace backports {
    bool endsWith(const std::string& str, const std::string& suffix) {
        return str.size() >= suffix.size() && 0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
    }

    bool startsWith(const std::string& str, const std::string& prefix) {
        return str.size() >= prefix.size() && 0 == str.compare(0, prefix.size(), prefix);
    }

    bool endsWith(const std::string& str, const char* suffix, unsigned suffixLen) {
        return str.size() >= suffixLen && 0 == str.compare(str.size() - suffixLen, suffixLen, suffix, suffixLen);
    }

    bool endsWith(const std::string& str, const char* suffix) {
        return endsWith(str, suffix, std::string::traits_type::length(suffix));
    }

    bool startsWith(const std::string& str, const char* prefix, unsigned prefixLen) {
        return str.size() >= prefixLen && 0 == str.compare(0, prefixLen, prefix, prefixLen);
    }

    bool startsWith(const std::string& str, const char* prefix) {
        return startsWith(str, prefix, std::string::traits_type::length(prefix));
    }
}