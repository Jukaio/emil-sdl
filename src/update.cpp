#include "update.h"

#include <vector>
#include <SDL/SDL.h>

static std::vector<update::callback> updates;

void update::add(const update::callback& update)
{
	updates.push_back(update);
}

void update::remove(const update::callback& update)
{
	for (auto it = updates.begin(); it < updates.end(); it++) {
		auto curent = (*it);

		// Compare the function targets - BAD... so bad... Pass in pointers tbh...
		if (curent.target<void*>() == update.target<void*>()) {
			updates.erase(it);
			break;
		}
	}
}



void update::run(float dt)
{
	for (auto& update : updates) {
		update(dt);
	}
}
