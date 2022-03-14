#pragma once
#include <functional>

struct SDL_Rect;

struct collision
{
	using callback = std::function<void(const SDL_Rect&)>;

	static void add(SDL_Rect& collider, const callback& on_collision);
	static void remove(const SDL_Rect& collider);

	static void run(float dt);
};
