#pragma once

#include "resolver.h"
#include "resolver_serial.h"

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

        uint32_t iterations = 0;
        NumberMasker masker;
        ResolverResult find_result(std::ifstream& file, uint8_t percentile);
        void eval(std::vector<uint64_t> frequencies, uint8_t percentile, PartialResult* pr);
        void resolve(std::ifstream& file, uint64_t filesize, uint32_t iterations, uint8_t percentile, PartialResult* pr);
    };
}
