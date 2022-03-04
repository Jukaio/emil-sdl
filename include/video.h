
#pragma once
#include "SDL/SDL_rect.h"

struct SDL_Texture;


namespace video
{
	bool load_texture(const char* path, SDL_Texture*& out_texture);

	void initialise(int width, int height);
	void load_entities_texture(const char* path);
	void load_tiles_texture(const char* path);
	void set_entity_size(int width, int height);
	void set_tile_size(int width, int height);

	void poll();
	void draw_tile(int x, int y, const SDL_Rect& dst);

	void clear();
	void present();

	class entity
	{
	public:
		entity() = default;
		entity(int x, int y);
		void draw(const SDL_Rect& dst);

	private:
		int x = 0;
		int y = 0;
	};

	class tile
	{
	public:
		tile() = default;
		tile(int x, int y);
		void draw(const SDL_Rect& dst);

	private:
		int x = 0;
		int y = 0;
	};
}