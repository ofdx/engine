#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

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

#define SCREEN_WIDTH  384
#define SCREEN_HEIGHT 216
#define SCREEN_FPS    120

using namespace std;

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

struct EngineContext {
	bool run;
	SDL_Window *pWin;
	SDL_Renderer *pRend;
	map<int, bool> *pKeys;
	Scene::Controller *pCtrl;

	int ticks_last;

	EngineContext() :
		run(true),
		pKeys(new map<int, bool>())
	{
		// Automatically set default value based on desktop resolution.
		int render_scale = 5;
		int render_scale_max = 5;
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

		pWin = SDL_CreateWindow(GAME_NAME, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, (SCREEN_WIDTH * render_scale), (SCREEN_HEIGHT * render_scale), SDL_WINDOW_SHOWN);
		pRend = SDL_CreateRenderer(pWin, -1, 0);

		SDL_SetRenderDrawBlendMode(pRend, SDL_BLENDMODE_BLEND);
		SDL_RenderSetLogicalSize(pRend, SCREEN_WIDTH, SCREEN_HEIGHT);

		// Create controller and load the first scene.
		pCtrl = new Scene::Controller(pWin, pRend, render_scale, render_scale_max, pKeys);
		registerScenes(pCtrl);
		pCtrl->set_scene(Scene::create(pCtrl, "intro"));

		//pCtrl->set_volume(preferences->data->volume);

		/*if(preferences->data->fullscreen){
			SDL_SetWindowFullscreen(pCtrl->win, SDL_WINDOW_FULLSCREEN_DESKTOP);
			pCtrl->fullscreen = true;
		}*/

		ticks_last = SDL_GetTicks();
	}
};

static void gameloop(void *pCtxVoid){
	EngineContext *pCtx = (EngineContext*) pCtxVoid;

	while(pCtx->run){
		int ticks_now = SDL_GetTicks();
		const int ticks = ticks_now - pCtx->ticks_last;
		SDL_Event event;

		// Check for an event without waiting.
		while(SDL_PollEvent(&event)){
			switch(event.type){
#ifndef __EMSCRIPTEN__
				case SDL_QUIT:
					pCtx->run = false;
					break;
#endif

				// Map out keystates
				case SDL_KEYUP:
					(*(pCtx->pKeys))[event.key.keysym.sym] = false;
					break;
				case SDL_KEYDOWN:
					(*(pCtx->pKeys))[event.key.keysym.sym] = true;
					pCtx->pCtrl->keydown(event.key);
					break;

				case SDL_MOUSEMOTION:
				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP:
				case SDL_MOUSEWHEEL:
				case SDL_FINGERDOWN:
				case SDL_FINGERUP:
					pCtx->pCtrl->check_mouse(event);
					break;

				case SDL_WINDOWEVENT:
					// Handle window sub-events.
					switch(event.window.event){
						case SDL_WINDOWEVENT_FOCUS_LOST:
							// Disable fullscreen if we lose focus, because SDL doesn't handle it well.
							if(pCtx->pCtrl->fullscreen){
								SDL_SetWindowFullscreen(pCtx->pCtrl->win, 0);
								pCtx->pCtrl->fullscreen = false;
							}
							break;
					}
					break;
			}
		}

		// Draw cursor and flip to display this frame.
		pCtx->pCtrl->draw_cursor();
		SDL_RenderPresent(pCtx->pRend);

		// Draw the current scene.
		pCtx->pCtrl->draw(ticks);

		// Delay to limit to approximately SCREEN_FPS.
		{
			int frame_time = SDL_GetTicks() - ticks_now;
			int delta = (1000 / SCREEN_FPS) - frame_time - 1;

			if(delta > 0)
				SDL_Delay(delta);
		}

		// Update tick counter.
		pCtx->ticks_last = ticks_now;
	}
}

int main(int argc, char **argv){
#include "assetblob"

	// Load preferences (might override render_scale or volume setting).
	// TODO
	//PersistenceHandler *preferences = PersistenceHandler::inst();

	if(SDL_Init(SDL_INIT_EVERYTHING & ~(SDL_INIT_TIMER | SDL_INIT_HAPTIC))){
		cout << "Failed to init SDL: " << SDL_GetError() << endl;
		return -1;
	}
	SDL_ShowCursor(SDL_DISABLE);

	// Enable audio
	if(Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, MIX_CHANNELS, 1024)){
		cout << "Failed to initialize audio." << endl;
		return -2;
	}

	EngineContext *pCtx = new EngineContext();

#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop_arg(gameloop, (void*) pCtx, 0, 1);
#else
	while(pCtx->run) { gameloop(pCtx); }

	// Clean up and close SDL library.
	Mix_CloseAudio();
	SDL_Quit();
#endif

	return 0;
}
