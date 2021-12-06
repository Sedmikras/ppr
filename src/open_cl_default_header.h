#pragma once
#define CL_TARGET_OPENCL_VERSION 200
#define CL_HPP_TARGET_OPENCL_VERSION 200
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#define CL_LOG_ERRORS stdout
#pragma comment(lib, "OpenCL.lib")

#include <CL/cl.h>
#include "CL/cl2.hpp"

/**
 * OpenCL program source in OpenCL C
 */
const std::string cl_program_src = R"CLC(
    __constant const uint  max_int = 0xFFFFFFFF;

    bool is_valid(double value) {
        int result = isnormal(value);
        return result > 0 || value == 0 || value == -0;
    }

    bool is_in_range(const double value, const double min, const double max) {
        return value >= min && value < max;
    }

    __kernel void bucket_index(__global const double *data, __global uint *indexes, const uint shift, const int mask, const uint offset, const double min, const double max) {
        int i = get_global_id(0);
        double value = data[i];
        uint index = max_int;
        if (is_valid(value) && is_in_range(value, min, max)) {
            long unsigned_value = as_long(value);
            if (value < 0) {
                index = (~(unsigned_value >> shift)) & 524287;
            } else {
                index = offset + ((unsigned_value >> shift) & 524287);
            }
        }
        indexes[i] = index;
    }

)CLC";

/**
 * arguments for OpenCL
 */
const unsigned int BUCKET_INDEX_ARG_DATA = 0;
const unsigned int BUCKET_INDEX_ARG_INDEXES = 1;
const unsigned int BUCKET_INDEX_ARG_SHIFT = 2;
const unsigned int BUCKET_INDEX_ARG_MASK = 3;
const unsigned int BUCKET_INDEX_ARG_OFFSET = 4;
const unsigned int BUCKET_INDEX_ARG_MIN = 5;
const unsigned int BUCKET_INDEX_ARG_MAX = 6;

namespace percentile_finder {
        /**
         * Returns list of available CL devices
         * @return vector of OpenCL devices
         */
        std::vector<cl::Device> get_cl_devices();
        /**
         * List available devices and prints them to STDOUT
         */
        void list_available_device();
        /**
         * Checks if device exist(given the string name)
         * @param name of the device
         * @return true if device exists
         */
        bool device_exists(std::string name);

        /**
         * Returns device by given name or null device
         * @param device_name name of the device
         * @return device instance
         */
        cl::Device get_device_by_name(std::string device_name);
};