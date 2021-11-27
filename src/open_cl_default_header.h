#pragma once
#define CL_TARGET_OPENCL_VERSION 200
#define CL_HPP_TARGET_OPENCL_VERSION 200
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#define CL_LOG_ERRORS stdout
#pragma comment(lib, "OpenCL.lib")

#include <CL/cl.h>
#include "CL/cl2.hpp"

namespace percentile_finder {
    class OpenCLUtils {
    public:
        std::vector<cl::Device> get_cl_devices();
        void list_available_device();
        bool device_exists(std::string name);

    };




}