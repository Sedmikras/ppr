#pragma once
#include <cstdint>
#include <chrono>
namespace percentile_finder {
	const std::uint64_t MAX_BUFFER_SIZE = (1 << 27);
	const std::uint64_t MAX_VECTOR_SIZE = MAX_BUFFER_SIZE / 8;
	const std::uint8_t PHASE_ZERO_BITS = 21;
	const std::uint8_t PHASE_ONE_BITS = 21;
	const std::uint8_t PHASE_TWO_BITS = 22;
	const std::chrono::seconds DEFAULT_TIMEOUT = std::chrono::seconds(3);
	const uint32_t FIRST_INDEX = (1 << (PHASE_ZERO_BITS-1));

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
}