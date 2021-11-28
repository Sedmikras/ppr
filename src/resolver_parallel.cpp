#include "resolver_parallel.h"
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <tbb/parallel_pipeline.h>
#include <execution>

namespace percentile_finder {

    PercentileFinderParallel::PercentileFinderParallel() noexcept {
        this->masker.stage = Stage::ZERO;
    }

    PartialResult PercentileFinderParallel::get_value_positions_smp(std::ifstream &file, uint8_t percentile) {
        parallelism_config.vector_size = masker.get_masked_vector_size();
        Histogram h{std::vector<uint64_t>(parallelism_config.vector_size), config.numbers_before};
        uint64_t numbers_count_t = 0;
        tbb::parallel_pipeline(
                MAX_LIVE_TOKENS,
                tbb::make_filter<void, std::vector<double>>(
                        tbb::filter_mode::serial_in_order, DataMiner(&parallelism_config, &file, &numbers_count_t)
                )
                &
                tbb::make_filter<std::vector<double>, std::pair<uint64_t, std::vector<uint64_t>>>(
                        tbb::filter_mode::parallel, DataMasker(&parallelism_config, watchdog)
                )
                &
                tbb::make_filter<std::pair<uint64_t, std::vector<uint64_t>>, void>(
                        tbb::filter_mode::serial_out_of_order, ChunkMerger(&h, watchdog, &mutex)
                )
        );
        auto counter = 0;
        for(auto value : h.buckets) {
            counter += value;
        }
        if(config.total_number_count == 0) {
            config.total_number_count = h.numbers_count;
        }

        //check which of the number is on the percentile
        PartialResult pr = get_bucket_index(h.buckets, &config, watchdog);
        h.buckets.clear();
        h.numbers_count = 0;
        return pr;
    }

    ResolverResult PercentileFinderParallel::find_percentile(std::ifstream &file, uint8_t percentile) {
        watchdog->notify();
        reset_config();
        reset_filereader(file);
        file.seekg(0, std::ios::end);
        auto filesize = file.tellg();
        auto iterations = (uint32_t) ceil((double) filesize / MAX_BUFFER_SIZE);
        config.filesize = filesize;
        config.numbers_before = 0;
        config.looked_up_percentile = percentile;
        parallelism_config.masker = &masker;
        parallelism_config.filesize = filesize;
        reset_filereader(file);
        PartialResult pr {};
        //iterate the file and find in which bucket the number is located
        do {
            watchdog->notify();
            masker.increment_stage(pr.bucket_index);
            pr = get_value_positions_smp(file, percentile);
            if(masker.stage == Stage::LAST) {
                return find_result_last_try(file, &config, &masker, pr, watchdog);
            }
        } while (pr.numbers_in_index > MAX_VECTOR_SIZE && masker.stage != Stage::LAST);
        reset_filereader(file);
        PositionsMap positions;
        std::vector<double> final_result;
        reset_filereader(file);
        file.seekg(std::ios::beg);

        tbb::parallel_pipeline(
                MAX_LIVE_TOKENS,
                tbb::make_filter<void, std::pair<size_t, std::vector<double>>>(
                        tbb::filter_mode::serial_in_order, DataMinerWithPositions(&parallelism_config, &file)
                )
                &
                tbb::make_filter<std::pair<size_t, std::vector<double>>, std::vector<double>>(
                        tbb::filter_mode::parallel, DataMaskerPoistions(&positions, &parallelism_config, pr.bucket_index, watchdog)
                )
                &
                tbb::make_filter<std::vector<double>, void>(
                        tbb::filter_mode::serial_out_of_order, LastStand(&final_result, watchdog)
                )
        );
        watchdog->notify();
        std::sort(std::execution::par_unseq, final_result.begin(), final_result.end());
        watchdog->notify();
        uint32_t index = get_index_from_sorted_vector(final_result, &config, watchdog);
        if(index == UINT32_MAX) {
            return ResolverResult{NAN, Position{NULL, NULL}};
        } else {
            return ResolverResult{final_result[index], positions.get(final_result[index])};
        }
    }

    void PercentileFinderParallel::reset_config() {
        bucket_histogram.clear();
        this->masker.high = 0;
        this->masker.low = 0;
        this->masker.stage = Stage::ZERO;
        this->masker.zero_phase_index = 0;
        this->config.filesize = 0;
        this->config.iterations = 0;
        this->config.total_number_count = 0;
        this->config.looked_up_percentile = 0;
        this->config.numbers_before = 0;
    }

