
#include "events.h"
#include <unordered_map>

using callback_lookup = std::unordered_map<SDL_EventType, std::vector<events::callback>>;
static callback_lookup lookup;


void events::add(const SDL_EventType& type, events::callback action)
{
	lookup[type].push_back(action);
}


void events::remove(const SDL_EventType& type, events::callback action)
{
	auto it = lookup.find(type);
	if (it == lookup.end()) {

		return;
	}
	auto& callbacks = (*it).second;
	for (int i = 0; i < callbacks.size(); i++) {
		bool same_type = callbacks[i].target_type() == action.target_type();
		if (same_type && callbacks[i].target<void*>() == action.target<void*>()) {
			callbacks.erase(callbacks.begin() + i);
			i--;
		}
	}
}

void events::run()
{
	SDL_Event event;
	while (SDL_PollEvent(&event)) // Get first in queue
	{
		auto it = lookup.find((SDL_EventType)event.type);
		if (it != lookup.end()) {
			for (auto& callback : (*it).second) {
				callback(event);
			}
		}
	}
}
