/*
 * loopTimer.hpp
 *
 *  Created on: 14.06.2016
 *	  Author: Jan Schöppach
 */

#ifndef LOOPTIMER_HPP_
#define LOOPTIMER_HPP_

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <atomic>

namespace bestsens {
	class loopTimer {
	public:
		explicit loopTimer(std::chrono::microseconds wait_time, bool start_value = false);
		loopTimer(int wait_time_ms, bool start_value) : loopTimer(std::chrono::milliseconds(wait_time_ms), start_value) {};
		~loopTimer();

		void wait_on_tick();

		static void kill_all();

		auto set_wait_time(std::chrono::microseconds wait_time) -> void;
		auto get_wait_time() -> std::chrono::microseconds;
	private:
		std::chrono::steady_clock::time_point next_cycle{};
		std::chrono::microseconds wait_time;

		std::atomic<bool> running{false};

		static std::atomic<bool> kill;
		static std::condition_variable cv_trigger;
		static std::mutex m_trigger;

		void stop();
	};
} // namespace bestsens

#endif /* LOOPTIMER_HPP_ */
