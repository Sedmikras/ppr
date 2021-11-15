#include "number_masker.h"

std::uint8_t FIRST_ITER_NUMBER_BITS = 13;
std::uint8_t SECOND_ITER_NUMBER_BITS = 23;
std::uint8_t THIRD_ITER_NUMBER_BITS = 28;
int64_t lowest_number = INT64_MIN;
int64_t highest_number = INT64_MAX;

std::uint8_t phase;

void initialize() {
	phase = 0;
}

void increment_phase(int64_t pLowest_number, int64_t pHighest_number)
{
	phase++;
	lowest_number = pLowest_number;
	highest_number = pHighest_number;
}

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

uint32_t return_index(uint64_t number) {
	if (phase == 0) {
		return number >> (64 - FIRST_ITER_NUMBER_BITS);
	}
	else if (phase == 1) {
		uint32_t var = number << FIRST_ITER_NUMBER_BITS;
		return var >> SECOND_ITER_NUMBER_BITS;
	}
	else if (phase == 2) {
		return number << (FIRST_ITER_NUMBER_BITS + SECOND_ITER_NUMBER_BITS);
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
		return (int64_t) index << (64 - FIRST_ITER_NUMBER_BITS);
	}
	else if (phase == 1) {
		return (int64_t) index << (64 - (FIRST_ITER_NUMBER_BITS + SECOND_ITER_NUMBER_BITS));
	}
	else if (phase == 2) {
		return (int64_t) index << (64 - (FIRST_ITER_NUMBER_BITS + SECOND_ITER_NUMBER_BITS + THIRD_ITER_NUMBER_BITS));
	}
	return int64_t();
}

int64_t get_highest_possible_number(int32_t index)
{
	if (phase == 0) {
		return (int64_t)index << (64 - FIRST_ITER_NUMBER_BITS) & (((uint64_t)pow(2, 64 - FIRST_ITER_NUMBER_BITS)) - 1);
	}
	else if
		(phase == 1) {
		int64_t number = (int64_t) index << (64 - (FIRST_ITER_NUMBER_BITS + SECOND_ITER_NUMBER_BITS));
		return number & (((uint64_t)pow(2, 64 - (FIRST_ITER_NUMBER_BITS + SECOND_ITER_NUMBER_BITS)) - 1));
	}
	else if (phase == 2) {
		int64_t number = (int64_t) index << (64 - (FIRST_ITER_NUMBER_BITS + SECOND_ITER_NUMBER_BITS + THIRD_ITER_NUMBER_BITS));
		return number & (((uint64_t)pow(2, 64 - (FIRST_ITER_NUMBER_BITS + SECOND_ITER_NUMBER_BITS + THIRD_ITER_NUMBER_BITS)) - 1));
	}
	return int64_t();
}
