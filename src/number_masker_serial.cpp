#include "resolver_serial.h"
#include "bit_masker.h"


namespace percentile_finder {

	uint32_t NumberMasker::get_bit_shift() {
		return bit_shift;
	}

	uint32_t NumberMasker::get_offset() {
		return offset;
	}

	uint32_t NumberMasker::get_mask() {
		return mask;
	}

	double NumberMasker::get_min() {
		return low;
	}

	double NumberMasker::get_max() {
		return high;
	}

    bool NumberMasker::is_number_valid(double number) const {
        return (std::fpclassify(number) == -1 || std::fpclassify(number) == 0);
    }


    bool NumberMasker::in_span(double number) const {
        return (number >= this->low && number < this->high);
    }

	uint32_t NumberMasker::return_index_from_double(double number) const {
        int64_t num = *(int64_t*)&number;
        if(is_number_valid(number) && in_span(number)) {
            if (number < 0) {
                num = (~(num >> bit_shift)) & mask;
                return static_cast<uint32_t>(num);
            }
            else {
                num = (num >> bit_shift) & mask;
                uint32_t result = static_cast<uint32_t>(offset + (num));
                return result;
            }
        } else {
            return UINT32_MAX;
        }
	}

	Border NumberMasker::get_border_values_second_stage(uint32_t index) {
		Border b {-INFINITY, INFINITY };
		int64_t var_index = index;
		if (index < SPLITERATOR_FIRST_INDEX) {
			var_index = ~var_index & BIT_MASK_FIRST_STAGE;
			var_index = var_index + SPLITERATOR_FIRST_INDEX;
			int64_t lowint = var_index << LEFT_SHIFT_COMPLEMENT_FIRST_STAGE;
			double var_low = *(double*)&lowint;
			lowint = (var_index -1) << LEFT_SHIFT_COMPLEMENT_FIRST_STAGE;
			double var_high = *(double*)&lowint;
			b.low = var_low;
			b.high = var_high;
		}
		else {

			var_index = index - SPLITERATOR_FIRST_INDEX;
			int64_t lowint = var_index << LEFT_SHIFT_COMPLEMENT_FIRST_STAGE;
			double low = *(double*)&lowint;
			b.low = low;
			lowint = (var_index + 1) << LEFT_SHIFT_COMPLEMENT_FIRST_STAGE;
			double high = *(double*)&lowint;
			b.high = high;
		}
		return b;
	}

	Border NumberMasker::get_border_values_last_stage(uint32_t index) const {
		Border b {-INFINITY,INFINITY};
        uint64_t complement, complement2;

        if(this->low < 0) {
            double source = this->low;
            uint64_t all_bits_index = index;
            int64_t highint = *(int64_t*)&source;
            complement = ((all_bits_index) << LEFT_SHIFT_COMPLEMENT_SECOND_STAGE);
            complement2 = ((all_bits_index + 1) << LEFT_SHIFT_COMPLEMENT_SECOND_STAGE);
            complement = (highint - complement);
            complement2 = (highint - complement2);
        } else {
            double source = this->high;
            uint64_t all_bits_index = index;
            int64_t highint = *(int64_t*)&source;
            complement2 = ((all_bits_index) << LEFT_SHIFT_COMPLEMENT_SECOND_STAGE);
            complement = ((all_bits_index + 1) << LEFT_SHIFT_COMPLEMENT_SECOND_STAGE);
            complement = (highint - complement);
            complement2 = (highint - complement2);
        }
        b.low = *(double*)&complement;
        b.high = *(double*)&complement2;
		return b;
	}

	Border NumberMasker::get_border_values(uint32_t index) {
		switch (stage)
		{
			case Stage::ZERO: return Border{-std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity() }; break;
			case Stage::FIRST: 
				return get_border_values_second_stage(index); break;
			case Stage::SECOND: 
				return get_border_values_last_stage(index); break;
            case Stage::LAST:
				return Border{ -std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity() }; break;
        }
		return Border{ -std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity() };
	}

	void NumberMasker::increment_stage(uint32_t index) {
		Border b = get_border_values(index);
		this->low = b.low;
		this->high = b.high;
		switch (stage)
		{
		case percentile_finder::Stage::ZERO:
		{
			stage = Stage::FIRST;
			bit_shift = LEFT_SHIFT_COMPLEMENT_FIRST_STAGE;
			mask = BIT_MASK_FIRST_STAGE;
			offset = SPLITERATOR_FIRST_INDEX;
			break;
		}
		case percentile_finder::Stage::FIRST:
			zero_phase_index = index;
			bit_shift = LEFT_SHIFT_COMPLEMENT_SECOND_STAGE;
			mask = BIT_MASK_SECOND_STAGE;
			offset = 0;
			stage = Stage::SECOND;
			break;
		case percentile_finder::Stage::SECOND:
			stage = Stage::LAST;
			mask = BIT_MASK_LAST_STAGE;
			break;
		case percentile_finder::Stage::LAST:
			stage = Stage::LAST;
			break;
		default:
			break;
		}
    }

    NumberMasker::NumberMasker() {
		stage = Stage::ZERO;
		bit_shift = 0;
		offset = 0;
		mask = 0;
		low = -std::numeric_limits<double>::infinity();
		high = +std::numeric_limits<double>::infinity();
    }

	uint32_t NumberMasker::get_masked_vector_size() const
	{
		if (stage == Stage::FIRST) {
			return 1 << BIT_SHIFT_FIRST_STAGE;
		}
		else if (stage == Stage::SECOND) {
			return 1 << BIT_SHIFT_SECOND_STAGE;
		}
		else if (stage == Stage::LAST) {
			return 1 << BIT_SHIFT_LAST_STAGE;
		}
		else return NULL;
	}

}

