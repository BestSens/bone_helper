/*
 * stdlib_backports.hpp
 *
 *  Created on: 10.09.2021
 *      Author: Jan Schöppach
 */

#ifndef CPP_BACKPORTS_HPP_
#define CPP_BACKPORTS_HPP_

#include <type_traits>
#include <string>

namespace backports {
	template <class T, class U>
	auto cmp_equal(T t, U u) noexcept -> bool {
		using UT = std::make_unsigned_t<T>;
		using UU = std::make_unsigned_t<U>;
		if (std::is_signed<T>::value == std::is_signed<U>::value)
			return t == u;
		else if (std::is_signed<T>::value)
			return t < 0 ? false : UT(t) == u;
		else
			return u < 0 ? false : t == UU(u);
	}

	template <class T, class U>
	auto cmp_not_equal(T t, U u) noexcept -> bool {
		return !cmp_equal(t, u);
	}

	template <class T, class U>
	auto cmp_less(T t, U u) noexcept -> bool {
		using UT = std::make_unsigned_t<T>;
		using UU = std::make_unsigned_t<U>;
		if (std::is_signed<T>::value == std::is_signed<U>::value)
			return t < u;
		else if (std::is_signed<T>::value)
			return t < 0 ? true : UT(t) < u;
		else
			return u < 0 ? false : t < UU(u);
	}

	template <class T, class U>
	auto cmp_greater(T t, U u) noexcept -> bool {
		return cmp_less(u, t);
	}

	template <class T, class U>
	auto cmp_less_equal(T t, U u) noexcept -> bool {
		return !cmp_greater(t, u);
	}

	template <class T, class U>
	auto cmp_greater_equal(T t, U u) noexcept -> bool {
		return !cmp_less(t, u);
	}

	bool endsWith(const std::string& str, const std::string& suffix);
	bool startsWith(const std::string& str, const std::string& prefix);
	bool endsWith(const std::string& str, const char* suffix, unsigned suffixLen);
	bool endsWith(const std::string& str, const char* suffix);
	bool startsWith(const std::string& str, const char* prefix, unsigned prefixLen);
	bool startsWith(const std::string& str, const char* prefix);
}  // namespace backports

#endif /* CPP_BACKPORTS_HPP_ */