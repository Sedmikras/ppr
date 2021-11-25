#pragma once

#include "resolver.h"

namespace percentile_finder {

    /**
    * structure for saving border values
    * for next iteration of algorithm (finds only low > number < high
    */
    struct Border {
        double low;
        double high;
    };


    /**
    * Stage of algorithm
    */
    enum class Stage {
        FIRST,
        SECOND,
        LAST

    };

    /**
    * Class
    * Number masker returns uint32_t index from double value 
    * indexing depends on stage of algorithm
    */
    class NumberMasker {
    public:
        /**
        * stage contains iteration of algorithm
        * First iteration
        * Second and last iteration
        * masking bits from the number
        */
        Stage stage = Stage::FIRST;
        /**
        * index from first phase of algorithm
        */
        uint32_t zero_phase_index = 0;
        /**
        * lowest value for next iterations
        */
        double low = -std::numeric_limits<double>::max();
        /**
        * highest value for next iterations
        */
        double high = std::numeric_limits<double>::max();
        /**
        * returns index from double value by masking it as int by bits
        * @param number double number
        */
        uint32_t return_index_from_double(double number) const;
        /**
        * increment stage of algorithm and sets border values (low, high)
        * @param index - index from last iteration
        */
        void increment_stage(uint32_t index);
        /**
        * returns size of vector to use when masking numbers
        */
        uint32_t get_masked_vector_size();
        /**
        * returns border values for next stage of algorithm
        * @param index
        */
        Border get_border_values(uint32_t index);
        /**
        * constructor - sets stage = 0
        */
        NumberMasker();
    private:
            /**
            * returns border for the second stage
            */
            Border get_border_values_second_stage(uint32_t index);
            /**
             * returns border for the last stage
             */
            Border get_border_values_last_stage(uint32_t index);
            /**
            * returns index of bucket when in first stage
            */
            uint32_t return_index_from_double_first_stage(double number) const;
            /**
            * returns index of bucket when in second stage
            */
            uint32_t return_index_from_double_second_stage(double number) const;
            /**
            * returns index of bucket when in last stage
            */
            uint32_t return_index_from_double_last_stage(double number) const;
    };

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
        //PercentileFinderSerial(Watchdog watchdog) noexcept;

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
