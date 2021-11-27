#pragma once
#include <fstream>

namespace percentile_finder {
	void reset_filereader(std::ifstream& file);

    std::vector<cl::Device> get_cl_devices() {
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
        for (int i = 0; i < n; i++) std::cout << "ID: " << i << ", Device: " << devices[i].getInfo<CL_DEVICE_NAME>() << std::endl;
        return devices_available;
    }
}