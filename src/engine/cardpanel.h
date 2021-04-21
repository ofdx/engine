/*
	CardPanel
	mperron (2019)

	A set of multiple drawables, which can be displayed one at a time.
*/
template<class C> class CardPanel : public Drawable, public Clickable {
	vector<C*> cards;

	C *active_card = NULL;
	int active = -1;

public:
	CardPanel() : Drawable(NULL), Clickable() {}

	~CardPanel(){
		for(C *card : cards)
			delete card;
	}

	void draw(int ticks){
		if(active_card)
			active_card->draw(ticks);
	}
	void check_mouse(SDL_Event event){
		if(active_card)
			active_card->check_mouse(event);
	}

	CardPanel *add(C *card){
		cards.push_back(card);
		active = cards.size();
		active_card = card;

		return this;
	}

	C *next(){
		int size = cards.size();

		if(size == 0){
			active = -1;
			active_card = NULL;

			return NULL;
		}
		
		if(++active >= size)
			active = 0;

		return active_card = cards[active];
	}

	C *get_active(){
		return active_card;
	}
};
