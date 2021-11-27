//
// Created by dev on 27.11.2021.
//
#include "open_cl_default_header.h"
#include "resolver.h"
#include "resolver_opencl.h"

namespace percentile_finder {
    PercentileFinderOpenCL::PercentileFinderOpenCL(percentile_finder::Watchdog *w,
                                                   const cl::Device &device) {
        this->watchdog = w;

        cl_int error;

        // setup OpenCL context
        context = cl::Context(device, nullptr, nullptr, nullptr, &error);
        if (error != CL_SUCCESS) {
            throw std::runtime_error("Error while creating the OpenCL context: " + std::to_string(error));
        }

        // setup OpenCL command queue
        queue = cl::CommandQueue(context, device, 0, &error);
        if (error != CL_SUCCESS) {
            throw std::runtime_error("Error while creating the OpenCL command queue: " + std::to_string(error));
        }

        cl::Program program(context, cl_program_src);
        error = program.build(std::vector{device});

        if (error != CL_BUILD_SUCCESS) {
            std::cerr << "[ERROR] OpenCL program build error: " << error << std::endl
                      << "Log:" << std::endl
                      << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device) << std::endl;

            throw std::runtime_error("Error while building the OpenCL program: " + std::to_string(error));
        }

        queue = cl::CommandQueue(context, device, 0, &error);
        if (error != CL_SUCCESS) {
            throw std::runtime_error("Error while creating the OpenCL command queue: " + std::to_string(error));
        }

        kernel_bucket_index = cl::Kernel(program, "get_bucket_index", &error);
    }













}
