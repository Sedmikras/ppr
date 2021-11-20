#include "resolver_serial.h"
#include "default_config.h"

namespace percentile_finder {

	uint32_t return_index_from_double_zero_phase(double number) {
		if (fpclassify(number) == 0) {
			return FIRST_INDEX;
		}

		else if (fpclassify(number) == -1) {
			if (number < 0) {
				double one_bucket = std::numeric_limits<double>::max() / FIRST_INDEX;
				uint32_t result = (uint32_t)ceil((-number) / one_bucket);
				result = FIRST_INDEX - (result);
				return result;
			}
			else {
				double one_bucket = std::numeric_limits<double>::max() / FIRST_INDEX;
				uint32_t result = (uint32_t)ceil(number / one_bucket);
				if (result == FIRST_INDEX) {
					result = result - 1;
				}
				result = FIRST_INDEX + (result);
				return result;
			}
		}
		return UINT32_MAX;
	}

	uint32_t NumberMasker::return_index_from_double_first_phase(double number) {
		double one_bucket = std::numeric_limits<double>::max() / (4398046511104);
		if (number >= low && number < high) {
			double diff = number - low;
			return 0;
		}
		else {
			return UINT32_MAX;
		}
		/*/if (zero_phase_index == FIRST_INDEX) {
			if (fpclassify(number) == 0) {
				return 0;
			}
		}
		if (fpclassify(number) == -1) {
			if (number < 0) {
				double one_bucket = std::numeric_limits<double>::max() / FIRST_INDEX;
				uint32_t result = (uint32_t)ceil((-number) / one_bucket);
				result = FIRST_INDEX - (result);
				return result;
			}
			else {
				double one_bucket = std::numeric_limits<double>::max() / FIRST_INDEX;
				uint32_t result = (uint32_t)ceil(number / one_bucket);
				if (result == FIRST_INDEX) {
					result = result - 1;
				}
				result = FIRST_INDEX + (result);
				return result;
			}
		}
		return UINT32_MAX;*/
	}

    uint32_t NumberMasker::return_index_from_double(double number) {
		switch (phase)
		{
			case 0: return return_index_from_double_zero_phase(number);
			case 1: return return_index_from_double_first_phase(number);
			case 2: return return_index_from_double_first_phase(number);
			break;
			default: return UINT32_MAX;
		}
    }
	void NumberMasker::increment_phase(uint32_t index) {
		double one_bucket_size = ((std::numeric_limits<double>::max()) /(double)(2* FIRST_INDEX));
		if (index == FIRST_INDEX) {
			this->low = -0.0;
			this->high = one_bucket_size;
		}
		else if (index > FIRST_INDEX) {
			this->low = (index - FIRST_INDEX) * one_bucket_size;
			this->high = (index + 1 - FIRST_INDEX) * one_bucket_size;
		}
		else {
			this->low = -(double)(FIRST_INDEX - index)* one_bucket_size;
			this->high = -(double)(FIRST_INDEX + 1 - index) * one_bucket_size;
		}
		zero_phase_index = index;
        phase++;
    }

    NumberMasker::NumberMasker() {
        phase = 0;
    }

	uint32_t NumberMasker::get_masked_vector_size()
	{
		if (phase == 0) {
			return 1 << PHASE_ZERO_BITS;
		}
		else if (phase == 1) {
			return 1 << PHASE_ONE_BITS;
		}
		else if (phase == 2) {
			return 1 << PHASE_TWO_BITS;
		}
		else return NULL;
	}
}

