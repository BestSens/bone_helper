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
#include <vector>

namespace bestsens {
	template <typename T>
	auto subtractWithRollover(const T& a, const T& b, const T& limit = std::numeric_limits<T>::max()) -> T {
		assert(a >= 0 && b >= 0);
		assert(a <= limit && b <= limit);

		if (a - b < 0)
			return (limit - b) + a + 1;

		return a - b;
	}

	template <typename T>
	auto addWithRollover(const T& a, const T& b, const T& limit = std::numeric_limits<T>::max()) -> T {
		assert(a >= 0 && b >= 0);
		assert(a <= limit && b <= limit);

		const auto diff_to_overflow = limit - a;

		if (diff_to_overflow < b)
			return b - diff_to_overflow - 1;
		
		return a + b;	
	}

	template <typename T>
	void incrementWithRollover(T& a, const T& limit = std::numeric_limits<T>::max()) {
		a = addWithRollover(a, 1, limit);
	}

	template <typename T>
	void decrementWithRollover(T& a, const T& limit = std::numeric_limits<T>::max()) {
		a = subtractWithRollover(a, 1, limit);
	}

	template < typename T, int N >
	class CircularBuffer {
	public:
		CircularBuffer() noexcept = default;
		CircularBuffer(const CircularBuffer& src) noexcept;
		CircularBuffer(CircularBuffer&& src) noexcept;
		
		~CircularBuffer() = default;

		auto operator=(const CircularBuffer& rhs) -> CircularBuffer&;
		auto operator=(CircularBuffer&& rhs) noexcept -> CircularBuffer&;
		auto operator[](int id) const -> T;

		auto add(const T& value) -> int;
		auto add(T&& value) -> int;
		auto add(const std::vector<T>& values) -> int;

		auto get(int id) const -> T;
		auto get(T * target, int &amount, int last_value = 0) const -> int;

		auto getVector(int amount) const -> std::vector<T>;
		auto getVector(int amount, int &last_value, bool exactly = false) const -> std::vector<T>;
		
		auto getPosition(int pos) const -> T;
		auto getBaseID() const -> int;
		auto getNewDataAmount(int last_value = 0) const -> int;

		auto size() const -> int;
		auto maxSize() const -> int;

		void clear();
	private:
		std::array<T, N> buffer;

		int current_position{0};
		int item_count{0};
		int base_id{0};

		auto getRange(T * target, int start, int end) const -> int;

		mutable std::mutex mutex;
	};

	template < typename T, int N >
	CircularBuffer<T, N>::CircularBuffer(CircularBuffer&& src) noexcept {
		std::swap(this->current_position, src.current_position);
		std::swap(this->item_count, src.item_count);
		std::swap(this->base_id, src.base_id);
		std::swap(this->buffer, src.buffer);
	}

	template < typename T, int N >
	CircularBuffer<T, N>::CircularBuffer(const CircularBuffer& src) noexcept {
		this->current_position = src.current_position;
		this->item_count = src.item_count;
		this->base_id = src.base_id;
		this->buffer = src.buffer;
	}

	template < typename T, int N >
	auto CircularBuffer<T, N>::operator=(CircularBuffer&& rhs) noexcept -> CircularBuffer<T, N>& {
		std::swap(this->current_position, rhs.current_position);
		std::swap(this->item_count, rhs.item_count);
		std::swap(this->base_id, rhs.base_id);
		std::swap(this->buffer, rhs.buffer);

		return *this;
	}

	template < typename T, int N >
	auto CircularBuffer<T, N>::operator=(const CircularBuffer& rhs) -> CircularBuffer<T, N>& {
		if (this != &rhs) {
			current_position = rhs.current_position;
			item_count = rhs.item_count;
			base_id = rhs.base_id;

			this->buffer = rhs.buffer;
		}

		return *this;
	}

	template < typename T, int N >
	auto CircularBuffer<T, N>::operator[](int id) const -> T {
		return this->getPosition(id);
	}

	template < typename T, int N >
	auto CircularBuffer<T, N>::add(T&& value) -> int {
		if (N == 0)
			throw std::runtime_error("out of bounds");

		std::lock_guard<std::mutex> lock(this->mutex);
		this->buffer[this->current_position] = std::forward<T>(value);

		this->current_position = (this->current_position + 1) % N;

		if (this->item_count < N)
			this->item_count++;

		incrementWithRollover(this->base_id);

		return 0;
	}

