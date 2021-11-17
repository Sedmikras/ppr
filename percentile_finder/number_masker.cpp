#include "number_masker.h"

std::uint64_t FIRST_ITER_NUMBER_BITS = 21;
std::uint64_t SECOND_ITER_NUMBER_BITS = 21;
std::uint64_t THIRD_ITER_NUMBER_BITS = 22;
int64_t lowest_number = INT64_MIN;
int64_t highest_number = INT64_MAX;
const auto MSB_MASK = 0x8000000000000000;
const auto FIRST_MASK = 0x7FFFFFFFFFFFFFFF;
uint64_t mask = 0x7FFF800000000000;
uint64_t second_mask = 0x00007FFFFC000000;
uint64_t masker_complement = 0x1FFFFF;
uint64_t second_masker_complement = 0x1FFFFF;
std::uint8_t phase;
uint64_t all_numbers = 0;
uint64_t numbers_before = 0;
uint64_t phase_one_number_count = 0;
uint64_t numbers_before_phase_one = 0;
uint32_t phase_zero_index = 0;
uint32_t phase_one_index = 0;

void initialize() {
	phase = 0;
}

void increment_phase(int32_t index, uint64_t pnumbercount, uint64_t numbers_before)
{
	lowest_number = get_lowest_possible_number(index);
	highest_number = get_highest_possible_number(index);
	if (phase == 0) {
		all_numbers = pnumbercount;
		numbers_before = numbers_before;
	}
	else if (phase == 1) {
		phase_one_number_count = pnumbercount;
		numbers_before_phase_one = numbers_before;
	}
	phase++;

}

/*void increment_phase(int64_t pLowest_number, int64_t pHighest_number)
{
	phase++;
	lowest_number = pLowest_number;
	highest_number = pHighest_number;
}*/

uint32_t vector_size() {
	if (phase == 0) {
		return ((uint32_t)pow(2,FIRST_ITER_NUMBER_BITS));
	}
	else if (phase == 1) {
		return ((uint32_t)pow(2, SECOND_ITER_NUMBER_BITS));
	}
	else if (phase == 2) {
		return ((uint32_t)pow(2, THIRD_ITER_NUMBER_BITS));
	}
	else return NULL;
}

uint32_t return_index(int64_t number) {
	if (phase == 0) {
		auto msb = number >> 63;
		uint32_t var = (uint32_t)((number & FIRST_MASK) >> (SECOND_ITER_NUMBER_BITS + THIRD_ITER_NUMBER_BITS));
		auto complement = (var & 0xFFFFF);
		if (msb) {
			return complement;
		}
		else {
			return 0x100000 + complement;
		}
		/*uint64_t aa = number & maskaaa;
		aa = aa >> 51;
		int64_t complement = (number & 0x8000000000000000) >> 63;
		uint32_t complement32 = (complement == 1) ? 0 : (1 << (FIRST_ITER_NUMBER_BITS - 1)) - 1;
		uint64_t masked = (number & mask);
		uint32_t var = masked >> (SECOND_ITER_NUMBER_BITS + THIRD_ITER_NUMBER_BITS);
		var = (var& masker_complement) + complement32;
		return var;*/
	}
	else if (phase == 1) {
		uint32_t var = (uint32_t)((number & second_mask) >> (THIRD_ITER_NUMBER_BITS));
		auto complement = (var & 0xFFFFF);
		return complement;
	}
	else if (phase == 2) {
		return (uint32_t)(number << (FIRST_ITER_NUMBER_BITS + SECOND_ITER_NUMBER_BITS));
	}
	else return NULL;
}

int64_t get_lowest_possible_number() {
	return lowest_number;
}

int64_t get_highest_possible_number() {
	return highest_number;
}

int64_t get_lowest_possible_number(int32_t index)
{
	if (phase == 0) {
		if (index < (1 << (FIRST_ITER_NUMBER_BITS - 1))) {
			return INT64_MIN + ((int64_t) index << (SECOND_ITER_NUMBER_BITS + THIRD_ITER_NUMBER_BITS));
		}
		else {
			index = index - ((int32_t) 1 << (FIRST_ITER_NUMBER_BITS - 1));
			return (int64_t) index << (SECOND_ITER_NUMBER_BITS + THIRD_ITER_NUMBER_BITS);
		}
	}
	else if (phase == 1) {
		int64_t number = (int64_t)index << (THIRD_ITER_NUMBER_BITS);
		return lowest_number + number;
	}
	else if (phase == 2) {
		return lowest_number;
	}
	else {
		return NULL;
	}
}

int64_t get_highest_possible_number(int32_t index)
{
	if (phase == 0) {
		if (index < (1 << (FIRST_ITER_NUMBER_BITS - 1))) {
			int64_t number = ((int64_t)index << (SECOND_ITER_NUMBER_BITS + THIRD_ITER_NUMBER_BITS));
			int64_t complement = ((int64_t)1 << (SECOND_ITER_NUMBER_BITS + THIRD_ITER_NUMBER_BITS)) -1;
			return INT64_MIN  + number + complement;
		}
		else {
			int64_t number = ((int64_t)index << (SECOND_ITER_NUMBER_BITS + THIRD_ITER_NUMBER_BITS));
			int64_t complement = ((int64_t)1 << (SECOND_ITER_NUMBER_BITS + THIRD_ITER_NUMBER_BITS)) - 1;
			return INT64_MIN + number + complement;
		}
	}
	else if (phase == 1) {
		int64_t number = ((int64_t)index << (THIRD_ITER_NUMBER_BITS));
		int64_t complement = ((int64_t)1 << (THIRD_ITER_NUMBER_BITS));
		return lowest_number + number + complement;
	}
	else if (phase == 2) {
		return highest_number;
	}
	else {
		return NULL;
	}
}

uint64_t get_phase_one_count() {
	return phase_one_number_count;
}

uint64_t get_max_count() {
	return all_numbers;
}

uint64_t get_numbers_before() {
	if (phase == 0) {
		return numbers_before;
	}
	else if (phase == 1) {
		return numbers_before + numbers_before_phase_one;
	}
	else {
		return NULL;
	}
}

uint64_t get_numbers_before_phase_one() {
	return numbers_before_phase_one;
} 

uint8_t get_phase() {
	return phase;
}
