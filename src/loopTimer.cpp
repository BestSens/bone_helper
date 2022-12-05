#include "bone_helper/loopTimer.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>

namespace bestsens{
	loopTimer::loopTimer(std::chrono::microseconds wait_time, bool start_value) : wait_time{wait_time} {
		this->next_cycle =
			start_value ? std::chrono::steady_clock::now() : std::chrono::steady_clock::now() + this->wait_time;
	}

	loopTimer::~loopTimer() {
		this->stop();
	}

	void loopTimer::kill_all() {
		{
			const std::lock_guard<std::mutex> lk(loopTimer::m_trigger);
			loopTimer::kill = true;
		}
		loopTimer::cv_trigger.notify_all();
	}

	void loopTimer::stop() {
		{
			const std::lock_guard<std::mutex> lk(loopTimer::m_trigger);
			this->running = false;
		}
		loopTimer::cv_trigger.notify_all();
	}

	std::atomic<bool> loopTimer::kill{false};
	std::condition_variable loopTimer::cv_trigger;
	std::mutex loopTimer::m_trigger;

	void loopTimer::wait_on_tick() {
		do {
			std::unique_lock<std::mutex> lk(loopTimer::m_trigger);
			loopTimer::cv_trigger.wait_until(lk, this->next_cycle);
		} while (!loopTimer::kill && this->running && std::chrono::steady_clock::now() < this->next_cycle);

		this->next_cycle = std::chrono::steady_clock::now() + this->wait_time;
	}

	auto loopTimer::set_wait_time(std::chrono::microseconds wait_time) -> void {
		this->wait_time = wait_time;
	}

	auto loopTimer::get_wait_time() -> std::chrono::microseconds {
		return this->wait_time;
	}
}  // bestsens