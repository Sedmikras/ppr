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
        config.filesize = 0;
    }

    ResolverResult PercentileFinderParallel::find_result_last_try(std::ifstream& file, uint8_t looked_up_percentile, PartialResult pr) {
        reset_filereader(file);
        auto iterations = ((config.filesize / 8) / MAX_VECTOR_SIZE);
        uint64_t max_readable_vector_size = config.filesize < MAX_BUFFER_SIZE ? (uint64_t)ceil(config.filesize / 8.0) : MAX_VECTOR_SIZE;
        std::vector<double_t> fileData(max_readable_vector_size);
        std::map<double, Position> positions;
        double result = NAN;
        for (int i = 0; i < iterations; i++) {
            watchdog->notify();
            uint64_t to_read = ((i + 1 * MAX_BUFFER_SIZE) > config.filesize) ? (config.filesize - (i * MAX_BUFFER_SIZE)) : MAX_BUFFER_SIZE;
            file.read((char*)&fileData[0], to_read);
            uint32_t size = (uint32_t)(to_read + to_read % 8) / 8;

            for (int j = 0; j < size; j++) {
                uint32_t index = masker.return_index_from_double(fileData[j]);
                if (index == pr.index) {
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
        ResolverResult r{ r.result = result, positions.at(result) };
        return r;
    }

    PartialResult PercentileFinderParallel::get_value_positions_smp(std::ifstream &file, uint8_t percentile) {
        reset_filereader(file);
        if(config.filesize == 0) {
            file.seekg(0, std::ios::end);
            auto filesize = file.tellg();
            auto iterations = (uint32_t) ceil((double) filesize / MAX_BUFFER_SIZE);
            config.filesize = filesize;
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
                        tbb::filter_mode::parallel, DataMasker(&config, watchdog)
                )
                &
                tbb::make_filter<std::pair<uint64_t, std::vector<uint64_t>>, void>(
                        tbb::filter_mode::serial_out_of_order, ChunkMerger(&h, watchdog)
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
            actual_percentile = ((double)(config.numbers_before + h.buckets[i]) / h.numbers_count) * 100;
            if (actual_percentile >= percentile && last_percentile < percentile) {
                index = i;
                break;
            }
            last_percentile = actual_percentile;
            this->config.numbers_before += h.buckets[i];
        }
        uint64_t numbers_in_index = h.buckets[index];
        PartialResult r{h.numbers_count, h.buckets[index], index};
        h.buckets.clear();
        h.numbers_count = 0;
        return r;
    }

    ResolverResult PercentileFinderParallel::find_percentile(std::ifstream &file, uint8_t percentile) {
        watchdog->notify();
        reset_config();
        PartialResult pr {};
        //iterate the file and find in which bucket the number is located
        do {
            watchdog->notify();
            masker.increment_stage(pr.index);
            pr = get_value_positions_smp(file, percentile);
            if(masker.stage == Stage::LAST) {
                return find_result_last_try(file, percentile, pr);
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
                        tbb::filter_mode::serial_in_order, DataMinerWithPositions(&config, &file)
                )
                &
                tbb::make_filter<std::pair<size_t, std::vector<double>>, std::vector<double>>(
                        tbb::filter_mode::parallel, DataMaskerPoistions(&positions, &config, pr.index, watchdog)
                )
                &
                tbb::make_filter<std::vector<double>, void>(
                        tbb::filter_mode::serial_out_of_order, LastStand(&final_result, watchdog)
                )
        );
        watchdog->notify();
        std::sort(std::execution::par_unseq, final_result.begin(), final_result.end());
        watchdog->notify();
        uint32_t index = 0;
        double actual_percentile;
        double last_percentile = (double)config.numbers_before / pr.numbers_count;
        watchdog->notify();
        for(int i = 0; i < final_result.size(); i++) {
            actual_percentile = ((config.numbers_before + i + 1) / (double)pr.numbers_count) * 100;
            if (actual_percentile >= percentile && last_percentile < percentile) {
                index = i;
                break;
            }
            last_percentile = actual_percentile;
        }
        percentile = percentile;
        watchdog->notify();
        return ResolverResult{final_result[index], positions.get(final_result[index])};
    }

    void PercentileFinderParallel::reset_config() {
        config.numbers_before = 0;
        config.filesize = 0;
        config.vector_size = 0;
        frequencies.clear();
        masker.high = 0;
        masker.low = 0;
        masker.stage = Stage::ZERO;
        masker.zero_phase_index = 0;
    }

    std::vector<double> DataMiner::operator()(tbb::flow_control &fc) const {
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

        return std::vector<double>(data_buffer.begin(), data_buffer.begin() + read);
    }

    std::pair<uint64_t, std::vector<uint64_t>> DataMasker::operator()(const std::vector<double> numbers) const {
        watchdog->notify();
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
                positions->position_update_safe(value, i, pair.first);
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
    };

    void PositionsMap::position_update_safe(double value, int i, size_t first_pos) {
        std::unique_lock<std::mutex> lock(mutex);
        if (positions.contains(value)) {
            Position *p = &(positions.at(value));
            auto new_pos = first_pos + i * 8;
            p->last = new_pos;
        } else {
            Position p {p.first = first_pos + i * 8, p.last = first_pos + i * 8};
            positions.insert({value, p});
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



