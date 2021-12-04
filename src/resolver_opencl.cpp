//
// Created by dev on 27.11.2021.
//
#include <iostream>
#include <execution>
#include "open_cl_default_header.h"
#include "resolver.h"
#include "resolver_opencl.h"

namespace percentile_finder {
    PercentileFinderOpenCL::PercentileFinderOpenCL(percentile_finder::Watchdog *w,
                                                   const cl::Device &device) {
        this->watchdog = w;
        this->data_buffer = std::vector<double> (MAX_VECTOR_SIZE, .0);
        cl_int error;

        // setup OpenCL context
        context = cl::Context(device, nullptr, nullptr, nullptr, &error);
        if (error != CL_SUCCESS) {
            throw std::runtime_error("Error while creating the OpenCL context: " + std::to_string(error));
        }

        // setup OpenCL command queue
        queue = cl::CommandQueue(context, device, 0, &error);
        if (error != CL_SUCCESS) {
            std::cout << "Error while creating the OpenCL command queue: " << std::to_string(error);
            throw std::runtime_error("Error while creating the OpenCL command queue: " + std::to_string(error));
        }

        // build OpenCL program
        cl::Program program(context, cl_program_src);
        try {
            error = program.build(std::vector<cl::Device>{device});
        }
        catch (...) {
            std::cout << "Chyba pri sestaveni kernelu!" << std::endl;
        }

        cl_int buildErr = CL_SUCCESS;
        auto buildInfo = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(&buildErr);
        for (auto &pair : buildInfo) {
            std::cout << pair.second << std::endl << std::endl;
        }


        if (error != CL_BUILD_SUCCESS) {
            std::cerr << "[ERROR] OpenCL program build error: " << error << std::endl
                      << "Log:" << std::endl
                      << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device) << std::endl;

            throw std::runtime_error(+"Error while building the OpenCL program: " + std::to_string(error));
        }

        kernel_bucket_index = cl::Kernel(program, "bucket_index", &error);
        if(error != CL_SUCCESS) {
            std::cout << "ERROR WHILE LOADING PROGRAM" ;
        }

