#include <stdio.h>
#include <SDL/SDL.h>
#include <iostream>

#include "video.h"

int main()
{
	SDL_Init(SDL_INIT_EVERYTHING);

	// Title, x, y, width, height, flags
	// To keep the program running:
	bool running = true;
	video::initialise(800, 600);
	video::load_entities_texture("res/objects.png");
	video::load_tiles_texture("res/tiles.png");
	video::set_entity_size(32, 32);
	video::set_tile_size(32, 32);
	//video::sprite sprite{ { 0, 0, 700, 580 / 2 }, texture };
	video::entity sprite1{ 0, 0 };
	video::entity sprite2{ 1, 0 };
	
	while (running) // Each frame
	{
		SDL_Event event;
		while (SDL_PollEvent(&event)) // Get first in queue
		{
			switch (event.type)
			{
				case SDL_QUIT:
				{
					running = false;
					break;
				}
			case SDL_KEYDOWN:
				{
					int scancode = event.key.keysym.scancode;
					if (scancode == SDL_SCANCODE_ESCAPE)
					{
						running = false;
					}
					break;
				}
			}
		}
		
		video::clear();

		sprite1.draw({ 0, 0, 32, 32 });
		sprite2.draw({ 32, 32, 32, 32 });

		//video::draw();
		video::present();

		// Update
		// Render
	}
}