	template < typename T, int N >
	auto CircularBuffer<T, N>::add(const T& value) -> int {
		if (N == 0)
			throw std::runtime_error("out of bounds");

		std::lock_guard<std::mutex> lock(this->mutex);
		this->buffer[this->current_position] = value;

		this->current_position = (this->current_position + 1) % N;

		if (this->item_count < N)
			this->item_count++;

		incrementWithRollover(this->base_id);

		return 0;
	}

	template < typename T, int N >
	auto CircularBuffer<T, N>::add(const std::vector<T>& values) -> int {
		if (N == 0)
			throw std::runtime_error("out of bounds");

		std::lock_guard<std::mutex> lock(this->mutex);

		for (const auto& e : values) {
			this->buffer[this->current_position] = e;

			this->current_position = (this->current_position + 1) % N;

			if (this->item_count < N)
				this->item_count++;

			incrementWithRollover(this->base_id);
		}

		return 0;
	}

	template < typename T, int N >
	auto CircularBuffer<T, N>::getRange(T * target, int start, int end) const -> int {
		auto offset = (this->current_position - end) % N;

		if (offset < 0)
			offset += N;

		auto len = end - start;

		if (len > N)
			len = N;

		auto len2 = 0;

		if (len > N - offset) {
			len2 = len - (N - offset);
			len = N - offset;
		}

		if (len < 0 || len2 < 0)
			throw std::runtime_error("out of bounds");

		std::copy(this->buffer.data() + offset, this->buffer.data() + offset + len, target);
		std::copy(this->buffer.data(), this->buffer.data() + len2, target + len);

		const auto amount = len + len2;

		return amount;
	}

	/*
	 * return single value
	 */
	template < typename T, int N >
	auto CircularBuffer<T, N>::get(int id) const -> T {
		std::lock_guard<std::mutex> lock(this->mutex);

		if (this->item_count == 0)
			throw std::runtime_error("out of bounds");

		return this->buffer.at((this->current_position + id) % N);
	}

	template < typename T, int N >
	auto CircularBuffer<T, N>::get(T * target, int &amount, int last_value) const -> int {
		std::lock_guard<std::mutex> lock(this->mutex);

		int end = 0;
		int last_position = 0;

		if (amount == 0)
			amount = -1;

		if (last_value > 0 && last_value <= this->base_id) {
			const auto temp = this->base_id - last_value;
			end = temp % N;
		} else {
			end = amount;
		}

		if (end > this->item_count)
			end = this->item_count;

		if (end > amount)
			end = amount;

		last_position = this->base_id;

		amount = getRange(target, 0, end);

		return last_position;
	}

	template < typename T, int N >
	auto CircularBuffer<T, N>::getVector(int amount) const -> std::vector<T> {
		int last_value = 0;
		return this->getVector(amount, last_value);
	}

	template < typename T, int N >
	auto CircularBuffer<T, N>::getVector(int amount, int &last_value, bool exactly) const -> std::vector<T> {
		if (exactly) {
			const auto amount_available = this->getNewDataAmount(last_value);

			if (amount_available < amount)
				return {};
		}

		if (amount > this->item_count)
			amount = this->item_count;

		std::vector<T> vect(amount);

		last_value = this->get(vect.data(), amount, last_value);

		vect.resize(amount);

		return vect;
	}

	template < typename T, int N >
	auto CircularBuffer<T, N>::getPosition(int pos) const -> T {
		std::lock_guard<std::mutex> lock(this->mutex);

		if (pos >= this->item_count || this->item_count == 0)
			throw std::runtime_error("out of bounds");

		auto offset = ((this->current_position - 1) - pos) % N;

		if (offset < 0)
			offset += N;

		return this->buffer.at(offset);
	}

	template < typename T, int N >
	auto CircularBuffer<T, N>::getBaseID() const -> int {
		return this->base_id;
	}

	template < typename T, int N >
	auto CircularBuffer<T, N>::getNewDataAmount(int last_value) const -> int {
		const auto difference = subtractWithRollover(this->base_id, last_value);
		return std::min(difference, this->item_count);
	}

	template < typename T, int N >
	auto CircularBuffer<T, N>::size() const -> int {
		return this->item_count;
	}

	template < typename T, int N >
	auto CircularBuffer<T, N>::maxSize() const -> int {
		return N;
	}

	template < typename T, int N >
	void CircularBuffer<T, N>::clear() {
		std::lock_guard<std::mutex> lock(this->mutex);
		this->item_count = 0;
	}
}  // namespace bestsens

#endif /* CIRCULAR_BUFFER_HPP_ */
