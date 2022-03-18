
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

float delta_time = 0.0f;

using entity = size_t;
using component_id = size_t;
// Invalid entity is 0 should cause a lot of cool out of the box behaviour
#define INVALID_ENTITY 0
#define INVALID_COMPONENT size_t(~0)

#define MAXIMUM_ENTITIES 256
#define MAXIMUM_UPDATE_SYSTEMS 64
#define MAXIMUM_RENDER_SYSTEMS 32

// Entity
size_t entities_available_pivot{ MAXIMUM_ENTITIES };
entity entities_available[MAXIMUM_ENTITIES];

size_t entities_used_pivot{ 0 };
entity entities_used[MAXIMUM_ENTITIES];

bool entity_used_lookup[MAXIMUM_ENTITIES];
bool entity_is_valid(entity e)
{
	return e < MAXIMUM_ENTITIES&& e != INVALID_ENTITY && entity_used_lookup[e];
}

namespace components
{
	namespace control
	{
		void set_valid(entity e, component_id component_index, bool is_valid);
		bool is_valid(entity e, component_id component_index);
	}
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
const component_id name##_id{ GET_COUNT_AND_INCREMENT }; \
bool name##_exists(const entity e) \
{ \
	if (!entity_is_valid(e) || name##_id == INVALID_COMPONENT) { \
		return false; \
	} \
	return components::control::is_valid(e, name##_id); \
} \
void name##_set(const entity e, const type v) \
{ \
	if (entity_is_valid(e)) { \
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
	if (!entity_is_valid(e) || name##_id == INVALID_COMPONENT) { \
		components::control::set_valid(e, name##_id, false); \
	} \
} \

#define COMPONENT_AREA_START enum COUNTER_BASE { COUNTER_BASE_VALUE = __COUNTER__ };
#define GET_COUNT_AND_INCREMENT (__COUNTER__ - COUNTER_BASE::COUNTER_BASE_VALUE - 1)
#define COMPONENT_AREA_END enum COMPONENT_COUNT { COMPONENT_COUNT_VALUE = GET_COUNT_AND_INCREMENT };
#define MAXIMUM_COMPONENTS ::components::COMPONENT_COUNT::COMPONENT_COUNT_VALUE

struct text
{
	char string[16];
};

typedef void(*button_event)();

namespace components
{
	COMPONENT_AREA_START
	// ...
	COMPONENT(controller, controller)
	COMPONENT(float, speed)
	COMPONENT(SDL_FPoint, direction)
	COMPONENT(SDL_FPoint, collider_offset)
	COMPONENT(SDL_FCircle, circle_collider)
	COMPONENT(SDL_FHorizontalCapsule, capsule_collider)
	COMPONENT(SDL_FRect, rect_collider)
	COMPONENT(sprite_type, sprite_type)
	COMPONENT(SDL_Point, sprite_index)
	COMPONENT(SDL_FPoint, position)
	COMPONENT(SDL_FPoint, size)
	COMPONENT(SDL_Colour, debug_color);
	COMPONENT(SDL_Point, mouse_position);
	COMPONENT(float, paddle_downset_manipulator);
	COMPONENT(text, button_text);
	COMPONENT(SDL_FRect, button_text_padding);
	COMPONENT(SDL_Colour, unhover_colour);
	COMPONENT(SDL_Colour, hover_colour);
	COMPONENT(button_event, button_event);
	// ...
	COMPONENT_AREA_END
}

namespace components
{
	namespace control
	{
		// Could be unsigned long long(?)
		bool valid_lookup[MAXIMUM_ENTITIES * MAXIMUM_COMPONENTS]{ false };

		void set_valid(entity e, component_id component_index, bool is_valid)
		{
			if (component_index >= MAXIMUM_COMPONENTS) {
				// Error here
				return;
			}
			valid_lookup[e * MAXIMUM_COMPONENTS + component_index] = is_valid;
		}

		bool is_valid(entity e, component_id component_index)
		{
			return valid_lookup[e * MAXIMUM_COMPONENTS + component_index];
		}
	}
}

struct signature
{
	bool field[MAXIMUM_COMPONENTS]; 
	size_t count;
};

struct filter
{
	entity list[MAXIMUM_ENTITIES];
	size_t count;
};

void entities_initialise()
{
	for (int i = 0; i < MAXIMUM_ENTITIES; i++) {
		entities_available[i] = i;
		entities_used[i] = INVALID_ENTITY;
	}
}

signature signature_create_from_entity(const entity e)
{
	bool* start = &components::control::valid_lookup[e * MAXIMUM_COMPONENTS + 0];

	signature signature{ {}, 0 };
	memset(signature.field, 0, sizeof(signature.field));
	for (int i = 0; i < MAXIMUM_COMPONENTS; i++) {
		signature.field[i] = start[i];
		signature.count = SDL_max(signature.count, i + 1);
	}
	return signature;
}

signature signature_create_from_entity_considering(const entity e, size_t count, ...)
{
	va_list args;
	va_start(args, count);

	bool* start = &components::control::valid_lookup[e * MAXIMUM_COMPONENTS + 0];

	signature signature{ {}, 0 };
	memset(signature.field, 0, sizeof(signature.field));
	for (int i = 0; i < count; i++) {
		size_t component_id = va_arg(args, size_t);
		signature.field[component_id] = start[component_id];
		signature.count = SDL_max(signature.count, component_id + 1);
	}

	va_end(args);
	return signature;
}

signature signature_create(size_t count, ...)
{
	va_list args;
	va_start(args, count);

	signature signature{ {}, 0 };
	memset(signature.field, 0, sizeof(signature.field));
	for (int i = 0; i < count; i++) {
		component_id component_id = va_arg(args, size_t);
		signature.field[component_id] = true;
		signature.count = SDL_max(signature.count, component_id + 1);
	}

	va_end(args);
	return signature;
}


#define SIGNATURE_START INVALID_COMPONENT
#define SIGNATURE_END INVALID_COMPONENT
signature signature_create_with_end(size_t index, ...)
{
	va_list args;
	va_start(args, index);

	signature signature{};
	memset(signature.field, 0, sizeof(signature.field));
	component_id component_id = va_arg(args, size_t);
	while (component_id != SIGNATURE_END) {
		signature.field[component_id] = true;
		signature.count = SDL_max(signature.count, component_id + 1);
		component_id = va_arg(args, size_t);
	}

	va_end(args);
	return signature;
}

bool signature_entity_fulfils(const entity e, const signature* signa)
{
	bool* entity_signature = &components::control::valid_lookup[e * MAXIMUM_COMPONENTS + 0];
	for (int i = 0; i < signa->count; i++) {
		bool is_relevant = signa->field[i];
		if (is_relevant && entity_signature[i] != is_relevant) {
			return false;
		}
	}
	return true;
}

filter entities_filter(const signature* signature)
{
	filter f { { INVALID_ENTITY }, 0 };
	for (int i = 0; i < entities_used_pivot; i++) {
		entity e = entities_used[i];
		for (int j = 0; j < signature->count; j++) {
			if (signature_entity_fulfils(e, signature)) {
				f.list[f.count] = e;
				f.count += 1;
			}
		}
	}
	return f;
}

filter entities_filter_ex(const signature* signa, const signature* can_not_have)
{
	filter f{ { INVALID_ENTITY }, 0 };
	for (int i = 0; i < entities_used_pivot; i++) {
		entity e = entities_used[i];
		if (signature_entity_fulfils(e, signa) && !signature_entity_fulfils(e, can_not_have)) {
			f.list[f.count] = e;
			f.count += 1;
		}
	}
	return f;
}

bool signature_is_identical(const signature* lhs, const signature* rhs)
{
	size_t max;
	size_t min;
	const signature* check;

	if (lhs->count > rhs->count) {
		check = lhs;
		min = rhs->count;
		max = lhs->count;
	}
	else {
		check = rhs;
		min = lhs->count;
		max = rhs->count;
	}

	for (size_t i = min; i < max; i++) {
		if (check->field[i]) {
			return false;
		}
	}
	for (size_t i = 0; i < min; i++) {
		if (lhs->field[i] != rhs->field[i]) {
			return false;
		}
	}
	return true;
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
	entity_used_lookup[e] = true;
	return e;
}

void invalidate_components(entity e)
{
	bool* start = &components::control::valid_lookup[e * MAXIMUM_COMPONENTS + 0];
	memset(start, 0, sizeof(bool) * MAXIMUM_COMPONENTS);
}


void entity_destroy(entity* e)
{
	if (entities_used[entities_used_pivot - 1] == *e) { // just track one back if we remove last one
		invalidate_components(*e); // meh, let's just try this
		entity_used_lookup[*e] = false;
		entities_available[entities_available_pivot] = *e;
		entities_available_pivot += 1;
		entities_used_pivot -= 1;
		entities_used[entities_used_pivot] = INVALID_ENTITY;
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
				// How to invalidate all the commponents? :( Entity signature? Maybe? No. FUCK!
				invalidate_components(*e); // meh, let's just try this
				entity_used_lookup[*e] = false;
				entities_available[entities_available_pivot] = *e;
				entities_available_pivot += 1;
				entities_used_pivot -= 1;
				entities_used[entities_used_pivot] = INVALID_ENTITY;
				break;
			}
		}
	}
}

void entities_clear()
{
	for (int i = entities_used_pivot - 1; i >= 0; i--) {
		entity_destroy(&entities_used[i]);
	}
}

namespace systems
{
	typedef void(*function)(entity);
	namespace _internal
	{
		size_t update_buffer_pivot{ 0 };
		signature update_signatures[MAXIMUM_UPDATE_SYSTEMS];
		function update_buffer[MAXIMUM_UPDATE_SYSTEMS];

		size_t render_buffer_pivot{ 0 };
		signature render_signatures[MAXIMUM_RENDER_SYSTEMS];
		function render_buffer[MAXIMUM_RENDER_SYSTEMS];
	}

	// TODO: Add signature to it :D
	bool add_on_update(signature signature, function func)
	{
		if (_internal::update_buffer_pivot >= MAXIMUM_UPDATE_SYSTEMS) {
			return false;
		}
		_internal::update_buffer[_internal::update_buffer_pivot] = func;
		memcpy(&_internal::update_signatures[_internal::update_buffer_pivot], &signature, sizeof(signature));
		_internal::update_buffer_pivot += 1;
		return true;
	}

	bool add_on_render(signature signature, function func)
	{
		if (_internal::render_buffer_pivot >= MAXIMUM_RENDER_SYSTEMS) {
			return false;
		}
		memcpy(&_internal::render_signatures[_internal::render_buffer_pivot], &signature, sizeof(signature));
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

	void clear_all()
	{
		_internal::update_buffer_pivot = 0;
		_internal::render_buffer_pivot = 0;
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
				const entity e = entities_used[j];
				if (signature_entity_fulfils(e, &_internal::update_signatures[i])) {
					_internal::update_buffer[i](e);
				}
			}
		}

		engine::render_clear();
		for (size_t i = 0; i < _internal::render_buffer_pivot; i++) {
			for (int j = 0; j < entities_used_pivot; j++) {
				const entity e = entities_used[j];
				if (signature_entity_fulfils(e, &_internal::render_signatures[i])) {
					_internal::render_buffer[i](e);
				}
			}
		}

		engine::render_present();
	}
}

void draw_system_each(entity e)
{
	SDL_FPoint p = components::position_get(e);
	SDL_FPoint s = components::size_get(e);
	SDL_FRect destination{ p.x, p.y, s.x, s.y };
	switch (components::sprite_type_get(e)) {
		case SPRITE_TYPE_ENTITY:
			engine::draw_entity(components::sprite_index_get(e), destination);
			break;

		case SPRITE_TYPE_TILE:
			engine::draw_tile(components::sprite_index_get(e), destination);
			break;
	}
}

void player_system_each(entity e)
{
	int move{ 0 };
	controller c{ components::controller_get(e) };

	if (input::is_down(c.left)) {
		move -= 1;
	}
	if (input::is_down(c.right)) {
		move += 1;
	}

	components::position_get(e).x += (move * components::speed_get(e) * delta_time);
}

void ball_system_each(entity e)
{

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

	position.x += velocity.x * delta_time;
	position.y += velocity.y * delta_time;
}


void ball_collision_system_each(entity e)
{
	using namespace components;
	signature has = signature_create(1, rect_collider_id);
	signature can_not = signature_create(1, controller_id);
	filter entities = entities_filter_ex(&has, &can_not);

	size_t pivot{ 0 };
	SDL_Rect collider[8];
	SDL_Point normals[8];

	for (int i = 0; i < entities.count; i++) {
		entity other = entities.list[i];
		SDL_FCircle ball_collider = circle_collider_get(e);
		SDL_FRect other_collider = rect_collider_get(other);
		SDL_FPoint normal;
		if (SDL_IntersectFCircleFRect(ball_collider, other_collider, normal)) { // This is rubbish...

			SDL_FPoint& direction = direction_get(e);
			direction = SDL_FPointReflect(direction, normal);
			entity_destroy(&other);
			break;
		}
	}
}

void collider_update_position_system_each(entity e)
{
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
	if (components::capsule_collider_exists(e)) {
		SDL_FHorizontalCapsule& capsule = components::capsule_collider_get(e);
		if (components::collider_offset_exists(e)) {
			SDL_FPoint offset = components::collider_offset_get(e);
			position.x += offset.x;
			position.y += offset.y;
		}
		capsule.position.x = position.x;
		capsule.position.y = position.y;
	}
}

void paddle_ball_collision_system_each(entity e)
{
	using namespace components;
	SDL_FCircle& ball_collider = circle_collider_get(e);
	for (int i = 0; i < entities_used_pivot; i++) {
		if (i != e) {
			entity other = entities_used[i];
			if (rect_collider_exists(other) && paddle_downset_manipulator_exists(other)) {
				SDL_FRect other_collider = rect_collider_get(other);
				SDL_FPoint normal;
				SDL_FPoint& direction = direction_get(e);
				if (SDL_IntersectFCircleFRect(ball_collider, other_collider, normal)) { // This is rubbish...

					float centreX = (other_collider.x + other_collider.w * 0.5f);
					float centreY = (other_collider.y + other_collider.h * 0.5f);
					centreY += paddle_downset_manipulator_get(e);
					direction = { ball_collider.x - centreX, ball_collider.y - centreY };

					ball_system_each(e);
					collider_update_position_system_each(e);
					break;
				}
			}
		}
	}
}

void debug_rect_collider_system_each(entity e)
{
	SDL_FRect rect = components::rect_collider_get(e);
	if (components::debug_color_exists(e)) {
		engine::draw_rect(rect, components::debug_color_get(e));
	}
	else {
		engine::draw_rect(rect, { 255, 0, 0, 255 });
	}
}

void debug_circle_collider_system_each(entity e)
{
	using namespace components;

	SDL_FCircle circle = circle_collider_get(e);
	engine::draw_circle(circle, { 255, 255, 0, 255 });
}

void debug_capsule_collider_system_each(entity e)
{
	using namespace components;

	SDL_FHorizontalCapsule capsule = capsule_collider_get(e);
	engine::draw_capsule(capsule, { 255, 255, 0, 255 });
}


void mouse_position_update_each(entity e)
{
	using namespace components;
	SDL_Point* position = &mouse_position_get(e);
	*position = input::get_mouse_position();
}

void debug_circle_collider_position_each(entity e)
{
	using namespace components;
	SDL_Point position = mouse_position_get(e);
	SDL_FCircle* collider = &circle_collider_get(e);
	collider->x = position.x;
	collider->y = position.y;

	for (int i = 0; i < entities_used_pivot; i++) {
		if (i != e) {
			entity other = entities_used[i];
			if (rect_collider_exists(other)) {
				if (debug_color_exists(other)) {
					SDL_FRect other_collider = rect_collider_get(other);
					SDL_FPoint normal;
					SDL_Colour* debug_colour = &debug_color_get(other);

					if (SDL_IntersectFCircleFRect(*collider, other_collider, normal)) { // This is rubbish...
						*debug_colour = SDL_Colour{ 0, 255, 0, 255 };
					}
					else {
						*debug_colour = SDL_Colour{ 255, 0, 0, 255 };
					}
				}
			}
		}
	}
}

void debug_draw_normals(entity e)
{
	using namespace components;
	SDL_FRect rect = rect_collider_get(e);
	signature include = signature_create(2, mouse_position_id, circle_collider_id);
	filter filter = entities_filter(&include);

	for (int i = 0; i < filter.count; i++) {
		entity other = filter.list[i];
		SDL_FCircle circle_collider = circle_collider_get(other);

		SDL_FPoint normal;
		if (SDL_IntersectFCircleFRect(circle_collider, rect, normal)) { // This is rubbish...
			SDL_FPoint start = { rect.x + rect.w * 0.5f, rect.y + rect.h * 0.5f};
			engine::draw_line
			(
				start,
				{ start.x + normal.x * 48.0f, start.y + normal.y * 48.0f },
				{ 255, 255, 0, 255 }
			);
		}
	}
}

// Menu
void buttom_draw_system_each(entity e)
{
	using namespace components;
	SDL_FPoint normal;
	SDL_FRect rect = rect_collider_get(e);
	SDL_FRect padding = button_text_padding_get(e);

	SDL_Point mouse_position = input::get_mouse_position();
	SDL_FCircle mouse_collider{ mouse_position.x, mouse_position.y, 1.0f };

	SDL_Colour colour = unhover_colour_get(e);
	if (SDL_IntersectFCircleFRect(mouse_collider, rect, normal)) {
		if (input::mouse_was_pressed(SDL_BUTTON_LMASK)) {
			button_event_get(e)();
		}
		colour = hover_colour_get(e);
	}

	engine::draw_rect(rect, colour);
	rect.x += padding.x - padding.w * 0.5f;
	rect.y += padding.y - padding.h * 0.5f;
	rect.w -= padding.w;
	rect.h -= padding.h;

	text t = button_text_get(e);
	engine::draw_text(t.string, rect);
}


void load_gameplay()
{
	// Construct level
	entity blocks[10 * 5];
	float block_height = 32.0f;
	float block_width = 64.0f;
	SDL_FPoint offset{ 80, 20 };
	for (int x = 0; x < 10; x++) {
		for (int y = 0; y < 5; y++) {
			entity block = entity_create();
			SDL_FPoint position{ offset.x + (x * block_width), offset.y + (y * block_height) };
			SDL_FRect collider{ position.x, position.y, block_width, block_height };
			components::position_set(block, position);
			components::size_set(block, { block_width, block_height });
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
	components::speed_set(player, 180.0f);
	components::controller_set(player, { SDL_SCANCODE_A, SDL_SCANCODE_D });
	components::collider_offset_set(player, { 0, 0 });
	components::rect_collider_set(player, { 400 - 32, 500 , 64.0f, 16.0f });
	components::paddle_downset_manipulator_set(player, 64.0f);

	//Construct ball
	entity ball{ entity_create() };
	components::position_set(ball, { 400 - 32, 400 });
	components::size_set(ball, { 16, 16 });
	components::sprite_index_set(ball, { 1, 1 });
	components::sprite_type_set(ball, SPRITE_TYPE_ENTITY);
	components::speed_set(ball, 240.0f);
	components::direction_set(ball, { 0.0f, -1.0f });
	components::collider_offset_set(ball, { 8, 8 });
	components::circle_collider_set(ball, { 400 - 32, 500, 8.0f });

	//// Construct Debug rect
	//entity debug_rect{ entity_create() };
	//components::mouse_position_set(debug_rect, { 0, 0 });
	//components::collider_offset_set(debug_rect, { 4, 4 });
	//components::circle_collider_set(debug_rect, { 0, 0, 8.0f });


	using namespace components;
	signature ball_signature = signature_create(4, position_id, speed_id, direction_id, circle_collider_id);

	systems::add_on_update(signature_create(3, controller_id, position_id, speed_id), player_system_each);
	systems::add_on_update(ball_signature, ball_system_each);

	systems::add_on_update(signature_create(1, position_id), collider_update_position_system_each);
	systems::add_on_update(ball_signature, ball_collision_system_each);
	systems::add_on_update(ball_signature, paddle_ball_collision_system_each);

	systems::add_on_update(signature_create(1, mouse_position_id), mouse_position_update_each);
	//systems::add_on_update(signature_create(2, mouse_position_id, circle_collider_id), debug_circle_collider_position_each);

	systems::add_on_render(signature_create(4, sprite_type_id, sprite_index_id, position_id, size_id), draw_system_each);
	systems::add_on_render(signature_create(1, rect_collider_id), debug_rect_collider_system_each);
	systems::add_on_render(signature_create(1, circle_collider_id), debug_circle_collider_system_each);
	systems::add_on_render(signature_create(1, capsule_collider_id), debug_capsule_collider_system_each);
	systems::add_on_render(signature_create(1, rect_collider_id), debug_draw_normals);
}

void load_menu()
{
	entity start_button{ entity_create() };
	components::rect_collider_set(start_button, {32, 32, 128, 64 });
	components::button_text_set(start_button, { "Start!" });
	components::button_text_padding_set(start_button, { 16, 16, 16, 16 });
	components::hover_colour_set(start_button, { 255, 255, 0, 255 });
	components::unhover_colour_set(start_button, { 255, 0, 255, 255 });
	components::button_event_set(start_button, []() {
		systems::clear_all();
		entities_clear();
		load_gameplay();
	});

	entity load_button{ entity_create() };
	components::rect_collider_set(load_button, { 32, 32 + 64, 128, 64 });
	components::button_text_set(load_button, { "Load Level!" });
	components::button_text_padding_set(load_button, { 16, 16, 16, 16 });
	components::hover_colour_set(load_button, { 255, 255, 0, 255 });
	components::unhover_colour_set(load_button, { 255, 0, 255, 255 });
	components::button_event_set(load_button, []() {
		printf("Go fuck yourself!\n");
	});

	using namespace components;
	systems::add_on_render(signature_create_from_entity(start_button), buttom_draw_system_each);
}


int main()
{
	entities_initialise();
	engine::initialise(SCREEN_WIDTH, SCREEN_HEIGHT);
	engine::load_entities_texture("res/objects.png");
	engine::set_entity_source_size(32, 32);

	engine::load_tiles_texture("res/rock_packed.png");
	engine::set_tile_source_size(18, 18);

	engine::load_font("res/roboto.ttf");

	load_menu();

	//load_gameplay();

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
		delta_time = ((float)delta_ticks / SDL_GetPerformanceFrequency());

		events::run();
		input::run();

		systems::run();
		// Update
		// Render
	}
}
