/*
	Typable
	mperron (2022)

	Classes which inherit from this one can receive typed text input, one
	character at a time, with a keydown method.
*/

class Typable {
protected:
	bool m_typing_active;

public:
	Typable(bool active) :
		m_typing_active(active)
	{}

	virtual ~Typable(){}

	virtual void keydown(SDL_KeyboardEvent event){}
};
