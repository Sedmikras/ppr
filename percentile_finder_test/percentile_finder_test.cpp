#include "pch.h"
#include "CppUnitTest.h"
#include "../percentile_finder/number_masker.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace percentilefindertest
{
	TEST_CLASS(percentilefindertest)
	{
	public:
		
		TEST_METHOD(TestMethod1)
		{
			uint32_t index = 0;
			/*int64_t lowest = get_lowest_possible_number(index);*/
			int64_t highest = get_highest_possible_number(index);
			Assert::AreEqual(30, 30);
		}
	};
}
