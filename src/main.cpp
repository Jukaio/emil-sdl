
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
#define MAXIMUM_COMPONENTS 256
#define MAXIMUM_UPDATE_SYSTEMS 64
#define MAXIMUM_RENDER_SYSTEMS 32

// Entity
size_t entities_available_pivot{ MAXIMUM_ENTITIES };
entity entities_available[MAXIMUM_ENTITIES];

size_t entities_used_pivot{ 0 };
entity entities_used[MAXIMUM_ENTITIES];

namespace components
{
	namespace control
	{
		size_t component_count{ 0 };

		// Could be unsigned long long(?)
		bool valid_loopup[MAXIMUM_ENTITIES * MAXIMUM_COMPONENTS]{ false };

		void set_valid(entity e, size_t component_index, bool is_valid)
		{
			if (component_index >= component_count) {
				// Error here
				return;
			}
			valid_loopup[e * MAXIMUM_ENTITIES + component_index] = is_valid;
		}

		bool is_valid(entity e, size_t component_index)
		{
			return valid_loopup[e * MAXIMUM_ENTITIES + component_index];
		}
	}
}

void entities_initialise()
{
	for (int i = 0; i < MAXIMUM_ENTITIES; i++) {
		entities_available[i] = i;
		entities_used[i] = INVALID_ENTITY;
	}
}


bool filter[MAXIMUM_COMPONENTS];
bool* entities_filter(const entity e, size_t* count, ...)
{
	va_list args;
	va_start(args, count);

	bool* start = &components::control::valid_loopup[e * MAXIMUM_COMPONENTS + 0];
	memset(filter, 0, sizeof(filter));
	int bound = 0;
	for (int i = 0; i < *count; i++) {
		size_t index = va_arg(args, size_t);
		filter[index] = start[index];
		bound = SDL_max(bound, index);
	}
	*count = bound;
	return filter;
}


entity entity_create()
{
	if (entities_available_pivot == 0) {
		return INVALID_ENTITY;
	}
	entities_available_pivot -= 1;
	entity e = entities_available[entities_available_pivot];
	entities_available[entities_available_pivot] = INVALID_ENTITY;
	entities_used[entities_used_pivot] = e;
	entities_used_pivot += 1;

	return e;
}

void invalidate_components(entity e)
{
	bool* start = &components::control::valid_loopup[e * MAXIMUM_COMPONENTS + 0];
	memset(start, 0, sizeof(bool) * components::control::component_count);
}

