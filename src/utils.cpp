#include "filereader.h"
#include "open_cl_default_header.h"
#include "default_config.h"
#include <iostream>

namespace percentile_finder {
    std::vector<cl::Device> percentile_finder::OpenCLUtils::get_cl_devices() {
        std::vector<cl::Device> devices;
        std::vector<cl::Platform> platforms; // get all platforms
        std::vector<cl::Device> devices_available;
        int n = 0; // number of available devices
        cl::Platform::get(&platforms);
        for (int i = 0; i < (int) platforms.size(); i++) {
            devices_available.clear();
            platforms[i].getDevices(CL_DEVICE_TYPE_ALL, &devices_available);
            if (devices_available.size() == 0) continue; // no device found in plattform i
            for (int j = 0; j < (int) devices_available.size(); j++) {
                n++;
                devices.push_back(devices_available[j]);
            }
        }
        return devices_available;
    }

    void percentile_finder::OpenCLUtils::list_available_device() {
        std::vector<cl::Device> devices = get_cl_devices();
        for (auto &device: devices) {
            std::cout << ", Device: " << device.getInfo<CL_DEVICE_NAME>() << std::endl;
        }
    }

    bool percentile_finder::OpenCLUtils::device_exists(std::string name) {
        std::vector<cl::Device> devices = get_cl_devices();
        for (auto &device: devices) {
            if (device.getInfo<CL_DEVICE_NAME>() == name) {
                return true;
            } else {
                return false;
            }
        }
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

    Config parse_arguments(int argc, char *argv[]) {
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
        } else {
            config.percentile = percentile;
        }

        std::string exec_mode_string = argv[3];
        if (exec_mode_string == "single") {
            config.solver_type = FinderType::Serial;
        } else if (exec_mode_string == "SMP") {
            config.solver_type = FinderType::SMP;
        } else {
            config.solver_type = FinderType::OpenCL;
            config.device_name = argv[3];
        }
        return config;
    }
}