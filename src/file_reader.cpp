#include "filereader.h"
#include "open_cl_default_header.h"
#include <iostream>

std::vector<cl::Device> percentile_finder::OpenCLUtils::get_cl_devices() {
    std::vector<cl::Device> devices;
    std::vector<cl::Platform> platforms; // get all platforms
    std::vector<cl::Device> devices_available;
    int n = 0; // number of available devices
    cl::Platform::get(&platforms);
    for (int i = 0; i < (int)platforms.size(); i++) {
        devices_available.clear();
        platforms[i].getDevices(CL_DEVICE_TYPE_ALL, &devices_available);
        if (devices_available.size() == 0) continue; // no device found in plattform i
        for (int j = 0; j < (int)devices_available.size(); j++) {
            n++;
            devices.push_back(devices_available[j]);
        }
    }
    return devices_available;
}

void percentile_finder::OpenCLUtils::list_available_device() {
    std::vector<cl::Device> devices = get_cl_devices();
    for(auto &device : devices) {
        std::cout << ", Device: " << device.getInfo<CL_DEVICE_NAME>() << std::endl;
    }
}

bool percentile_finder::OpenCLUtils::device_exists(std::string name) {
    std::vector<cl::Device> devices = get_cl_devices();
    for(auto &device : devices) {
        if(device.getInfo<CL_DEVICE_NAME>() == name) {
            return true;
        } else {
            return false;
        }
    }
}

void percentile_finder::reset_filereader(std::ifstream& file)
{
	if (file.is_open()) {
		file.clear();
		file.seekg(std::ios::beg);
	}
}