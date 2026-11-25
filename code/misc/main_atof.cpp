#include "../ilu_core.h"
#include <stdlib.h> // atof

#define TEST_FUNCTIONALITY 1
#define TEST_PERFORMANCE 1

f32 myatof(const char *str)
{
	f32 value = StrToFloat(str);
	return value;
}

int main()
{
	const char *numbers [] = {
		"0", "0.0f", "10.54", "10.54f", ".43", ".43f", "5.f",
		"-0", "-0.0f", "-10.54", "-10.54f", "-.43", "-.43f", "-5.f"
	};

#if TEST_FUNCTIONALITY
	LOG(Info, "Functional test:\n");

	for ( u32 i = 0; i < ARRAY_COUNT(numbers); ++i )
	{
		const char *numberStr = numbers[i];
		LOG(Info, "- atof: %f\n", atof(numberStr));
		LOG(Info, "- myatof: %f\n", myatof(numberStr));
	}
#endif

#if TEST_PERFORMANCE
	LOG(Info, "Performance test:\n");

	f32 sum = 0.0f;

	Clock c0 = GetClock();
	for ( u32 j = 0; j < 999999; ++j )
	{
		for ( u32 i = 0; i < ARRAY_COUNT(numbers); ++i )
		{
			const char *numberStr = numbers[i];
			sum += atof(numberStr);
		}
	}

	Clock c1 = GetClock();
	for ( u32 j = 0; j < 999999; ++j )
	{
		for ( u32 i = 0; i < ARRAY_COUNT(numbers); ++i )
		{
			const char *numberStr = numbers[i];
			sum += myatof(numberStr);
		}
	}

	Clock c2 = GetClock();

	LOG(Info, "- atof: %f seconds\n", GetSecondsElapsed(c0, c1));
	LOG(Info, "- myatof: %f seconds\n", GetSecondsElapsed(c1, c2));
	LOG(Info, "- checksum: %f\n", sum); // NOTE: Using 'sum' prevents optimizing the loops away
#endif

	return 0;
}

