#include "open_cl_default_header.h"
#include "default_config.h"
#include "watchdog.h"
#include "resolver.h"
#include <iostream>
#include "resolver_opencl.h"
#include "resolver_serial.h"
#include "resolver_parallel.h"

namespace percentile_finder {
    std::vector<std::string> percentile_finder::get_cl_device_names() {
        std::vector<std::string> devices_names;
        std::vector<cl::Platform> platforms;
        cl::Platform::get(&platforms);

        for (const auto &platform : platforms) {
            std::vector<cl::Device> devices;
            platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);
            for (auto &device : devices) {
                devices_names.emplace_back(device.getInfo<CL_DEVICE_NAME>().c_str());
            }
        }

        return devices_names;
    }

    cl::Device percentile_finder::get_device_by_name(std::string p_device_name) {
        std::vector<cl::Platform> platforms;
        cl::Platform::get(&platforms);

        for (const auto &platform : platforms) {
            std::vector<cl::Device> devices;
            platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);
            for (auto &device : devices) {
                std::string device_name = device.getInfo<CL_DEVICE_NAME>();

                if (device_name == p_device_name) {
                    return device;
                }
            }
        }

        throw std::runtime_error("OpenCL device not found");
    }

    void percentile_finder::list_available_device() {
        std::wcout << "List of available devices: \n";
        std::vector<std::string> devices = get_cl_device_names();
        for (auto &device: devices) {
            std::cout << "* " << device << std::endl;
        }
    }

    bool percentile_finder::device_exists(std::string name) {
        std::vector<std::string> devices = get_cl_device_names();
        for (auto &device: devices) {
            if (device == name) {
                return true;
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

    void end_with_error_message(ERRORS e, std::string message) {
        print_error_message(e);
        std::wcout << message.c_str();
        std::exit((int )e);
    }

    void print_error_message(ERRORS e) {
        switch (e) {
            case percentile_finder::ERRORS::NOT_ENOUGH_ARGUMENTS: {
                std::wcout << "ERROR:INVALID NUMBER OF ARGUMENTS\n";
                break;
            }

            case percentile_finder::ERRORS::UNKNOWN_DEVICE_NAME: {
                std::wcout << "ERROR:UNKNOWN OPENCL DEVICE\n";
                break;
            }

            case percentile_finder::ERRORS::INVALID_FILE_NAME: {
                std::wcout << "ERROR:FILE NAME IS NOT VALID - IS NOT READABLE\n";
                break;
            }

            case percentile_finder::ERRORS::INVALID_PERCENTILE: {
                std::wcout << "ERROR:INVALID_PERCENTILE - must be [1,100]\n";
                break;
            }

            case percentile_finder::ERRORS::NOT_RESPONDING: {
                std::wcout << "ERROR:APP IS NOT RESPONDING\n";
                break;
            }
            case percentile_finder::ERRORS::FINDER_DIVERGING: {
                std::wcout << "ERROR:APP IS DIVERGING FROM RESULT\n";
                break;
            }
            default:
                break;
        }
    }

    void end_without_message(ERRORS e) {
        std::exit((int)e);
    }


    void end_with_error_message(ERRORS e) {
        switch (e) {
            case percentile_finder::ERRORS::NOT_ENOUGH_ARGUMENTS: {
                print_error_message(e);
                std::exit((int) ERRORS::NOT_ENOUGH_ARGUMENTS);
            }

            case percentile_finder::ERRORS::UNKNOWN_DEVICE_NAME: {
                print_error_message(e);
                std::exit((int) ERRORS::UNKNOWN_DEVICE_NAME);
            }

            case percentile_finder::ERRORS::INVALID_FILE_NAME: {
                print_error_message(e);
                std::exit((int) ERRORS::INVALID_FILE_NAME);
            }

            case percentile_finder::ERRORS::INVALID_PERCENTILE: {
                print_error_message(e);
                std::exit((int) ERRORS::INVALID_PERCENTILE);
            }

            case percentile_finder::ERRORS::NOT_RESPONDING: {
                print_error_message(e);
                std::exit((int) ERRORS::INVALID_PERCENTILE);
            }
            case percentile_finder::ERRORS::FINDER_DIVERGING: {
                print_error_message(e);
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


        if (argc < 4 || argc > 5) {
            std::string s;
            s.append("Invalid program params, usage:\n");
            s.append("percentile_finder.exe <file> <percentile> <execution_type> <benchmark_flag>\n");
            s.append("<file> - file path (aboslute or relative):\n");
            s.append("<percentile> - number 1 - 100\n");
            s.append("<execution_type> string - \"single\" \"SMP\" <OpenCL device name>\n");
            s.append("optional <benchmark_flag> - \"benchmark\" or \"-b\"\n");
            end_with_error_message(ERRORS::NOT_ENOUGH_ARGUMENTS, s);
        }

        file_name = argv[1];
        if (!file_exists(file_name)) {
            end_with_error_message(ERRORS::INVALID_FILE_NAME);
        }
        config.filename = file_name;

        try {
            auto value = std::stoi(argv[2]);
            if (value < 1 || value > 100) {
                end_with_error_message(ERRORS::INVALID_PERCENTILE);
            } else {
                percentile = static_cast<uint8_t>(value);
                config.percentile = percentile;
            }
        }
        catch(...){
            end_with_error_message(ERRORS::INVALID_PERCENTILE);
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
        percentile_finder::Watchdog watchdog (percentile_finder::DEFAULT_TIMEOUT,
               []() {
                   std::wcout << "Program is not responding. Wait for it ? Y/N";
                   int response = getchar();
                   if (response == 'N' || response == 'n')
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
                if (percentile_finder::device_exists(config.device_name)) {
                    cl::Device device = percentile_finder::get_device_by_name(config.device_name);
                    finder = std::make_unique<percentile_finder::PercentileFinderOpenCL>(&watchdog, device); break;
                }
                else {
                    percentile_finder::print_error_message(ERRORS::UNKNOWN_DEVICE_NAME);
                    percentile_finder::list_available_device();
                    percentile_finder::end_without_message(ERRORS::UNKNOWN_DEVICE_NAME);
                    break;
                }
                
            }
            case FinderType::UNLIMITED_RAM_TESTER: {
                finder = std::make_unique<percentile_finder::PercentileFinder>();
            } break;
        }
        std::ifstream file(config.filename, std::ios::binary);
        //auto start = std::chrono::system_clock::now();
        auto result = finder->find_percentile(file, config.percentile);
        //auto end = std::chrono::system_clock::now();
        if (result.result == INFINITY) {
            std::string message;
            message.append("Unable to find percentile ");
            message.append(std::to_string(config.percentile));
            message.append("in file ");
            message.append(config.filename);
            end_with_error_message(ERRORS::FINDER_DIVERGING, message.c_str());
        }
        std::wcout
                    << std::hex << result.result
                    << std::dec << " " << result.position.first
                    << std::dec << " " << result.position.last
                    << std::endl;
        file.close();
        //auto ms_int = duration_cast<std::chrono::milliseconds>(end - start);
        //std::wcout << "duration:"<< ms_int << "\n";
        watchdog.stop();
    }
}