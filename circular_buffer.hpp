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

namespace bestsens {
	template <typename T=unsigned long>
	class CircularBuffer {
	public:
		CircularBuffer(int size);
		~CircularBuffer();

		int add(T value);
		T get(int id);
		T getPosition(unsigned long pos);
		int get(T * target, int * amount, unsigned long last_value = 0);

	private:
		T * buffer;

		int size;
		int current_position;
		int item_count;

		int base_id;

		int getRange(T * target, int start, int end);

		std::mutex mutex;
	};

	template <typename T>
	CircularBuffer<T>::CircularBuffer(int size) {
		this->size = size;
		this->current_position = 0;
		this->item_count = 0;
		this->base_id = 0;

		/* allocate buffer with zeros */
		if((this->buffer = (T*)calloc(this->size, sizeof(T))) == NULL) {
			std::cout << "error allocating buffer\r\n";

			free(this->buffer);
		}
	}

	template <typename T>
	CircularBuffer<T>::~CircularBuffer() {
		free(this->buffer);
	}

	template <typename T>
	int CircularBuffer<T>::add(T value) {
		this->mutex.lock();
		this->buffer[this->current_position] = value;

		this->current_position = (this->current_position + 1) % this->size;

		if(this->item_count < this->size)
			this->item_count++;

		this->base_id = (this->base_id + 1) % INT_MAX;
		//this->base_id = (this->base_id + 1) % this->size;

		this->mutex.unlock();

		return 0;
	}

	/*
	 * return single value
	 */
	template <typename T>
	T CircularBuffer<T>::get(int id) {
		return *(this->buffer + ((this->current_position + id + 1) % this->size));
	}

	template <typename T>
	T CircularBuffer<T>::getPosition(unsigned long pos) {
		int offset = this->current_position - pos;

		if(offset < this->size)
			return *(this->buffer + ((this->current_position - offset) % this->size));

		return NULL;
	}

	template <typename T>
	int CircularBuffer<T>::getRange(T * target, int start, int end) {
		int offset = (this->current_position - end) % this->size;

		if(offset < 0)
			offset += this->size;

		int len = end - (start + 1);

		if(len > this->size)
			len = this->size;

		int len2 = 0;

		if(len > this->size - offset) {
			len2 = len - (this->size - offset);
			len = this->size - offset;
		}

		memcpy(target, this->buffer + offset, len * sizeof(T));
		memcpy(target + len, this->buffer, len2 * sizeof(T));

		int amount = len + len2;

		return amount;
	}

	template <typename T>
	int CircularBuffer<T>::get(T * target, int * amount, unsigned long last_value) {
		int end;
		int last_position;

		if(*amount == 0)
			*amount = -1;
		else if(*amount > this->item_count)
			*amount = this->item_count;

		this->mutex.lock();

		if(last_value > 0) {
			int temp = this->base_id - last_value;

			if(temp < 0)
				temp = INT_MAX - last_value + this->base_id;

			end = temp % this->size;

			if(end < 0)
				end += this->size;

		} else
			end = *amount;

		last_position = this->base_id;

		*amount = getRange(target, 0, end);

		this->mutex.unlock();

		return last_position;
	}
}

#endif /* CIRCULAR_BUFFER_HPP_ */
