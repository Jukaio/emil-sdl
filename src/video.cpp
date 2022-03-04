
#include "video.h"
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

namespace video
{
	static SDL_Window* window{ nullptr };
	static SDL_Renderer* renderer{ nullptr };

	static SDL_Texture* entity_texture{ nullptr };
	static SDL_Texture* tiles_texture{ nullptr };

	static SDL_Point entity_size;
	static SDL_Point tile_size;

	bool load_texture(const char* path, SDL_Texture*& out_texture)
	{
		out_texture = IMG_LoadTexture(renderer, path);
		return out_texture != nullptr;
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
	}

	void load_entities_texture(const char* path)
	{
		free_texture(entity_texture);
		if (load_texture(path, entity_texture)) {
			// Error here
		}
	}

	void set_entity_size(int width, int height)
	{
		entity_size = { width, height };
	}

	void set_tile_size(int width, int height)
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

	void poll()
	{
	}

	void draw(SDL_Texture*& texture, const SDL_Rect& src, const SDL_Rect& dst)
	{
		SDL_RenderCopy(renderer, texture, &src, &dst);
	}

	void draw_tile(int x, int y, const SDL_Rect& dst)
	{
		SDL_Rect src{ x * tile_size.x, y * tile_size.y, tile_size.x, tile_size.y };
		SDL_RenderCopy(renderer, tiles_texture, &src, &dst);
	}

	void draw_entity(int x, int y, const SDL_Rect& dst)
	{
		SDL_Rect src{ x * tile_size.x, y * tile_size.y, tile_size.x, tile_size.y };
		SDL_RenderCopy(renderer, entity_texture, &src, &dst);
	}

	void clear()
	{
		SDL_RenderClear(renderer);
	}

	void present()
	{
		SDL_RenderPresent(renderer);
	}

	entity::entity(int x, int y)
		: x(x)
		, y(y)
	{

	}

	void entity::draw(const SDL_Rect& dst)
	{
		video::draw_entity(x, y, dst);
	}


	tile::tile(int x, int y)
		: x(x)
		, y(y)
	{

	}

	void tile::draw(const SDL_Rect& dst)
	{
		video::draw_tile(x, y, dst);
	}


}