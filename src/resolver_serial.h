#pragma once

#include "resolver.h"
#include "bit_masker.h"
#include "default_config.h"

namespace percentile_finder {


    /**
     * Serial percentile finder.
     *
     */
    class PercentileFinderSerial : public PercentileFinder {
    public:
        PercentileFinderSerial() noexcept;
        /**
         * Initialize a serial percentile finder.
         *
         * @param watchdog The watchdog.
         */

        PercentileFinderSerial(Watchdog* w) noexcept;

        ~PercentileFinderSerial() override;

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
         * config
         */
        PercentileFinderConfig config;
        /**
         * number masker instance
         */
        NumberMasker masker;
        /**
         * data buffer
         */
        std::vector<double> data_buffer;

        /**
        * Find a value from file on the given percentile.
        *
        * @param ifstream - opened file stream
        * @param percentile The percentile.
        * @return The result value or nullopt if the file doesn't contain any normal double.
        */
        ResolverResult find_result(std::ifstream& file, uint8_t percentile);

        /**
         * resolve fills histogram
         * @param file
         * @return partial result with index of bucket and number of elements in bucket
         */
        PartialResult resolve(std::ifstream& file);
    };
}
