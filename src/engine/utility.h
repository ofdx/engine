/*
	Various utility methods, just so they don't have to live in main.cc.
*/

bool operator != (const SDL_Color &a, const SDL_Color &b){
	return !(
		(a.r == b.r) &&
		(a.g == b.g) &&
		(a.b == b.b)
	);
}

SDL_Texture *textureFromBmp(SDL_Renderer *rend, const char *fn, bool trans = false){
	FileLoader *fl = FileLoader::get(fn);
	if(!fl)
		return NULL;

	SDL_Surface *sf = fl->surface();
	if(sf && trans)
		SDL_SetColorKey(sf, SDL_TRUE, SDL_MapRGB(sf->format, 0xff, 0x00, 0xff));

	SDL_Texture *tx = SDL_CreateTextureFromSurface(rend, sf);

	return tx;
}

void rectSum(SDL_Rect &holder, SDL_Rect a, SDL_Rect b){
	holder.x = (a.x + b.x);
	holder.y = (a.y + b.y);
	holder.w = (a.w + b.w);
	holder.h = (a.h + b.h);
}
void rectFloatOffset(SDL_Rect &holder, SDL_Rect rect, double x, double y){
	holder.x = (int)(rect.x + x);
	holder.y = (int)(rect.y + y);
	holder.w = rect.w;
	holder.h = rect.h;
}

// Slide along x^2 from x=-16...0, mapped onto the difference to-from.
// Time is milliseconds to complete the transition. Ticks are milliseconds
// since the last drawn frame. The progress var should be set to 0.0f
// initially and will be 16.0f when the transition is complete.
int slide_quad_left(int from, int to, int time, int ticks, float &progress){
	progress += ticks * 16.0f / time;

	if(progress >= 16.0f){
		progress = 16.0f;
		return to;
	}

	float slide = progress - 16.0f;
	return (((to - from) * (256 - (slide * slide)) / 256) + from);
}

// Same as slide_quad_left, except that it runs from x=0...16. This means
// progress is slow at first and then fast.
int slide_quad_right(int from, int to, int time, int ticks, float &progress){
	progress += ticks * 16.0f / time;

	if(progress >= 16.0f){
		progress = 16.0f;
		return to;
	}

	return (((to - from) * (progress * progress) / 256) + from);
}

// Slides from x=0...16, then x=-16...0, creating a curve which is slow at
// the start and end, but fast in the middle.
int slide_quad(int from, int to, int time, int ticks, float &progress){
	int width = to - from;

	if(progress < 16.0f){
		// Run the positive side of the curve (x=0...16)
		return (from + slide_quad_right(0, width / 2, time / 2, ticks, progress));
	} else if(progress < 32.0f){
		// Run x=-16...0
		float slide = (progress - 16.0f);
		float ret = ((from + width / 2) + slide_quad_left(0, (width - (width / 2)), time / 2, ticks, slide));

		progress = (slide + 16.0f);
		return ret;
	}

	// Done
	progress = 32.0f;
	return to;
}

uint32_t get_surface_pixel(SDL_Surface *surface, int x, int y)
{
	int bpp = surface->format->BytesPerPixel;
	uint8_t *p = (uint8_t*) surface->pixels + (y * surface->pitch) + (x * bpp);

	switch (bpp)
	{
		case 1: return *p;
		case 2: return *((uint16_t*) p);
		case 3: return ((SDL_BYTEORDER == SDL_BIG_ENDIAN) ? ((p[0] << 16) | (p[1] << 8) | p[2]) : (p[0] | (p[1] << 8) | (p[2] << 16)));
		case 4: return *(uint32_t *)p;
	}

	// Hopefully shouldn't happen.
	return 0;
}
