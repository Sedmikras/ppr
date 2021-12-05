#pragma once

#include <cmath>
#include <optional>
#include <fstream>

#include "watchdog.h"
#include "default_config.h"
#include "bit_masker.h"

namespace percentile_finder {
    /**
     * Struct with all info for algorithm
     */
    struct PercentileFinderConfig {
        size_t filesize = 0;
        uint64_t total_number_count = 0;
        uint64_t numbers_before = 0;
        uint32_t iterations = 0;
        uint8_t looked_up_percentile = 0;
    };

    /**
     * * USED ACROSS ALL VERSIONS
     * Last stage of algorithm - numbers are masked by last XX bits (default 23bits)
     * reads data from file and sets position of a number in file (number is known)
     * @param file to read numbers from
     * @param config pointer to config of percentile finder - contains needed variables (@see{PercentileFinderConfig})
     * @param masker pointer to number bit masker
     * @param pr result of last part of algorithm (finding index of bucket which contains percentile number) @see{get_bucket_index or default_config.h}
     * @param watchdog pointer to watchdog for notifications
     * @return ResolverResult which contains found percentile number and its positions in file or value <NAN, Position {NULL,NULL}> if percentile was not found
     */
    ResolverResult find_result_last_stage(std::ifstream &file, PercentileFinderConfig* config, NumberMasker* masker, PartialResult pr, Watchdog* watchdog, std::vector<double>* data_buffer);


    /**
     * * USED ACROSS ALL VERSIONS
     * Return index of a bucket where the percentile is
     * @param buckets vector of bucket_histogram of numbers in buckets
     * @param config pointer to config of percentile finder - contains needed variables (@see{PercentileFinderConfig})
     * @param watchdog pointer to watchdog for notifications
     * @return PartialResult - information about bucket. How many numbers are there in the bucket and index of bucket
     */
    PartialResult get_bucket_index(std::vector<uint64_t> buckets, PercentileFinderConfig* config, Watchdog* watchdog);

    /**
     * * USED ACROSS ALL VERSIONS
     * If there is enough memory, algorithm ends before last stage
     * returns index of number in sorted vector which is looked up percentile
     * @param sorted_vector sorted vector of numbers in bucket which contains looked up percentile
     * @param config pointer to config of percentile finder - contains needed variables (@see{PercentileFinderConfig})
     * @param watchdog pointer to watchdog for notifications
     * @return index of number in sorted vector - this is result !
     */
    uint32_t get_index_from_sorted_vector(const std::vector<double>& sorted_vector, PercentileFinderConfig* config, Watchdog* watchdog);


    /**
     * goes through the file and find positions of file. When found, insert into map
     * USED ACROSS ALL VERSIONS
     * @param file ifstream
     * @param config config reference
     * @param masker number masker reference
     * @param pr PartialResult
     * @param watchdog watchdog reference for notyfing
     * @param data_buffer data buffer reference
     * @param results_buffer result "buffer" reference for storing numbers
     * @param position_map reference to position map, where the positions are stored
     */
    void find_positions(std::ifstream& file, PercentileFinderConfig* config, NumberMasker* masker, PartialResult pr, Watchdog* watchdog, std::vector<double>* data_buffer, std::vector<double>* results_buffer, std::unordered_map<double, Position>* position_map);
    /**
     * Abstract class for percentile solvers.
     *
     * A percentile solver reads the input file as a sequence of doubles and finds
     * the number on the given percentile as well as the number's first and last position
     * in the file. Any double, that is not normal or zero is ignored.
     *
     * Basic algorithm:
     * 1) set bit mask for shifting values
     * 2) mask numbers in file (read file) using bit masking creating histogram (frequence of numbers in buckets)
     * 3) get index of bucket where percentile is located
     * 4)
     *      a) if all bits have been masked, continue          => 8
     *      b) if numbers in bucket can be contained in memory => 5
     *      c) if numbers in bucket can't contained in memory  => 1
     * 5) read eligible numbers into vector and safe their positions
     * 6) sort the vector and find percentile (number on percentile)
     * 7) (END)return number and position
     * 8) (END)read file for first and last position and return. It's known which number is the result cause all other have been masked
     *
     * Algorithm should end max after 4:
     * 3 - iterations (stages) which masks in default by 20,21,23 bits
     * 1 - iteration for finding positions in file of given percentile
     */
class PercentileFinder {

    public:
        Watchdog* watchdog{};
        /**
         * Find a value from file on the given percentile.
         *
         * @param percentile The percentile.
         * @return The result value or nullopt if the file doesn't contain any normal double.
         */
        virtual ResolverResult find_percentile(std::ifstream& file, uint8_t percentile);


        virtual void reset_config();

        /**
         * Virtual destructor to enable subclasses to cleanup after themselves.
         */
        virtual ~PercentileFinder() noexcept;

    /**
    * Initialize a new percentile solver.
    *
    * @param watchdog The watchdog.
    * @param max_interval_size The maximum size of the final interval.
    */
    explicit PercentileFinder() noexcept;

    /**
     * Constructor of percentile finder with pointer to watchdog
     * @param watchdog reference
     */
    explicit PercentileFinder(Watchdog* w) {
        this->watchdog = w;
    }

private:
    };
}

