
#include "engine.h"
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include <SDL/SDL_image.h>
#include <stdio.h>
void SDL_FPointNormalise(SDL_FPoint* v)
{
	float length = sqrtf(v->x * v->x + v->y * v->y);
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

void SDL_RenderDrawCircleF(SDL_Renderer* renderer, const SDL_FCircle* circle)
{
	int resolution = 48;
	float step = (2 * 3.14f) / resolution;

	for (int i = 0; i < resolution; ++i)
	{
		float angle = step * i;
		float x1 = cosf(angle);
		float y1 = sinf(angle);

		float next_angle = step * (i + 1);
		float x2 = cosf(next_angle);
		float y2 = sinf(next_angle);

		SDL_RenderDrawLineF(renderer,
			x1 * circle->radius + circle->x,
			y1 * circle->radius + circle->y,
			x2 * circle->radius + circle->x,
			y2 * circle->radius + circle->y
		);
	}
}

float SDL_FPointDot(SDL_FPoint lhs, SDL_FPoint rhs)
{
	return lhs.x * rhs.x + lhs.y * rhs.y;
}

/*
void Reflect(VEC3 &out, const VEC3 &incidentVec, const VEC3 &normal)
{
  out = incidentVec - 2.f * Dot(incidentVec, normal) * normal;
}
*/

SDL_FPoint SDL_FPointReflect(SDL_FPoint direction, SDL_FPoint normal)
{
	float factor = 2.0f * SDL_FPointDot(normal, direction);
	return { direction.x - factor * normal.x, direction.y - factor * normal.y };
}


bool SDL_IntersectFCircleFRect(const SDL_FCircle& circle, const SDL_FRect& rect, SDL_FPoint& out_normal)
{
	out_normal = { 0, 0 };

	float x = circle.x;
	float y = circle.y;
	if (circle.x < rect.x)
	{
		x = rect.x;
	}
	else if (circle.x > rect.x + rect.w)
	{
		x = rect.x + rect.w;
	}

	if (circle.y < rect.y)
	{
		y = rect.y;
	}
	else if (circle.y > rect.y + rect.h)
	{
		y = rect.y + rect.h;
	}

	float distX = circle.x - x;
	float distY = circle.y - y;
	float distance = sqrtf((distX * distX) + (distY * distY));

	if (distance <= circle.radius)
	{
		SDL_FPoint cp{ rect.x + rect.w * 0.5f, rect.y + rect.h * 0.5f };
		if (circle.x < cp.x)
		{
			out_normal.x = -1;
		}
		else if (circle.x > cp.x)
		{
			out_normal.x = 1;
		}

		if (circle.y < cp.y)
		{
			out_normal.y = -1;
		}
		else if (circle.y > cp.y)
		{
			out_normal.y = 1;
		}

		out_normal = SDL_FPointGetNormalised(out_normal);
		if (out_normal.x != 0.0f && out_normal.y != 0.0f) {
			SDL_FPoint distance;
			distance.x = fabsf((circle.x - cp.x) / rect.w);
			distance.y = fabsf((circle.y - cp.y) / rect.h);
			

			if (distance.x > distance.y) {
				out_normal.y = 0.0f;
			}
			else {
				out_normal.x = 0.0f;
			}
			out_normal = SDL_FPointGetNormalised(out_normal);
		}
		return true;
	}
	return false;
}

bool SDL_IntersectFCircleFCircle(const SDL_FCircle& a, const SDL_FCircle& b, SDL_FPoint& out_normal)
{
	out_normal = { 0, 0 };

	float distX = a.x - b.x;
	float distY = a.y - b.y;
	float distance = sqrtf((distX * distX) + (distY * distY));

	if (distance <= a.radius + b.radius) {
		out_normal.x = a.x - b.x;
		out_normal.y = a.y - b.y;
		SDL_FPointNormalise(&out_normal);
		return true;
	}
	return false;
}

bool SDL_IntersectFCircleFCapsule(const SDL_FCircle& a, const SDL_FHorizontalCapsule& b, SDL_FPoint& out_normal)
{
	SDL_FRect rect{ b.position.x - b.size.x * 0.5f, b.position.y - b.size.y * 0.5f, b.size.x, b.size.y };
	if (SDL_IntersectFCircleFRect(a, rect, out_normal)) {
		return true;
	}

	SDL_FCircle circle_a;
	SDL_FCircle circle_b;
	circle_a.x = b.position.x - b.size.x * 0.5f;
	circle_a.y = b.position.y;
	circle_a.radius = b.size.y;
	circle_b.x = b.position.x + b.size.x * 0.5f;
	circle_b.y = b.position.y;
	circle_b.radius = b.size.y;
	if (SDL_IntersectFCircleFCircle(a, circle_a, out_normal)) {
		return true;
	}
	return SDL_IntersectFCircleFCircle(a, circle_b, out_normal);
}

namespace engine
{
	static SDL_Window* window{ nullptr };
	static SDL_Renderer* renderer{ nullptr };

	static SDL_Texture* entity_texture{ nullptr };
	static SDL_Texture* tiles_texture{ nullptr };
	static TTF_Font* font{ nullptr };

	static SDL_Point entity_size;
	static SDL_Point tile_size;

	bool load_texture(const char* path, SDL_Texture*& out_texture)
	{
		out_texture = IMG_LoadTexture(renderer, path);
		return out_texture != nullptr;
	}

	void load_font(const char* path)
	{
		font = TTF_OpenFont(path, 72);
		if (font == nullptr) {
			// Error
		}
	}

	void free_texture(SDL_Texture*& texture)
	{
		if (texture != nullptr) {
			SDL_DestroyTexture(texture);
			texture = nullptr;
		}
	}

	void initialise(int width, int height)
	{
		SDL_Init(SDL_INIT_EVERYTHING);
		SDL_CreateWindowAndRenderer(width, height, 0, &window, &renderer);
		IMG_Init(IMG_INIT_JPG);
		TTF_Init();
	}

	void load_entities_texture(const char* path)
	{
		free_texture(entity_texture);
		if (load_texture(path, entity_texture)) {
			// Error here
		}
	}

	void set_entity_source_size(int width, int height)
	{
		entity_size = { width, height };
	}

	void set_tile_source_size(int width, int height)
	{
		tile_size = { width, height };
	}

	void load_tiles_texture(const char* path)
	{
		free_texture(tiles_texture);
		if (load_texture(path, tiles_texture)) {
			// Error here
		}
	}

	void draw(SDL_Texture*& texture, const SDL_Rect& src, const SDL_FRect& dst)
	{
		SDL_RenderCopyF(renderer, texture, &src, &dst);
	}

	void draw_tile(const SDL_Point& tile, const SDL_FRect& dst)
	{
		SDL_Rect src{ tile.x * tile_size.x, tile.y * tile_size.y, tile_size.x, tile_size.y };
		SDL_RenderCopyF(renderer, tiles_texture, &src, &dst);
	}

	void draw_rect(const SDL_FRect& rect, SDL_Colour colour)
	{
		SDL_Colour p_c;
		SDL_GetRenderDrawColor(renderer, &p_c.r, &p_c.g, &p_c.b, &p_c.a);

		SDL_Colour c = colour;
		SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
		SDL_RenderDrawRectF(renderer, &rect);
		SDL_SetRenderDrawColor(renderer, p_c.r, p_c.g, p_c.b, p_c.a);
	}
	void draw_line(const SDL_FPoint a, const SDL_FPoint b, SDL_Colour colour)
	{
		SDL_Colour p_c;
		SDL_GetRenderDrawColor(renderer, &p_c.r, &p_c.g, &p_c.b, &p_c.a);
		SDL_Colour c = colour;
		SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);

		SDL_RenderDrawLineF(renderer, a.x, a.y, b.x, b.y);

		SDL_SetRenderDrawColor(renderer, p_c.r, p_c.g, p_c.b, p_c.a);

	}
	void draw_circle(const SDL_FCircle& circle, SDL_Colour colour) 
	{
		SDL_Colour p_c;
		SDL_GetRenderDrawColor(renderer, &p_c.r, &p_c.g, &p_c.b, &p_c.a);

		SDL_Colour c = colour;
		SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);

		SDL_RenderDrawCircleF(renderer, &circle);

		SDL_SetRenderDrawColor(renderer, p_c.r, p_c.g, p_c.b, p_c.a);
	}
	void draw_text(const char* text, const SDL_FRect& dst)
	{
		SDL_Surface* surface = TTF_RenderText_Blended(font, text, { 255, 255, 255, 255 });
		SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
		uint32_t format;
		int access;
		int width;
		int height;
		SDL_QueryTexture(texture, &format, &access, &width, &height);
		draw(texture, { 0, 0, width, height }, dst);
		SDL_DestroyTexture(texture);
		SDL_FreeSurface(surface);
	}
	void draw_capsule(const SDL_FHorizontalCapsule& capsule, SDL_Colour colour)
	{
		SDL_Colour p_c;
		SDL_GetRenderDrawColor(renderer, &p_c.r, &p_c.g, &p_c.b, &p_c.a);
		SDL_Colour c = colour;
		SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);

		// If size is the same on each side, just draw a circle
		if (capsule.size.x > capsule.size.y - 0.00001f
			&& capsule.size.x < capsule.size.y + 0.00001f) {
			SDL_FCircle circle = { capsule.position.x, capsule.position.y, capsule.size.x };
			SDL_RenderDrawCircleF(renderer, &circle);
		}
		else {
			SDL_FCircle a;
			SDL_FCircle b;
			a.x = capsule.position.x - capsule.size.x * 0.5f;
			a.y = capsule.position.y;
			a.radius = capsule.size.y;
			b.x = capsule.position.x + capsule.size.x * 0.5f;
			b.y = capsule.position.y;
			b.radius = capsule.size.y;

			SDL_RenderDrawLine(renderer, a.x, capsule.position.y - capsule.size.y, b.x, capsule.position.y - capsule.size.y);
			SDL_RenderDrawLine(renderer, a.x, capsule.position.y + capsule.size.y, b.x, capsule.position.y + capsule.size.y);
			SDL_RenderDrawCircleF(renderer, &a);
			SDL_RenderDrawCircleF(renderer, &b);

		}
		SDL_SetRenderDrawColor(renderer, p_c.r, p_c.g, p_c.b, p_c.a);
	}

	void draw_entity(const SDL_Point& entity, const SDL_FRect& dst)
	{
		SDL_Rect src{ entity.x * entity_size.x, entity.y * entity_size.y, entity_size.x, entity_size.y };
		SDL_RenderCopyF(renderer, entity_texture, &src, &dst);
	}

	void render_clear()
	{
		SDL_RenderClear(renderer);
	}

	void render_present()
	{
		SDL_RenderPresent(renderer);
	}
}

