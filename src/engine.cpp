
#include "engine.h"
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include <SDL/SDL_image.h>

void SDL_RenderDrawCircleF(SDL_Renderer* renderer, const SDL_FCircle* circle)
{
	int resolution = 48;
	float step = (2 * 3.14f) / resolution;

	for (int i = 0; i < resolution; ++i)
	{
		float angle = step * i;
		float x1 = cos(angle);
		float y1 = sin(angle);

		float next_angle = step * (i + 1);
		float x2 = cos(next_angle);
		float y2 = sin(next_angle);

		SDL_RenderDrawLineF(renderer,
			x1 * circle->radius + circle->x,
			y1 * circle->radius + circle->y,
			x2 * circle->radius + circle->x,
			y2 * circle->radius + circle->y
		);
	}
}

namespace engine
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

	void draw(SDL_Texture*& texture, const SDL_Rect& src, const SDL_Rect& dst)
	{
		SDL_RenderCopy(renderer, texture, &src, &dst);
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

	void draw_circle(const SDL_FCircle& circle, SDL_Colour colour) 
	{
		SDL_Colour p_c;
		SDL_GetRenderDrawColor(renderer, &p_c.r, &p_c.g, &p_c.b, &p_c.a);

		SDL_Colour c = colour;
		SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
		SDL_RenderDrawCircleF(renderer, &circle);
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