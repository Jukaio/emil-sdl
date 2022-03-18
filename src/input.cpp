
#include "input.h"
#include <SDL/SDL_events.h>
#include <string.h> 

static Uint32 prev_mouse_buttons{ 0 };
static Uint32 curr_mouse_buttons{ 0 };
static SDL_Point mouse_position{ 0, 0 };

static Uint8 prev_state[SDL_NUM_SCANCODES]{ false };
static Uint8 curr_state[SDL_NUM_SCANCODES]{ false };

void input::run()
{
	for (int i = 0; i < SDL_NUM_SCANCODES; i++) {
		prev_state[i] = curr_state[i];
	}
	prev_mouse_buttons = curr_mouse_buttons;
	curr_mouse_buttons = SDL_GetMouseState(&mouse_position.x, &mouse_position.y);
	memcpy(&curr_state, SDL_GetKeyboardState(NULL), SDL_NUM_SCANCODES);
}

bool input::mouse_was_pressed(Uint32 mouse_button_mask)
{
	return curr_mouse_buttons & mouse_button_mask && !(prev_mouse_buttons & mouse_button_mask);
}

bool input::mouse_is_down(Uint32 mouse_button_mask)
{
	return curr_mouse_buttons & mouse_button_mask;
}

bool input::mouse_is_up(Uint32 mouse_button_mask)
{
	return !(curr_mouse_buttons & mouse_button_mask);
}

bool input::mouse_was_released(Uint32 mouse_button_mask)
{
	return !(curr_mouse_buttons & mouse_button_mask) && prev_mouse_buttons & mouse_button_mask;
}

SDL_Point input::get_mouse_position()
{
	return mouse_position;
}

bool input::is_down(SDL_Scancode code)
{
	return curr_state[code];
}

bool input::is_up(SDL_Scancode code)
{
	return !curr_state[code];
}
bool input::was_pressed(SDL_Scancode code)
{
	return curr_state[code] && !prev_state[code];
}
bool input::was_realeased(SDL_Scancode code)
{
	return !curr_state[code] && prev_state[code];
}

