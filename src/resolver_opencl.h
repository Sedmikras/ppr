#pragma once
#include "open_cl_default_header.h"
#include "resolver.h"

namespace  percentile_finder {
    class  PercentileFinderOpenCL: public PercentileFinder {

    public:
        PercentileFinderOpenCL(Watchdog* w, const cl::Device &device);

        ~PercentileFinderOpenCL() noexcept override;

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
        cl::Context context;
        cl::CommandQueue queue;
        cl::Kernel kernel_bucket_index;
        NumberMasker masker;
        std::vector<double> data_buffer;
        /**
        * Config for algorithm
        */
        PercentileFinderConfig config {0,0,0,0,0};

        PartialResult resolve(std::ifstream &file);
    };
}
