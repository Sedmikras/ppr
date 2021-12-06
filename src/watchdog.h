#pragma once

#include <memory>
#include <chrono>
#include <condition_variable>
#include <functional>

namespace percentile_finder {

    /**
     * Watchdog for monitoring progress of a computation.
     * It checks if the subject is letting the watchdog know about itself
     * and if the value it provides is decreasing.
     */
    class Watchdog {
    public:
        virtual ~Watchdog();

        /**
         * Initialize a new watchdog.
         *
         * @param timeout The timeout indicating that the watched subject failed.
         * @param timeout_divergence The function to call on value divergence.
         * @param timeout_callback The function to call on timeout.
         */
        Watchdog(std::chrono::seconds timeout,
                 std::function<void()> timeout_callback) noexcept;

        /**
         * Notify the watchdog to stop executing and join its thread.
         */
        void stop();

        /**
         * Set activity flag and notify the watchdog.
         */
        void notify();

    private:
        /**
         * The timeout to wait fro notification from the watched subject.
         */
        std::chrono::milliseconds timeout;

        /**
         * The function to call on value divergence.
         */
        std::function<void()> timeout_callback;

        /**
         * Flag indicating whether to stop the watchdog execution.
         */
        bool stop_flag;


        /**
         * Mutex for the condition variable and flags.
         */
        std::unique_ptr<std::mutex> cond_var_mutex;

        /**
         * The condition variable for flags.
         */
        std::unique_ptr<std::condition_variable> condition_variable;

        /**
         * Mutex for the condition variable and flags.
         */
        std::unique_ptr<std::mutex> flag_mutex;

        /**
         * The running watchdog thread.
         */
        std::thread thread;

        /**
         * The old value.
         */
        std::uint64_t old_value;

        /**
         * The new value.
         */
        std::uint64_t new_value;

        /**
         * initialize watchdog - private
         */
        void initialize();

        /**
         * private run function
         */
        void run();
    };
}
