/*
	PicoText
	mperron (2022)

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

	size_t scroll_pos = 0;
	vector<string> message_lines;
	int window_lines = 0;

protected:
	virtual void populateLineVector(){
		stringstream ss;
		bool wrapped = false;

		size_t chars_printed = 0;
		size_t chars_perline = ((region.w > c_width) ? ((region.w / c_width) - 1) : 0);
		size_t chars_thisline = 0;

		message_lines.clear();

		for(char c : message){
			chars_printed++;

			if(wrapped || (c == '\n')){
				chars_thisline = 0;
				wrapped = false;

				message_lines.push_back(ss.str());
				ss.str("");

				// If the text just wrapped at the edge of the text region, skip a
				// following space which would be at the start of the line and look
				// bad, and a following newline which would otherwise result in a
				// double newline.
				if((c == '\n') || (c == ' '))
					continue;
			}

			ss << c;
			chars_thisline++;

			// Automatically wrap at the edge of the text region.
			if(chars_thisline > chars_perline)
				wrapped = true;

			// Wrap if a word won't fit unless the word is really big.
			if(!wrapped && (c == ' ') && (chars_thisline > (chars_perline / 2))){
				size_t next_space_pos = message.find_first_of(" \n", chars_printed);

				if(next_space_pos == string::npos)
					next_space_pos = (message.size() - 1);

				if((next_space_pos - chars_printed + chars_thisline) > chars_perline)
					wrapped = true;
			}
		}

		// Last line.
		{
			string final = ss.str();

			if(final.size())
				message_lines.push_back(final);
		}

		// The number of lines of text that fit inside this window.
		window_lines = region.h / (c_height + leading);

		// If we're scrolled past the end of the new message text, this will put
		// us on the last line.
		set_scroll(scroll_pos);
	}

public:
	// Debug flags
	bool draw_frame = false;
	char pointer_char = 0;

	PicoText(SDL_Renderer *rend, SDL_Rect region, string message) :
		Drawable(rend)
	{
		this->region = region;
		set_message(message);

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

	size_t get_scroll(){
		return scroll_pos;
	}
	void set_scroll(size_t pos){
		scroll_pos = min(pos, message_lines.size() - 1);
	}
	void set_scroll_offset(int offset){
		if((offset < 0) && ((size_t)(offset * -1) > scroll_pos))
			scroll_pos = 0;
		else
			scroll_pos += offset;

		if(scroll_pos > message_lines.size() - 1)
			scroll_pos = message_lines.size() - 1;
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
		bool shadow = (shadow_offset_x || shadow_offset_y);

		size_t chars_printed = 0;
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

		size_t scroll = 0;
		int printed_lines = 0;
		for(string l : message_lines){
			if(scroll++ < scroll_pos)
				continue;

			for(char c : l){
				// Break early for the typewriter effect.
				if((chars_max >= 0) && (chars_printed++ > chars_max)){
					// Show a little caret character at the end of the current line.
					if(pointer_char){
						src.x = (pointer_char - ' ') * c_width;
						SDL_RenderCopy(rend, font, &src, &dst);
					}

					return;
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
			}

			// Can't fit any more text in this box.
			if(++printed_lines >= window_lines)
				break;

			// New line
			dst.x = region.x;
			dst.y += (c_height + leading);
		}
	}

	string get_message(){ return message; }
	void set_message(string message){
		this->message = message;
		populateLineVector();
		ticks_passed = 0;
	}

	// The number of lines of text after wrapping.
	size_t get_line_count(){ return message_lines.size(); }

	// The number of lines of text that will fit in the window.
	int get_window_lines(){ return window_lines; }

	// Set the color of the text at any time.
	virtual void set_color(char r, char g, char b, bool shadow = false){
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
	void set_size(int w, int h){
		region.w = w;
		region.h = h;

		populateLineVector();
	}
};

/*
	TextBox
	mperron (2022)

    Text box with an interactive scroll bar.
*/
class TextBox : public PicoText, public Clickable {
    const static int SCROLL_BAR_WIDTH = 4;
    const static int SCROLL_ARROW_HEIGHT = 4;

    SDL_Rect m_bounds;
    uint8_t sb_r = 0xff, sb_g = 0xff, sb_b = 0xff;
	uint8_t sb_r_hl = 0xff, sb_g_hl = 0xff, sb_b_hl = 0xff;

	uint8_t m_mb_down = 0;

    enum SCROLL_BAR_ARROW { NONE, UP, DOWN, BAR, BAR_ABOVE, BAR_BELOW } m_arrow_in;
	bool m_sb_active = false;
	int m_sb_active_at = 0;
	int m_sb_active_pos = 0;
	int m_sb_height = 0, m_sb_height_max = 0;

    // Returns true if changed. Detect whether the mouse was over the up or down button.
    virtual bool is_mouse_in(int screen_x, int screen_y){
        bool was_mouse_in = mouse_in;

        int of_x = screen_x - m_bounds.x;
        int of_y = screen_y - m_bounds.y;

        // In X
        if((of_x >= (m_bounds.w - SCROLL_BAR_WIDTH)) && (of_x < m_bounds.w)){
			int sb_pos = get_sb_pos();

			mouse_in = true;

			if((screen_y >= sb_pos) && (screen_y < (sb_pos + m_sb_height))){
				// Mouse over the scroll bar.
				m_arrow_in = BAR;
			} else if((of_y > 0) && (of_y <= SCROLL_ARROW_HEIGHT)){
                // Mouse over the up scroll arrow.
                m_arrow_in = UP;
            } else if((of_y >= (m_bounds.h - SCROLL_ARROW_HEIGHT)) && (of_y < m_bounds.h)){
                // Mouse over the down scroll arrow.
                m_arrow_in = DOWN;
			} else if((of_y > SCROLL_ARROW_HEIGHT) && (screen_y < sb_pos)){
				// Mouse between up arrow and scroll bar.
				m_arrow_in = BAR_ABOVE;
			} else if((of_y < (m_bounds.h - SCROLL_ARROW_HEIGHT)) && (screen_y > (sb_pos + m_sb_height))){
				// Mouse between scroll bar and down arrow.
				m_arrow_in = BAR_BELOW;
            } else {
				m_arrow_in = NONE;
                mouse_in = false;
			}
        }

        bool changed = (was_mouse_in == mouse_in);
        was_mouse_in = mouse_in;

        return changed;
    }

