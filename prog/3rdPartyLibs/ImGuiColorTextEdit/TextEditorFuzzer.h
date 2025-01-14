#pragma once

#define ENABLE_EDITOR_FUZZING 0

#if ENABLE_EDITOR_FUZZING
struct TextEditorFuzzer
{
	unsigned seed;
	unsigned iteration;
	bool enabled;

	TextEditorFuzzer()
	{
		enabled = false;
		seed = 1;
		iteration = 0;
	}

	bool prob(int probability)
	{
		return enabled && (probability == 0 || rand() % probability == 0);
	}

	int getKey()
	{
		return !enabled ? 0 : rand() % (127 - 32) + 32;
	}

	void newIteration()
	{
		iteration++;
	}

private:

	int rand()
	{
		seed = 214013 * seed + 2531011;
		return (seed >> 16) & 0x7FFF;
	}
};
#endif