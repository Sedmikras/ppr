#include "resolver_serial.h"
#include <map>
#include <algorithm>
#include <iostream>

using namespace percentile_finder;

/*ResolverResult PercentileFinderSerial::find_result_last_try(std::ifstream& file, uint8_t looked_up_percentile, PartialResult pr) {
    reset_filereader(file);
    uint64_t max_readable_vector_size = filesize < MAX_BUFFER_SIZE ? (uint64_t)ceil(filesize / 8.0) : MAX_VECTOR_SIZE;
    std::vector<double_t> fileData(max_readable_vector_size);
    std::unordered_map<double, Position> positions;
    double result = NAN;
    for (int i = 0; i < iterations; i++) {
        watchdog->notify();
        uint64_t to_read = ((i + 1 * MAX_BUFFER_SIZE) > filesize) ? (filesize - (i * MAX_BUFFER_SIZE)) : MAX_BUFFER_SIZE;
        file.read((char*)&fileData[0], to_read);
        uint32_t size = (uint32_t)(to_read + to_read % 8) / 8;

        for (int j = 0; j < size; j++) {
            uint32_t index = masker.return_index_from_double(fileData[j]);
            if (index == pr.bucket_index) {
                result = fileData[j];
                try
                {
                    Position* p = &(positions.at(fileData[j]));
                    auto new_pos = i * max_readable_vector_size * 8 + j * 8;
                    p->last = new_pos;
                }
                catch (const std::out_of_range& oor)
                {
                    Position p{ p.first = i * max_readable_vector_size * 8 + j * 8, p.last = i * max_readable_vector_size * 8 + j * 8};
                    positions.insert({ fileData[j], p });
                }
            }
        }
    }
    if(result == NAN && positions.empty()) {
        std::wcout << "APP IS DIVERGING FROM RESULT";
        std::exit((int)ERRORS::INVALID_PERCENTILE);
    }
    ResolverResult r{ r.result = result, positions.at(result) };
    return r;
}*/

ResolverResult PercentileFinderSerial::find_result(std::ifstream& file, uint8_t looked_up_percentile) {
    watchdog->notify();
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
            return find_result_last_try(file, &config, &masker, pr, watchdog);
        }
    } while (pr.numbers_in_index > MAX_VECTOR_SIZE && masker.stage != Stage::LAST);

    //find positions of the numbers in the file (bucket is known)

    reset_filereader(file);
    uint64_t max_readable_vector_size = config.filesize < MAX_BUFFER_SIZE ? (uint64_t)ceil(config.filesize / 8.0) : MAX_VECTOR_SIZE;
    std::vector<double_t> fileData(max_readable_vector_size);
    std::vector<double> real_data;
    std::map<double, Position> positions;
    uint64_t to_read = 0;
    for (int i = 0; i < config.iterations; i++) {
        watchdog->notify();
        to_read = ((i + 1 * MAX_BUFFER_SIZE) > config.filesize) ? (config.filesize - (i * MAX_BUFFER_SIZE)) : MAX_BUFFER_SIZE;
        file.read((char*)&fileData[0], to_read);
        uint32_t size = (uint32_t)(to_read + to_read % 8) / 8;

        for (int j = 0; j < size; j++) {
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

    std::sort(real_data.begin(), real_data.end());
    uint32_t index = get_index_from_sorted_vector(real_data, &config, watchdog);
    if(index == UINT32_MAX) {
        return ResolverResult { NAN, Position{NULL, NULL}};
    } else {
        return ResolverResult { real_data[index], positions.at(real_data[index])};
    }
}

PartialResult PercentileFinderSerial::resolve(std::ifstream& file) {
    reset_filereader(file);
    uint64_t max_readable_vector_size = config.filesize < MAX_BUFFER_SIZE ? (uint64_t)ceil(config.filesize / 8.0) : MAX_VECTOR_SIZE;
    uint64_t numbers_counter = 0;
    std::vector<uint64_t> frequencies(masker.get_masked_vector_size());
    std::vector<double_t> fileData(max_readable_vector_size);
    uint64_t last_increment = 0;
    uint64_t to_read = 0;

    for (int i = 0; i < config.iterations; i++) {
        to_read = (((i + 1) * MAX_BUFFER_SIZE) > config.filesize) ? (config.filesize - (i * MAX_BUFFER_SIZE) + (8 - to_read%8)) : MAX_BUFFER_SIZE;
        file.read((char*)&fileData[0], to_read);
        uint32_t size = (uint32_t)(to_read) / 8;

        for (int j = 0; j < size; j++) {
            uint32_t index = masker.return_index_from_double(fileData[j]);
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
        auto value = numbers_counter - last_increment;
        last_increment = numbers_counter;
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
    this->masker.low = 0;
    this->masker.high = 0;
    this->config.filesize = 0;
    this->config.iterations = 0;
    this->config.total_number_count = 0;
    this->config.looked_up_percentile = 0;
    this->config.numbers_before = 0;
}

PercentileFinderSerial::PercentileFinderSerial() noexcept {
    this->masker.stage = Stage::ZERO;
    this->config.filesize = 0;
    this->config.iterations = 0;
    this->config.total_number_count = 0;
    this->config.looked_up_percentile = 0;
    this->config.numbers_before = 0;
    this->masker.low = 0;
    this->masker.high = 0;
}


