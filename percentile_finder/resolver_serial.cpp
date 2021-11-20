#include "resolver_serial.h"
#include "default_config.h"
#include <map>

using namespace percentile_finder;

namespace percentile_finder {

    ResolverResult PercentileFinderSerial::find_result(std::ifstream& file, uint8_t looked_up_percentile) {
        file.seekg(0, std::ios::end);
        uint64_t filesize = file.tellg();
        uint32_t iterations = (uint32_t)ceil((double)filesize / MAX_BUFFER_SIZE);
        PartialResult pr{ pr.numbers_count = UINT64_MAX };
        do {
            reset_filereader(file);
            resolve(file, filesize, iterations, looked_up_percentile, &pr);
            masker.increment_phase(pr.index);
        } while (pr.numbers_in_index > MAX_VECTOR_SIZE);


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
                uint32_t index = masker.return_index_from_double(fileData[i]);
                if (index == pr.index) {
                    real_data.push_back(fileData[j]);
                    try
                    {
                        auto element = positions.at(fileData[j]);
                        element.last = i * max_readable_vector_size * 8 + j * 8;
                    }
                    catch (const std::exception&)
                    {
                        Position p{ p.first = i * max_readable_vector_size * 8 + j * 8, p.last = i * max_readable_vector_size * 8 + j * 8};
                        positions.insert({ fileData[j], p });
                    }
                }
            }
        }

        double last_percentile = 0;
        uint64_t index = 0;
        for (uint64_t i = 0; i < real_data.size(); i++) {
            double percentile = (((i + 1) / (double)numbers_count) * 100);
            if (percentile >= looked_up_percentile && last_percentile < looked_up_percentile) {
                index = i;
                break;
            }
            last_percentile = percentile;
        }
        double number = real_data[index];
        ResolverResult r{ r.result = real_data[index], positions.at(number) };
        return r;
    }

    void PercentileFinderSerial::eval(std::vector<uint64_t> frequencies, uint8_t percentile, PartialResult* pr) {
        uint64_t counter = this->numbers_before;
        double last_percentile = ((double)numbers_before / numbers_count) * 100;
        uint32_t index = 0;

        for (int i = 0; i < frequencies.size(); i++) {
            counter += frequencies[i];

            double actual_percentile = ((double)(counter) / numbers_count) * 100;
            if (actual_percentile > percentile && last_percentile < percentile) {
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
        this->masker.phase = 0;
        this->numbers_count = 0;
    }

    ResolverResult PercentileFinderSerial::find_percentile(std::ifstream& file, uint8_t percentile)
    {
        return find_result(file, percentile);
    }
}

