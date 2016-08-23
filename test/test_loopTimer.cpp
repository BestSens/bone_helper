#include <boost/test/unit_test.hpp>
#include <chrono>
#include "../loopTimer.hpp"

BOOST_AUTO_TEST_CASE(loopTimer_test) {
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
    BOOST_CHECK(std::chrono::duration<double>(runtime).count() >= 0.1);

    /*
     * check second tick
     */
    timer->wait_on_tick();
    calculation_end = std::chrono::steady_clock::now();
    runtime = calculation_end - calculation_start;
    BOOST_CHECK(std::chrono::duration<double>(runtime).count() >= 0.2);

    delete timer;

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
    BOOST_CHECK(std::chrono::duration<double>(runtime).count() < 0.1);

    /*
     * check second tick
     */
    timer2->wait_on_tick();
    calculation_end = std::chrono::steady_clock::now();
    runtime = calculation_end - calculation_start;
    BOOST_CHECK(std::chrono::duration<double>(runtime).count() < 0.2 && std::chrono::duration<double>(runtime).count() > 0.1);

    delete timer2;
}
