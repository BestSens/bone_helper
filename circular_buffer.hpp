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
	template < typename T = unsigned long >
	class CircularBuffer {
	public:
		CircularBuffer(int size);
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
		T * buffer;

		int size;
		int current_position;
		int item_count;

		int base_id;

		int getRange(T * target, int start, int end);

		std::mutex mutex;
	};

	template < typename T >
	CircularBuffer<T>::CircularBuffer(CircularBuffer&& src) {
		std::swap(this->size, src.size);
		std::swap(this->current_position, src.current_position);
		std::swap(this->item_count, src.item_count);
		std::swap(this->base_id, src.base_id);
		std::swap(this->buffer, src.buffer);
	}

	template < typename T >
	CircularBuffer<T>& CircularBuffer<T>::operator=(CircularBuffer&& rhs) {
		std::swap(this->size, rhs.size);
		std::swap(this->current_position, rhs.current_position);
		std::swap(this->item_count, rhs.item_count);
		std::swap(this->base_id, rhs.base_id);
		std::swap(this->buffer, rhs.buffer);

		return *this;
	}

	template < typename T >
	CircularBuffer<T>& CircularBuffer<T>::operator=(const CircularBuffer& rhs) {
		if(this != &rhs) {
			size = rhs.size;
			current_position = rhs.current_position;
			item_count = rhs.item_count;
			base_id = rhs.base_id;

			T * new_buffer;

			new_buffer = (T*)calloc(rhs.size, sizeof(T));
			std::memcpy(new_buffer, rhs.buffer, rhs.size);

			free(this->buffer);

			this->buffer = new_buffer;
		}

		return *this;
	}

	template < typename T >
	T CircularBuffer<T>::operator[](int id) {
		return this->getPosition(id);
	}

	template < typename T >
	CircularBuffer<T>::CircularBuffer(int size) {
		this->size = size;
		this->current_position = 0;
		this->item_count = 0;
		this->base_id = 0;

		/* allocate buffer with zeros */
		if((this->buffer = (T*)calloc(this->size, sizeof(T))) == NULL)
			throw std::runtime_error("error allocating buffer");
	}

	template < typename T >
	CircularBuffer<T>::CircularBuffer(const CircularBuffer& src) {
		this->size = src.size;
		this->current_position = src.current_position;
		this->item_count = src.item_count;
		this->base_id = src.base_id;

		/* allocate buffer with zeros */
		if((this->buffer = (T*)calloc(this->size, sizeof(T))) == NULL)
			throw std::runtime_error("error allocating buffer");

		std::copy(src.buffer, src.buffer + src.size, this->buffer);
	}

	template < typename T >
	CircularBuffer<T>::~CircularBuffer() {
		free(this->buffer);
	}

	template < typename T >
	int CircularBuffer<T>::add(const T& value) {
		if(this->size == 0)
			throw std::runtime_error("out of bounds");

		this->mutex.lock();
		this->buffer[this->current_position] = value;

		this->current_position = (this->current_position + 1) % this->size;

		if(this->item_count < this->size)
			this->item_count++;

		this->base_id = (this->base_id + 1) % INT_MAX;

		this->mutex.unlock();

		return 0;
	}

	/*
	 * return single value
	 */
	template < typename T >
	T CircularBuffer<T>::get(int id) {
		if(this->item_count == 0)
			throw std::runtime_error("out of bounds");

		T retval = T(*(this->buffer + ((this->current_position + id) % this->size)));

		return retval;
	}

	template < typename T >
	T CircularBuffer<T>::getPosition(int pos) {
		if(pos >= this->item_count || this->item_count == 0)
			throw std::runtime_error("out of bounds");

		int offset = ((this->current_position - 1) - pos) % this->size;

		if(offset < 0)
			offset += this->size;

		T retval = T(*(this->buffer + offset));

		return retval;
	}

	template < typename T >
	int CircularBuffer<T>::getRange(T * target, int start, int end) {
		if(this->size == 0)
			throw std::runtime_error("out of bounds");

		int offset = (this->current_position - end) % this->size;

		if(offset < 0)
			offset += this->size;

		int len = end - start ;

		if(len > this->size)
			len = this->size;

		int len2 = 0;

		if(len > this->size - offset) {
			len2 = len - (this->size - offset);
			len = this->size - offset;
		}

		if(len < 0 || len2 < 0)
			throw std::runtime_error("out of bounds");

		std::copy(this->buffer + offset, this->buffer + offset + len, target);
		std::copy(this->buffer, this->buffer + len2, target + len);

		int amount = len + len2;

		return amount;
	}

	template < typename T >
	std::vector<T> CircularBuffer<T>::getVector(int amount) {
		int last_value = 0;

		return this->getVector(amount, last_value);
	}

	template < typename T >
	std::vector<T> CircularBuffer<T>::getVector(int amount, int &last_value) {
		if(amount > this->item_count)
			amount = this->item_count;

		std::vector<T> vect(amount);

		last_value = this->get(vect.data(), amount, last_value);

		vect.resize(amount);

		return vect;
	}

	template < typename T >
	int CircularBuffer<T>::get(T * target, int &amount, int last_value) {
		if(this->size == 0)
			throw std::runtime_error("out of bounds");

		this->mutex.lock();

		int end;
		int last_position;

		if(amount == 0)
			amount = -1;

		if(last_value > 0 && last_value <= this->base_id) {
			int temp = this->base_id - last_value;
			end = temp % this->size;

			if(end < 0)
				end += this->size;
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
