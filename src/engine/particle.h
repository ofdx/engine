/*
	ParticleEffect
	mperron (2019)

	A base class, intended to be extended by particle effects which affect some
	specific region (SDL_Rect) of the screen.
*/
class ParticleEffect : public Drawable {
protected:
	SDL_Rect area;

	ParticleEffect(SDL_Renderer *rend, SDL_Rect area) : Drawable(rend) {
		this->area = area;
	}

public:
	// Your implementation of this function  should return the number of
	// particles remaining.
	virtual int update(int ticks){
		return 0;
	}
	virtual void draw(int ticks){
		update(ticks);
	}

	virtual ~ParticleEffect(){}
};
