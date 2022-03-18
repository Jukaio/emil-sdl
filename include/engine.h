
#pragma once
#include "SDL/SDL_rect.h"

struct SDL_Texture;

struct SDL_FCircle
{
	float x, y;
	float radius;
};

struct SDL_FHorizontalCapsule
{
	SDL_FPoint position;
	SDL_FPoint size;
};

void SDL_FPointNormalise(SDL_FPoint* v);
SDL_FPoint SDL_FPointGetNormalised(SDL_FPoint v);
float SDL_FPointDot(SDL_FPoint lhs, SDL_FPoint rhs);
SDL_FPoint SDL_FPointReflect(SDL_FPoint direction, SDL_FPoint normal);
bool SDL_IntersectFCircleFCircle(const SDL_FCircle& a, const SDL_FCircle& b, SDL_FPoint& out_normal);
bool SDL_IntersectFCircleFRect(const SDL_FCircle& circle, const SDL_FRect& rect, SDL_FPoint& out_normal);
bool SDL_IntersectFCircleFCapsule(const SDL_FCircle& a, const SDL_FHorizontalCapsule& b, SDL_FPoint& out_normal);


namespace engine
{
	bool load_texture(const char* path, SDL_Texture*& out_texture);
	void load_font(const char* path);
	void initialise(int width, int height);
	void load_entities_texture(const char* path);
	void load_tiles_texture(const char* path);
	void set_entity_source_size(int width, int height);
	void set_tile_source_size(int width, int height);
	
	void draw_text(const char* text, const SDL_FRect& dst);
	void draw_entity(const SDL_Point& sprite_index, const SDL_FRect& dst);
	void draw_tile(const SDL_Point& sprite_index, const SDL_FRect& dst);
	void draw_rect(const SDL_FRect& rect, SDL_Colour colour);
	void draw_circle(const SDL_FCircle& circle, SDL_Colour colour);
	void draw_capsule(const SDL_FHorizontalCapsule& capsule, SDL_Colour colour);
	void draw_line(const SDL_FPoint a, const SDL_FPoint b, SDL_Colour colour);
	void render_clear();
	void render_present();
}