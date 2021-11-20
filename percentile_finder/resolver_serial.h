#pragma once

#include "resolver.h"

namespace percentile_finder {

    class NumberMasker {
        public:
            uint8_t phase;
            uint32_t zero_phase_index = 0;
            double low = -std::numeric_limits<double>::max();
            double high = std::numeric_limits<double>::max();
            uint32_t return_index_from_double(double number);
            void increment_phase(uint32_t index);
            uint32_t get_masked_vector_size();
            NumberMasker();
    private:
            uint32_t return_index_from_double_first_phase(double number);
    };

    /**
     * Serial percentile solver.
     *
     */
    class PercentileFinderSerial : public PercentileFinder {
    public:
        /**
         * Initialize a serial percentile solver.
         *
         * Total size of the internal buffer is `max_interval_size` + `load_buffer_size`.
         *
         * @param watchdog The watchdog.
         * @param max_interval_size The size of the final interval.
         * @param load_buffer_size The size of the loading buffer.
         */
        PercentileFinderSerial(Watchdog watchdog) noexcept;

        /**
       * Find a value from file on the given percentile.
       *
       * @param percentile The percentile.
       * @return The result value or nullopt if the file doesn't contain any normal double.
       */
        ResolverResult find_percentile(std::ifstream& file, uint8_t percentile) override;

    private:
        uint64_t filesize = 0;
        uint64_t numbers_count = 0;
        uint64_t numbers_before = 0;
        NumberMasker masker;
        ResolverResult find_result(std::ifstream& file, uint8_t percentile);
        void eval(std::vector<uint64_t> frequencies, uint8_t percentile, PartialResult* pr);
        void resolve(std::ifstream& file, uint64_t filesize, uint32_t iterations, uint8_t percentile, PartialResult* pr);
    };
}
