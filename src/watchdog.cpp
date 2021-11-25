#include "watchdog.h"
namespace percentile_finder {

    Watchdog::Watchdog(std::chrono::seconds timeout, std::function<void()> divergence_callback, std::function<void()> timeout_callback) noexcept:
        timeout(timeout),
        divergence_callback(std::move(divergence_callback)),
        timeout_callback(std::move(timeout_callback)),
        stop_flag(false),
        start_flag(false),
        condition_variable(std::make_unique<std::condition_variable>()),
        mutex(std::make_unique<std::mutex>()),
        old_value(0),
        new_value(0) {}

    void Watchdog::run() {
        stop_flag = false;

        // run watchdog in separate thread
        thread = std::thread([this]() {
            std::unique_lock<std::mutex> lock(*mutex);

            while (!stop_flag) {
                bool result = condition_variable->wait_for(lock, timeout, [this]() {
                    return stop_flag || !start_flag;
                    });

                if (!result) {
                    // watched subject timed out
                    timeout_callback();
                }
                else if (new_value == old_value) {
                    divergence_callback();
                }
                old_value = new_value;
            }
        });
    }

    void Watchdog::stop() {
        std::unique_lock<std::mutex> lock(*mutex);
        stop_flag = true;
        condition_variable->notify_one();
        lock.unlock();

        if (thread.joinable()) {
            thread.join();
        }
    }

    void Watchdog::notify() {
        std::lock_guard<std::mutex> lock(*mutex);
        new_value++;
    }

    Watchdog::Watchdog(Watchdog&& other) noexcept : 
        timeout(other.timeout),
        divergence_callback(other.divergence_callback),
        timeout_callback(other.timeout_callback),
        stop_flag(other.stop_flag),
        start_flag(other.start_flag),
        condition_variable(std::move(other.condition_variable)),
        mutex(std::move(other.mutex)),
        thread(std::move(other.thread)),
        old_value(other.old_value),
        new_value(other.new_value) {}

}