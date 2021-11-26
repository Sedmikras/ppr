#include "resolver_parallel.h"
#include "default_config.h"
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <tbb/parallel_pipeline.h>
#include <map>
#include <execution>

namespace percentile_finder {
    PercentileFinderParallel::PercentileFinderParallel() noexcept {
        this->masker.stage = Stage::FIRST;
        config.filesize = 0;
    }

    PartialResult PercentileFinderParallel::get_value_positions_smp(std::ifstream &file, uint8_t percentile) {
        reset_filereader(file);
        if(config.filesize == 0) {
            file.seekg(0, std::ios::end);
            auto filesize = file.tellg();
            auto iterations = (uint32_t) ceil((double) filesize / MAX_BUFFER_SIZE);
            config.filesize = filesize;
            config.iterations = iterations;
            config.masker = &masker;
            config.numbers_before = 0;
            file.seekg(std::ios::beg);
        }
        config.vector_size = masker.get_masked_vector_size();
        Histogram h{std::vector<uint64_t>(config.vector_size), config.numbers_before};

        tbb::parallel_pipeline(
                MAX_LIVE_TOKENS,
                tbb::make_filter<void, std::vector<double>>(
                        tbb::filter_mode::serial_in_order, DataMiner(&config, &file)
                )
                &
                tbb::make_filter<std::vector<double>, std::pair<uint64_t, std::vector<uint64_t>>>(
                        tbb::filter_mode::parallel, DataMasker(&config)
                )
                &
                tbb::make_filter<std::pair<uint64_t, std::vector<uint64_t>>, void>(
                        tbb::filter_mode::serial_out_of_order, ChunkMerger(&h)
                )
        );
        //check which of the number is on the percentile
        double last_percentile = 0, actual_percentile = 0;
        uint32_t index = 0;
        if (h.buckets.size() == 0) {
            index = index;
        }

        for (int i = 0; i < h.buckets.size(); i++) {
            if (h.buckets[i] == 0)
                continue;

            double actual_percentile = (((config.numbers_before + h.buckets[i] + 1) / (double) h.numbers_count) * 100);
            if (actual_percentile >= percentile && last_percentile < percentile) {
                index = i;
                break;
            }
            last_percentile = actual_percentile;
            this->config.numbers_before += h.buckets[i];
        }
        uint64_t numbers_in_index = h.buckets[index];
        PartialResult r{h.numbers_count, h.buckets[index], index};
        return r;
    }

    ResolverResult PercentileFinderParallel::find_percentile(std::ifstream &file, uint8_t percentile) {
        PartialResult p = get_value_positions_smp(file, percentile);
        reset_filereader(file);
        std::unordered_map<double, Position> positions;
        std::vector<double> final_result;
        file.seekg(std::ios::beg);

        tbb::parallel_pipeline(
                MAX_LIVE_TOKENS,
                tbb::make_filter<void, std::pair<size_t, std::vector<double>>>(
                        tbb::filter_mode::serial_in_order, DataMinerWithPositions(&config, &file)
                )
                &
                tbb::make_filter<std::pair<size_t, std::vector<double>>, std::vector<double>>(
                        tbb::filter_mode::parallel, DataMaskerPoistions(&positions, &config, p.index)
                )
                &
                tbb::make_filter<std::vector<double>, void>(
                        tbb::filter_mode::serial_out_of_order, LastStand(&final_result)
                )
        );

        std::sort(std::execution::par_unseq, final_result.begin(), final_result.end());
        uint32_t index = 0;
        double actual_percentile;
        double last_percentile = (double)config.numbers_before / p.numbers_count;
        for(int i = 0; i < final_result.size(); i++) {
            actual_percentile = ((config.numbers_before + i) / (double)p.numbers_count) * 100;
            if (actual_percentile >= percentile && last_percentile < percentile) {
                index = i;
                break;
            }
            last_percentile = actual_percentile;
        }
        percentile = percentile;
        if(positions.contains(final_result[index])) {
            return ResolverResult{final_result[index], positions.at(final_result[index])};
        } else {
            return ResolverResult{final_result[index], Position{0,0}};
        }
    }

    void PercentileFinderParallel::reset_config() {
        std::cout << "reset";
    }

    std::vector<double> DataMiner::operator()(tbb::flow_control &fc) const {
        std::vector<double> data_buffer(MAX_VECTOR_SIZE);
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

        return std::vector<double>(data_buffer.begin(), data_buffer.begin() + read);
    }

    std::pair<uint64_t, std::vector<uint64_t>> DataMasker::operator()(const std::vector<double> numbers) const {
        std::vector<uint64_t> frequencies(config->vector_size);
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
        histogram->numbers_count += chunk.first;
        std::transform(chunk.second.begin(), chunk.second.end(), histogram->buckets.begin(), histogram->buckets.begin(),
                       std::plus<uint64_t>());
        return;
    }

    std::pair<size_t, std::vector<double>> DataMinerWithPositions::operator()(tbb::flow_control &fc) const {
        std::vector<double> data_buffer(MAX_VECTOR_SIZE);
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
        std::vector<double> values;
        for (int i = 0; i < pair.second.size(); i++) {
            if (config->masker->return_index_from_double(pair.second[i]) == index) {
                values.push_back(pair.second[i]);
                if (positions->contains(pair.second[i])) {
                    Position *p = &(positions->at(pair.second[i]));
                    auto new_pos = pair.first + i * 8;
                    p->last = new_pos;
                } else {
                    Position p {p.first = pair.first + i * 8, p.last = pair.first + i * 8};
                    positions->insert({pair.second[i], p});
                }
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
    };
}
