
#include "collision.h"

#include <vector>

#include "SDL/SDL_rect.h"


using callback = std::function<void(const SDL_Rect&)>;
static std::vector<SDL_Rect*> colliders;
static std::vector<callback> callbacks;

void collision::add(SDL_Rect& collider, const callback& on_collision)
{
	colliders.push_back(&collider);
	callbacks.push_back(on_collision);
}


void collision::remove(const SDL_Rect& collider)
{
	for (int i = 0; i < colliders.size(); i++) {
		auto& current = colliders[i];
		if (current == &collider) {
			colliders.erase(colliders.begin() + i);
			callbacks.erase(callbacks.begin() + i);
			break;
		}
	}
}

void collision::run(float dt)
{
	for (int i = 0; i < colliders.size(); i++) {
		const auto& lhs = colliders[i];
		for (int j = i + 1; j < colliders.size(); j++) {
			auto& rhs = colliders[j];
			SDL_Rect result;
			if (SDL_IntersectRect(lhs, rhs, &result)) {
				callbacks[i](*rhs);
				callbacks[j](*lhs);
			}
		}
	}
}
