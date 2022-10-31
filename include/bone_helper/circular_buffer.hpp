/*
 * circular_buffer.hpp
 *
 *  Created on: 10.06.2015
 *      Author: Jan Schöppach
 */

#ifndef CIRCULAR_BUFFER_HPP_
#define CIRCULAR_BUFFER_HPP_

#include <array>
#include <cassert>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <vector>

namespace bestsens {
	template <typename T>
	auto subtractWithRollover(const T& a, const T& b, const T& limit = std::numeric_limits<T>::max()) -> T {
		assert(a >= 0 && b >= 0);
		assert(a <= limit && b <= limit);

		if (a - b < 0) {
			return (limit - b) + a + 1;
		}

		return a - b;
	}

	template <typename T>
	auto addWithRollover(const T& a, const T& b, const T& limit = std::numeric_limits<T>::max()) -> T {
		assert(a >= 0 && b >= 0);
		assert(a <= limit && b <= limit);

		const auto diff_to_overflow = limit - a;

		if (diff_to_overflow < b) {
			return b - diff_to_overflow - 1;
		}
		
		return a + b;	
	}

	template <typename T>
	void incrementWithRollover(T& a, const T& limit = std::numeric_limits<T>::max()) {
		a = addWithRollover(a, T{1u}, limit);
	}

	template <typename T>
	void decrementWithRollover(T& a, const T& limit = std::numeric_limits<T>::max()) {
		a = subtractWithRollover(a, T{1u}, limit);
	}

	template < typename T, size_t N >
	class CircularBuffer {
	public:
		CircularBuffer() = default;
		CircularBuffer(const CircularBuffer& src) noexcept;
		CircularBuffer(CircularBuffer&& src) noexcept;
		
		~CircularBuffer() = default;

		auto operator=(const CircularBuffer& rhs) -> CircularBuffer&;
		auto operator=(CircularBuffer&& rhs) noexcept -> CircularBuffer&;
		auto operator[](size_t id) const -> T;

		auto add(const T& value) -> size_t;
		auto add(T&& value) -> size_t;
		auto add(const std::vector<T>& values) -> size_t;

		auto get(size_t id) const -> T;
		auto get(T * target, size_t &amount, size_t last_value = 0, bool return_continous = false) const -> size_t;

		auto getVector(size_t amount) const -> std::vector<T>;
		auto getVector(size_t amount, size_t& last_value, bool exactly = false, bool return_continous = false) const
			-> std::vector<T>;

		auto getPosition(size_t pos) const -> T;
		auto getBaseID() const -> size_t;
		auto getNewDataAmount(size_t last_value = 0) const -> size_t;

		auto size() const -> size_t;
		constexpr auto maxSize() const -> size_t;

		void clear();
	private:
		std::array<T, N> buffer;

		size_t current_position{0};
		size_t item_count{0};
		size_t base_id{0};

		auto getRange(T * target, size_t start, size_t end) const -> size_t;

		mutable std::mutex mutex;

		auto incrementCounters() -> void;
	};

	template < typename T, size_t N >
	CircularBuffer<T, N>::CircularBuffer(CircularBuffer&& src) noexcept {
		std::swap(this->current_position, src.current_position);
		std::swap(this->item_count, src.item_count);
		std::swap(this->base_id, src.base_id);
		std::swap(this->buffer, src.buffer);
	}

	template < typename T, size_t N >
	CircularBuffer<T, N>::CircularBuffer(const CircularBuffer& src) noexcept {
		this->current_position = src.current_position;
		this->item_count = src.item_count;
		this->base_id = src.base_id;
		this->buffer = src.buffer;
	}

	template < typename T, size_t N >
	auto CircularBuffer<T, N>::operator=(CircularBuffer&& rhs) noexcept -> CircularBuffer<T, N>& {
		std::swap(this->current_position, rhs.current_position);
		std::swap(this->item_count, rhs.item_count);
		std::swap(this->base_id, rhs.base_id);
		std::swap(this->buffer, rhs.buffer);

		return *this;
	}

	template < typename T, size_t N >
	auto CircularBuffer<T, N>::operator=(const CircularBuffer& rhs) -> CircularBuffer<T, N>& {
		if (this != &rhs) {
			current_position = rhs.current_position;
			item_count = rhs.item_count;
			base_id = rhs.base_id;

			this->buffer = rhs.buffer;
		}

		return *this;
	}

	template < typename T, size_t N >
	auto CircularBuffer<T, N>::operator[](size_t id) const -> T {
		return this->getPosition(id);
	}

	template <typename T, size_t N>
	auto CircularBuffer<T, N>::incrementCounters() -> void {
		if (this->item_count < N) {
			this->item_count++;
		};

		incrementWithRollover(this->current_position, N - 1);
		incrementWithRollover(this->base_id);
	}

	template < typename T, size_t N >
	auto CircularBuffer<T, N>::add(T&& value) -> size_t {
		static_assert(N > 0, "zero length buffer cannot be filled");

		std::lock_guard<std::mutex> lock(this->mutex);
		this->buffer[this->current_position] = std::forward<T>(value);
		this->incrementCounters();

		return 0;
	}

