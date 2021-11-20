#include "resolver.h"
#include "watchdog.h"
#include "filereader.h"
#include <map>
#include <algorithm>
#include <functional>
#include <array>
#include <iostream>
#include <string_view>
#include <execution>

namespace percentile_finder {

	PercentileFinder::~PercentileFinder() noexcept {
		// force watchdog to stop and join its thread on destruction
		watchdog.stop();
	}

    PercentileFinder::PercentileFinder(Watchdog watchdog) noexcept:
        watchdog(std::move(watchdog)) {}


	/**
		* Find a value from file on the given percentile.
		*
		* @param percentile The percentile.
		* @return The result value or nullopt if the file doesn't contain any normal double.
		*/
    ResolverResult PercentileFinder::find_percentile(std::ifstream& file, uint8_t looked_up_percentile) {
        reset_filereader(file);

        file.seekg(0, std::ios::end);
        uint64_t filesize = file.tellg();
        file.seekg(std::ios::beg);

        std::vector<double> fileData((uint64_t)(ceil(filesize / 8)));
        std::vector<double> realData(0);
        file.read((char*)&fileData[0], filesize);
        std::map<double, Position> positions;
        for (uint64_t i = 0; i < fileData.size(); i++)
        {
            if (std::fpclassify(fileData[i]) == FP_ZERO || std::fpclassify(fileData[i]) == FP_NORMAL) {
                realData.push_back(fileData[i]);
                try
                {
                    auto element = positions.at(fileData[i]);
                    element.last = i * 8;
                }
                catch (const std::exception&)
                {
                    Position p{ p.first = i * 8, p.last = i * 8 };
                    positions.insert({ fileData[i], p });
                }
            }
        }

        std::sort(std::execution::par_unseq, realData.begin(), realData.end());


        double last_percentile = 0;
        uint64_t index = 0;
        for (uint64_t i = 0; i < realData.size(); i++) {
            double percentile = (((i + 1) / (double)realData.size()) * 100);
            if (percentile >= looked_up_percentile && last_percentile < looked_up_percentile) {
                index = i;
                break;
            }
            last_percentile = percentile;
        }
        double number = realData[index];
        ResolverResult r{ r.result = realData[index], positions.at(number) };
        return r;
	}
}