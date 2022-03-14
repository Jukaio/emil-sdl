
#pragma once
#include "SDL/SDL_rect.h"

struct SDL_Texture;


namespace engine
{
	bool load_texture(const char* path, SDL_Texture*& out_texture);

	void initialise(int width, int height);
	void load_entities_texture(const char* path);
	void load_tiles_texture(const char* path);
	void set_entity_source_size(int width, int height);
	void set_tile_source_size(int width, int height);

	void draw_entity(const SDL_Point& sprite_index, const SDL_FRect& dst);
	void draw_tile(const SDL_Point& sprite_index, const SDL_FRect& dst);
	void draw_rect(const SDL_FRect& rect, SDL_Colour colour);

	void render_clear();
	void render_present();
}