#include "resolver.h"
#include "resolver_serial.h"
#include "resolver_parallel.h"
#include "resolver_opencl.h"
#include "open_cl_default_header.h"
#include "default_config.h"
#include <string>
#include <iostream>
#include <algorithm>
#include <chrono>

void test_resolver(int argc, char* argv[]) {
    percentile_finder::Watchdog w (percentile_finder::DEFAULT_TIMEOUT,
            []() {
                std::wcout << "Program is not responding. Wait for it ? Y/N";
                int response = getchar();
                if (response == 'N' || response == 'n')
                    end_with_error_message(percentile_finder::ERRORS::NOT_RESPONDING);
            }
    );

	percentile_finder::Config config = percentile_finder::parse_arguments(argc, argv);
	std::unique_ptr<percentile_finder::PercentileFinder> finder, finder_serial, finder_naive, finder_opencl;
	finder = std::make_unique<percentile_finder::PercentileFinderParallel>(&w);
    finder_serial = std::make_unique<percentile_finder::PercentileFinderSerial>(&w);
    finder_naive = std::make_unique<percentile_finder::PercentileFinder>(&w);
    auto devices = percentile_finder::get_cl_device_names();
    auto device = percentile_finder::get_device_by_name(devices[0]);
    finder_opencl = std::make_unique<percentile_finder::PercentileFinderOpenCL>(&w, device);


    std::ifstream file(config.filename, std::ios::binary);

	for (uint8_t i = 1; i <= 100; i++) {
		config.percentile = i;
		try {
            auto start = std::chrono::system_clock::now();
           auto r3 = finder_naive->find_percentile(file, i);

            auto end = std::chrono::system_clock::now();
            auto ms_int = std::chrono::duration<double, std::milli>(end - start);
            std::wcout << "TIMES:";
            std::wcout << "naive:" << ms_int.count() << "\n";
            w.notify();
            start = std::chrono::system_clock::now();
            auto r2 = finder_serial->find_percentile(file, i);
            end = std::chrono::system_clock::now();
            ms_int = std::chrono::duration<double, std::milli>(end - start);
            std::wcout << "serial:"<< ms_int.count() << "\n";
            start = std::chrono::system_clock::now();
            auto r1 = finder->find_percentile(file, i);
            end = std::chrono::system_clock::now();
            ms_int = std::chrono::duration<double, std::milli>(end - start);
            std::wcout << "parallel:"<< ms_int.count() << "\n";
            start = std::chrono::system_clock::now();
            auto r4 = finder_opencl->find_percentile(file, i);
            end = std::chrono::system_clock::now();
            ms_int = std::chrono::duration<double, std::milli>(end - start);
            std::wcout << "opencl:"<< ms_int.count() << "\n";
            w.stop();
            std::wcout << "POSITIONS:";
            std::wcout << "serial:" << r2.position.first << ";" << r2.position.last << "\n";
            std::wcout << "naive:" << r3.position.first << ";" << r3.position.last << "\n";
            std::wcout << "SMP:" << r1.position.first << ";" << r1.position.last << "\n";
            std::wcout << "OpenCL:" << r4.position.first << ";" << r4.position.last << "\n";
            std::wcout << "RESULTS:";
            std::wcout << "[" << i << "]:" <<r3.result << ";" << r2.result << ";" <<  r1.result << ";" << r4.result << "\n";
        }
        catch (const std::exception& e) {
                std::wcout << "Error occurred: " << e.what() << std::endl;

        }
		
	}
}


int main(int argc, char* argv[])
{
    percentile_finder::Config config = percentile_finder::parse_arguments(argc, argv);
    if (config.benchmark) {
        test_resolver(argc, argv);
    }
    else {
	    try {
		    percentile_finder::solve(config);
	    }
	    catch (const std::exception& e) {
		    std::wcout << "Error occurred: " << e.what() << std::endl;
		    return 1;
	    }
    }
	return 0;
}