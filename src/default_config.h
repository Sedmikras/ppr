#pragma once
#include <cstdint>
#include <chrono>
#include <string>
#include <mutex>
#include <unordered_map>

namespace percentile_finder {
    // ------------------------- CONSTANTS ----------------------------------------------------------------------

    /**
     * MAX_LIVE_TOKENS - number of parallel pipelines in SMP
     */
    const unsigned int MAX_LIVE_TOKENS = 12;
    /**
     * MAX_BUFFER_SIZE set to 128MB cause there is limit of 250MB - for reading and checking if memory is OK
     */
	const std::uint64_t MAX_BUFFER_SIZE = (1 << 27);

    /**
     * Max buffer size for OpenCL
     */
    const std::uint64_t MAX_BUFFER_SIZE_OPENCL = MAX_BUFFER_SIZE / 6;

    /**
     * Max vector size for OpenCL
     */
    const std::uint64_t MAX_VECTOR_SIZE_OPENCL = MAX_BUFFER_SIZE_OPENCL / 8;
    /**
     * MAX_VECTOR_SIZE same as MAX_BUFFER_SIZE but for vectors of double (128MB)
     */
	const std::uint64_t MAX_VECTOR_SIZE = MAX_BUFFER_SIZE / 8;
    /**
     * sets max vector size for parallel processing
     */
    const std::uint64_t MAX_VECTOR_SIZE_PARALLEL = MAX_VECTOR_SIZE / 16;
    /**
     * bit shift in first stage
     */
	const std::uint8_t BIT_SHIFT_FIRST_STAGE = 20;
    /**
     * bit shift in second stage
     */
	const std::uint8_t BIT_SHIFT_SECOND_STAGE = 21;
    /**
     * bit shift in last stage
     */
	const std::uint8_t BIT_SHIFT_LAST_STAGE = 23;
    /**
     * bit shift in last stage
     */
    const std::uint8_t BIT_SHIFT_AAAA = 8;
    /**
     * used in masking numbers rshift first stage
     */
    const std::uint8_t LEFT_SHIFT_COMPLEMENT_FIRST_STAGE = 64 - BIT_SHIFT_FIRST_STAGE;
    /**
     * used in masking numbers rshift second stage
     */
    const std::uint8_t LEFT_SHIFT_COMPLEMENT_SECOND_STAGE = LEFT_SHIFT_COMPLEMENT_FIRST_STAGE - BIT_SHIFT_SECOND_STAGE;

    /**
     * splits first interval into positive and negative numbers
     */
	const uint32_t SPLITERATOR_FIRST_INDEX = (1 << (BIT_SHIFT_FIRST_STAGE - 1));

    /**
     * bit mask used in number masking first stage
     */
	const uint32_t BIT_MASK_FIRST_STAGE = SPLITERATOR_FIRST_INDEX - 1;
    /**
     * bit mask used in number masking second stage
     */
	const uint32_t BIT_MASK_SECOND_STAGE = (1 << BIT_SHIFT_SECOND_STAGE) - 1;
    /**
     * bit mask used in number masking last stage
     */
	const uint32_t BIT_MASK_LAST_STAGE = (1 << BIT_SHIFT_LAST_STAGE) - 1;

    const std::chrono::seconds DEFAULT_TIMEOUT = std::chrono::seconds(20);

	/**
	 * The type of percentile solver.
	 */
	enum class FinderType {
		Serial,
		SMP,
		OpenCL,
		UNLIMITED_RAM_TESTER
	};

    // ------------------------- STRUCTS ----------------------------------------------------------------------

    /**
     * Config for passing to PercentileFinders
     */
	struct Config {
		std::string filename = "test.txt";
		uint8_t percentile = 50;
		FinderType solver_type = FinderType::Serial;
		std::string device_name = "";
        bool benchmark = false;
	};

    /**
     * enum of possible errors
     */
	enum class ERRORS : int {
		NOT_ENOUGH_ARGUMENTS=-1,
		UNKNOWN_DEVICE_NAME=-2,
		INVALID_FILE_NAME=-3,
		INVALID_PERCENTILE=-4,
		NOT_RESPONDING=-5,
		FINDER_DIVERGING=-6

	};

    /**
    * A value's positions - first and last occurrence in the file and bool if any position was found.
    */
    struct Position {
        uint64_t first;
        uint64_t last;
    };

    /**
     * Struct that contains final result of the algorithm
     */
    struct ResolverResult {
        /**
         * number found on percentile
         */
        double result;
        /**
         * position of the number in given file
         */
        Position position;
    };

    /**
     * Struct that contains partial result - when bucket_index of the bucket is found
     */
    struct PartialResult {
        /**
         * count of numbers in index
         */
        uint64_t numbers_in_index;
        /**
         * index of a bucket
         */
        uint32_t bucket_index;
    };

    /**
     * Contains buckets and count of all numbers
     */
    struct Histogram {
        std::vector<uint64_t> buckets;
        uint64_t  numbers_count;
    };

    // ------------------------- CLASSES ----------------------------------------------------------------------

    /**
     * PositionsMap class for thread safe inserting into map cause it can be accessed in parallel
     */
    class PositionsMap {
        public:

        /**
         * Inserts or update position given by key
         * Position is computed using bucket_index and first position of file
         * @param key key to positions map
         * @param index bucket_index of number (for computing new position)
         * @param first_pos starting position of a file (for computing a new position)
         */
        void position_insert_update_thread_safe(double key, uint32_t index, size_t first_pos);

        /**
         * Gets element from the map if exists
         * NOT THREAD SAFE
         * @param key to the map (double value)
         * @return element in map with given key or  Position{NULL,NULL}
         */
        Position get(double key);

        /**
         * destructor
         */
        virtual ~PositionsMap() {
            positions.clear();
        }

    private:
        /**
         * mutex for safe access to map
         */
        std::mutex mutex;
        /**
         * unordered map with positions
         */
        std::unordered_map<double, Position> positions;
    };

    // ------------------------- FUNCTIONS ----------------------------------------------------------------------

    /**
     * End program with error message
     * @param e error message
     */
    void end_with_error_message(ERRORS e);

    /**
     * Prints error to the console
     * @param error
     */
    void print_error_message(ERRORS e);

    /**
     * Ends without message to the console
     * @param e
     */
    void end_without_message(ERRORS e);

    /**
     * Checks if file exists
     * @param file_path absolute or relative path
     * @return true if exists
     */
    bool file_exists(const std::string &file_path);

    /**
     * Parses arguments given to the program and set them into structure @see{Config}
     * @param argc
     * @param argv
     * @return config structure with parsed arguments
     */
    Config parse_arguments(int argc, char *argv[]);

    /**
     * Main function - creates finder, finds percentile, writes result to user, ends
     * @param config user config parsed from arguments
     */
    void solve(Config config);

    /**
     * Utils I/O resets filereader to beginning position and clears errors
     * @param file
     */
    void reset_filereader(std::ifstream& file);
}