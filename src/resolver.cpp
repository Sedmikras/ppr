#include "resolver.h"
#include <map>
#include <algorithm>
#include <functional>
#include <array>
#include <iostream>
#include <string_view>
#include <execution>

namespace percentile_finder {

	PercentileFinder::~PercentileFinder() noexcept = default;

    PercentileFinder::PercentileFinder() noexcept = default;

	/**
		* Find a value from file on the given percentile.
		*
		* @param percentile The percentile.
		* @return The result value or nullopt if the file doesn't contain any normal double.
		*/
    ResolverResult PercentileFinder::find_percentile(std::ifstream& file, uint8_t looked_up_percentile) {
        reset_filereader(file);

        file.seekg(0, std::ios::end);
        size_t filesize = file.tellg();
        file.seekg(std::ios::beg);
        size_t iterations = (uint32_t)ceil((double)filesize / MAX_BUFFER_SIZE);
        std::vector<double> file_data(MAX_BUFFER_SIZE);
        std::vector<double> result_data_vector(0);
        uint64_t to_read = 0;
        double value = INFINITY;
        for (uint64_t i = 0; i < iterations; i++) {
            to_read = (((i + 1) * MAX_BUFFER_SIZE) > filesize) ? (filesize - (i * MAX_BUFFER_SIZE) + (8 - to_read%8)) : MAX_BUFFER_SIZE;
            file.read((char *) &file_data[0], to_read);
            uint32_t size = (uint32_t)(to_read / 8);

            for (uint64_t j = 0; j < size; j++) {
                value = file_data[j];
                if (std::fpclassify(value) == FP_ZERO || std::fpclassify(value) == FP_NORMAL) {
                    result_data_vector.push_back(value);
                }
            }
        }

        std::sort(std::execution::par_unseq, result_data_vector.begin(), result_data_vector.end());


        double last_percentile = 0;
        uint64_t index = 0;
        for (uint64_t i = 0; i < result_data_vector.size(); i++) {
            double percentile = (((i + 1) / (double)result_data_vector.size()) * 100);
            if (percentile >= looked_up_percentile && last_percentile < looked_up_percentile) {
                index = i;
                break;
            }
            last_percentile = percentile;
        }
        ResolverResult r{ r.result = result_data_vector[index], Position{NULL,NULL}};
        result_data_vector.clear();
        file_data.clear();
        return r;
	}

    void PercentileFinder::reset_config()
    {
    }

    ResolverResult find_result_last_try(std::ifstream &file, PercentileFinderConfig* config, NumberMasker* masker, PartialResult pr, Watchdog* watchdog) {
        reset_filereader(file);
        auto iterations = ((config->filesize / 8) / MAX_VECTOR_SIZE);
        uint64_t max_readable_vector_size = config->filesize < MAX_BUFFER_SIZE ? (uint64_t)ceil(config->filesize / 8.0) : MAX_VECTOR_SIZE;
        std::vector<double_t> fileData(max_readable_vector_size);
        std::pair<double, Position> positions;
        uint64_t to_read = 0;
        double result = INFINITY;
        for (uint64_t i = 0; i < iterations; i++) {
            watchdog->notify();
            to_read = (((i + 1) * MAX_BUFFER_SIZE) > config->filesize) ? (config->filesize - (i * MAX_BUFFER_SIZE) + (8 - to_read%8)) : MAX_BUFFER_SIZE;
            file.read((char*)&fileData[0], to_read);
            uint32_t size = (uint32_t)(to_read + to_read % 8) / 8;

            for (uint64_t j = 0; j < size; j++) {
                uint32_t index = masker->return_index_from_double(fileData[j]);
                if (index == pr.bucket_index) {
                    result = fileData[j];
                    Position* p = &positions.second;
                    auto new_pos = i * max_readable_vector_size * 8 + j * 8;
                    p->last = new_pos;
                }
            }
        }
        ResolverResult r{ r.result = result, positions.second };
        return r;
    }

    PartialResult get_bucket_index(std::vector<uint64_t> buckets, PercentileFinderConfig* config, Watchdog* watchdog) {
        PartialResult pr {};
        uint64_t counter = config->numbers_before;
        double last_percentile = ((double)counter / config->total_number_count) * 100;
        uint32_t index = 0;
        watchdog->notify();
        for (int i = 0; i < buckets.size(); i++) {
            if (buckets[i] == 0)
                continue;
            counter += buckets[i];
            if(i % 1024 == 0) {
                watchdog->notify();
            }

            double actual_percentile = ((double)(counter) / config->total_number_count) * 100;
            if (actual_percentile >= config->looked_up_percentile && last_percentile < config->looked_up_percentile) {
                index = i;
                break;
            }
            last_percentile = actual_percentile;
            config->numbers_before += buckets[i];
        }
        pr.bucket_index = index;
        pr.numbers_in_index = (counter - config->numbers_before);
        return pr;
    }

    uint32_t get_index_from_sorted_vector(const std::vector<double>& sorted_vector, PercentileFinderConfig* config, Watchdog* watchdog) {
        uint32_t index = UINT32_MAX;
        double actual_percentile;
        double last_percentile = ((double)config->numbers_before / config->total_number_count) * 100;
        watchdog->notify();
        for(int i = 0; i < sorted_vector.size(); i++) {
            actual_percentile = ((config->numbers_before + i + 1) / (double)config->total_number_count) * 100;
            if (actual_percentile >= config->looked_up_percentile && last_percentile < config->looked_up_percentile) {
                index = i;
                break;
            }
            last_percentile = actual_percentile;
        }
        watchdog->notify();
        return index;
    }
}