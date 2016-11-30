#include "libs/catch/include/catch.hpp"
#include <chrono>
#include "../loopTimer.hpp"

TEST_CASE("loopTimer_test") {
    /*
     * check first timer with delayed start
     */
    auto calculation_start = std::chrono::steady_clock::now();
    bestsens::loopTimer * timer = new bestsens::loopTimer(std::chrono::milliseconds(100), 0);

    /*
     * check first tick
     */
    timer->wait_on_tick();
    auto calculation_end = std::chrono::steady_clock::now();
    auto runtime = calculation_end - calculation_start;
    CHECK(std::chrono::duration<double>(runtime).count() == Approx(0.1).epsilon(0.025));

    /*
     * check second tick
     */
    timer->wait_on_tick();
    calculation_end = std::chrono::steady_clock::now();
    runtime = calculation_end - calculation_start;
    CHECK(std::chrono::duration<double>(runtime).count() == Approx(0.2).epsilon(0.025));

    /*
     * check second timer with direct start
     */
    calculation_start = std::chrono::steady_clock::now();
    bestsens::loopTimer * timer2 = new bestsens::loopTimer(std::chrono::milliseconds(100), 1);

    /*
     * check first tick
     */
    timer2->wait_on_tick();
    calculation_end = std::chrono::steady_clock::now();
    runtime = calculation_end - calculation_start;
    CHECK(std::chrono::duration<double>(runtime).count() < 0.1);

    /*
     * check second tick
     */
    timer2->wait_on_tick();
    calculation_end = std::chrono::steady_clock::now();
    runtime = calculation_end - calculation_start;
    CHECK(std::chrono::duration<double>(runtime).count() == Approx(0.1).epsilon(0.025));

    /*
     * get wait time
     */
    CHECK(timer2->get_wait_time() == std::chrono::milliseconds(100));

    /*
     * update wait time
     */
    timer2->set_wait_time(std::chrono::milliseconds(200));
    CHECK(timer2->get_wait_time() == std::chrono::milliseconds(200));

    timer2->wait_on_tick();
    calculation_end = std::chrono::steady_clock::now();
    auto runtime1 = calculation_end - calculation_start;

    timer2->wait_on_tick();
    calculation_end = std::chrono::steady_clock::now();
    auto runtime2 = calculation_end - calculation_start;

    runtime = runtime2 - runtime1;
    CHECK(std::chrono::duration<double>(runtime).count() == Approx(0.2).epsilon(0.025));
}
