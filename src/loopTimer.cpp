#include "bone_helper/loopTimer.hpp"

namespace bestsens{
	loopTimer::loopTimer(std::chrono::microseconds wait_time, int start_value) : wait_time{wait_time} {
		this->start(start_value);
	}

	loopTimer::~loopTimer() {
		this->stop();
		if(this->timer_thread.joinable())
		{
			this->timer_thread.join();
		}
	}

	void loopTimer::kill_all() {
		{
			std::lock_guard<std::mutex> lk(loopTimer::m_trigger);
			loopTimer::kill = true;
		}
		loopTimer::cv_trigger.notify_all();
	}

	void loopTimer::stop() {
		{
			std::lock_guard<std::mutex> lk(loopTimer::m_trigger);
			this->running = false;
		}
		loopTimer::cv_trigger.notify_all();
	}

	void loopTimer::start(int start_value = 0) {
		if (loopTimer::kill)
			return;

		this->running = true;
		this->ready = (start_value == 1);

		new (&this->timer_thread) std::thread([this] {
			bool exit = false;

			auto predicate = [this]() -> bool {
				return loopTimer::kill || !this->running;
			};

			while (!exit) {
				const auto expires = std::chrono::steady_clock::now() + this->wait_time;
				
				{
					std::unique_lock<std::mutex> lk(loopTimer::m_trigger);
					exit = loopTimer::cv_trigger.wait_until(lk, expires, predicate);
				}

				{
					std::lock_guard<std::mutex> lk(this->m);
					this->ready = true;
				}

				this->cv.notify_all();
			}

			{
				std::lock_guard<std::mutex> lk(this->m);
				this->ready = true;
			}

			this->cv.notify_all();
		});
	}

	std::atomic<bool> loopTimer::kill{false};
	std::condition_variable loopTimer::cv_trigger;
	std::mutex loopTimer::m_trigger;

	void loopTimer::wait_on_tick() {
		std::unique_lock<std::mutex> lk(this->m);

		while (!this->ready && !loopTimer::kill)
			this->cv.wait(lk);

		this->ready = false;
	}

	auto loopTimer::set_wait_time(std::chrono::microseconds wait_time) -> int {
		this->wait_time = wait_time;

		return 0;
	}

	auto loopTimer::get_wait_time() -> std::chrono::microseconds {
		return this->wait_time;
	}
}  // bestsens