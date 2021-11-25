#include "resolver_parallel.h"
#include "default_config.h"
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <tbb/parallel_pipeline.h>

namespace percentile_finder {

    PercentileFinderParallel::PercentileFinderParallel(Watchdog watchdog) noexcept :
        PercentileFinder(std::move(watchdog)) {
        this->masker.stage = Stage::FIRST;
        this->numbers_count = 0;
    }

    void PercentileFinderParallel::barrier_wait() {
        std::unique_lock<std::mutex> lLock{ mMutex };
        auto lGen = mGeneration;
        if (!--mCount) {
            mGeneration++;
            mCount = mThreshold;
            parsed_buffer = data_buffer;
            mCond.notify_all();

        }
        else {
            mCond.wait(lLock, [this, lGen] { return lGen != mGeneration; });
        }
    }

    void PercentileFinderParallel::read_data(std::ifstream file, size_t to_read) {
        file.read((char*)&data_buffer[0], to_read);
        barrier_wait();
    }

    void PercentileFinderParallel::eval(std::vector<uint64_t> frequencies, uint8_t percentile, PartialResult* pr)
    {
    }

    void PercentileFinderParallel::resolve(void* numbers_counter)
    {
        uint64_t numbers = (uint64_t)&numbers_counter;
        for (int j = 0; j < data_buffer.size(); j++) {
            uint32_t index = masker.return_index_from_double(data_buffer[j]);
            if (index == UINT32_MAX) {
                continue;
            }
            else {
                frequencies[index]++;
                
                numbers++;
            }
        }
        barrier_wait();
    }


    ResolverResult PercentileFinderParallel::find_percentile(std::ifstream& file, uint8_t percentile)
    {
        if (filesize == 0) {
            reset_filereader(file);
            file.seekg(0, std::ios::end);
            filesize = file.tellg();
            iterations = (uint32_t)ceil((double)filesize / MAX_BUFFER_SIZE);
            file.seekg(std::ios::beg);
        }
        uint64_t max_readable_vector_size = filesize < MAX_BUFFER_SIZE ? (uint64_t)ceil(filesize / 8.0) : MAX_VECTOR_SIZE;
        int i = 0;
        uint64_t to_read = ((i + 1 * MAX_BUFFER_SIZE) > filesize) ? (filesize - (i * MAX_BUFFER_SIZE)) : MAX_BUFFER_SIZE;
        i++;
        data_buffer = std::vector<double_t>(max_readable_vector_size);
        read_data(std::move(file), to_read);
        parsed_buffer = data_buffer;
        uint64_t numbers_count;
        std::vector<std::thread> threads;
        for (i; i < iterations; i++) {
            
            data_buffer = std::vector<double_t>(max_readable_vector_size);
            std::thread miner(read_data, file, to_read);
            threads.emplace_back(miner);
            std::thread worker(&resolve, (void*)&numbers_count);
            threads.emplace_back(worker);
        }
        for (unsigned int i = 0; i < threads.size(); ++i)
        {
            if (threads[i].joinable())
                threads.at(i).join();
        }

        ResolverResult r;
        r.result = 5;
        return r;
    }

    void percentile_finder::PercentileFinderParallel::reset_config()
    {
    }

};

