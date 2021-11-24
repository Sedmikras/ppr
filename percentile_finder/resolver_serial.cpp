#include "resolver_serial.h"
#include "default_config.h"
#include <map>
#include <algorithm>
using namespace percentile_finder;

ResolverResult PercentileFinderSerial::find_result(std::ifstream& file, uint8_t looked_up_percentile) {
    //prepare variables
    PartialResult pr{ pr.numbers_count = UINT64_MAX };
    if (filesize == 0) {
        reset_filereader(file);
        file.seekg(0, std::ios::end);
        filesize = file.tellg();
        iterations = (uint32_t)ceil((double)filesize / MAX_BUFFER_SIZE);
    }

    //iterate the file and find in which bucket the number is located
    do {
        reset_filereader(file);
        resolve(file, filesize, iterations, looked_up_percentile, &pr);
        if (pr.index == FIRST_INDEX && masker.stage == Stage::FIRST) {
            break;
        }
        if (pr.numbers_in_index > TEST_MAX_VECTOR_SIZE) {
            masker.increment_stage(pr.index);
        }
    } while (pr.numbers_in_index > TEST_MAX_VECTOR_SIZE);

    //find positions of the numbers in the file (bucket is known)
    reset_filereader(file);
    uint64_t max_readable_vector_size = filesize < MAX_BUFFER_SIZE ? (uint64_t)ceil(filesize / 8.0) : MAX_VECTOR_SIZE;
    std::vector<double_t> fileData(max_readable_vector_size);
    std::vector<double> real_data;
    std::map<double, Position> positions;

    for (int i = 0; i < iterations; i++) {
        uint64_t to_read = ((i + 1 * MAX_BUFFER_SIZE) > filesize) ? (filesize - (i * MAX_BUFFER_SIZE)) : MAX_BUFFER_SIZE;
        file.read((char*)&fileData[0], to_read);
        uint32_t size = (uint32_t)(to_read + to_read % 8) / 8;

        for (int j = 0; j < size; j++) {
            uint32_t index = masker.return_index_from_double(fileData[j]);
            if (index == pr.index) {
                real_data.push_back(fileData[j]);
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

    //optimalization - zero is return in first stage and won't be in other stages
    if (pr.index == FIRST_INDEX && masker.stage == Stage::FIRST) {
        ResolverResult r{ r.result = real_data[0], positions.at(real_data[0]) };
        return  r;
    }

    //check which of the number is on the percentile
    double last_percentile = 0;
    uint64_t index = 0;
    if (real_data.size() == 0) {
        index = index;
    }
    std::sort(real_data.begin(), real_data.end());
    for (uint64_t i = 0; i < real_data.size(); i++) {
        double percentile = (((numbers_before + i + 1) / (double)numbers_count) * 100);
        if (percentile >= looked_up_percentile && last_percentile < looked_up_percentile) {
            index = i;
            break;
        }
        last_percentile = percentile;
    }
    //return result
    double number = real_data[index];
    ResolverResult r{ r.result = real_data[index], positions.at(number) };
    return r;
}

void PercentileFinderSerial::eval(std::vector<uint64_t> frequencies, uint8_t percentile, PartialResult* pr) {
    uint64_t counter = this->numbers_before;
    double last_percentile = ((double)numbers_before / numbers_count) * 100;
    uint32_t index = 0;

    for (int i = 0; i < frequencies.size(); i++) {
        if (frequencies[i] == 0)
            continue;
        counter += frequencies[i];

        double actual_percentile = ((double)(counter) / numbers_count) * 100;
        if (actual_percentile >= percentile && last_percentile < percentile) {
            index = i;
            break;
        }
        last_percentile = actual_percentile;
        this->numbers_before += frequencies[i];
    }
    pr->index = index;
    this->numbers_before = numbers_before;
    pr->numbers_in_index = (counter - numbers_before);
}

void PercentileFinderSerial::resolve(std::ifstream& file, uint64_t filesize, uint32_t iterations, uint8_t percentile, PartialResult* pr) {
    uint64_t max_readable_vector_size = filesize < MAX_BUFFER_SIZE ? (uint64_t)ceil(filesize / 8.0) : MAX_VECTOR_SIZE;
    uint64_t numbers_counter = 0;
    std::vector<uint64_t> frequencies(masker.get_masked_vector_size());
    std::vector<double_t> fileData(max_readable_vector_size);

    for (int i = 0; i < iterations; i++) {
        uint64_t to_read = ((i + 1 * MAX_BUFFER_SIZE) > filesize) ? (filesize - (i * MAX_BUFFER_SIZE)) : MAX_BUFFER_SIZE;
        file.read((char*)&fileData[0], to_read);
        uint32_t size = (uint32_t)(to_read + to_read % 8) / 8;

        for (int j = 0; j < size; j++) {
            uint32_t index = masker.return_index_from_double(fileData[j]);
            if (index == UINT32_MAX) {
                continue;
            }
            else {
                frequencies[index]++;
                numbers_counter++;
            }
        }
    }

    if (this->numbers_count == 0) {
        this->numbers_count = numbers_counter;
        pr->numbers_count = 0;
    }
    eval(frequencies, percentile, pr);
    return;
}

PercentileFinderSerial::PercentileFinderSerial(Watchdog watchdog) noexcept :
    PercentileFinder(std::move(watchdog)) {
    this->masker.stage = Stage::FIRST;
    this->numbers_count = 0;
}

ResolverResult PercentileFinderSerial::find_percentile(std::ifstream& file, uint8_t percentile)
{
    return find_result(file, percentile);
}

void PercentileFinderSerial::reset_config()
{
    this->filesize = 0;
    this->numbers_before = 0;
    this->numbers_count = 0;
    this->masker.stage = Stage::FIRST;
    this->masker.low = 0;
    this->masker.high = 0;
}