	double get_scroll_ratio(){
		if(get_line_count() > 1)
			return ((double) get_scroll()) / (get_line_count() - 1);

		return 1.0;
	}
	int get_sb_pos(){
		return m_bounds.y + SCROLL_ARROW_HEIGHT + 1 + (int)((m_sb_height_max - m_sb_height) * get_scroll_ratio());
	}

	virtual void populateLineVector(){
		PicoText::populateLineVector();

		// Calculate size of scroll bar.
		m_sb_height_max = m_bounds.h - (2 * SCROLL_ARROW_HEIGHT) - 2;

		if(m_sb_height_max <= 0){
			// Can't render a bar in this space.
			m_sb_height = 0;
		} else {
			int window = get_window_lines();

			m_sb_height = (int) (m_sb_height_max * (((double) window) / (get_line_count() + window - 1)));
		}
	}

public:
    TextBox(SDL_Renderer *rend, SDL_Rect bounds, string message) :
        PicoText(rend, bounds, message),
        Clickable(bounds),
        m_bounds(bounds),
        m_arrow_in(NONE)
    {
        // Set text output size to exclude scrollbar area.
        set_size(bounds.w - (SCROLL_BAR_WIDTH + 1), bounds.h);
        set_pos(bounds.x, bounds.y);
    }

    ~TextBox(){}

	virtual void check_mouse(SDL_Event event){
		Clickable::check_mouse(event);

		if(m_sb_active){
			switch(event.type){
				case SDL_MOUSEMOTION:
					int delta_px = event.button.y - m_sb_active_at;
					int sb_pos = m_sb_active_pos + delta_px;

					int sb_min = (m_bounds.y + SCROLL_ARROW_HEIGHT + 1);
					int sb_max = sb_min + m_sb_height_max - m_sb_height;

					if(sb_pos < sb_min)
						sb_pos = sb_min;

					if(sb_pos > sb_max)
						sb_pos = sb_max;

					double ratio = ((double) (sb_pos - sb_min)) / (sb_max - sb_min);

					set_scroll((size_t)(ratio * get_line_count()));
					break;
			}
		}
	}

	virtual void on_mouse_down(SDL_MouseButtonEvent event){
		m_mb_down |= event.button;

		if(m_arrow_in == BAR){
			m_sb_active_pos = get_sb_pos();
			m_sb_active_at = event.y;
			m_sb_active = true;
		}
	}
	virtual void on_mouse_up(SDL_MouseButtonEvent event){
		m_mb_down &= ~event.button;

		m_sb_active_pos = -1;
		m_sb_active_at = -1;
		m_sb_active = false;
	}

	virtual void on_mouse_click(SDL_MouseButtonEvent event){
        switch(m_arrow_in){
            case UP:
                set_scroll_offset(-1);
                break;

            case DOWN:
                set_scroll_offset(1);
                break;

			case BAR_ABOVE:
				set_scroll_offset(-get_window_lines());
				break;

			case BAR_BELOW:
				set_scroll_offset(get_window_lines());
				break;

			default:
				break;
        }
    }

	virtual void set_color(char r, char g, char b, bool shadow = false){
        PicoText::set_color(r, g, b, shadow);

        sb_r = r;
        sb_g = g;
        sb_b = b;
	}
	void set_color_hl(char r, char g, char b){
		sb_r_hl = r;
		sb_g_hl = g;
		sb_b_hl = b;
	}

	void draw(int ticks){
        PicoText::draw(ticks);

        SDL_Rect arrow_at = (SDL_Rect){
            m_bounds.x + m_bounds.w - SCROLL_BAR_WIDTH, m_bounds.y,
            SCROLL_BAR_WIDTH, SCROLL_ARROW_HEIGHT
        };
	
		#define sethlcolor(BTN) \
			if(m_mb_down && (m_arrow_in == BTN)) \
				SDL_SetRenderDrawColor(rend, sb_r_hl, sb_g_hl, sb_b_hl, 0xff); \
			else \
				SDL_SetRenderDrawColor(rend, sb_r, sb_g, sb_b, 0xff);

		sethlcolor(UP);
		SDL_RenderFillRect(rend, &arrow_at);

		sethlcolor(DOWN);
        arrow_at.y = m_bounds.y + m_bounds.h - SCROLL_ARROW_HEIGHT;
		SDL_RenderFillRect(rend, &arrow_at);

		// Draw the scroll bar if appropriate.
		if(m_sb_height){
			SDL_Rect sb_at = (SDL_Rect){
				m_bounds.x + m_bounds.w - SCROLL_BAR_WIDTH, get_sb_pos(),
				SCROLL_BAR_WIDTH, m_sb_height
			};

			sethlcolor(BAR);
			SDL_RenderFillRect(rend, &sb_at);
		}
    }
};