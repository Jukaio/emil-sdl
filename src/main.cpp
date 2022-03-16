
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

// template draft idea
//template<typename T>
//struct component_buffers {
//	bool valid[MAXIMUM_ENTITIES]{ false };
//	T buffer[MAXIMUM_ENTITIES];
//};
//
//template<auto& Global_Buffer>
//struct component_logic {
//	using Buffer_T = std::decay_t<decltype(Global_Buffer.buffer)>;
//
//	static bool exists(const entity& e) {
//		if (e == INVALID_ENTITY || e >= MAXIMUM_ENTITIES) {
//			return false;
//		}
//		return Global_Buffer.valid[e];
//	}
//
//	static void set(const entity& e, const Buffer_T& v) {
//		if (e != INVALID_ENTITY || e >= MAXIMUM_ENTITIES) {
//			Global_Buffer.valid[e] = true;
//			Global_Buffer.buffer[e] = v;
//		}
//		else {
//			printf("Error, sadly can't show you the type name nor the component name :(");
//		}
//	}
//
//	static Buffer_T get(const entity& e) {
//		return Global_Buffer.buffer[e];
//	}
//
//	static void destroy(const entity& e) {
//		if (e != INVALID_ENTITY) {
//			Global_Buffer.buffer[e] = false;
//		}
//	}
//};
//
//// define component
//auto controller_components = component_buffers<controller>{};
//// access
//component_logic<controller_components>::get(entity); // etc

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
		available_entities[available_entities_pivot] = e;
		available_entities_pivot += 1;
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
				used_entities_pivot -= 1;
				used_entities[used_entities_pivot] = INVALID_ENTITY;
				available_entities[available_entities_pivot] = e;
				available_entities_pivot += 1;
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
bool name##_valid[MAXIMUM_ENTITIES]{ false }; \
type name##_buffer[MAXIMUM_ENTITIES]; \
bool name##_exists(const entity e) \
{ \
	if (e == INVALID_ENTITY || e >= MAXIMUM_ENTITIES) { \
		return false; \
	} \
	return name##_valid[e]; \
} \
void name##_set(const entity e, const type& v) \
{ \
	if (e != INVALID_ENTITY || e >= MAXIMUM_ENTITIES) { \
		name##_valid[e] = true; \
		name##_buffer[e] = v; \
	} \
	else { \
		printf("Error in %s of type %s", #name, #type); \
	} \
} \
type& name##_get(const entity e) \
{ \
	return name##_buffer[e]; \
} \
void name##_destroy(const entity e) \
{ \
	if (e != INVALID_ENTITY) { \
		name##_valid[e] = false; \
	} \
} \

namespace components
{
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
	COMPONENT(SDL_Colour,	debug_color)	
}

// Systems - Optimisation idea, instead of iterating through EVERYTHING, subsribe them : ) 
// TODO: Make filter so systems only care for THEIR entities. IDK :D 
bool entity_can_use_draw_system(const entity& e)
{
	using namespace components;
	return sprite_type_exists(e)
		&& sprite_index_exists(e)
		&& position_exists(e)
		&& size_exists(e);
}

void draw_system_each(const entity& e)
{
	if (entity_can_use_draw_system(e)) {
		SDL_FPoint p = components::position_get(e);
		SDL_FPoint s = components::size_get(e);
		SDL_FRect destination{ p.x, p.y, s.x, s.y};
		switch (components::sprite_type_get(e)) {
			case SPRITE_TYPE_ENTITY:
				engine::draw_entity(components::sprite_index_get(e), destination);
				break;

			case SPRITE_TYPE_TILE:
				engine::draw_tile(components::sprite_index_get(e), destination);
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
	return components::controller_exists(e) && components::position_exists(e) && components::speed_exists(e);
}


void player_system_each(const entity& e)
{
	if (entity_can_use_player_system(e)) {
		int move{ 0 };
		controller c{ components::controller_get(e) };

		if (input::is_down(c.left)) {
			move -= 1;
		}
		if (input::is_down(c.right)) {
			move += 1;
		}

		components::position_get(e).x += (move * components::speed_get(e));
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
	return !components::controller_exists(e)
		&& components::position_exists(e)
		&& components::speed_exists(e)
		&& components::direction_exists(e)
		&& components::circle_collider_exists(e);
}

void SDL_FPointNormalise(SDL_FPoint* v)
{
	float length = sqrt(v->x * v->x + v->y * v->y);
	if (length > 0.0f) {
		v->x /= length;
		v->y /= length;
	}
}

SDL_FPoint SDL_FPointGetNormalised(SDL_FPoint v)
{
	SDL_FPointNormalise(&v);
	return v;
}

void ball_system_each(const entity& e)
{
	if (entity_can_use_ball_system(e)) {
		SDL_FPoint& direction{ components::direction_get(e) };
		SDL_FPoint dir{ direction };
		// Normalise and apply
		float speed{ components::speed_get(e) };

		SDL_FPointNormalise(&dir);
		direction = dir;

		// Reuse dir velocity
		SDL_FPoint velocity{ dir.x * speed, dir.y * speed };

		// Apply velocity - Reflect from screen edges
		SDL_FPoint& position{ components::position_get(e) };
		SDL_FCircle circle{ components::circle_collider_get(e) };
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

float SDL_FPointDot(SDL_FPoint lhs, SDL_FPoint rhs)
{ 
	return lhs.x * rhs.x + lhs.y * rhs.y; 
}

SDL_FPoint SDL_FPointReflect(SDL_FPoint direction, SDL_FPoint normal)
{
	float factor = -2.0f * SDL_FPointDot(normal, direction);
	return { factor * normal.x + direction.x, factor * normal.y + direction.y };
}


bool SDL_IntersectFCircleFRect(const SDL_FCircle& circle, const SDL_FRect& rect, SDL_FPoint& out_normal)
{
	float x = circle.x;
	float y = circle.y;
	out_normal = { 0, 0 };
	if (circle.x < rect.x)
	{
		x = rect.x;
		out_normal.x = -1;
	}
	else if (circle.x > rect.x + rect.w)
	{
		x = rect.x + rect.w;
		out_normal.x = 1;
	}

	if (circle.y < rect.y)
	{
		y = rect.y;
		out_normal.y = -1;
	}
	else if (circle.y > rect.y + rect.h)
	{
		y = rect.y + rect.h;
		out_normal.y = 1;
	}

	SDL_FPointNormalise(&out_normal);

	float distX = circle.x - x;
	float distY = circle.y - y;
	float distance = sqrt((distX * distX) + (distY * distY));

	if (distance <= circle.radius)
	{
		return true;
	}
	return false;
}


void ball_collision_system_each(const entity& e)
{
	// check if E really is a ball fulfilling entity
	if (!entity_can_use_ball_system(e)) {
		return;
	}
	using namespace components;
	for (int i = 0; i < used_entities_pivot; i++) {
		if (i != e) {
			entity other = used_entities[i];
			if (rect_collider_exists(other)) {
				SDL_FCircle ball_collider = circle_collider_get(e);
				SDL_FRect other_collider = rect_collider_get(other);
				SDL_FPoint normal;
				if (SDL_IntersectFCircleFRect(ball_collider, other_collider, normal)) { // This is rubbish...
					printf("Normal: %f - %f\n", normal.x, normal.y);
					SDL_FPoint center =
					{
						other_collider.x + (other_collider.w * 0.5f),
						other_collider.y + (other_collider.h * 0.5f)
					};
					SDL_FPoint& direction = direction_get(e);
					direction = SDL_FPointReflect(direction, normal);
					entity_destroy(other);
					break;
				}
			}
		}
	}
}

void ball_collision_system_run()
{
	for (int i = 0; i < used_entities_pivot; i++) {
		ball_collision_system_each(used_entities[i]);
	}
}

void collider_update_position_system_each(const entity& e)
{
	if (!components::position_exists(e)) {
		return;
	}

	SDL_FPoint position = components::position_get(e);
	if (components::rect_collider_exists(e)) {
		SDL_FRect& rect = components::rect_collider_get(e);
		if (components::collider_offset_exists(e)) {
			SDL_FPoint offset = components::collider_offset_get(e);
			position.x += offset.x;
			position.y += offset.y;
		}
		rect.x = position.x;
		rect.y = position.y;
	}
	if (components::circle_collider_exists(e)) {
		SDL_FCircle& circle = components::circle_collider_get(e);
		if (components::collider_offset_exists(e)) {
			SDL_FPoint offset = components::collider_offset_get(e);
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
	if (components::rect_collider_exists(e)) {
		SDL_FRect rect = components::rect_collider_get(e);
		if (components::debug_color_exists(e)) {
			engine::draw_rect(rect, components::debug_color_get(e));
		}
		else {
			engine::draw_rect(rect, {255, 0, 0, 255});
		}
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
	if (components::circle_collider_exists(e)) {
		SDL_FCircle circle = components::circle_collider_get(e);
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
	ball_collision_system_run();

	engine::render_clear();
	draw_system_run();
	debug_rect_collider_system_run();
	debug_circle_collider_system_run();
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
			components::position_set(block, position);
			components::size_set(block, { 64, 64 });
			components::sprite_index_set(block, { 1, 1 });
			components::sprite_type_set(block, SPRITE_TYPE_TILE);
			components::rect_collider_set(block, collider);
			components::debug_color_set(block, { 255, 0, 0, 255 });
			blocks[(y * 10) + x] = block;
		}
	}

	// Construct player
	entity player{ entity_create() };
	components::position_set(player, { 400 - 32, 500 });
	components::size_set(player, { 64, 64 });
	components::sprite_index_set(player, { 1, 1 });
	components::sprite_type_set(player, SPRITE_TYPE_ENTITY);
	components::speed_set(player, 10.0f);
	components::controller_set(player, { SDL_SCANCODE_A, SDL_SCANCODE_D });
	components::collider_offset_set(player, { 32, 32 });
	components::circle_collider_set(player, { 400 - 32, 500, 32.0f });

	// Construct ball
	entity ball{ entity_create() };
	components::position_set(ball, { 400 - 32, 500 });
	components::size_set(ball, { 64, 64 });
	components::sprite_index_set(ball, { 1, 1 });
	components::sprite_type_set(ball, SPRITE_TYPE_ENTITY);
	components::speed_set(ball, 2.0f);
	components::direction_set(ball, { 1.0f, -1.0f });
	components::collider_offset_set(ball, { 32, 32 });
	components::circle_collider_set(ball, { 400 - 32, 500, 32.0f });

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