        data_buffer = std::vector<double>(MAX_VECTOR_SIZE_OPENCL);
    }

    PartialResult PercentileFinderOpenCL::resolve(std::ifstream &file) {
        reset_filereader(file);
        uint64_t numbers_counter = 0;
        uint64_t to_read = 0;
        cl_int error;

        std::vector<uint64_t> buckets(masker.get_masked_vector_size());
        std::vector<uint32_t> indexes(MAX_VECTOR_SIZE_OPENCL, UINT32_MAX);

        cl::Buffer opencl_data_buffer(context, CL_MEM_READ_ONLY, MAX_BUFFER_SIZE_OPENCL, nullptr, &error);
        cl::Buffer indexes_buffer(context, CL_MEM_WRITE_ONLY, MAX_BUFFER_SIZE_OPENCL, nullptr, &error);

        /*__kernel void bucket_index(__global const ulong *data, __global uint *indexes, const uint shift, const uint mask, const ulong offset, const double min, const double max)

        low = -INFINITY;
        high = + INFINITY;
        bit_shift = LEFT_SHIFT_COMPLEMENT_FIRST_STAGE;
        mask = BIT_MASK_FIRST_STAGE;
        offset = SPLITERATOR_FIRST_INDEX;*/
        double min = -std::numeric_limits<double>::max();
        double max = std::numeric_limits<double>::max();
        uint32_t  offset = SPLITERATOR_FIRST_INDEX;
        uint32_t  shift = LEFT_SHIFT_COMPLEMENT_FIRST_STAGE;
        auto mask = BIT_MASK_FIRST_STAGE;

        error = kernel_bucket_index.setArg(BUCKET_INDEX_ARG_DATA, opencl_data_buffer);
        error = kernel_bucket_index.setArg(BUCKET_INDEX_ARG_INDEXES, indexes_buffer);
        error = kernel_bucket_index.setArg(BUCKET_INDEX_ARG_MASK, mask);
        error = kernel_bucket_index.setArg(BUCKET_INDEX_ARG_MAX, max);
        error = kernel_bucket_index.setArg(BUCKET_INDEX_ARG_MIN, min);
        error = kernel_bucket_index.setArg(BUCKET_INDEX_ARG_OFFSET, offset);
        error = kernel_bucket_index.setArg(BUCKET_INDEX_ARG_SHIFT, shift);

        for (uint64_t i = 0; i < config.iterations; i++) {
            to_read = (((i + 1) * MAX_BUFFER_SIZE_OPENCL) > config.filesize) ? (config.filesize - (i * MAX_BUFFER_SIZE_OPENCL)) : MAX_BUFFER_SIZE_OPENCL;
            to_read = to_read  - (to_read % 8);
            file.read((char*)&data_buffer[0], to_read);
            error = queue.enqueueWriteBuffer(opencl_data_buffer, CL_TRUE, 0, (cl::size_type) to_read, data_buffer.data());
            error = queue.enqueueNDRangeKernel(kernel_bucket_index, cl::NullRange, cl::NDRange((cl::size_type) to_read));
            error = queue.enqueueReadBuffer(indexes_buffer, CL_TRUE, 0, (cl::size_type) MAX_VECTOR_SIZE_OPENCL * sizeof(uint32_t), indexes.data());
            queue.finish();
            for (int i = 0; i < (to_read / 8); i++) {
                uint32_t index = indexes[i];
                if (index != UINT32_MAX) {
                    buckets[index]++;
                    numbers_counter++;
                } else {
                    index = index;
                }

            }
        }

        if (config.total_number_count == 0) {
            config.total_number_count = numbers_counter;
        }
        return get_bucket_index(buckets, &config, watchdog);
    }

    ResolverResult PercentileFinderOpenCL::find_percentile(std::ifstream &file, uint8_t percentile) {
        watchdog->notify();
        PartialResult pr {};
        reset_filereader(file);
        file.seekg(0, std::ios::end);
        config.filesize = file.tellg();
        config.iterations = (uint32_t)ceil((double)config.filesize / MAX_BUFFER_SIZE_OPENCL);
        config.numbers_before = 0;
        config.looked_up_percentile = percentile;
        masker.stage = Stage::ZERO;

        do {
            watchdog->notify();
            masker.increment_stage(pr.bucket_index);
            pr = resolve(file);
            if(masker.stage == Stage::LAST) {
                return find_result_last_try(file, &config, &masker, pr, watchdog);
            }
        } while (pr.numbers_in_index > MAX_VECTOR_SIZE && masker.stage != Stage::LAST);

        //find positions of the numbers in the file (bucket is known)

        reset_filereader(file);
        uint64_t max_readable_vector_size = config.filesize < MAX_BUFFER_SIZE ? (uint64_t)ceil(config.filesize / 8.0) : MAX_VECTOR_SIZE;
        std::vector<double_t> fileData(max_readable_vector_size);
        std::vector<double> real_data;
        std::unordered_map<double, Position> positions;
        uint64_t to_read = 0;
        for (uint64_t i = 0; i < config.iterations; i++) {
            watchdog->notify();
            to_read = ((i + 1 * MAX_BUFFER_SIZE) > config.filesize) ? (config.filesize - (i * MAX_BUFFER_SIZE)) : MAX_BUFFER_SIZE;
            file.read((char*)&fileData[0], to_read);
            uint32_t size = (uint32_t)(to_read + to_read % 8) / 8;

            for (uint64_t j = 0; j < size; j++) {
                double number = fileData[j];
                uint32_t index = masker.return_index_from_double(number);
                if (index == pr.bucket_index) {
                    real_data.push_back(number);
                    if(positions.contains(number)) {
                        Position* p = &(positions.at(number));
                        auto new_pos = i * max_readable_vector_size * 8 + j * 8;
                        p->last = new_pos;
                    } else {
                        Position p{ p.first = i * max_readable_vector_size * 8 + j * 8, p.last = i * max_readable_vector_size * 8 + j * 8};
                        positions.insert({ number, p });
                    }
                }
            }
        }

        std::sort(std::execution::par_unseq, real_data.begin(), real_data.end());
        uint32_t index = get_index_from_sorted_vector(real_data, &config, watchdog);
        if(index == UINT32_MAX) {
            return ResolverResult { INFINITY , Position{NULL, NULL}};
        } else {
            return ResolverResult { real_data[index], positions.at(real_data[index])};
        }
    }

    PercentileFinderOpenCL::~PercentileFinderOpenCL() noexcept {
        this->watchdog->stop();
    }

    void PercentileFinderOpenCL::reset_config() {

    }
}
