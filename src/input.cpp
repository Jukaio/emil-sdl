
#include "input.h"
#include <SDL/SDL_events.h>
#include <string.h> 

static Uint8 prev_state[SDL_NUM_SCANCODES]{ false };
static Uint8 curr_state[SDL_NUM_SCANCODES]{ false };


void input::run()
{
	for (int i = 0; i < SDL_NUM_SCANCODES; i++) {
		prev_state[i] = curr_state[i];
	}
	memcpy(&curr_state, SDL_GetKeyboardState(NULL), SDL_NUM_SCANCODES);
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

