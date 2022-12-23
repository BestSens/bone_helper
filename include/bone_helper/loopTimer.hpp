/*
 * loopTimer.hpp
 *
 *  Created on: 14.06.2016
 *	  Author: Jan Schöppach
 */

#ifndef LOOPTIMER_HPP_
#define LOOPTIMER_HPP_

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>

namespace bestsens {
	class loopTimer {
	public:
		loopTimer(std::chrono::microseconds wait_time, bool start_value = false) : wait_time{wait_time} {
			this->next_cycle =
				start_value ? std::chrono::steady_clock::now() : std::chrono::steady_clock::now() + this->wait_time;
		}

		loopTimer(int wait_time_ms, bool start_value)
			: loopTimer(std::chrono::milliseconds(wait_time_ms), start_value){};
		~loopTimer() {
			this->stop();
		}

		void wait_on_tick() {
			do {
				std::unique_lock<std::mutex> lk(loopTimer::m_trigger);
				loopTimer::cv_trigger.wait_until(lk, this->next_cycle);
			} while (!loopTimer::kill && this->running && std::chrono::steady_clock::now() < this->next_cycle);

			this->next_cycle = std::chrono::steady_clock::now() + this->wait_time;
		}

		static void kill_all() {
			{
				const std::lock_guard<std::mutex> lk(loopTimer::m_trigger);
				loopTimer::kill = true;
			}
			loopTimer::cv_trigger.notify_all();
		};

		auto set_wait_time(std::chrono::microseconds wait_time) -> void {
			this->wait_time = wait_time;
		}

		auto get_wait_time() -> std::chrono::microseconds {
			return this->wait_time;
		}

	private:
		std::chrono::steady_clock::time_point next_cycle{};
		std::chrono::microseconds wait_time;

		std::atomic<bool> running{false};

		static inline std::atomic<bool> kill{false};
		static inline std::condition_variable cv_trigger{};
		static inline std::mutex m_trigger{};

		void stop() {
			{
				const std::lock_guard<std::mutex> lk(loopTimer::m_trigger);
				this->running = false;
			}
			loopTimer::cv_trigger.notify_all();
		}
	};
}  // namespace bestsens

#endif /* LOOPTIMER_HPP_ */
