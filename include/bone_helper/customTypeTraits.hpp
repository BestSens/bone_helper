#pragma once

#include <expected>
#include <limits>
#include <utility>

enum class cast_error { negative_overflow, positive_overflow };

template <class T, class I>
constexpr auto safeCast(const I in) -> std::expected<T, cast_error> {
	if (std::cmp_less(in, std::numeric_limits<T>::min())) {
		return std::unexpected(cast_error::negative_overflow);
	}

	if (std::cmp_greater(in, std::numeric_limits<T>::max())) {
		return std::unexpected(cast_error::positive_overflow);
	}

	return static_cast<T>(in);
}

template <class T, class I>
constexpr auto coerceCast(const I in) -> T {
	const auto ret = safeCast<T>(in);

	if (ret) {
		return *ret;
	}

	if (ret.error() == cast_error::negative_overflow) {
		return std::numeric_limits<T>::min();
	} else {
		return std::numeric_limits<T>::max();
	}
}