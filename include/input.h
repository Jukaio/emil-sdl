#pragma once

#include <SDL/SDL_scancode.h>
#include <SDL/SDL_rect.h>
struct input
{
	inline static constexpr SDL_Point zero = { 0, 0 };

	static bool mouse_was_pressed(Uint32 mouse_button_mask);
	static bool mouse_is_down(Uint32 mouse_button_mask);
	static bool mouse_is_up(Uint32 mouse_button_mask);
	static bool mouse_was_released(Uint32 mouse_button_mask);

	static void run();
	static SDL_Point get_mouse_position();
	static bool is_down(SDL_Scancode code);
	static bool is_up(SDL_Scancode code);
	static bool was_pressed(SDL_Scancode code);
	static bool was_realeased(SDL_Scancode code);
};