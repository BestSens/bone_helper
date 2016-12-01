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
#include <condition_variable>
#include <mutex>
#include <syslog.h>

namespace bestsens {
    class loopTimer {
    public:
        loopTimer(std::chrono::microseconds wait_time, int start_value);
        loopTimer(int wait_time_ms, int start_value) : loopTimer(std::chrono::milliseconds(wait_time_ms), start_value) {};
        ~loopTimer();

        void wait_on_tick();

        int set_wait_time(std::chrono::microseconds wait_time);
        std::chrono::microseconds get_wait_time();
    private:
        bool ready;
        std::mutex m;
        std::condition_variable cv;

        std::thread timer_thread;
        std::chrono::microseconds wait_time;

        int running;

        void start(int start_value);
        void stop();
    };

    loopTimer::loopTimer(std::chrono::microseconds wait_time, int start_value = 0) {
        this->wait_time = wait_time;

        this->start(start_value);
    }

    loopTimer::~loopTimer() {
        this->stop();
        wait_on_tick();
        this->timer_thread.join();
    }

    void loopTimer::stop() {
        std::lock_guard<std::mutex> lk(this->m);
        this->running = 0;
        this->ready = true;
        this->cv.notify_all();
    }

    void loopTimer::start(int start_value = 0) {
        this->running = 1;

        this->ready = (start_value == 1);
        new (&this->timer_thread) std::thread([this] {
            while(this->running) {
                std::this_thread::sleep_for(this->wait_time);
                std::lock_guard<std::mutex> lk(this->m);
                this->ready = true;
                this->cv.notify_all();
            }

            this->ready = true;

            return EXIT_SUCCESS;
        });
    }

    void loopTimer::wait_on_tick() {
        std::unique_lock<std::mutex> lk(this->m);

        while(!this->ready)
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

#endif /* LOOPTIMER_HPP_ */
