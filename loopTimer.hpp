/*
 * loopTimer.hpp
 *
 *  Created on: 14.06.2016
 *      Author: Jan Schöppach
 */

#ifndef LOOPTIMER_HPP_
#define LOOPTIMER_HPP_

#include <thread>
#include <chrono>
#include <mutex>
#include <syslog.h>

namespace bestsens {
    class loopTimer {
    public:
        loopTimer(std::chrono::microseconds wait_time, int start_value);
        loopTimer(int wait_time_ms, int start_value) : loopTimer(std::chrono::milliseconds(wait_time_ms), start_value) {};
        ~loopTimer();

        void stop();
        void wait_on_tick();
    private:
        std::mutex m;
        std::thread timer_thread;
        std::chrono::microseconds wait_time;

        int running;
    };

    loopTimer::loopTimer(std::chrono::microseconds wait_time, int start_value = 0) {
        this->running = 1;
        this->wait_time = wait_time;

        if(start_value == 0)
            this->m.lock();

        new (&this->timer_thread) std::thread([this] {
            while(this->running) {
                std::this_thread::sleep_for(this->wait_time);
                this->m.unlock();
            }

            return EXIT_SUCCESS;
        });
    }

    loopTimer::~loopTimer() {
        this->running = 0;
        wait_on_tick();
        this->timer_thread.join();
    }

    void loopTimer::stop() {
        this->running = 0;
    }

    void loopTimer::wait_on_tick() {
        this->m.lock();
    }
}

#endif /* LOOPTIMER_HPP_ */
