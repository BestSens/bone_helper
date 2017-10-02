/*
 * circular_buffer.hpp
 *
 *  Created on: 10.06.2015
 *      Author: Jan Schöppach
 */

#ifndef CIRCULAR_BUFFER_HPP_
#define CIRCULAR_BUFFER_HPP_

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <climits>
#include <mutex>
#include <type_traits>
#include <vector>

namespace bestsens {
	template < typename T, int N >
	class CircularBuffer {
	public:
		CircularBuffer();
		CircularBuffer(const CircularBuffer& src);
		CircularBuffer(CircularBuffer&& src);

		CircularBuffer& operator=(const CircularBuffer& rhs);
		CircularBuffer& operator=(CircularBuffer&& rhs);
		T operator[](int id);

		~CircularBuffer();

		int add(const T& value);
		T get(int id);
		T getPosition(int pos);
		int get(T * target, int &amount, int last_value = 0);

		std::vector<T> getVector(int amount);
		std::vector<T> getVector(int amount, int &last_value);

	private:
		std::array<T, N> buffer;

		int current_position;
		int item_count;

		int base_id;

		int getRange(T * target, int start, int end);

		std::mutex mutex;
	};

	template < typename T, int N >
	CircularBuffer<T, N>::CircularBuffer(CircularBuffer&& src) {
		std::swap(this->current_position, src.current_position);
		std::swap(this->item_count, src.item_count);
		std::swap(this->base_id, src.base_id);
		std::swap(this->buffer, src.buffer);
	}

	template < typename T, int N >
	CircularBuffer<T, N>& CircularBuffer<T, N>::operator=(CircularBuffer&& rhs) {
		std::swap(this->current_position, rhs.current_position);
		std::swap(this->item_count, rhs.item_count);
		std::swap(this->base_id, rhs.base_id);
		std::swap(this->buffer, rhs.buffer);

		return *this;
	}

	template < typename T, int N >
	CircularBuffer<T, N>& CircularBuffer<T, N>::operator=(const CircularBuffer& rhs) {
		if(this != &rhs) {
			current_position = rhs.current_position;
			item_count = rhs.item_count;
			base_id = rhs.base_id;

			this->buffer = rhs.buffer;
		}

		return *this;
	}

	template < typename T, int N >
	T CircularBuffer<T, N>::operator[](int id) {
		return this->getPosition(id);
	}

	template < typename T, int N >
	CircularBuffer<T, N>::CircularBuffer() {
		this->current_position = 0;
		this->item_count = 0;
		this->base_id = 0;
	}

	template < typename T, int N >
	CircularBuffer<T, N>::CircularBuffer(const CircularBuffer& src) {
		this->current_position = src.current_position;
		this->item_count = src.item_count;
		this->base_id = src.base_id;
		this->buffer = src.buffer;
	}

	template < typename T, int N >
	CircularBuffer<T, N>::~CircularBuffer() {}

	template < typename T, int N >
	int CircularBuffer<T, N>::add(const T& value) {
		if(N == 0)
			throw std::runtime_error("out of bounds");

		this->mutex.lock();
		this->buffer[this->current_position] = value;

		this->current_position = (this->current_position + 1) % N;

		if(this->item_count < N)
			this->item_count++;

		this->base_id = (this->base_id + 1) % INT_MAX;

		this->mutex.unlock();

		return 0;
	}

	/*
	 * return single value
	 */
	template < typename T, int N >
	T CircularBuffer<T, N>::get(int id) {
		if(this->item_count == 0)
			throw std::runtime_error("out of bounds");

		return this->buffer[(this->current_position + id) % N];
	}

	template < typename T, int N >
	T CircularBuffer<T, N>::getPosition(int pos) {
		if(pos >= this->item_count || this->item_count == 0)
			throw std::runtime_error("out of bounds");

		int offset = ((this->current_position - 1) - pos) % N;

		if(offset < 0)
			offset += N;

		return this->buffer[offset];
	}

	template < typename T, int N >
	int CircularBuffer<T, N>::getRange(T * target, int start, int end) {
		int offset = (this->current_position - end) % N;

		if(offset < 0)
			offset += N;

		int len = end - start ;

		if(len > N)
			len = N;

		int len2 = 0;

		if(len > N - offset) {
			len2 = len - (N - offset);
			len = N - offset;
		}

		if(len < 0 || len2 < 0)
			throw std::runtime_error("out of bounds");

		std::copy(this->buffer.data() + offset, this->buffer.data() + offset + len, target);
		std::copy(this->buffer.data(), this->buffer.data() + len2, target + len);

		int amount = len + len2;

		return amount;
	}

	template < typename T, int N >
	std::vector<T> CircularBuffer<T, N>::getVector(int amount) {
		int last_value = 0;

		return this->getVector(amount, last_value);
	}

	template < typename T, int N >
	std::vector<T> CircularBuffer<T, N>::getVector(int amount, int &last_value) {
		if(amount > this->item_count)
			amount = this->item_count;

		std::vector<T> vect(amount);

		last_value = this->get(vect.data(), amount, last_value);

		vect.resize(amount);

		return vect;
	}

	template < typename T, int N >
	int CircularBuffer<T, N>::get(T * target, int &amount, int last_value) {
		this->mutex.lock();

		int end;
		int last_position;

		if(amount == 0)
			amount = -1;

		if(last_value > 0 && last_value <= this->base_id) {
			int temp = this->base_id - last_value;
			end = temp % N;

			if(end < 0)
				end += N;
		} else {
			end = amount;
		}

		if(end > this->item_count)
			end = this->item_count;

		if(end > amount)
			end = amount;

		last_position = this->base_id;

		amount = getRange(target, 0, end);

		this->mutex.unlock();

		return last_position;
	}
}

#endif /* CIRCULAR_BUFFER_HPP_ */
