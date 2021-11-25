#pragma once
#include <cstdint>
#include <chrono>
#include <string>
namespace percentile_finder {
	const std::uint64_t MAX_BUFFER_SIZE = (1 << 27);
	const std::uint64_t MAX_VECTOR_SIZE = MAX_BUFFER_SIZE / 8;
	const std::uint64_t TEST_MAX_VECTOR_SIZE = 2;
	const std::uint8_t PHASE_ZERO_BITS = 13;
	const std::uint8_t PHASE_ONE_BITS = 23;
	const std::uint8_t PHASE_TWO_BITS = 28;
	const std::chrono::seconds DEFAULT_TIMEOUT = std::chrono::seconds(3);
	const uint32_t FIRST_INDEX = (1 << (PHASE_ZERO_BITS-1));
	const uint32_t MASK_ZERO = FIRST_INDEX - 1;
	const uint32_t MASK_ONE = (1 << PHASE_ONE_BITS) -1;
	const uint32_t MASK_TWO = (1 << PHASE_TWO_BITS)- 1;

	/**
	 * The type of percentile solver.
	 */
	enum class FinderType {
		Serial,
		SMP,
		OpenCL,
		UNLIMITED_RAM_TESTER
	};

	struct Config {
		std::string filename = "test.txt";
		uint8_t percentile = 50;
		FinderType solver_type = FinderType::Serial;
		std::string device_name = "";
	};

	enum class ERRORS {
		NOT_ENOUGH_ARGUMENTS,
		UNKNOWN_DEVICE_NAME,
		INVALID_FILE_NAME,
		INVALID_PERCENTILE,
		NOT_RESPONDING,
		FINDER_DIVERGING

	};

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

    struct Histogram {
        std::vector<uint64_t> buckets;
        uint64_t  numbers_count;
    };
}