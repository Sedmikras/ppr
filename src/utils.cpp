#include "open_cl_default_header.h"
#include "default_config.h"
#include "watchdog.h"
#include "resolver.h"
#include <iostream>
#include "resolver_opencl.h"
#include "resolver_serial.h"
#include "resolver_parallel.h"

namespace percentile_finder {
    std::vector<cl::Device> percentile_finder::get_cl_devices() {
        std::vector<cl::Device> devices;
        std::vector<cl::Platform> platforms; // get all platforms
        std::vector<cl::Device> devices_available;
        auto n = 0; // number of available devices
        cl::Platform::get(&platforms);
        for (auto i = 0; i < platforms.size(); i++) {
            devices_available.clear();
            platforms[i].getDevices(CL_DEVICE_TYPE_ALL, &devices_available);
            if (devices_available.size() == 0) continue; // no device found in plattform i
            for (auto j = 0; j < devices_available.size(); j++) {
                n++;
                devices.push_back(devices_available[j]);
            }
        }
        return devices_available;
    }

    cl::Device percentile_finder::get_device_by_name(std::string device_name) {
        std::vector<cl::Device> devices = get_cl_devices();
        for (auto const device : devices) {
            if (device.getInfo<CL_DEVICE_NAME>() == device_name) {
                return device;
            }
        }
        return cl::Device{};
    }

    void percentile_finder::list_available_device() {
        std::vector<cl::Device> devices = get_cl_devices();
        for (auto &device: devices) {
            std::cout << ", Device: " << device.getInfo<CL_DEVICE_NAME>() << std::endl;
        }
    }

    bool percentile_finder::device_exists(std::string name) {
        std::vector<cl::Device> devices = get_cl_devices();
        for (auto &device: devices) {
            if (device.getInfo<CL_DEVICE_NAME>() == name) {
                return true;
            } else {
                return false;
            }
        }
        return false;
    }

    void percentile_finder::reset_filereader(std::ifstream &file) {
        if (file.is_open()) {
            file.clear();
            file.seekg(std::ios::beg);
        }
    }


    void end_with_error_message(ERRORS e) {
        switch (e) {
            case percentile_finder::ERRORS::NOT_ENOUGH_ARGUMENTS: {
                std::wcout << "INVALID NUMBER OF ARGUMENTS";
                std::exit((int) ERRORS::NOT_ENOUGH_ARGUMENTS);
            }

            case percentile_finder::ERRORS::UNKNOWN_DEVICE_NAME: {
                std::wcout << "UNKNOWN OPENCL DEVICE";
                std::exit((int) ERRORS::UNKNOWN_DEVICE_NAME);
            }

            case percentile_finder::ERRORS::INVALID_FILE_NAME: {
                std::wcout << "FILE NAME IS NOT VALID - IS NOT READABLE";
                std::exit((int) ERRORS::INVALID_FILE_NAME);
            }

            case percentile_finder::ERRORS::INVALID_PERCENTILE: {
                std::wcout << "INVALID_PERCENTILE - must be [1,100]";
                std::exit((int) ERRORS::INVALID_PERCENTILE);
            }

            case percentile_finder::ERRORS::NOT_RESPONDING: {
                std::wcout << "APP IS NOT RESPONDING";
                std::exit((int) ERRORS::INVALID_PERCENTILE);
            }
            case percentile_finder::ERRORS::FINDER_DIVERGING: {
                std::wcout << "APP IS DIVERGING FROM RESULT";
                std::exit((int) ERRORS::INVALID_PERCENTILE);
            }
            default:
                break;
        }

    }

    bool file_exists(const std::string &file_path) {
        std::ifstream file;
        file.open(file_path);
        if (file) {
            return true;
        } else {
            return false;
        }
    }

    Config parse_arguments(int argc, char* argv[]) {
        Config config;

        std::string file_name;
        uint8_t percentile;


        if (argc < 4 && argc > 5) {
            end_with_error_message(ERRORS::NOT_ENOUGH_ARGUMENTS);
        }

        file_name = argv[1];
        if (!file_exists(file_name)) {
            end_with_error_message(ERRORS::INVALID_FILE_NAME);
        }
        config.filename = file_name;


        percentile = static_cast<uint8_t>(std::stoi(argv[2]));
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

        if (argc == 5) {
            std::string benchmark_string = argv[4];
            if (benchmark_string == "benchmark" || benchmark_string == "-b")
                config.benchmark = true;
        }
        return config;
    }

    void solve(Config config) {
        percentile_finder::Watchdog watchdog (std::chrono::seconds(15),
           []() {
               end_with_error_message(percentile_finder::ERRORS::NOT_RESPONDING);
           }
        );

        std::unique_ptr<percentile_finder::PercentileFinder> finder;
        switch (config.solver_type)
        {
            case FinderType::Serial: {
                finder = std::make_unique<percentile_finder::PercentileFinderSerial>(&watchdog); break;
            }
            case FinderType::SMP: {
                finder = std::make_unique<percentile_finder::PercentileFinderParallel>(&watchdog); break;
            }
            case FinderType::OpenCL: {
                if(percentile_finder::device_exists(config.device_name)) {
                    cl::Device device = percentile_finder::get_device_by_name(config.device_name);
                    finder = std::make_unique<percentile_finder::PercentileFinderOpenCL>(&watchdog, device); break;
                }
            }
            case FinderType::UNLIMITED_RAM_TESTER: {
                finder = std::make_unique<percentile_finder::PercentileFinder>();
            } break;
        }
        std::ifstream file(config.filename, std::ios::binary);
        auto start = std::chrono::system_clock::now();
        auto result = finder->find_percentile(file, config.percentile);
        auto end = std::chrono::system_clock::now();
        std::wcout
                    << std::hex << result.result
                    << std::dec << " " << result.position.first
                    << std::dec << " " << result.position.last
                    << std::endl;
        file.close();
        auto ms_int = duration_cast<std::chrono::milliseconds>(end - start);
        std::wcout << "duration:"<< ms_int << "\n";
        watchdog.stop();
    }
}