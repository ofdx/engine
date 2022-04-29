#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

#include <iostream>
#include <map>
#include <unordered_map>
#include <string>
#include <sstream>
#include <list>
#include <cmath>
#include <vector>
#include <regex>
#include <filesystem>

int SCREEN_WIDTH  = 384;
int SCREEN_HEIGHT = 216;
int SCREEN_FPS    = 120;

#define PI 3.14159265359

using namespace std;

int render_scale = 5;
int render_scale_max = 5;

#include "loader.h"
#include "utility.h"

#include "ables/drawable.h"
#include "ables/movable.h"
#include "ables/clickable.h"
#include "ables/typable.h"

#include "gui/cardpanel.h"
#include "gui/text.h"
#include "gui/button.h"

#include "scene.h"

// Particle effects.
#include "fx/particle.h"
#include "fx/snow.h"

// Game code.
#include "game/__game.h"

// Default values for macros the game-specific code should define.
#ifndef GAME_NAME
 #define GAME_NAME "untitled"
#endif
#ifndef GAME_AUTHOR
 #define GAME_AUTHOR "Krakissi"
#endif
#ifndef GAME_SAVEPATH
 #define GAME_SAVEPATH "untitled"
#endif
#ifndef GAME_VERSION
 #define GAME_VERSION string("v0.01")
#endif

string get_save_path(){
	static char *pref_path = SDL_GetPrefPath(GAME_AUTHOR, GAME_SAVEPATH);
	return pref_path;
}

int main(int argc, char **argv){
#include "assetblob"

	// Load preferences (might override render_scale or volume setting).
	// TODO
	//PersistenceHandler *preferences = PersistenceHandler::inst();

	SDL_Event event;

	if(SDL_Init(SDL_INIT_EVERYTHING)){
		cout << "Failed to init SDL: " << SDL_GetError() << endl;
		return -1;
	}
	SDL_ShowCursor(SDL_DISABLE);

	// Enable audio
	if(Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, MIX_CHANNELS, 1024)){
		cout << "Failed to initialize audio." << endl;
		return -2;
	}

	// Automatically set default value based on desktop resolution.
	{
		SDL_DisplayMode mode;

		if(!SDL_GetDesktopDisplayMode(0, &mode)){
			int width = mode.w / SCREEN_WIDTH;
			int height = mode.h / SCREEN_HEIGHT;

			render_scale_max = ((width > height) ? height : width) - 1;
			render_scale = render_scale_max - 1;
		}

		/*if(preferences->data->render_scale && (preferences->data->render_scale <= render_scale_max))
			render_scale = preferences->data->render_scale;*/
	}


	SDL_Window *win = SDL_CreateWindow(GAME_NAME, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, (SCREEN_WIDTH * render_scale), (SCREEN_HEIGHT * render_scale), SDL_WINDOW_SHOWN);

	// Create a renderer for the window we'll draw everything to.
	SDL_Renderer *rend = SDL_CreateRenderer(win, -1, 0);
	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);
	SDL_RenderSetLogicalSize(rend, SCREEN_WIDTH, SCREEN_HEIGHT);

	map<int, bool> *keys = new map<int, bool>();

	// Create controller and load the first scene.
	Scene::Controller *ctrl = new Scene::Controller(win, rend, render_scale_max, keys);

	registerScenes(ctrl);
	ctrl->set_scene(Scene::create(ctrl, "intro"));
	//ctrl->set_volume(preferences->data->volume);

	/*if(preferences->data->fullscreen){
		SDL_SetWindowFullscreen(ctrl->win, SDL_WINDOW_FULLSCREEN_DESKTOP);
		ctrl->fullscreen = true;
	}*/

	int ticks_last = SDL_GetTicks();
	while(1){
		int ticks_now = SDL_GetTicks();
		const int ticks = ticks_now - ticks_last;

		// Check for an event without waiting.
		while(SDL_PollEvent(&event)){
			switch(event.type){
				case SDL_QUIT:
					goto quit;

				// Map out keystates
				case SDL_KEYUP:
					(*keys)[event.key.keysym.sym] = false;
					break;
				case SDL_KEYDOWN:
					(*keys)[event.key.keysym.sym] = true;
					ctrl->keydown(event.key);
					break;

				case SDL_MOUSEMOTION:
				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP:
				case SDL_MOUSEWHEEL:
				case SDL_FINGERDOWN:
				case SDL_FINGERUP:
					ctrl->check_mouse(event);
					break;

				case SDL_WINDOWEVENT:
					// Handle window sub-events.
					switch(event.window.event){
						case SDL_WINDOWEVENT_FOCUS_LOST:
							// Disable fullscreen if we lose focus, because SDL doesn't handle it well.
							if(ctrl->fullscreen){
								SDL_SetWindowFullscreen(ctrl->win, 0);
								ctrl->fullscreen = false;
							}
							break;
					}
					break;
			}
		}

		// Draw cursor and flip to display this frame.
		ctrl->draw_cursor();
		SDL_RenderPresent(rend);

		// Draw the current scene.
		ctrl->draw(ticks);

		// Delay to limit to approximately SCREEN_FPS.
		{
			int frame_time = SDL_GetTicks() - ticks_now;
			int delta = (1000 / SCREEN_FPS) - frame_time - 1;

			if(delta > 0)
				SDL_Delay(delta);
		}

		// Update tick counter.
		ticks_last = ticks_now;
	}

quit:
	// Clean up and close SDL library.
	Mix_CloseAudio();
	SDL_Quit();

	return 0;
}
