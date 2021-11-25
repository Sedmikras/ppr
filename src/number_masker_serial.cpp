#include "resolver_serial.h"
#include "default_config.h"

namespace percentile_finder {

	uint32_t NumberMasker::return_index_from_double_first_stage(double number) const {
		if (std::fpclassify(number) == 0) {
			return FIRST_INDEX;
		}
		else if (std::fpclassify(number) == -1) {
			int64_t num = *(int64_t*)&number;
			if (number < 0) {
				num = (num >> 43) & MASK_ZERO;
				uint32_t result = FIRST_INDEX - (num);
				if (result == FIRST_INDEX) {
					result = result;
				}
				return result;
			}
			else {
				num = (num >> 43) & MASK_ZERO;
				uint32_t result = FIRST_INDEX + (num);
				if (result == FIRST_INDEX) {
					result = result;
				}
				return result;
			}
		}
		return UINT32_MAX;
	}

	uint32_t NumberMasker::return_index_from_double_second_stage(double number) const {
		if (std::fpclassify(number) == -1 || std::fpclassify(number) == 0) {
			if (number >= low && number < high) {
				int64_t num = *(int64_t*)&number;
				if ((num >> 63) == 1) {
					num = (~num >> 22) & MASK_ONE;
				}
				else {
					num = (num >> 22) & MASK_ONE;
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
				if ((num >> 63) == 1) {
					num = (~num) & MASK_TWO;
				}
				else {
					num = (num) & MASK_TWO;
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

	uint32_t NumberMasker::return_index_from_double(double number) const {
		switch (stage)
		{
		case Stage::FIRST: return return_index_from_double_first_stage(number);
		case Stage::SECOND: return return_index_from_double_second_stage(number);
		case Stage::LAST: return return_index_from_double_last_stage(number);
			break;
		default: return UINT32_MAX;
		}
	}

	Border NumberMasker::get_border_values_second_stage(uint32_t index) {
		Border b;
		uint32_t var_index = index - 1;
		if (index < FIRST_INDEX) {
			var_index = ~var_index & MASK_ONE;
			var_index = var_index + FIRST_INDEX;
			int64_t lowint = ((int64_t)var_index << 43);
			double var_low = *(double*)&lowint;
			var_index = index - 2;
			var_index = ~var_index & MASK_ONE;
			var_index = var_index + FIRST_INDEX;
			lowint = ((int64_t)var_index << 43);
			double var_high = *(double*)&lowint;
			b.low = var_high;
			b.high = var_low;
		}
		else {
			var_index = index - FIRST_INDEX;
			int64_t lowint = ((int64_t)var_index << 43);
			double low = *(double*)&lowint;
			b.low = low;
			lowint = ((int64_t)var_index + 1 << 43);
			double high = *(double*)&lowint;
			b.high = high;
		}
		return b;
	}

	Border NumberMasker::get_border_values_last_stage(uint32_t index) {
		Border b;
		uint32_t var_index = index - 1;
		if (index < FIRST_INDEX) {
			var_index = ~var_index & MASK_ONE;
			var_index = var_index + FIRST_INDEX;
			int64_t lowint = ((int64_t)var_index << 43);
			double var_low = *(double*)&lowint;
			var_index = index - 2;
			var_index = ~var_index & MASK_ONE;
			var_index = var_index + FIRST_INDEX;
			lowint = ((int64_t)var_index << 43);
			double var_high = *(double*)&lowint;
			b.low = var_high;
			b.high = var_low;
		}
		else {
			var_index = index - FIRST_INDEX;
			int64_t lowint = ((int64_t)var_index << 43);
			double low = *(double*)&lowint;
			b.low = low;
			lowint = ((int64_t)var_index + 1 << 43);
			double high = *(double*)&lowint;
			b.high = high;
		}
		return b;
	}

	Border NumberMasker::get_border_values(uint32_t index) {
		switch (stage)
		{
			case Stage::FIRST: return get_border_values_second_stage(index); break;
			case Stage::SECOND: return get_border_values_last_stage(index); break;
			default: {Border b;
				b.low = 0;
				b.high = 1;
				return b; } break;
		}
		
	}

	void NumberMasker::increment_stage(uint32_t index) {
		if (stage == Stage::SECOND) {
			index = index;
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

	uint32_t NumberMasker::get_masked_vector_size()
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

