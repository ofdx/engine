/*
	Button
	mperron (2019)

	A mouse-clickable button, which can perform an action.
*/
class Button : public Drawable, public Clickable {
	bool hover = false;
	bool down = false;
	char alpha = 0xFF;
	int ticks_toshow = 0;

	SDL_Color color_normal = { 0xb0, 0xb0, 0xb0 };
	SDL_Color color_down = { 0x40, 0x40, 0x40 };
	SDL_Color color_label = { 0x70, 0x70, 0x70 };
	SDL_Color color_hover = { 0x60, 0x80, 0xf0 };

	PicoText *label;

	void label_create(string text){
		label = new PicoText(rend, (SDL_Rect){
			click_region.x + 3, click_region.y + (click_region.h / 2) - 3,
			click_region.w, click_region.h - 7
		}, text);
		label->set_color(color_label);
	}

public:
	// Create a button which is sized by pixels (with click_region).
	Button(
		SDL_Renderer *rend,
		SDL_Rect click_region,
		string text
	) : Drawable(rend), Clickable(click_region) {
		label_create(text);
	}

	// Create a button which is sized by characters.
	Button(
		SDL_Renderer *rend,
		int x, int y,
		int rows, int cols,
		string text
	) : Drawable(rend), Clickable((SDL_Rect){
		x, y,
		((cols * 6) + 5), ((rows * 7) + 8)
	}) {
		label_create(text);
	}

	~Button(){
		delete label;
	}

	virtual void on_mouse_down(SDL_MouseButtonEvent event){
		if(event.button & SDL_BUTTON_LEFT)
			down = true;
	}
	virtual void on_mouse_up(SDL_MouseButtonEvent event){
		if(event.button & SDL_BUTTON_LEFT)
			down = false;
	}
	virtual void on_mouse_click(SDL_MouseButtonEvent event){
		action();
	}


	virtual void on_mouse_in(SDL_MouseMotionEvent event){
		label->set_color(color_hover);
		hover = true;
	}
	virtual void on_mouse_out(SDL_MouseMotionEvent event){
		label->set_color(color_label);
		hover = false;
	}

	void draw(int ticks){
		// Delay before allowing the button to be clicked again.
		if(ticks_toshow > 0){
			if(ticks_toshow > ticks){
				ticks_toshow -= ticks;
			} else {
				clickable_disabled = false;
				ticks_toshow = 0;
			}
		}

		SDL_Color fill = (down ? color_down : color_normal);
		SDL_Color bord = (hover ? color_hover : color_label);

		// Fill
		SDL_SetRenderDrawColor(rend, fill.r, fill.g, fill.b, alpha);
		SDL_RenderFillRect(rend, &click_region);

		// Border
		SDL_SetRenderDrawColor(rend, bord.r, bord.g, bord.b, alpha);
		SDL_RenderDrawRect(rend, &click_region);

		// Text
		label->draw(ticks);
	}

	virtual void visible(bool vis){
		if(vis){
			// Show (delayed clickability)
			ticks_toshow = 100;
			drawable_hidden = false;
		} else {
			// Hide
			ticks_toshow = 0;
			drawable_hidden = true;
			clickable_disabled = true;
		}
	}

	virtual void action(){}

	void set_alpha(char alpha){
		label->set_alpha(alpha);
		this->alpha = alpha;
	}
};
