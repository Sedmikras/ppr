#include "resolver_parallel.h"
#include "default_config.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <future>
#include <string>
#include <mutex>
namespace percentile_finder {

    PercentileFinderParallel::PercentileFinderParallel(Watchdog watchdog) noexcept :
        PercentileFinder(std::move(watchdog)) {
        this->masker.stage = Stage::FIRST;
        this->numbers_count = 0;
    }



    void read_data(size_t to_read, std::ifstream& file, std::promise<std::vector<double>> prom) {
        std::vector<double> file_data(to_read / 8);
        file.read((char*)&file_data[0], to_read);
        prom.set_value(file_data);
    }

    void process_data(std::future<std::vector<double>>& fut, void* masker_pointer, void* frequencies_pointer, void* numbers_count_pointer) {
        std::vector<double> file_data = fut.get();
        NumberMasker* masker = (NumberMasker*)masker_pointer;
        std::vector<uint64_t> frequencies = *(std::vector<uint64_t>*)frequencies_pointer;
        uint64_t numbers_count = *(uint64_t*)numbers_count_pointer;
        for (int j = 0; j < file_data.size(); j++) {
            uint32_t index = masker->return_index_from_double(file_data[j]);
            if (index == UINT32_MAX) {
                continue;
            }
            else {

                frequencies[index]++;
                numbers_count++;
            }
        }
    }


    ResolverResult PercentileFinderParallel::find_result(std::ifstream& file, uint8_t percentile)
    {
        if (filesize == 0) {
            reset_filereader(file);
            file.seekg(0, std::ios::end);
            filesize = file.tellg();
            iterations = (uint32_t)ceil((double)filesize / MAX_BUFFER_SIZE);
        }
        uint64_t max_readable_vector_size = filesize < MAX_BUFFER_SIZE ? (uint64_t)ceil(filesize / 8.0) : MAX_VECTOR_SIZE;
        uint64_t numbers_counter = 0;
        std::vector<uint64_t> frequencies(masker.get_masked_vector_size());

        std::promise<std::vector<double>> prom;                     

        std::future<std::vector<double>> fut = prom.get_future();    

        std::thread th1(process_data, std::ref(fut), (void*)&this->masker, (void*)&frequencies, (void*)&numbers_count);  
        for (int i = 0; i < iterations; i++) {
            uint64_t to_read = ((i + 1 * MAX_BUFFER_SIZE) > filesize) ? (filesize - (i * MAX_BUFFER_SIZE)) : MAX_BUFFER_SIZE;
            read_data(to_read, file, std::move(prom));
        }                                                         
        th1.join();
        numbers_counter = numbers_counter;
        ResolverResult r;
        r.result = 5;
        Position p;
        p.last = 0;
        p.first = 0;
        r.position = p;
        return r;
    }



    void percentile_finder::PercentileFinderParallel::reset_config()
    {
    }

};

