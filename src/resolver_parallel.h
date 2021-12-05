#pragma once

#include "resolver.h"
#include "resolver_serial.h"
#include "default_config.h"
#include <tbb/flow_graph.h>
#include <map>
#include<mutex>

#if __has_include("tbb/pipeline.h")
#include <tbb/pipeline.h>
#else
#include <tbb/parallel_pipeline.h>
#endif


namespace percentile_finder {

    /**
     * Struct that contains data for parallel process (in pipeline) @see{DataMasker, DataMaskerWithPositions}
     */
    struct ParallelConfig {
        size_t filesize;
        NumberMasker* masker;
        size_t vector_size;
    };

    /**
     * Class for extracting values from file in pipeline
     */
    class DataMiner {
    public:
        /**
         * Constructor
         * @param pconfig pointer to config
         * @param file pointer to ifstream
         * //TODO REMOVE
         * @param pcounter pointer to counter
         */
        explicit DataMiner(ParallelConfig* pconfig, std::ifstream* file, uint64_t* pcounter) {
            this->config = pconfig;
            this->file = file;
            this->counter = pcounter;
        }

        ~DataMiner() = default;;


        std::vector<double> operator()(tbb::flow_control& fc) const;
    private:
        ParallelConfig* config;
        std::ifstream* file;
        uint64_t* counter;
    };

    /**
     * Class for pipeline - extracts values and positions from file
     */
    class DataMinerWithPositions {
    public:
        explicit DataMinerWithPositions(ParallelConfig* pconfig, std::ifstream* file) {
            this->config = pconfig;
            this->file = file;
        }

        ~DataMinerWithPositions() = default;;


        /**
         * @param flow control needed for pipeline ending
         * @return pair of <start position that was read ; vector of doubles read from file>
         */
        std::pair<size_t, std::vector<double>> operator()(tbb::flow_control& fc) const;

    private:
        ParallelConfig* config;
        std::ifstream* file;
    };

    /**
     * Class for pipeline - masking data and saving positions to PositionMap
     */
    class DataMaskerPoistions {
    public:
        explicit DataMaskerPoistions(PositionsMap* map, ParallelConfig* pconfig, uint32_t pindex, Watchdog* w) {
            this->positions = map;
            this->config = pconfig;
            this->index = pindex;
            this->watchdog = w;
        }

        /**
         * return index from number and if t is looked up index, saves number position to position map
         * @param pair
         * @return vector of double vales from bucket
         */
        std::vector<double> operator()(std::pair<size_t, std::vector<double>> pair) const;
    private:
        PositionsMap* positions{};
        ParallelConfig* config;
        uint32_t index;
        Watchdog* watchdog;
    };

    /**
     * Masks data (sorts them into buckets) using bit masking
     */
    class DataMasker {
        public:
        explicit DataMasker(ParallelConfig* pconfig, Watchdog* w) {
            this->config = pconfig;
            this->watchdog = w;
        }

        /**
         *
         * @param numbers
         * @return pair <numbers masked; bucket_histogram in buckets>
         */
        std::pair<uint64_t,std::vector<uint64_t>> operator()(std::vector<double> numbers) const;
        private:
        ParallelConfig* config;
        Watchdog* watchdog;
    };

    /**
     * Merges chunks from DataMasker into one histogram (bucket_histogram in buckets)
     */
    class ChunkMerger {
        public:
        explicit ChunkMerger(percentile_finder::Histogram* phistogram, Watchdog* w, std::mutex* pmutex) {
            this->histogram = phistogram;
            this->watchdog = w;
            this->mutex = pmutex;
        }
        /**
         *  merges all chunks into one histogram
         * @param chunk of histogram
         */
        void operator()(std::pair<uint64_t, std::vector<uint64_t>> chunk) const;

        private:
        percentile_finder::Histogram *histogram;
        Watchdog* watchdog;
        std::mutex* mutex;
    };

    /**
     * Class for pipelining - finds numbers from bucket and returns them in one vector
     */
    class DataVectorMerger {
    public:
        explicit DataVectorMerger(std::vector<double>* pfinal_result, Watchdog* w) {
            this->final_result = pfinal_result;
            this->watchdog = w;
        }

        /**
         * Insert values from bucket into vector that will be sorted and analyzed
         */
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
        /**
         * Config for algorithm
         */
        PercentileFinderConfig config {0,0,0,0,0};
        /**
         * Config for parallel pipelines
         */
        ParallelConfig parallelism_config{0, nullptr, 0};
        /**
         * histogram vector
         */
        std::vector<uint64_t> bucket_histogram;
        /**
         * number masker instance for masking bits of numbers
         */
        NumberMasker masker;
        /**
         * mutex for
         */
        std::mutex mutex;

        /**
         * data buffer instance
         */
        std::vector<double> data_buffer;

        /**
         *
         *
         * @param file
         * @param percentile
         * @return
         */
        PartialResult get_value_positions_smp(std::ifstream &file);
    };
}
