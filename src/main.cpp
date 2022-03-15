
#include <stdio.h>
#include <SDL/SDL.h>
#include <iostream>
#include <functional>
#include <vector>
#include<string>

#include "engine.h"
#include "input.h"
#include "events.h"
#include <math.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600


using entity = size_t;
// Invalid entity is 0 should cause a lot of cool out of the box behaviour
#define INVALID_ENTITY 0
#define MAXIMUM_ENTITIES 256

// Entity
size_t available_entities_pivot{ MAXIMUM_ENTITIES };
entity available_entities[MAXIMUM_ENTITIES];

size_t used_entities_pivot{ 0 };
entity used_entities[MAXIMUM_ENTITIES];

void initialise_entities()
{
	for (int i = 0; i < MAXIMUM_ENTITIES; i++) {
		available_entities[i] = i;
		used_entities[i] = INVALID_ENTITY;
	}
}

entity entity_create()
{
	if (available_entities_pivot == 0) {
		return INVALID_ENTITY;
	}
	available_entities_pivot -= 1;
	entity e = available_entities[available_entities_pivot];
	available_entities[available_entities_pivot] = INVALID_ENTITY;
	used_entities[used_entities_pivot] = e;
	used_entities_pivot += 1;

	return e;
}

void invalidate_components(const entity& e)
{
	// ...
}

void entity_destroy(entity& e)
{
	if (used_entities[used_entities_pivot - 1] == e) { // just track one back if we remove last one
		used_entities_pivot -= 1;
		used_entities[used_entities_pivot] = INVALID_ENTITY;
	}
	else {
		// otherwise... find...
		for (size_t i = 0; i < used_entities_pivot; i++) {
			entity target = used_entities[i];
			if (target == e) { // found 
				// shift over 
				for (int j = i; j < used_entities_pivot - 1; j++) {
					used_entities[j] = used_entities[j + 1];
				}
				used_entities[used_entities_pivot - 1] = INVALID_ENTITY;

				// How to invalidate all the commponents? :( Entity signature? Maybe? No. FUCK!
				invalidate_components(e); // meh, let's just try this
				break;
			}
		}
	}
	e = INVALID_ENTITY;
}

// Components
struct controller
{
	SDL_Scancode left;
	SDL_Scancode right;
};


enum sprite_type
{
	SPRITE_TYPE_ENTITY,
	SPRITE_TYPE_TILE
};

// 1. First time function is used -> Register Component 
// 2. Registration means: Each component gets associated to a number
// 3. ... ? 
// 4. Profit.

