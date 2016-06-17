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
#include <semaphore.h>
#include <syslog.h>

namespace bestsens {
    class loopTimer {
    public:
        loopTimer(std::chrono::microseconds wait_time, int start_value);
        loopTimer(int wait_time_ms, int start_value) : loopTimer(std::chrono::milliseconds(wait_time_ms), start_value) {};
        ~loopTimer();

        void stop();
        int wait_on_tick();
    private:
        sem_t * semaphore;

        std::thread timer_thread;

        int running;
        static int timing_thread(std::chrono::microseconds wait_time, sem_t * mutex, int * running);
    };

    loopTimer::loopTimer(std::chrono::microseconds wait_time, int start_value = 0) {
        this->running = 1;
        this->semaphore = new sem_t;

        sem_init(this->semaphore, 0, start_value);

        new (&this->timer_thread) std::thread(timing_thread, wait_time, this->semaphore, &(this->running));
    }

    loopTimer::~loopTimer() {
        this->running = 0;
        wait_on_tick();
        this->timer_thread.join();
        sem_destroy(this->semaphore);
    }

    int loopTimer::timing_thread(std::chrono::microseconds wait_time, sem_t * semaphore, int * running) {
    	while(*running) {
    		std::this_thread::sleep_for(wait_time);
    		sem_post(semaphore);
    	}

    	return EXIT_SUCCESS;
    }

    void loopTimer::stop() {
        this->running = 0;
    }

    int loopTimer::wait_on_tick() {
        return sem_wait(this->semaphore);
    }
}

#endif /* LOOPTIMER_HPP_ */
