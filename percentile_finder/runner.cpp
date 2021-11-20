#include "resolver.h"
#include "resolver_serial.h"
#include "default_config.h"
#include <string>
#include <iostream>

namespace percentile_finder {

	void end_with_error_message(ERRORS e) {
		switch (e)
		{
		case percentile_finder::ERRORS::NOT_ENOUGH_ARGUMENTS:
		{
			std::wcout << "INVALID NUMBER OF ARGUMENTS";
			std::exit((int)ERRORS::NOT_ENOUGH_ARGUMENTS);
		}
		break;
		case percentile_finder::ERRORS::UNKNOWN_DEVICE_NAME:
		{
			std::wcout << "UNKNOWN OPENCL DEVICE";
			std::exit((int)ERRORS::UNKNOWN_DEVICE_NAME);
		}
		break;
		case percentile_finder::ERRORS::INVALID_FILE_NAME:
		{
			std::wcout << "FILE NAME IS NOT VALID - IS NOT READABLE";
			std::exit((int)ERRORS::INVALID_FILE_NAME);
		}
		break;
		case percentile_finder::ERRORS::INVALID_PERCENTILE:
		{
			std::wcout << "INVALID_PERCENTILE - must be [1,100]";
			std::exit((int)ERRORS::INVALID_PERCENTILE);
		}
		break;
		case percentile_finder::ERRORS::NOT_RESPONDING:
		{
			std::wcout << "APP IS NOT RESPONDING";
			std::exit((int)ERRORS::INVALID_PERCENTILE);
		}
		case percentile_finder::ERRORS::FINDER_DIVERGING:
		{
			std::wcout << "APP IS DIVERGING FROM RESULT";
			std::exit((int)ERRORS::INVALID_PERCENTILE);
		}
		default:
			break;
		}

	}

	bool file_exists(std::string file_path) {
		std::ifstream ifile;
		ifile.open(file_path);
		if (ifile) {
			return true;
		}
		else {
			return false;
		}
	}

	Config parse_arguments(int argc, char* argv[]) {
		Config config;

		std::string file_name;
		uint8_t percentile;


		if (argc != 4) {
			end_with_error_message(ERRORS::NOT_ENOUGH_ARGUMENTS);
		}

		file_name = argv[1];
		if (!file_exists(file_name)) {
			end_with_error_message(ERRORS::INVALID_FILE_NAME);
		}
		config.filename = file_name;


		percentile = std::stoi(argv[2]);
		if (percentile > 100 || percentile == 0) {
			end_with_error_message(ERRORS::INVALID_PERCENTILE);
		}
		else {
			config.percentile = percentile;
		}

		std::string exec_mode_string = argv[3];
		if (exec_mode_string == "single") {
			config.solver_type = FinderType::Serial;
		}
		else if (exec_mode_string == "SMP") {
			config.solver_type = FinderType::SMP;
		}
		else {
			config.solver_type = FinderType::OpenCL;
			config.device_name = argv[3];
		}
		return config;

	}

	void solve(Config config) {
		// setup watchdog
		percentile_finder::Watchdog watchdog(std::chrono::seconds(DEFAULT_TIMEOUT),
			[]() {
				end_with_error_message(ERRORS::FINDER_DIVERGING);
			},
			[]() {
				end_with_error_message(ERRORS::NOT_RESPONDING);
			});


		// setup percentile finder
		std::unique_ptr<percentile_finder::PercentileFinder> finder;
		switch (config.solver_type)
		{
		case FinderType::Serial: {
			finder = std::make_unique<percentile_finder::PercentileFinderSerial>(std::move(watchdog));
		}
							   break;
		case FinderType::UNLIMITED_RAM_TESTER: {
			//finder = std::make_unique<percentile_finder::PercentileFinder>(std::move(watchdog));
		} break;
		}
		std::ifstream file(config.filename, std::ios::binary);
		auto result = finder->find_percentile(file, config.percentile);
		if (result.result != NAN) {
			std::wcout << std::hexfloat << result.result << std::dec
				<< " " <<
				result.position.first
				<< " " <<
				result.position.last
				<< std::endl;
		}
		else {
			std::wcout << result.result << std::endl;
		}

	}

	int main(int argc, char* argv[])
	{
		Config config =	parse_arguments(argc, argv);
		try {
			solve(config);
		}
		catch (const std::exception& e) {
			std::wcout << "Error occurred: " << e.what() << std::endl;
			return 1;
		}

		return 0;

	}
}