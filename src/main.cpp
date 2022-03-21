
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

typedef unsigned char DVD_byte;
typedef size_t DVD_entity;
typedef size_t DVD_component_id;

// Invalid entity is 0 should cause a lot of cool out of the box behaviour
#define INVALID_ENTITY 0
#define INVALID_COMPONENT size_t(~0)

#define MAXIMUM_ENTITIES 256
#define MAXIMUM_UPDATE_SYSTEMS 64
#define MAXIMUM_RENDER_SYSTEMS 32

// Entity
size_t DVD_entities_available_pivot{ MAXIMUM_ENTITIES };
DVD_entity DVD_entities_available[MAXIMUM_ENTITIES];

size_t DVD_entities_used_pivot{ 0 };
DVD_entity DVD_entities_used[MAXIMUM_ENTITIES];

bool DVD_entity_used_lookup[MAXIMUM_ENTITIES];

// Header
bool DVD_entity_is_valid(DVD_entity e);
void DVD_components_control_try_initialise(DVD_component_id component_index, DVD_byte* buffer_begin, size_t buffer_element_size);
void DVD_components_control_set_valid(DVD_entity e, DVD_component_id component_index, bool is_valid);
bool DVD_components_control_is_valid(DVD_entity e, DVD_component_id component_index);

#define COMPONENT(custom_namespace, type, name) \
type DVD_internal_##name##_buffer[MAXIMUM_ENTITIES]; \
const DVD_component_id custom_namespace##_##name##_id{ GET_COUNT_AND_INCREMENT }; \
inline bool custom_namespace##_##name##_exists(const DVD_entity e) \
{ \
	if (!DVD_entity_is_valid(e) || custom_namespace##_##name##_id == INVALID_COMPONENT) { \
		return false; \
	} \
	return DVD_components_control_is_valid(e, custom_namespace##_##name##_id); \
} \
inline void custom_namespace##_##name##_set(const DVD_entity e, const type v) \
{ \
	if (DVD_entity_is_valid(e)) { \
		DVD_components_control_try_initialise(custom_namespace##_##name##_id, (DVD_byte*)(DVD_internal_##name##_buffer), sizeof(type)); \
		DVD_components_control_set_valid(e, custom_namespace##_##name##_id, true); \
		DVD_internal_##name##_buffer[e] = v; \
	} \
	else { \
		printf("Error in %s of type %s", #name, #type); \
	} \
} \
inline type* custom_namespace##_##name##_get(const DVD_entity e) \
{ \
	return &DVD_internal_##name##_buffer[e]; \
} \
inline void custom_namespace##_##name##_destroy(const DVD_entity e) \
{ \
	if (!DVD_entity_is_valid(e) || custom_namespace##_##name##_id == INVALID_COMPONENT) { \
		DVD_components_control_set_valid(e, custom_namespace##_##name##_id, false); \
	} \
} \

// User:
// Encapsulate all used components with COMPONENT_AREA_START and COMPONENT_AREA_END to count all used components
// otherwise, if you are lazy, just define MAXIMUM_COMPONENTS with a hardcoded value by yourself
#define COMPONENT_AREA_START enum COUNTER_BASE { COUNTER_BASE_VALUE = __COUNTER__ };
#define GET_COUNT_AND_INCREMENT (__COUNTER__ - COUNTER_BASE_VALUE - 1)
#define COMPONENT_AREA_END enum COMPONENT_COUNT { COMPONENT_COUNT_VALUE = GET_COUNT_AND_INCREMENT };
#define MAXIMUM_COMPONENTS COMPONENT_COUNT_VALUE

// User-defined structs
struct controller
{
	SDL_Scancode left;
	SDL_Scancode right;
};

struct text
{
	char string[16];
};

enum sprite_type
{
	SPRITE_TYPE_ENTITY,
	SPRITE_TYPE_TILE
};

typedef void(*button_event)();

// User-defined Components implementation and interface generation (Optional, but convenient)
COMPONENT_AREA_START
// ...
COMPONENT(gameplay, controller, controller)
COMPONENT(gameplay, float, speed)
COMPONENT(gameplay, SDL_FPoint, direction)
COMPONENT(gameplay, SDL_FPoint, collider_offset)
COMPONENT(gameplay, SDL_FCircle, circle_collider)
COMPONENT(gameplay, SDL_FHorizontalCapsule, capsule_collider)
COMPONENT(gameplay, SDL_FRect, rect_collider)
COMPONENT(gameplay, sprite_type, sprite_type)
COMPONENT(gameplay, SDL_Point, sprite_index)
COMPONENT(gameplay, SDL_FPoint, position)
COMPONENT(gameplay, SDL_FPoint, size)
COMPONENT(gameplay, SDL_Colour, debug_color);
COMPONENT(gameplay, SDL_Point, mouse_position);
COMPONENT(gameplay, float, paddle_downset_manipulator);
COMPONENT(gameplay, text, button_text);
COMPONENT(gameplay, SDL_FRect, button_text_padding);
COMPONENT(gameplay, SDL_Colour, unhover_colour);
COMPONENT(gameplay, SDL_Colour, hover_colour);
COMPONENT(gameplay, button_event, button_event);
// ...
COMPONENT_AREA_END

// Separate into header
// In static storage (.cpp/.c)
size_t DVD_components_buffer_element_size_lookup[MAXIMUM_COMPONENTS]{ 0 };
DVD_byte* DVD_components_buffer_begin_lookup[MAXIMUM_COMPONENTS]{ nullptr };
bool DVD_components_valid_lookup[MAXIMUM_ENTITIES * MAXIMUM_COMPONENTS]{ false };

bool DVD_components_control_is_initialised(DVD_component_id component_index)
{
	return DVD_components_buffer_element_size_lookup[component_index] != 0;
}
void DVD_components_control_initialise(DVD_component_id component_index, DVD_byte* buffer_begin, size_t buffer_element_size)
{
	if (component_index >= MAXIMUM_COMPONENTS) {
		// Error here
		return;
	}
	DVD_components_buffer_element_size_lookup[component_index] = buffer_element_size;
	DVD_components_buffer_begin_lookup[component_index] = buffer_begin;
}
void DVD_components_control_try_initialise(DVD_component_id component_index, DVD_byte* buffer_begin, size_t buffer_element_size)
{
	if (!DVD_components_control_is_initialised(component_index)) {
		DVD_components_control_initialise(component_index, buffer_begin, buffer_element_size);
	}
}
void DVD_components_control_set_valid(DVD_entity e, DVD_component_id component_index, bool is_valid)
{
	if (component_index >= MAXIMUM_COMPONENTS) {
		// Error here
		return; 
	}
	DVD_components_valid_lookup[e * MAXIMUM_COMPONENTS + component_index] = is_valid;
}
bool DVD_components_control_is_valid(DVD_entity e, DVD_component_id component_index)
{
	return DVD_components_valid_lookup[e * MAXIMUM_COMPONENTS + component_index];
}
void DVD_components_single_copy(DVD_component_id component_index, DVD_entity from, DVD_entity to)
{
	if (DVD_components_control_is_initialised(component_index)) {
		size_t element_size = DVD_components_buffer_element_size_lookup[component_index];
		DVD_byte* start = DVD_components_buffer_begin_lookup[component_index];
		DVD_byte* src_data = (start)+(from * element_size);
		DVD_byte* dst_data = (start)+(to * element_size);
		memcpy(dst_data, src_data, element_size);
		DVD_components_control_set_valid(to, component_index, DVD_components_control_is_valid(from, component_index));
	}
}
void DVD_components_entities_deep_copy(DVD_entity from, DVD_entity to)
{
	for (int i = 0; i < MAXIMUM_COMPONENTS; i++) {
		DVD_components_single_copy(i, from, to);
	}
}

struct DVD_signature
{
	bool field[MAXIMUM_COMPONENTS]; 
	size_t count;
};
struct DVD_filter
{
	DVD_entity list[MAXIMUM_ENTITIES];
	size_t count;
};
void DVD_entities_initialise()
{
	for (int i = 0; i < MAXIMUM_ENTITIES; i++) {
		DVD_entities_available[i] = i;
		DVD_entities_used[i] = INVALID_ENTITY;
	}
}
bool DVD_entity_is_valid(DVD_entity e)
{
	return e < MAXIMUM_ENTITIES && e != INVALID_ENTITY && DVD_entity_used_lookup[e];
}
DVD_signature DVD_signature_create_from_entity(const DVD_entity e)
{
	bool* start = &DVD_components_valid_lookup[e * MAXIMUM_COMPONENTS + 0];

	DVD_signature signature{ {}, 0 };
	memset(signature.field, 0, sizeof(signature.field));
	for (int i = 0; i < MAXIMUM_COMPONENTS; i++) {
		signature.field[i] = start[i];
		signature.count = SDL_max(signature.count, i + 1);
	}
	return signature;
}
DVD_signature DVD_signature_create_intersection_with_entity(const DVD_entity e, size_t count, ...)
{
	va_list args;
	va_start(args, count);

	bool* start = &DVD_components_valid_lookup[e * MAXIMUM_COMPONENTS + 0];

	DVD_signature signature{ {}, 0 };
	memset(signature.field, 0, sizeof(signature.field));
	for (int i = 0; i < count; i++) {
		size_t component_id = va_arg(args, size_t);
		signature.field[component_id] = start[component_id];
		signature.count = SDL_max(signature.count, component_id + 1);
	}

	va_end(args);
	return signature;
}
DVD_signature DVD_signature_create(size_t count, ...)
{
	va_list args;
	va_start(args, count);

	DVD_signature signature{ {}, 0 };
	memset(signature.field, 0, sizeof(signature.field));
	for (int i = 0; i < count; i++) {
		DVD_component_id component_id = va_arg(args, size_t);
		signature.field[component_id] = true;
		signature.count = SDL_max(signature.count, component_id + 1);
	}

	va_end(args);
	return signature;
}
bool DVD_signature_entity_fulfils(const DVD_entity e, const DVD_signature* signa)
{
	bool* entity_signature = &DVD_components_valid_lookup[e * MAXIMUM_COMPONENTS + 0];
	for (int i = 0; i < signa->count; i++) {
		bool is_relevant = signa->field[i];
		if (is_relevant && entity_signature[i] != is_relevant) {
			return false;
		}
	}
	return true;
}
DVD_filter DVD_entities_filter(const DVD_signature* signature)
{
	DVD_filter f { { INVALID_ENTITY }, 0 };
	for (int i = 0; i < DVD_entities_used_pivot; i++) {
		DVD_entity e = DVD_entities_used[i];
		for (int j = 0; j < signature->count; j++) {
			if (DVD_signature_entity_fulfils(e, signature)) {
				f.list[f.count] = e;
				f.count += 1;
			}
		}
	}
	return f;
}
DVD_filter DVD_entities_filter_ex(const DVD_signature* signa, const DVD_signature* can_not_have)
{
	DVD_filter f{ { INVALID_ENTITY }, 0 };
	for (int i = 0; i < DVD_entities_used_pivot; i++) {
		DVD_entity e = DVD_entities_used[i];
		if (DVD_signature_entity_fulfils(e, signa) && !DVD_signature_entity_fulfils(e, can_not_have)) {
			f.list[f.count] = e;
			f.count += 1;
		}
	}
	return f;
}
bool DVD_signature_is_identical(const DVD_signature* lhs, const DVD_signature* rhs)
{
	size_t max;
	size_t min;
	const DVD_signature* check;

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
DVD_entity DVD_entities_create()
{
	if (DVD_entities_available_pivot == 0) {
		return INVALID_ENTITY;
	}
	DVD_entities_available_pivot -= 1;
	DVD_entity e = DVD_entities_available[DVD_entities_available_pivot];
	DVD_entities_available[DVD_entities_available_pivot] = INVALID_ENTITY;
	DVD_entities_used[DVD_entities_used_pivot] = e;
	DVD_entities_used_pivot += 1;
	DVD_entity_used_lookup[e] = true;
	return e;
}
DVD_entity DVD_entities_create_copy(DVD_entity of)
{
	DVD_entity e = DVD_entities_create();
	DVD_components_entities_deep_copy(of, e);
	return e;
}
void DVD_entities_invalidate_components(DVD_entity e)
{
	bool* start = &DVD_components_valid_lookup[e * MAXIMUM_COMPONENTS + 0];
	memset(start, 0, sizeof(bool) * MAXIMUM_COMPONENTS);
}
void DVD_entities_destroy(DVD_entity* e)
{
	if (DVD_entities_used[DVD_entities_used_pivot - 1] == *e) { // just track one back if we remove last one
		DVD_entities_invalidate_components(*e); // meh, let's just try this
		DVD_entity_used_lookup[*e] = false;
		DVD_entities_available[DVD_entities_available_pivot] = *e;
		DVD_entities_available_pivot += 1;
		DVD_entities_used_pivot -= 1;
		DVD_entities_used[DVD_entities_used_pivot] = INVALID_ENTITY;
	}
	else {
		// otherwise... find...
		for (size_t i = 0; i < DVD_entities_used_pivot; i++) {
			DVD_entity target = DVD_entities_used[i];
			if (target == *e) { // found 
				// shift over 
				for (size_t j = i; j < DVD_entities_used_pivot - 1; j++) {
					DVD_entities_used[j] = DVD_entities_used[j + 1];
				}
				// How to invalidate all the commponents? :( Entity signature? Maybe? No. FUCK!
				DVD_entities_invalidate_components(*e); // meh, let's just try this
				DVD_entity_used_lookup[*e] = false;
				DVD_entities_available[DVD_entities_available_pivot] = *e;
				DVD_entities_available_pivot += 1;
				DVD_entities_used_pivot -= 1;
				DVD_entities_used[DVD_entities_used_pivot] = INVALID_ENTITY;
				break;
			}
		}
	}
}
void DVD_entities_clear()
{
	for (int i = DVD_entities_used_pivot - 1; i >= 0; i--) {
		DVD_entities_destroy(&DVD_entities_used[i]);
	}
}

typedef void(*DVD_systems_function)(DVD_entity);
size_t DVD_systems_internal_update_buffer_pivot{ 0 };
DVD_signature DVD_systems_internal_update_signatures[MAXIMUM_UPDATE_SYSTEMS];
DVD_systems_function DVD_systems_internal_update_buffer[MAXIMUM_UPDATE_SYSTEMS];

size_t DVD_systems_internal_render_buffer_pivot{ 0 };
DVD_signature DVD_systems_internal_render_signatures[MAXIMUM_RENDER_SYSTEMS];
DVD_systems_function DVD_systems_internal_render_buffer[MAXIMUM_RENDER_SYSTEMS];

bool DVD_systems_add_on_update(DVD_signature signature, DVD_systems_function func)
{
	if (DVD_systems_internal_update_buffer_pivot >= MAXIMUM_UPDATE_SYSTEMS) {
		return false;
	}
	DVD_systems_internal_update_buffer[DVD_systems_internal_update_buffer_pivot] = func;
	memcpy(&DVD_systems_internal_update_signatures[DVD_systems_internal_update_buffer_pivot], &signature, sizeof(signature));
	DVD_systems_internal_update_buffer_pivot += 1;
	return true;
}
bool DVD_systems_add_on_render(DVD_signature signature, DVD_systems_function func)
{
	if (DVD_systems_internal_render_buffer_pivot >= MAXIMUM_RENDER_SYSTEMS) {
		return false;
	}
	memcpy(&DVD_systems_internal_render_signatures[DVD_systems_internal_render_buffer_pivot], &signature, sizeof(signature));
	DVD_systems_internal_render_buffer[DVD_systems_internal_render_buffer_pivot] = func;
	DVD_systems_internal_render_buffer_pivot += 1;
	return true;
}
bool DVD_systems_internal_remove_and_shift_buffer(DVD_systems_function* buffer, size_t* pivot, DVD_systems_function compare)
{
	if (buffer[*pivot - 1] == compare) {
		// No need to actually do this, but it keeps the memory clean and dandy
		buffer[*pivot - 1] = nullptr;
		*pivot -= 1;
		return true;
	}
	else {
		for (size_t i = 0; i < *pivot - 1; i++) {
			DVD_systems_function target = buffer[i];
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
void DVD_systems_remove_all_update()
{
	DVD_systems_internal_update_buffer_pivot = 0;
}
void DVD_systems_remove_all_render()
{
	DVD_systems_internal_render_buffer_pivot = 0;
}
void DVD_systems_remove_all()
{
	DVD_systems_remove_all_update();
	DVD_systems_remove_all_render();
}
bool DVD_systems_remove_on_update(DVD_systems_function func)
{
	return DVD_systems_internal_remove_and_shift_buffer(DVD_systems_internal_update_buffer, &DVD_systems_internal_update_buffer_pivot, func);
}
bool DVD_systems_remove_on_render(DVD_systems_function func)
{
	return DVD_systems_internal_remove_and_shift_buffer(DVD_systems_internal_render_buffer, &DVD_systems_internal_render_buffer_pivot, func);
}
void DVD_systems_run()
{
	for (size_t i = 0; i < DVD_systems_internal_update_buffer_pivot; i++) {
		for (int j = 0; j < DVD_entities_used_pivot; j++) {
			const DVD_entity e = DVD_entities_used[j];
			if (DVD_signature_entity_fulfils(e, &DVD_systems_internal_update_signatures[i])) {
				DVD_systems_internal_update_buffer[i](e);
			}
		}
	}

	engine::render_clear();
	for (size_t i = 0; i < DVD_systems_internal_render_buffer_pivot; i++) {
		for (int j = 0; j < DVD_entities_used_pivot; j++) {
			const DVD_entity e = DVD_entities_used[j];
			if (DVD_signature_entity_fulfils(e, &DVD_systems_internal_render_signatures[i])) {
				DVD_systems_internal_render_buffer[i](e);
			}
		}
	}

	engine::render_present();
}

// User-defined systems!
void draw_system_each(DVD_entity e)
{
	SDL_FPoint p = *gameplay_position_get(e);
	SDL_FPoint s = *gameplay_size_get(e);
	SDL_FRect destination{ p.x, p.y, s.x, s.y };
	switch (*gameplay_sprite_type_get(e)) {
		case SPRITE_TYPE_ENTITY:
			engine::draw_entity(*gameplay_sprite_index_get(e), destination);
			break;

		case SPRITE_TYPE_TILE:
			engine::draw_tile(*gameplay_sprite_index_get(e), destination);
			break;
	}
}

void player_system(DVD_entity e)
{
	int move{ 0 };
	controller* c{ gameplay_controller_get(e) };

	if (input::was_pressed(SDL_SCANCODE_BACKSPACE)) {
		DVD_systems_remove_all();
		DVD_entities_clear();
		(*gameplay_button_event_get(e))();
	}

	if (input::is_down(c->left)) {
		move -= 1;
	}
	if (input::is_down(c->right)) {
		move += 1;
	}

	gameplay_position_get(e)->x += (move * *gameplay_speed_get(e) * delta_time);
}

void ball_system(DVD_entity e)
{

	SDL_FPoint* direction{ gameplay_direction_get(e) };
	SDL_FPoint dir{ *direction };
	// Normalise and apply
	float speed{ *gameplay_speed_get(e) };
	
	SDL_FPointNormalise(&dir);
	(*direction) = dir;

	// Reuse dir velocity
	SDL_FPoint velocity{ dir.x * speed, dir.y * speed };

	// Apply velocity - Reflect from screen edges
	SDL_FPoint* position{ gameplay_position_get(e) };
	SDL_FCircle circle{ *gameplay_circle_collider_get(e) };
	if ((circle.x - circle.radius) + dir.x <= 0.0f || (circle.x + circle.radius) + dir.x >= SCREEN_WIDTH) {
		direction->x = -direction->x;
		velocity.x = -velocity.x;
	}
	if ((circle.y - circle.radius) + dir.y <= 0.0f || (circle.y + circle.radius) + dir.y >= SCREEN_HEIGHT) {
		direction->y = -direction->y;
		velocity.y = -velocity.y;
	}

	position->x += velocity.x * delta_time;
	position->y += velocity.y * delta_time;
}

void ball_collision_system(DVD_entity e)
{
	
	DVD_signature has = DVD_signature_create(1, gameplay_rect_collider_id);
	DVD_signature can_not = DVD_signature_create(1, gameplay_controller_id);
	DVD_filter entities = DVD_entities_filter_ex(&has, &can_not);
	
	size_t pivot{ 0 };
	SDL_Rect collider[8];
	SDL_Point normals[8];

	for (int i = 0; i < entities.count; i++) {
		DVD_entity other = entities.list[i];
		SDL_FCircle ball_collider = *gameplay_circle_collider_get(e);
		SDL_FRect other_collider = *gameplay_rect_collider_get(other);
		SDL_FPoint normal;
		if (SDL_IntersectFCircleFRect(ball_collider, other_collider, normal)) { // This is rubbish...

			SDL_FPoint* direction = gameplay_direction_get(e);
			(*direction) = SDL_FPointReflect(*direction, normal);
			DVD_entities_destroy(&other);
			break;
		}
	}
}

void collider_update_position_system(DVD_entity e)
{
	SDL_FPoint position = *gameplay_position_get(e);
	if (gameplay_rect_collider_exists(e)) {
		SDL_FRect* rect = gameplay_rect_collider_get(e);
		if (gameplay_collider_offset_exists(e)) {
			SDL_FPoint offset = *gameplay_collider_offset_get(e);
			position.x += offset.x;
			position.y += offset.y;
		}
		rect->x = position.x;
		rect->y = position.y;
	}
	if (gameplay_circle_collider_exists(e)) {
		SDL_FCircle* circle = gameplay_circle_collider_get(e);
		if (gameplay_collider_offset_exists(e)) {
			SDL_FPoint offset = *gameplay_collider_offset_get(e);
			position.x += offset.x;
			position.y += offset.y;
		}
		circle->x = position.x;
		circle->y = position.y;
	}
	if (gameplay_capsule_collider_exists(e)) {
		SDL_FHorizontalCapsule* capsule = gameplay_capsule_collider_get(e);
		if (gameplay_collider_offset_exists(e)) {
			SDL_FPoint offset = *gameplay_collider_offset_get(e);
			position.x += offset.x;
			position.y += offset.y;
		}
		capsule->position.x = position.x;
		capsule->position.y = position.y;
	}
}

void paddle_ball_collision_system(DVD_entity e)
{
	
	SDL_FCircle* ball_collider = gameplay_circle_collider_get(e);
	for (int i = 0; i < DVD_entities_used_pivot; i++) {
		if (i != e) {
			DVD_entity other = DVD_entities_used[i];
			if (gameplay_rect_collider_exists(other) && gameplay_paddle_downset_manipulator_exists(other)) {
				SDL_FRect other_collider = *gameplay_rect_collider_get(other);
				SDL_FPoint normal;
				SDL_FPoint* direction = gameplay_direction_get(e);
				if (SDL_IntersectFCircleFRect(*ball_collider, other_collider, normal)) { // This is rubbish...

					float centreX = (other_collider.x + other_collider.w * 0.5f);
					float centreY = (other_collider.y + other_collider.h * 0.5f);
					centreY += *gameplay_paddle_downset_manipulator_get(e);
					(*direction) = { ball_collider->x - centreX, ball_collider->y - centreY};

					ball_system(e);
					collider_update_position_system(e);
					break;
				}
			}
		}
	}
}

void debug_rect_collider_system(DVD_entity e)
{
	SDL_FRect rect = *gameplay_rect_collider_get(e);
	if (gameplay_debug_color_exists(e)) {
		engine::draw_rect(rect, *gameplay_debug_color_get(e));
	}
	else {
		engine::draw_rect(rect, { 255, 0, 0, 255 });
	}
}

void debug_circle_collider_system(DVD_entity e)
{
	

	SDL_FCircle circle = *gameplay_circle_collider_get(e);
	engine::draw_circle(circle, { 255, 255, 0, 255 });
}

void debug_capsule_collider_system(DVD_entity e)
{
	

	SDL_FHorizontalCapsule capsule = *gameplay_capsule_collider_get(e);
	engine::draw_capsule(capsule, { 255, 255, 0, 255 });
}


void mouse_position_update_system(DVD_entity e)
{
	
	SDL_Point* position = gameplay_mouse_position_get(e);
	*position = input::get_mouse_position();
}

void debug_circle_collider_position_each(DVD_entity e)
{
	
	SDL_Point position = *gameplay_mouse_position_get(e);
	SDL_FCircle* collider = gameplay_circle_collider_get(e);
	collider->x = position.x;
	collider->y = position.y;

	for (int i = 0; i < DVD_entities_used_pivot; i++) {
		if (i != e) {
			DVD_entity other = DVD_entities_used[i];
			if (gameplay_rect_collider_exists(other)) {
				if (gameplay_debug_color_exists(other)) {
					SDL_FRect other_collider = *gameplay_rect_collider_get(other);
					SDL_FPoint normal;
					SDL_Colour* debug_colour = gameplay_debug_color_get(other);

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

void debug_draw_normals_system(DVD_entity e)
{
	
	SDL_FRect rect = *gameplay_rect_collider_get(e);
	DVD_signature include = DVD_signature_create(2, gameplay_mouse_position_id, gameplay_circle_collider_id);
	DVD_filter filter = DVD_entities_filter(&include);

	for (int i = 0; i < filter.count; i++) {
		DVD_entity other = filter.list[i];
		SDL_FCircle circle_collider = *gameplay_circle_collider_get(other);

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
void buttom_draw_system(DVD_entity e)
{
	SDL_FRect rect = *gameplay_rect_collider_get(e);
	SDL_FRect padding = *gameplay_button_text_padding_get(e);

	SDL_Point mouse_position = input::get_mouse_position();
	SDL_FCircle mouse_collider{ mouse_position.x, mouse_position.y, 1.0f };

	SDL_Colour colour = *gameplay_unhover_colour_get(e);

	SDL_FPoint normal;
	if (SDL_IntersectFCircleFRect(mouse_collider, rect, normal)) {
		if (input::mouse_was_pressed(SDL_BUTTON_LMASK)) {
			(*gameplay_button_event_get(e))();
		}
		colour = *gameplay_hover_colour_get(e);
	}

	engine::draw_rect(rect, colour);
	rect.x += padding.x - padding.w * 0.5f;
	rect.y += padding.y - padding.h * 0.5f;
	rect.w -= padding.w;
	rect.h -= padding.h;

	text t = *gameplay_button_text_get(e);
	engine::draw_text(t.string, rect);
}

void load_menu();
void load_gameplay()
{
	// Construct level
	DVD_entity blocks[10 * 5];
	float block_height = 32.0f;
	float block_width = 64.0f;
	SDL_FPoint offset{ 80, 20 };
	for (int x = 0; x < 10; x++) {
		for (int y = 0; y < 5; y++) {
			DVD_entity block = DVD_entities_create();
			SDL_FPoint position{ offset.x + (x * block_width), offset.y + (y * block_height) };
			SDL_FRect collider{ position.x, position.y, block_width, block_height };
			gameplay_position_set(block, position);
			gameplay_size_set(block, { block_width, block_height });
			gameplay_sprite_index_set(block, { 1, 1 });
			gameplay_sprite_type_set(block, SPRITE_TYPE_TILE);
			gameplay_rect_collider_set(block, collider);
			gameplay_debug_color_set(block, { 255, 0, 0, 255 });
			blocks[(y * 10) + x] = block;
		}
	}

	// Construct player
	DVD_entity player{ DVD_entities_create() };
	gameplay_position_set(player, { 400 - 32, 500 });
	gameplay_size_set(player, { 64, 64 });
	gameplay_sprite_index_set(player, { 1, 1 });
	gameplay_sprite_type_set(player, SPRITE_TYPE_ENTITY);
	gameplay_speed_set(player, 180.0f);
	gameplay_controller_set(player, { SDL_SCANCODE_A, SDL_SCANCODE_D });
	gameplay_collider_offset_set(player, { 0, 0 });
	gameplay_rect_collider_set(player, { 400 - 32, 500 , 64.0f, 16.0f });
	gameplay_paddle_downset_manipulator_set(player, 64.0f);
	gameplay_button_event_set(player, []() {
		DVD_entities_clear();
		DVD_systems_remove_all();
		load_menu();
	});

	//Construct ball
	DVD_entity ball{ DVD_entities_create() };
	gameplay_position_set(ball, { 400 - 32, 400 });
	gameplay_size_set(ball, { 16, 16 });
	gameplay_sprite_index_set(ball, { 1, 1 });
	gameplay_sprite_type_set(ball, SPRITE_TYPE_ENTITY);
	gameplay_speed_set(ball, 240.0f);
	gameplay_direction_set(ball, { 0.0f, -1.0f });
	gameplay_collider_offset_set(ball, { 8, 8 });
	gameplay_circle_collider_set(ball, { 400 - 32, 500, 8.0f });


	
	DVD_signature ball_signature = DVD_signature_create(4, gameplay_position_id, gameplay_speed_id, gameplay_direction_id, gameplay_circle_collider_id);
	DVD_systems_add_on_update(DVD_signature_create(3, gameplay_controller_id, gameplay_position_id, gameplay_speed_id), player_system);
	DVD_systems_add_on_update(ball_signature, ball_system);

	DVD_systems_add_on_update(DVD_signature_create(1, gameplay_position_id), collider_update_position_system);
	DVD_systems_add_on_update(ball_signature, ball_collision_system);
	DVD_systems_add_on_update(ball_signature, paddle_ball_collision_system);

	DVD_systems_add_on_update(DVD_signature_create(1, gameplay_mouse_position_id), mouse_position_update_system);
	//DVD_systems_add_on_update(signature_create(2, mouse_position_id, circle_collider_id), debug_circle_collider_position_each);

	DVD_systems_add_on_render(DVD_signature_create(4, gameplay_sprite_type_id, gameplay_sprite_index_id, gameplay_position_id, gameplay_size_id), draw_system_each);
	DVD_systems_add_on_render(DVD_signature_create(1, gameplay_rect_collider_id), debug_rect_collider_system);
	DVD_systems_add_on_render(DVD_signature_create(1, gameplay_circle_collider_id), debug_circle_collider_system);
	DVD_systems_add_on_render(DVD_signature_create(1, gameplay_capsule_collider_id), debug_capsule_collider_system);
	DVD_systems_add_on_render(DVD_signature_create(1, gameplay_rect_collider_id), debug_draw_normals_system);
}

void load_menu()
{
	DVD_entity start_button{ DVD_entities_create() };
	gameplay_rect_collider_set(start_button, {0, 0, 128, 64 });
	gameplay_button_text_set(start_button, { "Start!" });
	gameplay_button_text_padding_set(start_button, { 16, 16, 16, 16 });
	gameplay_hover_colour_set(start_button, { 255, 255, 0, 255 });
	gameplay_unhover_colour_set(start_button, { 255, 0, 255, 255 });
	gameplay_button_event_set(start_button, []() {
		DVD_systems_remove_all();
		DVD_entities_clear();
		load_gameplay();
	});

	DVD_entity create_level_button{ DVD_entities_create_copy(start_button) };
	gameplay_rect_collider_set(create_level_button, { 0, 64, 128, 64 });
	gameplay_button_text_set(create_level_button, { "Create Level!" });
	gameplay_button_event_set(create_level_button, []() {
		printf("I love you!\n");
	});

	DVD_entity load_button{ DVD_entities_create_copy(start_button) };
	gameplay_rect_collider_set(load_button, { 0, 0 + 128, 128, 64 });
	gameplay_button_text_set(load_button, { "Load Level!" });
	gameplay_button_event_set(load_button, []() {
		printf("I hate you!\n");
	});

	DVD_entity save_level{ DVD_entities_create_copy(start_button) };
	gameplay_rect_collider_set(save_level, { 0, 0 + 128 + 64, 128, 64 });
	gameplay_button_text_set(save_level, { "Save Level!" });
	gameplay_button_event_set(save_level, []() {
		printf("Go fuck yourself!\n");
	});

	DVD_systems_add_on_render(DVD_signature_create_from_entity(start_button), buttom_draw_system);
}

// Everything below meant for porting all this stuff on top to C++ : ) 

// Utility for... template meta programming
template<typename... pack>
struct typename_pack {};

template<typename, typename>
struct pack_union;

template<typename T, typename... Set>
constexpr bool set_contains_unions = (std::is_same_v<T, Set> || ...);

template<typename... Dst, typename Set_T, typename... Set_Rest>
struct pack_union<typename_pack<Dst...>, typename_pack<Set_T, Set_Rest...>> {
	using result = typename pack_union<
		std::conditional_t<
			set_contains_unions<Set_T, Dst...>, 
			typename_pack<Dst...>, 
			typename_pack<Set_T, Dst...>>,
		typename_pack<Set_Rest...>
	>::result;
};

template<typename... Dst>
struct pack_union<typename_pack<Dst...>, typename_pack<>> {
	using result = typename_pack<Dst...>;
};

template<typename, typename, typename>
struct pack_intersection;

template<typename T, typename... Set>
constexpr bool set_contains_intersection = (std::is_same_v<T, Set> || ...);

template<typename... Dst, typename Set_T, typename... Set_Rest, typename... union_set>
struct pack_intersection<typename_pack<Dst...>, typename_pack<Set_T, Set_Rest...>, typename_pack<union_set...>> {
	using result = typename pack_intersection<
		typename_pack<Dst...>,
		typename_pack<Set_Rest...>,						
		std::conditional_t<
			set_contains_intersection<Set_T, Dst...>,
			typename_pack<Set_T, union_set...>,				 
			typename_pack<union_set...>>
		>::result;
};

template<typename... Dst, typename... union_set>
struct pack_intersection<typename_pack<Dst...>, typename_pack<>, typename_pack<union_set...>> {
	using result = typename_pack<union_set...>;
};


template<typename... types>
struct type_list;

template<>
struct type_list<>
{
	template<typename search>
	static constexpr bool contains()
	{
		return false;
	}
};

template <typename current_type, typename... types>
struct type_list<current_type, types...> : type_list<types...>
{
	using current = current_type;
	using base = type_list<types...>;

	template<typename... other_types>
	using intersection_result = pack_intersection<typename_pack<current_type, types...>, typename_pack<other_types...>, typename_pack<>>::result;

	template<typename... other_types>
	using union_result = pack_union<typename_pack<current_type, types...>, typename_pack<other_types...>>::result;
};

template<const size_t size, typename ...rest>
struct columns;

template<const size_t size>
struct columns<size>
{
	using current = columns<size>;
	using resource = decltype(nullptr);
	using base = decltype(nullptr);

	template<typename find_type, const bool is_valid = false>
	auto get()
	{
		static_assert(is_valid, "Type does not exist in columns!");
	}
};

template<const size_t size, typename parent_type_pack, typename type_pack>
struct sub_columns;

template<const size_t size, typename... parent_types>
struct sub_columns<size, typename_pack<parent_types...>, typename_pack<>>
{
	columns<size, parent_types...>* main;

	sub_columns(columns<size, parent_types...>* main)
	{
		this->main = main;
	}
};

template<const size_t size, typename... parent_types, typename type, typename ...rest>
struct sub_columns<size, typename_pack<parent_types...>, typename_pack<type, rest...>> 
	: sub_columns<size, typename_pack<parent_types...>, typename_pack<rest...>>
{
	using base = sub_columns<size, typename_pack<parent_types...>, typename_pack<rest...>>;
	using resource = type;
	type* column;

	sub_columns(columns<size, parent_types...>* main)
		: base(main)
	{
		column = main->get<type>()->data;
	}

	template<typename... types>
	void ForEach(std::function<void(types...)> function)
	{

	}
};

template<const size_t size, typename type, typename ...rest>
struct columns<size, type, rest...> : columns<size, rest...>
{
	using types = type_list<type, rest...>;
	using current = columns<size, type, rest...>;
	using base = columns<size, rest...>;
	using resource = type;

	resource data[size]{};

	resource& operator[](size_t index)
	{
		return data[index];
	}

	template<typename find_type>
	auto get()
	{
		using current = resource;
		if constexpr (std::is_same<find_type, current>::value) {
			return this;
		}
		else {
			return base::template get<find_type>();
		}
	};

	template<typename... find_types>
	auto where()
	{
		using intersection = types::template intersection_result<find_types...>;
		sub_columns<size, typename_pack<type, rest...>, intersection> sub(this);
		return sub;
	}
};

template<const size_t size, typename type, typename ...rest>
struct table
{
	using this_table = table<size, type, rest...>;
	using types_in_table = type_list<type, rest...>;
	const size_t count = size;

	bool valid_lookup[size * (sizeof...(rest) + 1)]{false};
	columns<size, type, rest...> cols;

	template<typename type_to_search>
	static constexpr bool exists = this_table::types_in_table::template contains<type_to_search>();

	template<typename type_to_search>
	auto get() -> decltype(cols.get<type_to_search>())
	{
		return cols.get<type_to_search>();
	}

	template<typename... find_types>
	auto where()
	{
		return cols.where<find_types...>();
	}
};


template<typename find_type, const size_t count, typename table_type, typename ...cols_rest>
auto has(table<count, table_type, cols_rest...> t)
{
	constexpr bool type_exists = table<count, table_type, cols_rest...>::types_in_table::template contains<find_type>();
	if constexpr (type_exists) {
		return find<find_type>(&t.cols);
	}
	else {
		static_assert(type_exists, "The type you are looking for does not exist in table");
	}
}


int main()
{
	table<256, float, int> main;
	//auto stuff = main.exists<double>();
	auto test = main.get<int>();
	auto that = main.exists<float>;
	std::function<void(int)> func = [](int) {

	};
	main.where<int, float>().ForEach(func);
	
	
	using some_list = type_list<int, float, double, std::string, wchar_t>;
	using intersection_type = some_list::intersection_result<char, int, wchar_t>;
	using union_type = some_list::union_result<char, int, int>;

	DVD_entities_initialise();
	engine::initialise(SCREEN_WIDTH, SCREEN_HEIGHT);
	engine::load_entities_texture("res/objects.png");
	engine::set_entity_source_size(32, 32);

	engine::load_tiles_texture("res/rock_packed.png");
	engine::set_tile_source_size(18, 18);

	engine::load_font("res/roboto.ttf");
	load_menu();

	bool running = true;
	events::add(SDL_QUIT, [&running](const SDL_Event&) { running = false; });
	events::add(SDL_KEYDOWN, 
	[&running](const SDL_Event& e)
	{
		if (e.key.keysym.scancode == SDL_SCANCODE_ESCAPE) running = false;
	});

	Uint64 prev_ticks = SDL_GetPerformanceCounter();
	while (running) // Each frame
	{
		Uint64 ticks = SDL_GetPerformanceCounter();
		Uint64 delta_ticks = ticks - prev_ticks;
		prev_ticks = ticks;
		delta_time = ((float)delta_ticks / SDL_GetPerformanceFrequency());

		events::run();
		input::run();

		DVD_systems_run();
	}
}
