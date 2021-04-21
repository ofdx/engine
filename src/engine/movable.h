/*
	Movable
	mperron(2020)

	An element that moves in 2D space.
*/
class Movable {
	int x_draw, y_draw;

protected:
	int x_prev, x_orig;
	int y_prev, y_orig;
	float progress_translate_x, progress_translate_y;

public:
	int x, y;
	int movable_time;

	Movable(int x, int y){
		x_draw = this->x = x_orig = x_prev = x;
		y_draw = this->y = y_orig = y_prev = y;

		progress_translate_x = 32.0f;
		progress_translate_y = 32.0f;
		movable_time = 250;
	}

	virtual void translate_to(int x, int y){
		progress_translate_x = progress_translate_y = 0.0f;

		x_prev = this->x;
		y_prev = this->y;

		this->x = x;
		this->y = y;
	}
	virtual void translate_to_orig(){
		translate_to(x_orig, y_orig);
	}
	virtual void translate(int dx, int dy){
		translate_to(x + dx, y + dy);
	}

	virtual void movable_update(int ticks){
		x_draw = (
			(progress_translate_x >= 32.0f) ?
				(x) :
				slide_quad(x_prev, x, movable_time, ticks, progress_translate_x)
		);

		y_draw = (
			(progress_translate_y >= 32.0f) ?
				(y) :
				slide_quad(y_prev, y, movable_time, ticks, progress_translate_y)
		);
	}

	inline int movable_x() const { return x_draw; }
	inline int movable_y() const { return y_draw; }
};
