/*
 * loopTimer.hpp
 *
 *  Created on: 14.06.2016
 *	  Author: Jan Schöppach
 */

#ifndef LOOPTIMER_HPP_
#define LOOPTIMER_HPP_

#include <thread>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <iostream>

namespace bestsens {
	class loopTimer {
	public:
		loopTimer(std::chrono::microseconds wait_time, int start_value);
		loopTimer(int wait_time_ms, int start_value) : loopTimer(std::chrono::milliseconds(wait_time_ms), start_value) {};
		~loopTimer();

		void wait_on_tick();

		static void kill_all();

		int set_wait_time(std::chrono::microseconds wait_time);
		std::chrono::microseconds get_wait_time();
	private:
		bool ready;
		std::mutex m;
		std::condition_variable cv;

		std::thread timer_thread;
		std::chrono::microseconds wait_time;

		int running;

		static int kill;
		static std::condition_variable cv_trigger;
		static std::mutex m_trigger;

		void start(int start_value);
		void stop();
	};
} // namespace bestsens

#endif /* LOOPTIMER_HPP_ */