	template < typename T, size_t N >
	auto CircularBuffer<T, N>::add(const T& value) -> size_t {
		static_assert(N > 0, "zero length buffer cannot be filled");

		std::lock_guard<std::mutex> lock(this->mutex);
		this->buffer[this->current_position] = value;
		this->incrementCounters();

		return 0;
	}

	template < typename T, size_t N >
	auto CircularBuffer<T, N>::add(const std::vector<T>& values) -> size_t {
		static_assert(N > 0, "zero length buffer cannot be filled");

		std::lock_guard<std::mutex> lock(this->mutex);

		for (const auto& e : values) {
			this->buffer[this->current_position] = e;
			this->incrementCounters();
		}

		return 0;
	}

	template < typename T, size_t N >
	auto CircularBuffer<T, N>::getRange(T * target, size_t start, size_t end) const -> size_t {
		auto len = end - start;

		assert(len <= N);
		assert(end > 0);

		const auto offset = subtractWithRollover(this->current_position, end, N - 1);

		if (offset > N) {
			throw std::runtime_error("out of bounds");
		}

		if ((N - offset) > len) {
			throw std::runtime_error("out of bounds");
		}

		size_t len2 = 0;

		if (len > N - offset) {
			len2 = len - (N - offset);
			len = N - offset;
		}

		std::copy(this->buffer.data() + offset, this->buffer.data() + offset + len, target);
		std::copy(this->buffer.data(), this->buffer.data() + len2, target + len);

		const auto amount = len + len2;

		return amount;
	}

	/*
	 * return single value
	 */
	template < typename T, size_t N >
	auto CircularBuffer<T, N>::get(size_t id) const -> T {
		std::lock_guard<std::mutex> lock(this->mutex);

		if (this->item_count == 0) {
			throw std::runtime_error("out of bounds");
		}

		return this->buffer.at(addWithRollover(this->current_position, id, N - 1));
	}

	template < typename T, size_t N >
	auto CircularBuffer<T, N>::get(T * target, size_t &amount, size_t last_value, bool return_continous) const -> size_t {
		std::lock_guard<std::mutex> lock(this->mutex);

		size_t end = 0;
		size_t last_position = 0;

		if (amount == 0) {
			return this->base_id;
		}

		if (last_value > 0 && last_value <= this->base_id) {
			end = subtractWithRollover(this->base_id, last_value);
		} else {
			end = amount;
		}

		if (end > this->item_count) {
			end = this->item_count;
		}

		size_t start = 0;

		if (!return_continous) {
			if (end > amount) {
				end = amount;
			}
		} else {
			if (end > amount) {
				start = end - amount;
			}
		}

		last_position = this->base_id - start;

		amount = getRange(target, start, end);

		return last_position;
	}

	template < typename T, size_t N >
	auto CircularBuffer<T, N>::getVector(size_t amount) const -> std::vector<T> {
		size_t last_value = 0;
		return this->getVector(amount, last_value);
	}

	template < typename T, size_t N >
	auto CircularBuffer<T, N>::getVector(size_t amount, size_t &last_value, bool exactly, bool return_continous) const -> std::vector<T> {
		if (exactly) {
			const auto amount_available = this->getNewDataAmount(last_value);

			if (amount_available < amount)
				return {};
		}

		if (amount > this->item_count)
			amount = this->item_count;

		std::vector<T> vect(amount);

		last_value = this->get(vect.data(), amount, last_value, return_continous);

		vect.resize(amount);

		return vect;
	}

	template < typename T, size_t N >
	auto CircularBuffer<T, N>::getPosition(size_t pos) const -> T {
		std::lock_guard<std::mutex> lock(this->mutex);

		if (pos >= this->item_count || this->item_count == 0)
			throw std::runtime_error("out of bounds");

		const auto offset = subtractWithRollover<size_t>(
			subtractWithRollover<size_t>(this->current_position, 1l, N - 1l), pos, N - 1l);

		return this->buffer.at(offset);
	}

	template < typename T, size_t N >
	auto CircularBuffer<T, N>::getBaseID() const -> size_t {
		return this->base_id;
	}

	template < typename T, size_t N >
	auto CircularBuffer<T, N>::getNewDataAmount(size_t last_value) const -> size_t {
		const auto difference = subtractWithRollover(this->base_id, last_value);
		return std::min(difference, this->item_count);
	}

	template < typename T, size_t N >
	auto CircularBuffer<T, N>::size() const -> size_t {
		return this->item_count;
	}

	template < typename T, size_t N >
	constexpr auto CircularBuffer<T, N>::maxSize() const -> size_t {
		return N;
	}

	template < typename T, size_t N >
	void CircularBuffer<T, N>::clear() {
		std::lock_guard<std::mutex> lock(this->mutex);
		this->item_count = 0;
	}
}  // namespace bestsens

#endif /* CIRCULAR_BUFFER_HPP_ */
