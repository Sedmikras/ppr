#include "resolver_parallel.h"
#include "default_config.h"
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <tbb/parallel_pipeline.h>

namespace percentile_finder {

    /*PercentileFinderParallel::PercentileFinderParallel(Watchdog watchdog) noexcept :
        PercentileFinder(std::move(watchdog)) {
        this->masker.stage = Stage::FIRST;
        this->numbers_count = 0;
    }*/

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
        get_value_positions_smp(file);
        percentile = percentile;
    }

    void percentile_finder::PercentileFinderParallel::reset_config()
    {
    }

    PercentileFinderParallel::PercentileFinderParallel() noexcept {
        this->masker.stage = Stage::FIRST;
        this->numbers_count = 0;
    }

    Position PercentileFinderParallel::get_value_positions_smp(std::ifstream &file) {
        size_t first_position = 0;
        size_t last_position = 0;
        ParallelConfig conf;
        reset_filereader(file);
        file.seekg(0, std::ios::end);
        auto filesize = file.tellg();
        auto iterations = (uint32_t)ceil((double)filesize / MAX_BUFFER_SIZE);
        conf.filesize = filesize;
        conf.iterations = iterations;
        conf.vector_size = masker.get_masked_vector_size();
        conf.masker = masker;
        Histogram h {frequencies = std::vector<uint64_t>(masker.get_masked_vector_size()), numbers_count = 0};
        reset_filereader(file);

        tbb::parallel_pipeline(MAX_LIVE_TOKENS,
           tbb::make_filter<void, std::vector<double>> (
                   tbb::filter_mode::serial_in_order, DataMiner(conf)
           )
           &
           tbb::make_filter<std::vector<double>, std::pair<uint64_t,std::vector<uint64_t>>> (
                   tbb::filter_mode::parallel, DataMasker(conf)
           )
           &
           tbb::make_filter<std::pair<uint64_t,std::vector<uint64_t>>, void> (
                   tbb::filter_mode::serial_out_of_order, ChunkMerger(&h)
           )
        );

        h = h;
    }

    ResolverResult PercentileFinderParallel::find_result(std::ifstream &file, uint8_t percentile) {

        return ResolverResult();
    }

    std::vector<double> DataMiner::operator()(std::ifstream* file, tbb::flow_control& fc) const {
        std::vector<double> data_buffer(MAX_VECTOR_SIZE);
        size_t buffer_size_bytes = data_buffer.size() * 8;

        size_t file_position = (size_t) file->tellg();
        if (file_position >= config.filesize) {
            fc.stop();
        }

        file->read((char*) &data_buffer[0], buffer_size_bytes);
        auto read = file->gcount() / 8;
        if (read < 1) {
            fc.stop();
        }

        return std::vector<double>(data_buffer.begin(), data_buffer.begin() + read);
    }

    std::pair<uint64_t,std::vector<uint64_t>> DataMasker::operator()(const std::vector<double> numbers) const {
        std::vector<uint64_t> frequencies(config.vector_size);
        int i = 0;
        for(int i = 0; i < numbers.size(); i++) {
            uint32_t index = config.masker.return_index_from_double(numbers[i]);
            if(index == UINT32_MAX) {
                continue;
            } else {
                frequencies[i]++;
                i++;
            }
        }
        auto result = std::pair<uint64_t,std::vector<uint64_t>>(i, frequencies);
        return result;
    }

    void ChunkMerger::operator()(std::pair<uint64_t, std::vector<uint64_t>> chunk) const {
        histogram->numbers_count += chunk.first;
        std::transform(chunk.second.begin(), chunk.second.end(), histogram->buckets.begin(), chunk.second.begin(), std::plus<>());
        return;
    }

    ChunkMerger::ChunkMerger(Histogram *histogram) : histogram(histogram) {
        this->histogram = histogram;
    }
};

