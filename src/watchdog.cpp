#include <iostream>
#include "watchdog.h"
namespace percentile_finder {

    Watchdog::Watchdog(std::chrono::seconds timeout, std::function<void()> timeout_callback) noexcept:
        timeout(timeout),
        timeout_callback(std::move(timeout_callback)),
        stop_flag(false),
        condition_variable(std::make_unique<std::condition_variable>()),
        flag_mutex(std::make_unique<std::mutex>()),
        cond_var_mutex(std::make_unique<std::mutex>()),
        old_value(0),
        new_value(0) {}

    void Watchdog::stop() {
        std::unique_lock<std::mutex> lock(*flag_mutex);
        stop_flag = true;
        condition_variable->notify_one();
        lock.unlock();

        if (thread.joinable()) {
            thread.join();
        }
    }

    void Watchdog::notify() {
        std::lock_guard<std::mutex> lock(*flag_mutex);
        if(!thread.joinable() && !stop_flag) {
            initialize();
        }
        new_value++;
    }

    void Watchdog::initialize() {
        new_value = 1;
        thread = std::thread(&Watchdog::run, this);
    }

    void Watchdog::run() {
        std::unique_lock<std::mutex> thread_lock (*cond_var_mutex);
        while(!stop_flag) {
            std::this_thread::sleep_for(timeout);
            std::unique_lock<std::mutex> lock(*flag_mutex);
            if(new_value == old_value) {
                std::wcout << "program stopped responding";
            } else {
                new_value = 0;
                old_value = 0;
            }
        }
    }

    Watchdog::~Watchdog() {
        if(thread.joinable()) {
            thread.join();
        }
    }
}