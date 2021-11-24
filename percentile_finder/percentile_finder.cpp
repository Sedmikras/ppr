#include "resolver.h"
#include "resolver_serial.h"
#include "resolver_parallel.h"
#include "default_config.h"
#include <string>
#include <iostream>
#include <map>
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
				finder = std::make_unique<percentile_finder::PercentileFinderSerial>(std::move(watchdog)); break;
			}
			/*case FinderType::SMP: {
				finder = std::make_unique<percentile_finder::PercentileFinderParallel>(std::move(watchdog)); break;
			}
			case FinderType::OpenCL: {

			} break;*/
			
			case FinderType::UNLIMITED_RAM_TESTER: {
				//finder = std::make_unique<percentile_finder::PercentileFinder>(std::move(watchdog));
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
		}

	}
}

/**
		* Find a value from file on the given percentile.
		*
		* @param percentile The percentile.
		* @return The result value or nullopt if the file doesn't contain any normal double.
		*/
percentile_finder::ResolverResult find_percentile(std::ifstream& file, uint8_t looked_up_percentile) {
	percentile_finder::reset_filereader(file);

	file.seekg(0, std::ios::end);
	uint64_t filesize = file.tellg();
	file.seekg(std::ios::beg);

	std::vector<double> fileData((uint64_t)(ceil(filesize / 8)));
	std::vector<double> realData(0);
	file.read((char*)&fileData[0], filesize);
	std::map<double, percentile_finder::Position> positions;
	for (uint64_t i = 0; i < fileData.size(); i++)
	{
		if (std::fpclassify(fileData[i]) == FP_ZERO || std::fpclassify(fileData[i]) == FP_NORMAL) {
			realData.push_back(fileData[i]);
			/*try
			{
				auto element = positions.at(fileData[i]);
				element.last = i * 8;
			}
			catch (const std::exception&)
			{
				percentile_finder::Position p{ p.first = i * 8, p.last = i * 8 };
				positions.insert({ fileData[i], p });
			}*/
		}
	}

	std::sort(std::execution::par_unseq, realData.begin(), realData.end());


	double last_percentile = 0;
	uint64_t index = 0;
	for (uint64_t i = 0; i < realData.size(); i++) {
		double percentile = (((i + 1) / (double)realData.size()) * 100);
		if (percentile >= looked_up_percentile && last_percentile < looked_up_percentile) {
			index = i;
			break;
		}
		last_percentile = percentile;
	}
	double number = realData[index];
	
	percentile_finder::ResolverResult r{ r.result = realData[index], percentile_finder::Position{0 ,0} };
	return r;
}

void test_masker() {
	percentile_finder::NumberMasker masker;
	uint32_t index = 0;
	uint32_t index2 = 0;
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

	index = index;
	uint64_t value = frequencies[index];
	index = index;
}

void test_resolver(int argc, char* argv[]) {
	// setup watchdog
	percentile_finder::Watchdog watchdog(std::chrono::seconds(percentile_finder::DEFAULT_TIMEOUT),
		[]() {
			end_with_error_message(percentile_finder::ERRORS::FINDER_DIVERGING);
		},
		[]() {
			end_with_error_message(percentile_finder::ERRORS::NOT_RESPONDING);
		});
	percentile_finder::Config config = percentile_finder::parse_arguments(argc, argv);
	percentile_finder::ResolverResult r1;
	percentile_finder::ResolverResult r2{ 0, NULL };
	std::unique_ptr<percentile_finder::PercentileFinder> finder;
	finder = std::make_unique<percentile_finder::PercentileFinderSerial>(std::move(watchdog));
	std::ifstream file(config.filename, std::ios::binary);

	for (int i = 51; i <= 100; i++) {
		config.percentile = i;
		try {
			finder->reset_config();
			r1 = finder->find_percentile(file, i); 
			std::wcout << r1.result << ";" << "\n";
			//r2 = find_percentile(file, i);
			if (r1.result == r2.result) {
				r1 = r1;
			}
			else {
				r2 = r2;
				std::wcout << "NOT THE SAME:{" << i << "}\n";
				std::wcout << r1.result << ";" << r2.result << "\n";
			}
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
	percentile_finder::Config config = percentile_finder::parse_arguments(argc, argv);
	try {
		percentile_finder::solve(config);
	}
	catch (const std::exception& e) {
		std::wcout << "Error occurred: " << e.what() << std::endl;
		return 1;
	}
	std::ifstream file(config.filename, std::ios::binary);
	percentile_finder::ResolverResult r = find_percentile(file, config.percentile);
	std::wcout << std::dec << r.result;
	return 0;

}