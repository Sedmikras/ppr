#pragma once

#include "resolver.h"
#include "resolver_serial.h"
#include "default_config.h"

namespace percentile_finder {
  
    /**
     * Serial percentile finder.
     *
     */
    class PercentileFinderParallel : public PercentileFinder {
    public:
        /**
         * Initialize a serial percentile finder.
         *
         * @param watchdog The watchdog.
         */
        PercentileFinderParallel(Watchdog watchdog) noexcept;

        /**
       * Find a value from file on the given percentile.
       *
       * @param ifstream - opened file stream
       * @param percentile The percentile.
       * @return The result value or nullopt if the file doesn't contain any normal double.
       */
        ResolverResult find_percentile(std::ifstream& file, uint8_t percentile) override;

        /**
        * resets configuration - usefull when testing multiple percentile in one run
        */
        void reset_config() override;

    private:
        /**
        * filesize so it don't have to read it again
        */
        uint64_t filesize = 0;

        /**
        * count of all numbers in a file
        */
        uint64_t numbers_count = 0;
        /**
        * numbers
        */
        uint64_t numbers_before = 0;

        std::vector<double_t> data_buffer;
        std::vector<double_t> parsed_buffer;

        std::vector<uint64_t> frequencies;

        uint32_t iterations = 0;
        NumberMasker masker;
        ResolverResult find_result(std::ifstream& file, uint8_t percentile);
        void eval(std::vector<uint64_t> frequencies, uint8_t percentile, PartialResult* pr);
        void resolve(void* numbers_counter);
        void barrier_wait();
        void read_data(std::ifstream file, size_t to_read);
        //void read_data(std::ifstream file, size_t to_read);
        std::mutex mMutex;
        std::condition_variable mCond;
        std::size_t mThreshold = 2;
        std::size_t mCount = 2;
        std::size_t mGeneration = 0;
    };
}
