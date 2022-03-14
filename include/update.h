
#pragma once
#include <functional>

// This shit is utterly stupid - But at least I practiced some function pointer fucker-y
struct update
{
	using callback = std::function<void(float)>;

	static void add(const callback& update);
	static void remove(const callback& update);
	static void run(float dt);
};