
#include <stdio.h>
#include <SDL/SDL.h>
#include <iostream>
#include <functional>
#include <vector>

#include "engine.h"
#include "input.h"
#include "events.h"
#include <math.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

using entity = size_t;
// Invalid entity is 0 should cause a lot of cool out of the box behaviour
#define INVALID_ENTITY 0
#define MAXIMUM_ENTITIES 255

// Entity
size_t available_entities_pivot = MAXIMUM_ENTITIES;
entity available_entities[MAXIMUM_ENTITIES];

size_t used_entities_pivot = 0;
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
bool controller_valid[MAXIMUM_ENTITIES]{ false };
controller controllers[MAXIMUM_ENTITIES];

bool speed_valid[MAXIMUM_ENTITIES]{ false };
float speeds[MAXIMUM_ENTITIES];

bool direction_valid[MAXIMUM_ENTITIES]{ false };
SDL_FPoint directions[MAXIMUM_ENTITIES];

bool rect_collider_valid[MAXIMUM_ENTITIES]{ false };
SDL_FRect rect_colliders[MAXIMUM_ENTITIES];

bool debug_colour_valid[MAXIMUM_ENTITIES]{ false };
SDL_Colour debug_colours[MAXIMUM_ENTITIES];

enum sprite_type
{
	SPRITE_TYPE_ENTITY,
	SPRITE_TYPE_TILE
};
bool sprite_type_valid[MAXIMUM_ENTITIES]{ false };
sprite_type sprite_types[MAXIMUM_ENTITIES];

bool sprite_index_valid[MAXIMUM_ENTITIES]{ false };
SDL_Point sprite_indeces[MAXIMUM_ENTITIES];

bool position_valid[MAXIMUM_ENTITIES]{ false };
SDL_FPoint positions[MAXIMUM_ENTITIES];

bool size_valid[MAXIMUM_ENTITIES]{ false };
SDL_FPoint sizes[MAXIMUM_ENTITIES];

bool component_controller_exists(const entity& e)
{
	if (e == INVALID_ENTITY) {
		return false;
	}
	return controller_valid[e];
}
void component_controller_set(const entity& e, const controller& controls)
{
	if (e != INVALID_ENTITY) {
		controller_valid[e] = true;
		controllers[e] = controls;
	}
}
void component_controller_destroy(const entity e)
{
	if (e != INVALID_ENTITY) {
		controller_valid[e] = false;
	}
}

bool component_speed_exists(const entity& e)
{
	if (e == INVALID_ENTITY) {
		return false;
	}
	return speed_valid[e];
}
void component_speed_set(const entity& e, const float& speed)
{
	if (e != INVALID_ENTITY) {
		speed_valid[e] = true;
		speeds[e] = speed;
	}
}
void component_speed_destroy(const entity e)
{
	if (e != INVALID_ENTITY) {
		speed_valid[e] = false;
	}
}

bool component_direction_exists(const entity& e)
{
	if (e == INVALID_ENTITY) {
		return false;
	}
	return direction_valid[e];
}
void component_direction_set(const entity& e, const SDL_FPoint& direction)
{
	if (e != INVALID_ENTITY) {
		direction_valid[e] = true;
		directions[e] = direction;
	}
}
void component_direction_destroy(const entity e)
{
	if (e != INVALID_ENTITY) {
		direction_valid[e] = false;
	}
}

bool component_rect_collider_exists(const entity& e)
{
	if (e == INVALID_ENTITY) {
		return false;
	}
	return rect_collider_valid[e];
}
void component_rect_collider_set(const entity& e, const SDL_FRect& collider)
{
	if (e != INVALID_ENTITY) {
		rect_collider_valid[e] = true;
		rect_colliders[e] = collider;
	}
}
void component_rect_collider_destroy(const entity e)
{
	if (e != INVALID_ENTITY) {
		rect_collider_valid[e] = false;
	}
}

bool component_colour_exists(const entity& e)
{
	if (e == INVALID_ENTITY) {
		return false;
	}
	return debug_colour_valid[e];
}
void component_colour_set(const entity& e, const SDL_Colour& colour)
{
	if (e != INVALID_ENTITY) {
		debug_colour_valid[e] = true;
		debug_colours[e] = colour;
	}
}
void component_colour_destroy(const entity e)
{
	if (e != INVALID_ENTITY) {
		debug_colour_valid[e] = false;
	}
}

