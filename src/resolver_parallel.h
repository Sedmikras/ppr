#pragma once

#include "resolver.h"
#include "resolver_serial.h"
#include "default_config.h"
#include <tbb/flow_graph.h>
#include <map>
#include<mutex>


namespace percentile_finder {

    struct ParallelConfig {
        size_t filesize;
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
        explicit DataMaskerPoistions(PositionsMap* map, ParallelConfig* pconfig, uint32_t pindex, Watchdog* w) {
            this->positions = map;
            this->config = pconfig;
            this->index = pindex;
            this->watchdog = w;
        }

        std::vector<double> operator()(std::pair<size_t, std::vector<double>> pair) const;
    private:
        PositionsMap* positions{};
        ParallelConfig* config;
        uint32_t index;
        Watchdog* watchdog;
    };

    class DataMasker {
        public:
        explicit DataMasker(ParallelConfig* pconfig, Watchdog* w) {
            this->config = pconfig;
            this->watchdog = w;
        }

        std::pair<uint64_t,std::vector<uint64_t>> operator()(std::vector<double> numbers) const;
        private:
        ParallelConfig* config;
        Watchdog* watchdog;
    };

    class ChunkMerger {
        public:
        explicit ChunkMerger(percentile_finder::Histogram* phistogram, Watchdog* w) {
            this->histogram = phistogram;
            this->watchdog = w;
        }

        void operator()(std::pair<uint64_t, std::vector<uint64_t>> chunk) const;

        private:
        percentile_finder::Histogram *histogram;
        Watchdog* watchdog;
    };

    class LastStand {
    public:
        explicit LastStand(std::vector<double>* pfinal_result, Watchdog* w) {
            this->final_result = pfinal_result;
            this->watchdog = w;
        }

        void operator()(std::vector<double>) const;
    private:
        std::vector<double>* final_result;
        Watchdog* watchdog;
    };
  
    /**
     * Serial percentile finder.
     *
     */
    class PercentileFinderParallel : public PercentileFinder {
    public:
        PercentileFinderParallel() noexcept;
        static std::mutex mutex;

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


        explicit PercentileFinderParallel(Watchdog* w) {
            this->watchdog = w;
        }

    private:
        ParallelConfig config;
        std::vector<uint64_t> frequencies;
        NumberMasker masker;
        PartialResult get_value_positions_smp(std::ifstream &file, uint8_t percentile);

        ResolverResult find_result_last_try(std::ifstream &file, uint8_t looked_up_percentile, PartialResult pr);
    };
}
