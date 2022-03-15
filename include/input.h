#pragma once

#include <SDL/SDL_scancode.h>
struct input
{
	static void run();
	static bool is_down(SDL_Scancode code);
	static bool is_up(SDL_Scancode code);
	static bool was_pressed(SDL_Scancode code);
	static bool was_realeased(SDL_Scancode code);
};