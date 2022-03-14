#pragma once
#include <functional>
#include "SDL/SDL_events.h"

struct events
{
	using callback = std::function<void(const SDL_Event&)>;

	static void add(const SDL_EventType& type, callback action);
	static void remove(const SDL_EventType& type, callback action);
	static void run();
};