#define COMPONENT(type, name) \
bool name##_initialised{false}; \
bool name##_valid[MAXIMUM_ENTITIES]{ false }; \
type name##_buffer[MAXIMUM_ENTITIES]; \
bool component_##name##_exists(const entity& e) \
{ \
	if (e == INVALID_ENTITY || e >= MAXIMUM_ENTITIES) { \
		return false; \
	} \
	return name##_valid[e]; \
} \
void component_##name##_set(const entity& e, const type& v) \
{ \
	if (e != INVALID_ENTITY || e >= MAXIMUM_ENTITIES) { \
		name##_valid[e] = true; \
		name##_buffer[e] = v; \
	} \
	else { \
		printf("Error in %s of type %s", #name, #type); \
	} \
} \
type& component_##name##_get(const entity& e) \
{ \
	return name##_buffer[e]; \
} \
void component_##name##_destroy(const entity& e) \
{ \
	if (e != INVALID_ENTITY) { \
		name##_valid[e] = false; \
	} \
} \

COMPONENT(controller,	controller)
COMPONENT(float,		speed)
COMPONENT(SDL_FPoint,	direction)
COMPONENT(SDL_FPoint,	collider_offset)
COMPONENT(SDL_FCircle,	circle_collider)
COMPONENT(SDL_FRect,	rect_collider)
COMPONENT(sprite_type,	sprite_type)
COMPONENT(SDL_Point,	sprite_index)
COMPONENT(SDL_FPoint,	position)
COMPONENT(SDL_FPoint,	size)


// Systems - Optimisation idea, instead of iterating through EVERYTHING, subsribe them : ) 
// TODO: Make filter so systems only care for THEIR entities. IDK :D 
bool entity_can_use_draw_system(const entity& e)
{
	return component_sprite_type_exists(e)
		&& component_sprite_index_exists(e)
		&& component_position_exists(e)
		&& component_size_exists(e);
}

void draw_system_each(const entity& e)
{
	if (entity_can_use_draw_system(e)) {
		SDL_FPoint p = component_position_get(e);
		SDL_FPoint s = component_size_get(e);
		SDL_FRect destination{ p.x, p.y, s.x, s.y};
		switch (component_sprite_type_get(e)) {
			case SPRITE_TYPE_ENTITY:
				engine::draw_entity(component_sprite_index_get(e), destination);
				break;

			case SPRITE_TYPE_TILE:
				engine::draw_tile(component_sprite_index_get(e), destination);
				break;
		}
	}
}

void draw_system_run()
{
	for (int i = 0; i < used_entities_pivot; i++) {
		draw_system_each(used_entities[i]);
	}
}

bool entity_can_use_player_system(const entity& e)
{
	return component_controller_exists(e) && component_position_exists(e) && component_speed_exists(e);
}


void player_system_each(const entity& e)
{
	if (entity_can_use_player_system(e)) {
		int move{ 0 };
		controller c{ component_controller_get(e) };

		if (input::is_down(c.left)) {
			move -= 1;
		}
		if (input::is_down(c.right)) {
			move += 1;
		}

		component_position_get(e).x += (move * component_speed_get(e));
	}
}

void player_system_run()
{
	for (int i = 0; i < used_entities_pivot; i++) {
		player_system_each(used_entities[i]);
	}
}

bool entity_can_use_ball_system(const entity& e)
{
	return !component_controller_exists(e)
		&& component_position_exists(e)
		&& component_speed_exists(e)
		&& component_direction_exists(e)
		&& component_circle_collider_exists(e);
}

void ball_system_each(const entity& e)
{
	if (entity_can_use_ball_system(e)) {
		SDL_FPoint& direction{ component_direction_get(e) };
		SDL_FPoint dir{ direction };
		// Normalise and apply
		float speed{ component_speed_get(e) };
		float length = dir.x * dir.x + dir.y * dir.y;
		if (length > 0.0f) {
			dir.x / length;
			dir.y / length;
		}
		direction = dir;

		// Reuse dir velocity
		SDL_FPoint velocity{ dir.x * speed, dir.y * speed };

		// Apply velocity - Reflect from screen edges
		SDL_FPoint& position{ component_position_get(e) };
		SDL_FCircle circle{ component_circle_collider_get(e) };
		if ((circle.x - circle.radius) + dir.x <= 0.0f || (circle.x + circle.radius) + dir.x >= SCREEN_WIDTH) {
			direction.x = -direction.x;
			velocity.x = -velocity.x;
		}
		if ((circle.y - circle.radius) + dir.y <= 0.0f || (circle.y + circle.radius) + dir.y >= SCREEN_HEIGHT) {
			direction.y = -direction.y;
			velocity.y = -velocity.y;
		}

		position.x += velocity.x;
		position.y += velocity.y;
	}
}

void ball_system_run()
{
	for (int i = 0; i < used_entities_pivot; i++) {
		ball_system_each(used_entities[i]);
	}
}

void collider_update_position_system_each(const entity& e)
{
	if (!component_position_exists(e)) {
		return;
	}

	SDL_FPoint position = component_position_get(e);
	if (component_rect_collider_exists(e)) {
		SDL_FRect& rect = component_rect_collider_get(e);
		if (component_collider_offset_exists(e)) {
			SDL_FPoint offset = component_collider_offset_get(e);
			position.x += offset.x;
			position.y += offset.y;
		}
		rect.x = position.x;
		rect.y = position.y;
	}
	if (component_circle_collider_exists(e)) {
		SDL_FCircle& circle = component_circle_collider_get(e);
		if (component_collider_offset_exists(e)) {
			SDL_FPoint offset = component_collider_offset_get(e);
			position.x += offset.x;
			position.y += offset.y;
		}
		circle.x = position.x;
		circle.y = position.y;
	}
}

void collider_update_position_system_run()
{
	for (int i = 0; i < used_entities_pivot; i++) {
		collider_update_position_system_each(used_entities[i]);
	}
}

void debug_rect_collider_system_each(const entity& e)
{
	if (component_rect_collider_exists(e)) {
		SDL_FRect rect = component_rect_collider_get(e);
		engine::draw_rect(rect, {255, 0, 0, 255});
	}
}

void debug_rect_collider_system_run()
{
	for (int i = 0; i < used_entities_pivot; i++) {
		debug_rect_collider_system_each(used_entities[i]);
	}
}

void debug_circle_collider_system_each(const entity& e)
{
	if (component_circle_collider_exists(e)) {
		SDL_FCircle circle = component_circle_collider_get(e);
		engine::draw_circle(circle, { 255, 255, 0, 255 });
	}
}

void debug_circle_collider_system_run()
{
	for (int i = 0; i < used_entities_pivot; i++) {
		debug_circle_collider_system_each(used_entities[i]);
	}
}

void run(float dt)
{
	player_system_run();
	ball_system_run();
	collider_update_position_system_run();
	

	engine::render_clear();
	draw_system_run();
	debug_rect_collider_system_run();
	debug_circle_collider_system_run();
	engine::render_present();
}

int& get_instance()
{
	static int instance{};
	return instance;
}

int main()
{
	initialise_entities();
	engine::initialise(SCREEN_WIDTH, SCREEN_HEIGHT);
	engine::load_entities_texture("res/objects.png");
	engine::set_entity_source_size(32, 32);

	engine::load_tiles_texture("res/rock_packed.png");
	engine::set_tile_source_size(18, 18);

	// Construct level
	entity blocks[10 * 5];
	SDL_FPoint offset{80, 20};
	for (int x = 0; x < 10; x++) {
		for (int y = 0; y < 5; y++) {
			entity block = entity_create();
			SDL_FPoint position{ offset.x + (x * 64), offset.y + (y * 64) };
			SDL_FRect collider{ position.x, position.y, 64, 64 };
			component_position_set(block, position);
			component_size_set(block, { 64, 64 });
			component_sprite_index_set(block, { 1, 1 });
			component_sprite_type_set(block, SPRITE_TYPE_TILE);
			component_rect_collider_set(block, collider);
			blocks[(y * 10) + x] = block;
		}
	}

	// Construct player
	entity player{ entity_create() };
	component_position_set(player, { 400 - 32, 500 });
	component_size_set(player, { 64, 64 });
	component_sprite_index_set(player, { 1, 1 });
	component_sprite_type_set(player, SPRITE_TYPE_ENTITY);
	component_speed_set(player, 10.0f);
	component_controller_set(player, { SDL_SCANCODE_A, SDL_SCANCODE_D });
	component_collider_offset_set(player, { 32, 32 });
	component_circle_collider_set(player, { 400 - 32, 500, 32.0f });

	// Construct ball
	entity ball{ entity_create() };
	component_position_set(ball, { 400 - 32, 500 });
	component_size_set(ball, { 64, 64 });
	component_sprite_index_set(ball, { 1, 1 });
	component_sprite_type_set(ball, SPRITE_TYPE_ENTITY);
	component_speed_set(ball, 2.0f);
	component_direction_set(ball, { 1.0f, -1.0f });
	component_collider_offset_set(ball, { 32, 32 });
	component_circle_collider_set(ball, { 400 - 32, 500, 32.0f });

	bool running = true;

	events::add(SDL_QUIT, [&running](const SDL_Event&) { running = false; });
	events::add(SDL_KEYDOWN, 
	[&running](const SDL_Event& e)
	{
		if (e.key.keysym.scancode == SDL_SCANCODE_ESCAPE) running = false;
	});

	const float SECONDS = 1 / 1000.0f;
	Uint64 prev_ticks = SDL_GetPerformanceCounter();
	while (running) // Each frame
	{
		Uint64 ticks = SDL_GetPerformanceCounter();
		Uint64 delta_ticks = ticks - prev_ticks;
		prev_ticks = ticks;
		float dt = ((float)delta_ticks / SDL_GetPerformanceFrequency()) * SECONDS;

	
		events::run();
		input::run();

		run(dt);

		SDL_Delay(16);
		// Update
		// Render
	}
}
