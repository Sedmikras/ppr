#include "resolver_serial.h"
#include <map>
#include <algorithm>
#include <iostream>

using namespace percentile_finder;

ResolverResult PercentileFinderSerial::find_result(std::ifstream& file, uint8_t looked_up_percentile) {
    watchdog->notify();

    // read end of file, set config
    PartialResult pr {};
    reset_filereader(file);
    file.seekg(0, std::ios::end);
    config.filesize = file.tellg();
    config.iterations = (uint32_t)ceil((double)config.filesize / MAX_BUFFER_SIZE);
    config.numbers_before = 0;
    config.looked_up_percentile = looked_up_percentile;


    do {
        watchdog->notify();
        masker.increment_stage(pr.bucket_index);
        pr = resolve(file);
        if(masker.stage == Stage::LAST) {
            return find_result_last_stage(file, &config, &masker, pr, watchdog, &data_buffer);
        }
    } while (pr.numbers_in_index > MAX_BUFFER_SIZE_TST && masker.stage != Stage::LAST);

    //find positions of the numbers in the file (bucket is known)
    std::vector<double> real_data;
    std::unordered_map<double, Position> positions;
    find_positions(file, &config, &masker, pr, watchdog, &data_buffer, &real_data, &positions);

    //sort vector and find index in that vector
    watchdog->notify();
    std::sort(real_data.begin(), real_data.end());
    uint32_t index = get_index_from_sorted_vector(real_data, &config, watchdog);
    if(index == UINT32_MAX) {
        return ResolverResult { INFINITY , Position{NULL, NULL}};
    } else {
        return ResolverResult { real_data[index], positions.at(real_data[index])};
    }
}

PartialResult PercentileFinderSerial::resolve(std::ifstream& file) {
    reset_filereader(file);
    uint64_t numbers_counter = 0;
    std::vector<uint64_t> frequencies(masker.get_masked_vector_size());
    uint64_t to_read = 0;

    for (uint64_t i = 0; i < config.iterations; i++) {
        watchdog->notify();
        to_read = (((i + 1) * MAX_BUFFER_SIZE) > config.filesize) ? (config.filesize - (i * MAX_BUFFER_SIZE) - to_read%8) : MAX_BUFFER_SIZE;
        file.read((char*)&data_buffer[0], to_read);
        uint32_t size = (uint32_t)(to_read) / 8;

        for (uint64_t j = 0; j < size; j++) {
            uint32_t index = masker.return_index_from_double(data_buffer[j]);
            if (index == UINT32_MAX) {
                continue;
            }
            else {
                if(index > frequencies.size()) {
                    std::wcout << "ERROR";
                }
                frequencies[index]++;
                numbers_counter++;
            }
        }
    }

    if (config.total_number_count == 0) {
        config.total_number_count = numbers_counter;
    }
    return get_bucket_index(frequencies, &config, watchdog);
}

ResolverResult PercentileFinderSerial::find_percentile(std::ifstream& file, uint8_t percentile)
{
    reset_config();
    return find_result(file, percentile);
}

void PercentileFinderSerial::reset_config()
{
    this->masker.stage = Stage::ZERO;
    this->config.filesize = 0;
    this->config.iterations = 0;
    this->config.total_number_count = 0;
    this->config.looked_up_percentile = 0;
    this->config.numbers_before = 0;
}

PercentileFinderSerial::PercentileFinderSerial() noexcept {
    this->data_buffer = std::vector<double> (MAX_VECTOR_SIZE, .0);
    this->masker.stage = Stage::ZERO;
    this->config.filesize = 0;
    this->config.iterations = 0;
    this->config.total_number_count = 0;
    this->config.looked_up_percentile = 0;
    this->config.numbers_before = 0;
}

PercentileFinderSerial::PercentileFinderSerial(Watchdog *w) noexcept: PercentileFinder(w) {
    this->watchdog = w;
    this->data_buffer = std::vector<double> (MAX_VECTOR_SIZE, .0);
    this->masker.stage = Stage::ZERO;
    this->config.filesize = 0;
    this->config.iterations = 0;
    this->config.total_number_count = 0;
    this->config.looked_up_percentile = 0;
    this->config.numbers_before = 0;
}

PercentileFinderSerial::~PercentileFinderSerial() {
    this->data_buffer.clear();
}


