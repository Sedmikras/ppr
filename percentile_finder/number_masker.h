#pragma once
#ifndef NUMBER_FIND_H
#define NUMBER_FIND_H
#include <iostream>

void initialize();
void increment_phase(int64_t lowest_number, int64_t highest_number);
int64_t get_lowest_possible_number();
int64_t get_highest_possible_number();
int64_t get_lowest_possible_number(int32_t index);
int64_t get_highest_possible_number(int32_t index);
uint32_t vector_size();

uint32_t return_index(uint64_t number);

#endif