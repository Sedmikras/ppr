#include "pch.h"
#include "CppUnitTest.h"
#include "../percentile_finder/number_masker.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace percentilefindertest
{
	TEST_CLASS(percentilefindertest)
	{
	public:
		TEST_METHOD(TestIndexer)
		{
			int expected = 0x100000;
			int32_t index = return_index(INT64_MIN);
			Assert::AreEqual(0, index);
			index = return_index(0x0000000000000000);
			Assert::AreEqual(expected, index);
			index = return_index(0xffffffffffffffff);
			Assert::AreEqual(expected - 1, index);
			index = return_index(0x80000000000);
			Assert::AreEqual(expected + 1, index);
			index = return_index(INT64_MAX);
			Assert::AreEqual((2 * expected - 1), index);
		}

		TEST_METHOD(TestIndexerPhase2)
		{
			increment_phase(0, 0, 0);
			uint64_t number = UINT64_MAX;
			int32_t index = return_index(number);
			//Assert::AreEqual(0, index);
			number = 0xfffffffffffffff0;
			index = return_index(number);
			//Assert::AreEqual(1, index);
			number = 0xffffffffffffff00;
			index = return_index(number);
			//Assert::AreEqual(4096, index);
			number = 0xffffffffffa00000;
			index = return_index(number);
			//Assert::AreEqual(8191, index);
			number = 0xffffffffffd00000;
			index = return_index(number);
			//Assert::AreEqual(8191, index);
			number = 0xffffffffff000000;
			index = return_index(number);
			//Assert::AreEqual(8191, index);
			number = 0xffffffff00000000;
			index = return_index(number);
			number = 0x0000000000000000;
			index = return_index(number);
			//Assert::AreEqual(8191, index);
		}

		/*TEST_METHOD(TestBorderValues)
		{
			uint32_t index = 0;
			int64_t lowest;
			lowest = get_lowest_possible_number(index);
			int64_t highest;
			highest = get_highest_possible_number(index);
			Assert::AreEqual(INT64_MIN, lowest);
			Assert::AreEqual((INT64_MIN + (int64_t)(pow(2, SECOND_PHASE_BITS_SHIFT + THIRD_PHASE_BITS_SHIFT) - 1)), highest);
			index = 1;
			lowest = get_lowest_possible_number(index);
			highest = get_highest_possible_number(index);
			Assert::AreEqual(INT64_MIN + ((int64_t)1 << (SECOND_PHASE_BITS_SHIFT + THIRD_PHASE_BITS_SHIFT)), lowest);
			Assert::AreEqual((INT64_MIN + (int64_t)((pow(2, SECOND_PHASE_BITS_SHIFT + THIRD_PHASE_BITS_SHIFT + 1)) - 1)), highest);
			index = 4096;
			lowest = get_lowest_possible_number(index);
			highest = get_highest_possible_number(index);
			Assert::AreEqual((int64_t)0, lowest);
			Assert::AreEqual((int64_t)(pow(2, SECOND_PHASE_BITS_SHIFT + THIRD_PHASE_BITS_SHIFT) - 1), highest);
			index = 8191;
			lowest = get_lowest_possible_number(index);
			highest = get_highest_possible_number(index);
			Assert::AreEqual(INT64_MAX - (int64_t)(pow(2, SECOND_PHASE_BITS_SHIFT + THIRD_PHASE_BITS_SHIFT) - 1), lowest);
			Assert::AreEqual(INT64_MAX, highest);
		}

		TEST_METHOD(TestBorderValuesPhase2)
		{
			uint32_t index = 0;
			increment_phase(index, 0, 0);
			int64_t lowest, highest;
			lowest = get_lowest_possible_number(index);
			highest = get_highest_possible_number(index);
			Assert::AreEqual(INT64_MIN, lowest);
			Assert::AreEqual((INT64_MIN + (int64_t)(pow(2, THIRD_PHASE_BITS_SHIFT))), highest);
			index = 1;
			lowest = get_lowest_possible_number(index);
			highest = get_highest_possible_number(index);
			Assert::AreEqual((INT64_MIN + (int64_t)(index * pow(2, THIRD_PHASE_BITS_SHIFT))), lowest);
			Assert::AreEqual((INT64_MIN + (int64_t)((((uint64_t)index + 1) * pow(2, THIRD_PHASE_BITS_SHIFT)))), highest);
			index = 4096;
			lowest = get_lowest_possible_number(index);
			highest = get_highest_possible_number(index);
			Assert::AreEqual((INT64_MIN + (int64_t)(index * pow(2, THIRD_PHASE_BITS_SHIFT))), lowest);
			Assert::AreEqual((INT64_MIN + (int64_t)((((uint64_t)index + 1) * pow(2, THIRD_PHASE_BITS_SHIFT)))), highest);
			index = 8191;
			lowest = get_lowest_possible_number(index);
			highest = get_highest_possible_number(index);
			Assert::AreEqual((INT64_MIN + (int64_t)(index * pow(2, THIRD_PHASE_BITS_SHIFT))), lowest);
			Assert::AreEqual((INT64_MIN + (int64_t)((((uint64_t)index + 1) * pow(2, THIRD_PHASE_BITS_SHIFT)))), highest);
		}

		TEST_METHOD(TestBorderValuesPhase3)
		{
			uint32_t index = 4096;
			increment_phase(index, 0, 0);
			int64_t lowest, highest;

			lowest = get_lowest_possible_number(index);
			highest = get_highest_possible_number(index);
			Assert::AreEqual((int64_t)(index * pow(2, THIRD_PHASE_BITS_SHIFT)), lowest);
			Assert::AreEqual((int64_t)(((int64_t)index + 1) * (pow(2, THIRD_PHASE_BITS_SHIFT))), highest);
			index = 0;
			lowest = get_lowest_possible_number(index);
			highest = get_highest_possible_number(index);
			Assert::AreEqual((int64_t)(index * pow(2, THIRD_PHASE_BITS_SHIFT)), lowest);
			Assert::AreEqual((int64_t)(((int64_t)index + 1) * (pow(2, THIRD_PHASE_BITS_SHIFT))), highest);
			index = 4096;
			lowest = get_lowest_possible_number(index);
			highest = get_highest_possible_number(index);
			Assert::AreEqual((int64_t)(index * pow(2, THIRD_PHASE_BITS_SHIFT)), lowest);
			Assert::AreEqual((int64_t)(((int64_t)index + 1) * (pow(2, THIRD_PHASE_BITS_SHIFT))), highest);
			index = 8191;
			lowest = get_lowest_possible_number(index);
			highest = get_highest_possible_number(index);
			Assert::AreEqual((int64_t)(index * pow(2, THIRD_PHASE_BITS_SHIFT)), lowest);
			Assert::AreEqual((int64_t)(((int64_t)index + 1) * (pow(2, THIRD_PHASE_BITS_SHIFT))), highest);
		}

		TEST_METHOD(TestBorderValuesPhase4)
		{
			uint32_t index = 8191;
			increment_phase(index, 0, 0);
			int64_t lowest, highest;
			index = 8388607;
			lowest = get_lowest_possible_number(index);
			highest = get_highest_possible_number(index);
			int64_t diff = (1 << SECOND_PHASE_BITS_SHIFT) - index;
			Assert::AreEqual((INT64_MAX - (diff * ((int64_t)1 << THIRD_PHASE_BITS_SHIFT))) + 1, lowest);
			//Assert::AreEqual(INT64_MAX + 1, highest);
			index = 0;
			lowest = get_lowest_possible_number(index);
			highest = get_highest_possible_number(index);
			diff = (1 << SECOND_PHASE_BITS_SHIFT) - index;
			int64_t expected_highest = INT64_MAX - ((diff + 1) * ((int64_t)1 << THIRD_PHASE_BITS_SHIFT));
			Assert::AreEqual((INT64_MAX - (diff * ((int64_t)1 << THIRD_PHASE_BITS_SHIFT))) + 1, lowest);
			Assert::AreEqual(9221120237309526016, highest);
			index = 4096;
			lowest = get_lowest_possible_number(index);
			highest = get_highest_possible_number(index);
			diff = (1 << SECOND_PHASE_BITS_SHIFT) - index;
			Assert::AreEqual((INT64_MAX - (diff * ((int64_t)1 << THIRD_PHASE_BITS_SHIFT))) + 1, lowest);
			Assert::AreEqual(9221121336821153792, highest);
			index = 8191;
			diff = (1 << SECOND_PHASE_BITS_SHIFT) - index;
			lowest = get_lowest_possible_number(index);
			highest = get_highest_possible_number(index);
			Assert::AreEqual((INT64_MAX - (diff * ((int64_t)1 << THIRD_PHASE_BITS_SHIFT))) + 1, lowest);
			Assert::AreEqual(9221122436064346112, highest);
		}


		TEST_METHOD(TestBorderValuesPhase5)
		{
			uint32_t index = 1;
			increment_phase(index, 0, 0);
			index = 0;
			int64_t lowest, highest;
			lowest = get_lowest_possible_number(index);
			highest = get_highest_possible_number(index);
			Assert::AreEqual(-9221120237041090560, lowest);
			Assert::AreEqual(-9221120236772655104, highest);
			index = 1;
			lowest = get_lowest_possible_number(index);
			highest = get_highest_possible_number(index);
			Assert::AreEqual(-9221120236772655104, lowest);
			Assert::AreEqual(-9221120236504219648, highest);
		}*/

	};
}

