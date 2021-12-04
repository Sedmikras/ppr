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
        PercentileFinderConfig config;
        NumberMasker masker;
        std::vector<double> data_buffer;
        ResolverResult find_result(std::ifstream& file, uint8_t percentile);
        PartialResult resolve(std::ifstream& file);
    };
}
