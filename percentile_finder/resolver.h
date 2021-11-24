#pragma once

#include <cmath>
#include <optional>

#include "filereader.h"
#include "watchdog.h"

namespace percentile_finder {

    /**
     * A value's positions - first and last occurrence in the file and bool if any position was found.
     */
    struct Position {
        uint64_t first;
        uint64_t last;
    };

    struct ResolverResult {
        double result;
        Position position;
    };

    struct PartialResult {
        uint64_t numbers_count;
        uint64_t numbers_in_index;
        uint32_t index;
    };

    /**
     * Abstract class for percentile solvers.
     *
     * A percentile solver reads the input file as a sequence of doubles and finds
     * the number on the given percentile as well as the number's first and last position
     * in the file. Any double, that is not normal or zero is ignored.
     *
     * Basic algorithm:
     * 1. Choose a random pivot number from file, lower bound = -inf, upper bound = inf
     * 2. Read file and count number of elements in each interval:
     *      - lt: lower < x < pivot
     *      - eq: x == pivot
     *      - gt: pivot < x < upper
     * 3. On first pass compute the index of the value on the percentile based on total number of elements (lt + eq + gt).
     * 4. Determine in which interval the value lies, and update pivot and bounds:
     *      - lt => upper = pivot, pivot = random number from file that lies in lt
     *      - eq => value on the percentile found = pivot --> go to 7.
     *      - gt => lower = pivot, pivot = random number from file that lies in gt
     * 5. Update the index of the value = index of the value int the selected interval.
     * 6. Does the selected interval fits into the max allowed memory?
     *      - NO -> repeat from 2.
     *      - YES -> go to 7.
     * 7. Load all values from the interval and select the nth smallest value - the value on the percentile --> go to 7.
     * 8. Find value's first and last position in the file.
     *
     * Every concrete implementation must implement logic for one file pass,
     * loading and finding the nth smallest value from interval and searching value's positions in the file.
     *
     * The percentile solver has a watchdog, which monitors the progress of the computation by checking flag from the solver.
     * Implementation of the solver must periodically notify the watchdog.
     */
class PercentileFinder {

    public:
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

    protected:

        /**
         * The watchdog to monitor the computation.
         */
        Watchdog watchdog;


        /**
         * Initialize a new percentile solver.
         *
         * @param watchdog The watchdog.
         * @param max_interval_size The maximum size of the final interval.
         */
        explicit PercentileFinder(Watchdog watchdog) noexcept;

     private:
    };
}

