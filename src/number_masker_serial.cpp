#include "resolver_serial.h"

namespace percentile_finder {

	uint32_t NumberMasker::return_index_from_double_first_stage(double number) {
		if (std::fpclassify(number) == 0) {
			return FIRST_INDEX;
		}
		else if (std::fpclassify(number) == -1) {
			int64_t num = *(int64_t*)&number;
			if (number < 0) {
				num = (~(num >> PHASE_ZERO_SHIFT)) & MASK_ZERO;
                return num;
			}
			else {
				num = (num >> PHASE_ZERO_SHIFT) & MASK_ZERO;
				uint32_t result = FIRST_INDEX + (num);
				return result;
			}
		}
		return UINT32_MAX;
	}

	uint32_t NumberMasker::return_index_from_double_second_stage(double number) const {
		if (std::fpclassify(number) == -1 || std::fpclassify(number) == 0) {
			if (number >= low && number < high) {
				int64_t num = *(int64_t*)&number;
				if (((uint64_t)num >> 63) == 1) {
                    if(number==low)
                        return 0;
                    num = (~(num >> PHASE_TWO_BITS)) & MASK_ONE;
                    return num;
				}
				else {
					num = (num >> PHASE_ONE_BITS) & MASK_ONE;
				}

				return num;
			}
			else {
				return UINT32_MAX;
			}
		}
		else {
			return UINT32_MAX;
		}
	}

	uint32_t NumberMasker::return_index_from_double_last_stage(double number) const {
		if (std::fpclassify(number) == -1 || std::fpclassify(number) == 0) {
			if (number >= low && number < high) {
				int64_t num = *(int64_t*)&number;
                num = (num) & MASK_TWO;
				return num;
			}
			else {
				return UINT32_MAX;
			}
		}
		else {
			return UINT32_MAX;
		}
	}

	uint32_t NumberMasker::return_index_from_double(double number) const {
		switch (stage)
		{
		case Stage::FIRST: return return_index_from_double_first_stage(number);
		case Stage::SECOND: return return_index_from_double_second_stage(number);
		case Stage::LAST: return return_index_from_double_last_stage(number);
		default: return UINT32_MAX;
		}
	}

	Border NumberMasker::get_border_values_second_stage(uint32_t index) {
		Border b;
		uint32_t var_index = index;
		if (index < FIRST_INDEX) {
			var_index = ~var_index & MASK_ZERO;
			var_index = var_index + FIRST_INDEX;
			int64_t lowint = ((int64_t)var_index << PHASE_ZERO_SHIFT);
			double var_low = *(double*)&lowint;
			lowint = ((int64_t)(var_index -1) << PHASE_ZERO_SHIFT);
			double var_high = *(double*)&lowint;
			b.low = var_low;
			b.high = var_high;
		}
		else {

			var_index = index - FIRST_INDEX;
			int64_t lowint = ((int64_t)var_index << PHASE_ZERO_SHIFT);
			double low = *(double*)&lowint;
			b.low = low;
			lowint = ((int64_t)(var_index + 1) << PHASE_ZERO_SHIFT);
			double high = *(double*)&lowint;
			b.high = high;
		}
		return b;
	}

	Border NumberMasker::get_border_values_last_stage(uint32_t index) const {
		Border b;
        double source = this->low;
        uint64_t all_bits_index = index;
        int64_t highint = *(int64_t*)&source;
        int64_t  complement = ((all_bits_index) << PHASE_TWO_BITS);
        int64_t  complement2 = ((all_bits_index + 1) << PHASE_TWO_BITS);
        if(this->low < 0) {
            complement = (highint - complement);
            complement2 = (highint - complement2);
        } else {
            complement = (highint + complement);
            complement2 = (highint + complement2);
        }
        b.low = *(double*)&complement;
        b.high = *(double*)&complement2;
		return b;
	}

	Border NumberMasker::get_border_values(uint32_t index) {
		switch (stage)
		{
			case Stage::FIRST: return get_border_values_second_stage(index);
			case Stage::SECOND: return get_border_values_last_stage(index);
            case Stage::ZERO:
                break;
            case Stage::LAST:
                break;
        }
		
	}

	void NumberMasker::increment_stage(uint32_t index) {
        if(stage == Stage::ZERO) {
            stage = Stage::FIRST;
            return;
        }
		Border b = get_border_values(index);
		this->low = b.low;
		this->high = b.high;
		zero_phase_index = index;
		switch (stage)
		{
		case percentile_finder::Stage::FIRST:
			stage = Stage::SECOND;
			break;
		case percentile_finder::Stage::SECOND:
			stage = Stage::LAST;
			break;
		case percentile_finder::Stage::LAST:
			stage = Stage::LAST;
			break;
		default:
			break;
		}
    }

    NumberMasker::NumberMasker() {
		stage = Stage::FIRST;
    }

	uint32_t NumberMasker::get_masked_vector_size() const
	{
		if (stage == Stage::FIRST) {
			return 1 << PHASE_ZERO_BITS;
		}
		else if (stage == Stage::SECOND) {
			return 1 << PHASE_ONE_BITS;
		}
		else if (stage == Stage::LAST) {
			return 1 << PHASE_TWO_BITS;
		}
		else return NULL;
	}
}

