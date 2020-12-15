#include "loopTimer.hpp"

namespace bestsens
{
	loopTimer::loopTimer(std::chrono::microseconds wait_time, int start_value = 0) {
		this->wait_time = wait_time;

		this->start(start_value);
	}

	loopTimer::~loopTimer() {
		this->stop();
		this->timer_thread.join();
	}

	void loopTimer::kill_all() {
		std::lock_guard<std::mutex> lk(loopTimer::m_trigger);
		loopTimer::kill = 1;
		loopTimer::cv_trigger.notify_all();
	}

	void loopTimer::stop() {
		std::lock_guard<std::mutex> lk(loopTimer::m_trigger);
		this->running = 0;
		loopTimer::cv_trigger.notify_all();
	}

	void loopTimer::start(int start_value = 0) {
		this->running = 1;

		this->ready = (start_value == 1);
		new (&this->timer_thread) std::thread([this] {
			while(this->running) {
				int exit = 0;
				{
					std::unique_lock<std::mutex> lk(loopTimer::m_trigger);
					auto expires = std::chrono::steady_clock::now() + this->wait_time;

					if(loopTimer::cv_trigger.wait_until(lk, expires, [this](){return loopTimer::kill == 1 || this->running == 0;}) == true)
						exit = 1;
				}

				{
					std::lock_guard<std::mutex> lk(this->m);
					this->ready = true;
					this->cv.notify_all();
				}

				if(exit)
					break;
			}

			this->ready = true;
		});
	}

	int loopTimer::kill = 0;
	std::condition_variable loopTimer::cv_trigger;
	std::mutex loopTimer::m_trigger;

	void loopTimer::wait_on_tick() {
		std::unique_lock<std::mutex> lk(this->m);

		while(!this->ready && !loopTimer::kill)
			this->cv.wait(lk);

		this->ready = false;
	}

	int loopTimer::set_wait_time(std::chrono::microseconds wait_time) {
		this->wait_time = wait_time;

		return 0;
	}

	std::chrono::microseconds loopTimer::get_wait_time() {
		return this->wait_time;
	}
}