    std::vector<double> DataMiner::operator()(tbb::flow_control &fc) const {
        size_t actual_position = file->tellg();
        size_t to_read = (config->filesize - actual_position > (MAX_VECTOR_SIZE_PARALLEL * 8)) ? MAX_VECTOR_SIZE_PARALLEL * 8 : (config->filesize - actual_position);
        std::vector<double> data_buffer(to_read / 8);
        if(to_read == 0) {
            fc.stop();
        } else {
            file->read((char *) &data_buffer[0], to_read);
        }
        return data_buffer;
    }

    std::pair<uint64_t, std::vector<uint64_t>> DataMasker::operator()(const std::vector<double> numbers) const {
        watchdog->notify();
        std::vector<uint64_t> frequencies(config->vector_size, 0);
        uint64_t numbers_count = 0;
        for (int i = 0; i < numbers.size(); i++) {
            uint32_t index = config->masker->return_index_from_double(numbers[i]);
            if (index == UINT32_MAX) {
                continue;
            } else {
                frequencies[index]++;
                numbers_count++;
            }
        }
        auto result = std::pair<uint64_t, std::vector<uint64_t>>(numbers_count, frequencies);
        return result;
    }

    void ChunkMerger::operator()(std::pair<uint64_t, std::vector<uint64_t>> chunk) const {
        auto chunk_numbers_count = chunk.first;
        auto sychr = 0;
        for(auto value : chunk.second)
            sychr+= value;

        bool error = (chunk_numbers_count == sychr);
        auto before = 0;
        auto after = 0;
        std::unique_lock<std::mutex> lock (*mutex);
        histogram->numbers_count += chunk.first;
        for(auto value : histogram->buckets)
            before+= value;
        std::transform(chunk.second.begin(), chunk.second.end(), histogram->buckets.begin(), histogram->buckets.begin(),
                       std::plus<uint64_t>());
        for(auto value : histogram->buckets)
            after+= value;
        bool error2 = (after == (before + sychr));
        if (!error || !error2) {
            sychr = sychr;
        }
        watchdog->notify();
    }

    std::pair<size_t, std::vector<double>> DataMinerWithPositions::operator()(tbb::flow_control &fc) const {
        std::vector<double> data_buffer(MAX_VECTOR_SIZE_PARALLEL);
        size_t buffer_size_bytes = data_buffer.size() * 8;

        size_t file_position = (size_t) file->tellg();
        if (file_position >= config->filesize) {
            fc.stop();
        }

        file->read((char *) &data_buffer[0], buffer_size_bytes);
        auto read = file->gcount() / 8;
        if (read < 1) {
            fc.stop();
        }
        std::pair<size_t, std::vector<double>> pair {file_position, std::vector<double>(data_buffer.begin(), data_buffer.begin() + read)};
        return pair;
    }

    std::vector<double> DataMaskerPoistions::operator()(std::pair<size_t, std::vector<double>> pair) const {
        watchdog->notify();
        std::vector<double> values;
        double value;
        for (int i = 0; i < pair.second.size(); i++) {
            value = pair.second[i];
            if (config->masker->return_index_from_double(value) == index) {
                values.push_back(pair.second[i]);
                positions->position_insert_update_thread_safe(value, i, pair.first);
            } else {
                continue;
            }
        }
        return values;
    }

    void LastStand::operator()(std::vector<double> partial_result_vector) const {
        final_result->insert(
                final_result->end(),
                std::make_move_iterator(partial_result_vector.begin()),
                std::make_move_iterator(partial_result_vector.end())
        );
        watchdog->notify();
        return;
    }

    void PositionsMap::position_insert_update_thread_safe(double key, int index, size_t first_pos) {
        std::unique_lock<std::mutex> lock(mutex);
        if (positions.contains(key)) {
            Position *p = &(positions.at(key));
            auto new_pos = first_pos + index * 8;
            p->last = new_pos;
        } else {
            Position p {p.first = first_pos + index * 8, p.last = first_pos + index * 8};
            positions.insert({key, p});
        }
    }

    Position PositionsMap::get(double key) {
        if(positions.contains(key)) {
            return positions.at(key);
        } else {
            return Position{NULL,NULL};
        }
    }
}