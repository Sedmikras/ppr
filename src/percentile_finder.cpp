#include "resolver.h"
#include "resolver_serial.h"
#include "resolver_parallel.h"
#include <string>
#include <iostream>
#include <algorithm>
#include <execution>

namespace percentile_finder {

	void end_with_error_message(ERRORS e) {
		switch (e)
		{
		case percentile_finder::ERRORS::NOT_ENOUGH_ARGUMENTS:
		{
			std::wcout << "INVALID NUMBER OF ARGUMENTS";
			std::exit((int)ERRORS::NOT_ENOUGH_ARGUMENTS);
		}

		case percentile_finder::ERRORS::UNKNOWN_DEVICE_NAME:
		{
			std::wcout << "UNKNOWN OPENCL DEVICE";
			std::exit((int)ERRORS::UNKNOWN_DEVICE_NAME);
		}

		case percentile_finder::ERRORS::INVALID_FILE_NAME:
		{
			std::wcout << "FILE NAME IS NOT VALID - IS NOT READABLE";
			std::exit((int)ERRORS::INVALID_FILE_NAME);
		}

		case percentile_finder::ERRORS::INVALID_PERCENTILE:
		{
			std::wcout << "INVALID_PERCENTILE - must be [1,100]";
			std::exit((int)ERRORS::INVALID_PERCENTILE);
		}

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

	bool file_exists(const std::string& file_path) {
		std::ifstream file;
		file.open(file_path);
		if (file) {
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

	void solve(const Config& config) {
		/*percentile_finder::Watchdog watchdog(std::chrono::seconds(DEFAULT_TIMEOUT),
		[]() {
			end_with_error_message(ERRORS::FINDER_DIVERGING);
		});


		// setup percentile finder
		std::unique_ptr<percentile_finder::PercentileFinder> finder;
		switch (config.solver_type)
		{
            case FinderType::UNLIMITED_RAM_TESTER: {
                finder = std::make_unique<percentile_finder::PercentileFinder>(&watchdog); break;
            }
			case FinderType::Serial: {
				finder = std::make_unique<percentile_finder::PercentileFinderSerial>(&watchdog); break;
			}
			case FinderType::SMP: {
				finder = std::make_unique<percentile_finder::PercentileFinderParallel>(&watchdog); break;
			}
			case FinderType::OpenCL: {

			} break;
		}
		std::ifstream file(config.filename, std::ios::binary);
		auto result = finder->find_percentile(file, config.percentile);
		if (result.result != NAN) {
			std::wcout << std::dec << result.result << std::dec
				<< " " <<
				result.position.first
				<< " " <<
				result.position.last
				<< std::endl;
		}
		else {
			std::wcout << result.result << std::endl;
		}*/

	}
}

void test_masker() {
	percentile_finder::NumberMasker masker;
	uint32_t index = 0;
    std::vector<uint64_t> frequencies(2097152);
	for (int64_t i = INT64_MIN; i <= INT64_MAX; i = i++) {
		double value = *(double*)&i;
		index = masker.return_index_from_double(value);
		if (index != UINT32_MAX) {
			frequencies[index]++;
			percentile_finder::Border b = masker.get_border_values(index);
			if ((value < b.low) || (value > b.high)) {
				std::cout << index << value << i << "\n";
			}
		}	
	}
}

void test_resolver(int argc, char* argv[]) {
    percentile_finder::Watchdog w (std::chrono::seconds(15),
            []() {
                end_with_error_message(percentile_finder::ERRORS::NOT_RESPONDING);
            }
    );

	percentile_finder::Config config = percentile_finder::parse_arguments(argc, argv);
	std::unique_ptr<percentile_finder::PercentileFinder> finder, finder_serial, finder_naive;
	finder = std::make_unique<percentile_finder::PercentileFinderParallel>(&w);
    finder_serial = std::make_unique<percentile_finder::PercentileFinderSerial>(&w);
    finder_naive = std::make_unique<percentile_finder::PercentileFinder>(&w);


	std::ifstream file(config.filename, std::ios::binary);

	for (int i = 1; i <= 100; i++) {
		config.percentile = i;
		try {
            auto start = std::chrono::system_clock::now();
           // auto r3 = finder_naive->find_percentile(file, i);
            auto end = std::chrono::system_clock::now();
            auto ms_int = duration_cast<std::chrono::milliseconds>(end - start);
            std::wcout << "naive:" << ms_int << "\n";
            w.notify();
            start = std::chrono::system_clock::now();
            auto r2 = finder_serial->find_percentile(file, i);
            end = std::chrono::system_clock::now();
            ms_int = duration_cast<std::chrono::milliseconds>(end - start);
            std::wcout << "serial:"<< ms_int << "\n";
            start = std::chrono::system_clock::now();
            auto r1 = finder->find_percentile(file, i);
            end = std::chrono::system_clock::now();
            ms_int = duration_cast<std::chrono::milliseconds>(end - start);
            std::wcout << "parallel:"<< ms_int << "\n";
            w.stop();
            std::wcout << "[" << i << "]:" << r2.result << ";" <<  r1.result << "\n";
            //std::wcout << "[" << i << "]:" <<r3.result << ";" << r2.result << ";" <<  r1.result << "\n";
        }
        catch (const std::exception& e) {
                std::wcout << "Error occurred: " << e.what() << std::endl;

        }
		
	}
}


int main(int argc, char* argv[])
{
	//test_masker();
	test_resolver(argc, argv);
	/*percentile_finder::Config config = percentile_finder::parse_arguments(argc, argv);
	try {
		percentile_finder::solve(config);
	}
	catch (const std::exception& e) {
		std::wcout << "Error occurred: " << e.what() << std::endl;
		return 1;
	}*/
	return 0;
}