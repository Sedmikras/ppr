#pragma once

#include "resolver.h"
#include "resolver_serial.h"
#include "default_config.h"
#include <tbb/flow_graph.h>
#include <map>


namespace percentile_finder {
    const unsigned int MAX_LIVE_TOKENS = 4;

    struct ParallelConfig {
        size_t filesize;
        uint32_t iterations;
        uint32_t i;
        NumberMasker* masker;
        size_t vector_size;
        uint64_t numbers_before;
    };

    class DataMiner {
    public:
        explicit DataMiner(ParallelConfig* pconfig, std::ifstream* file) {
            this->config = pconfig;
            this->file = file;
        }

        ~DataMiner() = default;;


        std::vector<double> operator()(tbb::flow_control& fc) const;

    private:
        ParallelConfig* config;
        std::ifstream* file;
    };

    class DataMinerWithPositions {
    public:
        explicit DataMinerWithPositions(ParallelConfig* pconfig, std::ifstream* file) {
            this->config = pconfig;
            this->file = file;
        }

        ~DataMinerWithPositions() = default;;


        std::pair<size_t, std::vector<double>> operator()(tbb::flow_control& fc) const;

    private:
        ParallelConfig* config;
        std::ifstream* file;
    };

    class DataMaskerPoistions {
    public:
        explicit DataMaskerPoistions(std::unordered_map<double, Position>* ppositions, ParallelConfig* pconfig, uint32_t pindex) {
            this->positions = ppositions;
            this->config = pconfig;
            this->index = pindex;
        }

        std::vector<double> operator()(std::pair<size_t, std::vector<double>> pair) const;
    private:
        std::unordered_map<double, Position>* positions{};
        ParallelConfig* config;
        uint32_t index;
    };

    class DataMasker {
        public:
        explicit DataMasker(ParallelConfig* pconfig) {
            this->config = pconfig;
        }

        std::pair<uint64_t,std::vector<uint64_t>> operator()(const std::vector<double> numbers) const;
        private:
        ParallelConfig* config;
    };

    class ChunkMerger {
        public:
        explicit ChunkMerger(percentile_finder::Histogram* phistogram) {
            this->histogram = phistogram;
        }

        void operator()(std::pair<uint64_t, std::vector<uint64_t>> chunk) const;

        private:
        percentile_finder::Histogram *histogram;
    };

    class LastStand {
    public:
        explicit LastStand(std::vector<double>* pfinal_result) {
            this->final_result = pfinal_result;
        }

        void operator()(std::vector<double>) const;
    private:
        std::vector<double>* final_result;
    };
  
    /**
     * Serial percentile finder.
     *
     */
    class PercentileFinderParallel : public PercentileFinder {
    public:
        PercentileFinderParallel() noexcept;

        /**
       * Find a value from file on the given percentile.
       *
       * @param ifstream - opened file stream
       * @param percentile The percentile.
       * @return The result value or nullopt if the file doesn't contain any normal double.
       */
        ResolverResult find_percentile(std::ifstream& file, uint8_t percentile) override;

        /**
        * resets configuration - usefull when testing multiple percentile in one run
        */
        void reset_config() override;

    private:
        ParallelConfig config;
        std::vector<uint64_t> frequencies;
        NumberMasker masker;
        PartialResult get_value_positions_smp(std::ifstream &file, uint8_t percentile);
    };
}
