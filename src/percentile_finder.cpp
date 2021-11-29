#include "resolver.h"
#include "resolver_serial.h"
#include "resolver_parallel.h"
#include <string>
#include <iostream>
#include <algorithm>
#include <execution>
#include <chrono>

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

	for (uint8_t i = 1; i <= 100; i++) {
		config.percentile = i;
		try {
            auto start = std::chrono::system_clock::now();
            auto r3 = finder_naive->find_percentile(file, i);
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
            //std::wcout << "[" << i << "]:" << r1.result << "\n";
            //std::wcout << "[" << i << "]:" << r3.result << ";" <<  r2.result << "\n";
            //std::wcout << "[" << i << "]:" << r2.result << ";" <<  r1.result << "\n";
            std::wcout << "[" << i << "]:" <<r3.result << ";" << r2.result << ";" <<  r1.result << "\n";
        }
        catch (const std::exception& e) {
                std::wcout << "Error occurred: " << e.what() << std::endl;

        }
		
	}
}


int main(int argc, char* argv[])
{
	//test_masker();
	//test_resolver(argc, argv);
	percentile_finder::Config config = percentile_finder::parse_arguments(argc, argv);
	try {
		percentile_finder::solve(config);
	}
	catch (const std::exception& e) {
		std::wcout << "Error occurred: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}