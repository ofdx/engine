/*
	Drawable
	mperron(2019)

	An element which can be rendered in someway onto the screen.
*/
class Drawable {
protected:
	SDL_Renderer *rend;

	Drawable(SDL_Renderer *rend){
		this->rend = rend;
	}

public:
	bool drawable_hidden = false;

	virtual void draw(int ticks) = 0;

	virtual ~Drawable(){}
};