bool component_sprite_type_exists(const entity& e)
{
	if (e == INVALID_ENTITY) {
		return false;
	}
	return sprite_type_valid[e];
}
void component_sprite_type_set(const entity& e, const sprite_type& type)
{
	if (e != INVALID_ENTITY) {
		sprite_type_valid[e] = true;
		sprite_types[e] = type;
	}
}
void component_sprite_type_destroy(const entity e)
{
	if (e != INVALID_ENTITY) {
		sprite_type_valid[e] = false;
	}
}

bool component_sprite_index_exists(const entity& e)
{
	if (e == INVALID_ENTITY) {
		return false;
	}
	return sprite_index_valid[e];
}
void component_sprite_index_set(const entity& e, const SDL_Point& index)
{
	if (e != INVALID_ENTITY) {
		sprite_index_valid[e] = true;
		sprite_indeces[e] = index;
	}
}
void component_sprite_index_destroy(const entity e)
{
	if (e != INVALID_ENTITY) {
		sprite_index_valid[e] = false;
	}
}

bool component_position_exists(const entity& e)
{
	if (e == INVALID_ENTITY) {
		return false;
	}
	return position_valid[e];
}
void component_position_set(const entity& e, const SDL_FPoint& position)
{
	if (e != INVALID_ENTITY) {
		position_valid[e] = true;
		positions[e] = position;
	}
}
void component_position_destroy(const entity& e)
{
	if (e != INVALID_ENTITY) {
		position_valid[e] = false;
	}
}

bool component_size_exists(const entity& e) 
{
	if (e == INVALID_ENTITY) {
		return false;
	}
	return size_valid[e];
}
void component_size_set(const entity& e, const SDL_FPoint& size)
{
	if (e != INVALID_ENTITY) {
		size_valid[e] = true;
		sizes[e] = size;
	}
}
void component_size_destroy(const entity& e)
{
	if (e != INVALID_ENTITY) {
		size_valid[e] = false;
	}
}

// Systems - Optimisation idea, instead of iterating through EVERYTHING, subsribe them : ) 
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
		SDL_FPoint p = positions[e];
		SDL_FPoint s = sizes[e];
		SDL_FRect destination{ p.x, p.y, s.x, s.y};
		switch (sprite_types[e]) {
			case SPRITE_TYPE_ENTITY:
				engine::draw_entity(sprite_indeces[e], destination);
				break;

			case SPRITE_TYPE_TILE:
				engine::draw_tile(sprite_indeces[e], destination);
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

		if (input::is_down(controllers[e].left)) {
			move -= 1;
		}
		if (input::is_down(controllers[e].right)) {
			move += 1;
		}
		positions[e].x += (move * speeds[e]);
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
		&& component_direction_exists(e);
}

void ball_system_each(const entity& e)
{
	if (entity_can_use_ball_system(e)) {
		SDL_FPoint dir{ directions[e] };
		float speed{ speeds[e] };

		float length = dir.x * dir.x + dir.y * dir.y;
		if (length > 0.0f) {
			dir.x / length;
			dir.y / length;
		}

		directions[e] = dir;

		dir.x *= speed;
		dir.y *= speed;

		positions[e].x += dir.x;
		if (positions[e].y + dir.y <= 0.0f || positions[e].y + dir.y >= SCREEN_HEIGHT) {
			directions[e].y = -directions[e].y;
		}
		positions[e].y += dir.y;
	}
}

void ball_system_run()
{
	for (int i = 0; i < used_entities_pivot; i++) {
		ball_system_each(used_entities[i]);
	}
}

void debug_rect_collider_system_each(const entity& e)
{
	if (component_rect_collider_exists(e), component_colour_exists(e)) {
		engine::draw_rect(rect_colliders[e], debug_colours[e]);
	}
}

void debug_rect_collider_system_run()
{
	for (int i = 0; i < used_entities_pivot; i++) {
		debug_rect_collider_system_each(used_entities[i]);
	}
}

void run(float dt)
{
	player_system_run();
	ball_system_run();


	engine::render_clear();
	draw_system_run();
	debug_rect_collider_system_run();
	engine::render_present();
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

			component_colour_set(block, { 255, 0, 0, 255 });
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
	component_colour_set(player, { 255, 0, 0, 255 });

	// Construct ball
	entity ball{ entity_create() };
	component_position_set(ball, { 400 - 32, 500 });
	component_size_set(ball, { 64, 64 });
	component_sprite_index_set(ball, { 1, 1 });
	component_sprite_type_set(ball, SPRITE_TYPE_ENTITY);
	component_speed_set(ball, 8.0f);
	component_direction_set(ball, { 0.0f, -1.0f });


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
