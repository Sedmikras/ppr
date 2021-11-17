#pragma once
#ifndef NUMBER_FIND_H
#define NUMBER_FIND_H
#include <iostream>

void initialize();
void increment_phase(int32_t index, uint64_t pnumbercount, uint64_t numbers_before);
int64_t get_lowest_possible_number();
int64_t get_highest_possible_number();
int64_t get_lowest_possible_number(int32_t index);
int64_t get_highest_possible_number(int32_t index);
uint32_t vector_size();
uint64_t get_phase_one_count();
uint64_t get_max_count();
uint64_t get_numbers_before();
uint64_t get_numbers_before_phase_one();
uint8_t get_phase();

uint32_t return_index(int64_t number);

#endif