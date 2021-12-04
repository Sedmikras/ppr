#pragma once
#define CL_TARGET_OPENCL_VERSION 200
#define CL_HPP_TARGET_OPENCL_VERSION 200
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#define CL_LOG_ERRORS stdout
#pragma comment(lib, "OpenCL.lib")

#include <CL/cl.h>
#include "CL/cl2.hpp"

/*
 * __constant const ulong d_exp = 0x7FF0000000000000;
    __constant const ulong p_zero = 0x0000000000000000;
    __constant const ulong n_zero = 0x8000000000000000;
    __constant const uint  max_int = 0xFFFFFFFF;

    bool is_valid(double value) {
        int result = fpclassify(value);
        if (result == -1
        bool inf_or_nan = (value & d_exp) == d_exp;
        bool sub_or_zero = (~value & d_exp) == d_exp;
        bool zero = value == p_zero || value == n_zero;
        bool normal = (!inf_or_nan && !sub_or_zero) || zero;
        return normal;
    }

    bool is_in_range(const double value, const double min, const double max) {
        return value >= min && value < max;
    }

    __kernel void bucket_index(__global const double *data, __global uint *indexes, const uint shift, const uint mask, const uint offset, const double min, const double max) {
        int i = get_global_id(0);
        double value = data[i];
        ulong unsigned_value = as_ulong(value);
        uint index = max_int;
        if (is_valid(unsigned_value) && is_in_range(value, min, max)) {
            if (value < 0) {
                index = (~(unsigned_value >> shift)) & mask;
            } else {
                index = offset + ((unsigned_value >> shift) & mask);
            }
        }
        indexes[i] = index;
    }*/
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

const unsigned int BUCKET_INDEX_ARG_DATA = 0;
const unsigned int BUCKET_INDEX_ARG_INDEXES = 1;
const unsigned int BUCKET_INDEX_ARG_SHIFT = 2;
const unsigned int BUCKET_INDEX_ARG_MASK = 3;
const unsigned int BUCKET_INDEX_ARG_OFFSET = 4;
const unsigned int BUCKET_INDEX_ARG_MIN = 5;
const unsigned int BUCKET_INDEX_ARG_MAX = 6;

namespace percentile_finder {
    class OpenCLUtils {
    public:
        std::vector<cl::Device> get_cl_devices();
        void list_available_device();
        bool device_exists(std::string name);

        cl::Device get_device_by_name(std::string device_name);
    };
}