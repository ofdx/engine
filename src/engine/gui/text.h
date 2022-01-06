/*
	PicoText
	mperron (2019)

	A class which draws text onto the screen using a bitmap font.
*/
class PicoText :
	public Drawable
{
	SDL_Texture *font, *font_shadow;
	SDL_Rect region;
	string message;

	// Size of the individual letters in pixels. This includes an extra pixel
	// on the right and bottom which isn't normally used, that represents the
	// space between characters and lines of text. Some characters have
	// descenders which occupy the extra bottom line.
	int c_width = 6;
	int c_height = 7;

	// Extra space between lines of text.
	int leading = 0;

	int ticks_perchar = 0;
	int ticks_passed = 0;

	int shadow_offset_x = 0;
	int shadow_offset_y = 0;

	unsigned int blink_on = 0;
	unsigned int blink_off = 0;

public:
	// Debug flags
	bool draw_frame = false;
	char pointer_char = 0;

	PicoText(SDL_Renderer *rend, SDL_Rect region, string message) :
		Drawable(rend)
	{
		this->region = region;
		this->message = message;

		// Load the default font image.
		font = textureFromBmp(rend, "fonts/6x7.bmp", true);
		font_shadow = textureFromBmp(rend, "fonts/6x7.bmp", true);
	}

	~PicoText(){
		SDL_DestroyTexture(font);
		SDL_DestroyTexture(font_shadow);
	}

	void set_shadow(int x, int y){
		shadow_offset_x = x;
		shadow_offset_y = y;
	}

	void set_font(string bitmap, int c_width, int c_height){
		SDL_DestroyTexture(font);
		SDL_DestroyTexture(font_shadow);

		font = textureFromBmp(rend, bitmap.c_str(), true);
		font_shadow = textureFromBmp(rend, bitmap.c_str(), true);

		this->c_width = c_width;
		this->c_height = c_height;
	}

	void draw(int ticks){
		static unsigned int blink_counter = 0;

		// Text frame for debug purposes.
		if(draw_frame){
			SDL_RenderDrawLine(rend, region.x, region.y, region.x + region.w, region.y);
			SDL_RenderDrawLine(rend, region.x, region.y, region.x, region.y + region.h);
			SDL_RenderDrawLine(rend, region.x + region.w, region.y + region.h, region.x, region.y + region.h);
			SDL_RenderDrawLine(rend, region.x + region.w, region.y + region.h, region.x + region.w, region.y);
		}

		// Blink text.
		if(blink_on || blink_off){
			blink_counter += ticks;

			// Skip draw if we are beyond the on period.
			if((blink_counter % (blink_on + blink_off)) > blink_on)
				return;

			// Keep the blink_counter from growing indefinitely.
			while(blink_counter > (blink_on + blink_off))
				blink_counter -= (blink_on + blink_off);
		}


		SDL_Rect src = (SDL_Rect){
			0,0,
			c_width, c_height
		};
		SDL_Rect dst = (SDL_Rect){
			region.x, region.y,
			c_width, c_height
		};
		bool wrapped = false;
		bool shadow = (shadow_offset_x || shadow_offset_y);

		size_t chars_max = -1;
		if(ticks_perchar){
			unsigned int chars = ((ticks_passed + ticks) / ticks_perchar);

			// We don't want chars_max or ticks_passed to count up
			// indefinitely. Cap these values as soon as we're beyond the
			// length of the string.
			if(chars <= message.size()){
				ticks_passed += ticks;
				chars_max = chars;
			}
		}

		size_t chars_printed = 0;
		size_t chars_perline = ((region.w > c_width) ? ((region.w / c_width) - 1) : 0);
		size_t chars_thisline = 0;

		// FIXME This renders the text character by character on every single
		// frame regardless of whether the text has changed. If we're not doing
		// a delayed typewriter effect, and the text content or color hasn't
		// changed, we should instead read from a texture which was
		// pre-rendered.
		for(char c : message){
			// Break early for the typewriter effect.
			if((chars_max >= 0) && (chars_printed++ > chars_max)){
				// Show a little caret character at the end of the current line.
				if(pointer_char){
					src.x = (pointer_char - ' ') * c_width;
					SDL_RenderCopy(rend, font, &src, &dst);
				}

				break;
			}

			if(wrapped){
				dst.x = region.x;
				dst.y += (c_height + leading);
				chars_thisline = 0;
				wrapped = false;

				// If the text just wrapped at the edge of the text region, skip a
				// following space which would be at the start of the line and look
				// bad, and a following newline which would otherwise result in a
				// double newline.
				if((c == '\n') || (c == ' '))
					continue;
			}

			// Can't fit this text in this box.
			if(dst.y - region.y + c_height > region.h)
				break;

			// New line and carriage return.
			if(c == '\n'){
				dst.x = region.x;
				dst.y += (c_height + leading);
				chars_thisline = 0;

				continue;
			}

			if((c >= 'a') && (c <= 'z'))
				c -= 0x20;

			// The font starts at space.
			c -= ' ';

			// Unknown characters print a space.
			if(c >= 65)
				c = 0;

			src.x = (c * c_width);

			if(shadow){
				SDL_Rect dst_shadow = (SDL_Rect){
					dst.x + shadow_offset_x, dst.y + shadow_offset_y,
					dst.w + shadow_offset_x, dst.h + shadow_offset_y
				};

				SDL_RenderCopy(rend, font_shadow, &src, &dst_shadow);
			}

			SDL_RenderCopy(rend, font, &src, &dst);

			dst.x += c_width;
			chars_thisline++;

			// Automatically wrap at the edge of the text region.
			if(chars_thisline > chars_perline)
				wrapped = true;

			// Wrap if a word won't fit unless the word is really big.
			if(!wrapped && !c && (chars_thisline > (chars_perline / 2))){
				size_t next_space_pos = message.find_first_of(" \n", chars_printed);

				if(next_space_pos == string::npos)
					next_space_pos = (message.size() - 1);

				if((next_space_pos - chars_printed + chars_thisline) > chars_perline)
					wrapped = true;
			}
		}
	}

	void set_message(string message){
		this->message = message;
		ticks_passed = 0;
	}

	// Set the color of the text at any time.
	void set_color(char r, char g, char b, bool shadow = false){
		SDL_SetTextureColorMod((shadow ? font_shadow : font), r, g, b);
	}
	void set_color(SDL_Color col, bool shadow = false){
		set_color(col.r, col.g, col.b, shadow);
	}

	// Set the alpha/transparency for the text at any time.
	void set_alpha(char a, bool shadow = false){
		SDL_SetTextureAlphaMod((shadow ? font_shadow : font), a);
	}

	void set_blink(unsigned int on, unsigned int off){
		blink_on = on;
		blink_off = off;
	}

	// Set the number of millisecond ticks to hang on each character when
	// simulating a typewriter. Set to 0 (default) to disable this effect.
	void set_ticks_perchar(int ticks){
		ticks_perchar = ticks;
		ticks_passed = 0;
	}

	void set_pos(int x, int y){
		region.x = x;
		region.y = y;
	}

	class Box :
		public Drawable,
		public Movable
	{
	public:
		PicoText *text_label;
		int width;

		Box(SDL_Renderer *rend, int x, int y, string text) :
			Drawable(rend),
			Movable(x, y)
		{
			text_label = new PicoText(rend, (SDL_Rect){ x, y, 100, 10, }, "");
			text_label->set_message(text);
		}

		~Box(){
			delete text_label;
		}

		void draw(int ticks){
			movable_update(ticks);
			width = 0;

			int x_cur = movable_x(), y_cur = movable_y();

			if(text_label->message.length()){
				width = 6 + (6 * text_label->message.length());

				SDL_Rect window = (SDL_Rect){
					x_cur, y_cur,
					width, 11
				};

				// Border
				SDL_SetRenderDrawColor(rend, 0x40, 0x40, 0x40, 0xff);
				SDL_RenderDrawRect(rend, &window);

				window = (SDL_Rect){
					window.x + 1, window.y + 1,
					window.w - 2, window.h - 2
				};

				// Background
				SDL_SetRenderDrawColor(rend, 0x80, 0x80, 0x80, 0xff);
				SDL_RenderFillRect(rend, &window);

				// Text content.
				text_label->set_pos(window.x + 2, window.y + 1);
				text_label->draw(ticks);
			}
		}
	};
};