void entity_destroy(entity* e)
{
	if (entities_used[entities_used_pivot - 1] == *e) { // just track one back if we remove last one
		entities_used_pivot -= 1;
		entities_used[entities_used_pivot] = INVALID_ENTITY;
		entities_available[entities_available_pivot] = *e;
		entities_available_pivot += 1;
		invalidate_components(*e); // meh, let's just try this
	}
	else {
		// otherwise... find...
		for (size_t i = 0; i < entities_used_pivot; i++) {
			entity target = entities_used[i];
			if (target == *e) { // found 
				// shift over 
				for (size_t j = i; j < entities_used_pivot - 1; j++) {
					entities_used[j] = entities_used[j + 1];
				}
				entities_used_pivot -= 1;
				entities_used[entities_used_pivot] = INVALID_ENTITY;
				entities_available[entities_available_pivot] = *e;
				entities_available_pivot += 1;
				// How to invalidate all the commponents? :( Entity signature? Maybe? No. FUCK!
				invalidate_components(*e); // meh, let's just try this
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

#define COMPONENT(type, name) \
namespace _internal \
{ \
	type name##_buffer[MAXIMUM_ENTITIES]; \
} \
size_t name##_id{ size_t(~0) }; \
bool name##_exists(const entity e) \
{ \
	if (e == INVALID_ENTITY || e >= MAXIMUM_ENTITIES || name##_id == size_t(~0)) { \
		return false; \
	} \
	return components::control::is_valid(e, name##_id); \
} \
void name##_set(const entity e, const type v) \
{ \
	if (e != INVALID_ENTITY || e >= MAXIMUM_ENTITIES) { \
		if(name##_id == size_t(~0)) { \
			name##_id = components::control::component_count; \
			components::control::component_count++; \
		} \
		components::control::set_valid(e, name##_id, true); \
		_internal::name##_buffer[e] = v; \
	} \
	else { \
		printf("Error in %s of type %s", #name, #type); \
	} \
} \
type& name##_get(const entity e) \
{ \
	return _internal::name##_buffer[e]; \
} \
void name##_destroy(const entity e) \
{ \
	if (e != INVALID_ENTITY || e >= MAXIMUM_ENTITIES ||name##_id == size_t(~0)) { \
		components::control::set_valid(e, name##_id, false); \
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

namespace systems
{
	typedef void(*function)(entity);
	namespace _internal
	{
		size_t update_buffer_pivot{ 0 };
		function update_buffer[MAXIMUM_UPDATE_SYSTEMS];

		size_t render_buffer_pivot{ 0 };
		function render_buffer[MAXIMUM_RENDER_SYSTEMS];
	}

	bool add_on_update(function func)
	{
		if (_internal::update_buffer_pivot >= MAXIMUM_UPDATE_SYSTEMS) {
			return false;
		}
		_internal::update_buffer[_internal::update_buffer_pivot] = func;
		_internal::update_buffer_pivot += 1;
		return true;
	}


	bool add_on_render(function func)
	{
		if (_internal::render_buffer_pivot >= MAXIMUM_RENDER_SYSTEMS) {
			return false;
		}
		_internal::render_buffer[_internal::render_buffer_pivot] = func;
		_internal::render_buffer_pivot += 1;
		return true;
	}

	bool remove_and_shift_buffer(function* buffer, size_t* pivot, function compare)
	{
		if (buffer[*pivot - 1] == compare) {
			// No need to actually do this, but it keeps the memory clean and dandy
			buffer[*pivot - 1] = nullptr;
			*pivot -= 1;
			return true;
		}
		else {
			for (size_t i = 0; i < *pivot - 1; i++) {
				function target = buffer[i];
				if (target == compare) { // found 
					// shift over 
					for (size_t j = i; j < *pivot - 1; j++) {
						buffer[j] = buffer[j + 1];
					}
					*pivot -= 1;
					buffer[*pivot] = nullptr;
					return true;
				}
			}
		}
		return false;
	}

	bool remove_on_update(function func)
	{
		return remove_and_shift_buffer(_internal::update_buffer, &_internal::update_buffer_pivot, func);
	}

	bool remove_on_render(function func)
	{
		return remove_and_shift_buffer(_internal::render_buffer, &_internal::render_buffer_pivot, func);
	}


	void run()
	{
		for (size_t i = 0; i < _internal::update_buffer_pivot; i++) {
			for (int j = 0; j < entities_used_pivot; j++) {
				_internal::update_buffer[i](entities_used[j]);
			}
		}

		engine::render_clear();
		for (size_t i = 0; i < _internal::render_buffer_pivot; i++) {
			for (int j = 0; j < entities_used_pivot; j++) {
				_internal::render_buffer[i](entities_used[j]);
			}
		}
		engine::render_present();
	}
}

// Systems - Optimisation idea, instead of iterating through EVERYTHING, subsribe them : ) 
// TODO: Make filter so systems only care for THEIR entities. IDK :D 
bool entity_can_use_draw_system(entity& e)
{
	using namespace components;
	return sprite_type_exists(e)
		&& sprite_index_exists(e)
		&& position_exists(e)
		&& size_exists(e);
}

void draw_system_each(entity e)
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

bool entity_can_use_player_system(entity& e)
{
	return components::controller_exists(e) && components::position_exists(e) && components::speed_exists(e);
}


void player_system_each(entity e)
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

bool entity_can_use_ball_system(entity& e)
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

void ball_system_each(entity e)
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


void ball_collision_system_each(entity e)
{
	// check if E really is a ball fulfilling entity
	if (!entity_can_use_ball_system(e)) {
		return;
	}
	using namespace components;
	for (int i = 0; i < entities_used_pivot; i++) {
		if (i != e) {
			entity other = entities_used[i];
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
					entity_destroy(&other);
					break;
				}
			}
		}
	}
}


void collider_update_position_system_each(entity e)
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


void debug_rect_collider_system_each(entity e)
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

void debug_circle_collider_system_each(entity e)
{
	if (components::circle_collider_exists(e)) {
		SDL_FCircle circle = components::circle_collider_get(e);
		engine::draw_circle(circle, { 255, 255, 0, 255 });
	}
}


int main()
{

	entities_initialise();
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

	size_t count = 5;
	using namespace components;

	// Filter first draft!
	bool* filter = entities_filter(player, &count, position_id, sprite_type_id, circle_collider_id, speed_id, controller_id);
	for (int i = 0; i < count; i++) {
		printf("%d - ", (int)filter[i]);
	}
	printf("\n");

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


	systems::add_on_update(player_system_each);
	systems::add_on_update(ball_system_each);
	systems::add_on_update(collider_update_position_system_each);
	systems::add_on_update(ball_collision_system_each);

	systems::add_on_render(draw_system_each);
	systems::add_on_render(debug_rect_collider_system_each);
	systems::add_on_render(debug_circle_collider_system_each);
	

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

		systems::run();

		SDL_Delay(16);
		// Update
		// Render
	}
}
