#include <errno.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

#include <qrpath/qrpath.h>

enum {
	SouthEast, NorthEast, NorthWest, SouthWest
};

enum {
	East, North, West, South
};

struct qrpath {
	bool modified;
	uint8_t *fields; /* bits indicating whether the bit is set (black) or unset (white) */
	int *areas; /* assigned area for each bit; = 0: outer white area; < 0: enclosed white area; > 0: black area */
	int w;
	int h;
};

/* corner cells for cell X:
 * BA
 * CX
 */

static const struct {
	int xa;
	int ya;
	int xb;
	int yb;
	int xc;
	int yc;
} CornerCells[4] = {
	{ .ya = 1, .xb = 1, .yb = 1, .xc = 1 },
	{ .xa = 1, .xb = 1, .yb = -1, .yc = -1 },
	{ .ya = -1, .xb = -1, .yb = -1, .xc = -1 },
	{ .xa = -1, .xb = -1, .yb = 1, .yc = 1 }
};

static const struct {
	/* the cell to check for corner */
	int cx;
	int cy;

	/* inner corner turn */
	int ix;
	int iy;

	/* outer corner turn */
	int ox;
	int oy;

	/* straight walk */
	int sx;
	int sy;
} WalkCCW[4] = {
	{ .cx = -1, .cy = -1, .iy = 1, .oy = -1, .sx = 1 },
	{ .cx = -1, .ix = 1, .ox = -1, .sy = -1 },
	{ .iy = -1, .oy = 1, .sx = -1 },
	{ .cy = -1, .ix = -1, .ox = 1, .sy = 1 }
};

/* Corners (here: orientation NorthWest)
 * C: current cell; X: neighbour-cell set; <space>: neighbour-cell unset;
 * O: pattern is outer corner; I: pattern is inner corner; N: pattern is no corner
 *
 *  0   1   2   3   4   5   6   7
 *
 *         X   X    X   X  XX  XX
 *  C  XC   C  XC   C  XC   C  XC
 *
 *  O   N   O   I   N   I   I   N
 *  --> outer corner fingerprint: 0b00000101 = 0x05
 *  --> inner corner fingerprint: 0b01101000 = 0x68
 *  --> corner fingerprint: 0b01101101 = 0x6d
 */
static const uint8_t OuterCorners = 0x05;
static const uint8_t InnerCorners = 0x68;
static const uint8_t Corners = 0x6d;

typedef struct {
	qrpath_t *self;
	int *areas;
	int cur;
	int width;
	int height;
} context_t;

static inline int get_area(
		qrpath_t *self,
		int x,
		int y)
{
	if(x < 0 || x >= self->w || y < 0 || y >= self->h)
		return 0;
	else
		return self->areas[y * self->w + x];
}

static inline void set_area(
		qrpath_t *self,
		int x,
		int y,
		int area)
{
	self->areas[y * self->w + x] = area;
}

static inline bool get_bit(
		qrpath_t *self,
		int x,
		int y)
{
	int bit;
	int byte;

	if(x < 0 || x >= self->w || y < 0 || y >= self->h)
		return false;
	bit = y * self->w + x;
	byte = bit / 8;
	bit = 7 - bit % 8;
	return (self->fields[byte] & (1 << bit)) != 0;
}

static inline void set_bit(
		qrpath_t *self,
		int x,
		int y)
{
	int bit = y * self->w + x;
	int byte = bit / 8;
	bit = 7 - bit % 8;
	self->fields[byte] |= 1 << bit;
}

static inline void unset_bit(
		qrpath_t *self,
		int x,
		int y)
{
	int bit = y * self->w + x;
	int byte = bit / 8;
	bit = 7 - bit % 8;
	self->fields[byte] &= ~(1 << bit);
}

static inline bool is_corner(
		qrpath_t *self,
		int x,
		int y,
		int area,
		int orientation,
		uint8_t corners)
{
	int pattern = 0;
	int xa = x + CornerCells[orientation].xa;
	int ya = y + CornerCells[orientation].ya;
	int xb = x + CornerCells[orientation].xb;
	int yb = y + CornerCells[orientation].yb;
	int xc = x + CornerCells[orientation].xc;
	int yc = y + CornerCells[orientation].yc;

	if(get_area(self, xa, ya) == area)
		pattern |= 1 << 0;
	if(get_area(self, xb, yb) == area)
		pattern |= 1 << 1;
	if(get_area(self, xc, yc) == area)
		pattern |= 1 << 2;
	return (corners & (1 << pattern)) != 0;
}

static void fill_set(
		qrpath_t *self,
		int x,
		int y,
		int area)
{
	if(x < -1 || x > self->w || y < -1 || y > self->h)
		return;
	else if(get_bit(self, x, y) && get_area(self, x, y) == 0) {
		set_area(self, x, y, area);
		fill_set(self, x + 1, y, area);
		fill_set(self, x - 1, y, area);
		fill_set(self, x, y + 1, area);
		fill_set(self, x, y - 1, area);
	}
}

static void fill_unset(
		qrpath_t *self,
		int x,
		int y,
		int area)
{
	if(x < -1 || x > self->w || y < -1 || y > self->h)
		return;
	else if(!get_bit(self, x, y) && get_area(self, x, y) == 1) {
		set_area(self, x, y, area);
		fill_unset(self, x + 1, y, area);
		fill_unset(self, x - 1, y, area);
		fill_unset(self, x, y + 1, area);
		fill_unset(self, x, y - 1, area);
		fill_unset(self, x + 1, y + 1, area);
		fill_unset(self, x - 1, y + 1, area);
		fill_unset(self, x + 1, y - 1, area);
		fill_unset(self, x - 1, y - 1, area);
	}
}

static void update(
		qrpath_t *self)
{
	int x;
	int y;
	int area;

	/* fill all areas to temporary values:
	 * where the bit is set, area becomes 0;
	 * where the bit is unset, area becomes 1.
	 * those values are used to identify unencountered areas in the next step. */
	for(y = 0; y < self->h; y++)
		for(x = 0; x < self->w; x++)
			set_area(self, x, y, get_bit(self, x, y) ? 0 : 1);

	/* fill outer area(s) where bit is not set; those all become 0 */
	for(x = 0; x < self->w; x++) {
		if(!get_bit(self, x, 0) && get_area(self, x, 0) == 1)
			fill_unset(self, x, 0, 0);
		if(!get_bit(self, x, self->h - 1) && get_area(self, x, self->h - 1) == 1)
			fill_unset(self, x, self->h - 1, 0);
	}
	for(y = 0; y < self->h; y++) {
		if(!get_bit(self, 0, y) && get_area(self, 0, y) == 1)
			fill_unset(self, 0, y, 0);
		if(!get_bit(self, self->w - 1, y) && get_area(self, self->w - 1, y) == 1)
			fill_unset(self, self->w - 1, y, 0);
	}


	/* fill all areas where the bit is set */
	area = 1;
	for(y = 0; y < self->h; y++)
		for(x = 0; x < self->w; x++)
			if(get_bit(self, x, y) && get_area(self, x, y) == 0) {
				fill_set(self, x, y, area);
				area++;
			}

	/* fill all areas where the bit is not set */
	area = -1;
	for(y = 0; y < self->h; y++)
		for(x = 0; x < self->w; x++)
			if(!get_bit(self, x, y) && get_area(self, x, y) == 1) {
				fill_unset(self, x, y, area);
				area--;
			}
}

static void walk_ccw(
		qrpath_t *self,
		int x,
		int y,
		int area,
		int dir,
		void (*begin)(void *user, int x, int y, int area),
		void (*end)(void *user),
		void (*lineto)(void *user, int x, int y),
		void *user)
{
	int x0 = x;
	int y0 = y;
	int cx;
	int cy;

	if(begin != NULL)
		begin(user, x, y, area);
	do {
		cx = x + WalkCCW[dir].cx;
		cy = y + WalkCCW[dir].cy;
		if(is_corner(self, cx, cy, area, dir, InnerCorners)) {
			if(lineto != NULL)
				lineto(user, x, y);
			x += WalkCCW[dir].ix;
			y += WalkCCW[dir].iy;
			dir--;
			if(dir < 0)
				dir = 3;
		}
		else if(is_corner(self, cx, cy, area, dir, OuterCorners)) {
			if(lineto != NULL)
				lineto(user, x, y);
			x += WalkCCW[dir].ox;
			y += WalkCCW[dir].oy;
			dir++;
			if(dir > 3)
				dir = 0;
		}
		else {
			x += WalkCCW[dir].sx;
			y += WalkCCW[dir].sy;
		}
	} while(x != x0 || y != y0);
	if(end != NULL)
		end(user);
}


qrpath_t *qrpath_new(
		int w,
		int h)
{
	qrpath_t *self;
	if(w <= 0 || h <= 0) {
		errno = -EINVAL;
		return NULL;
	}
	self = calloc(1, sizeof(qrpath_t));
	if(self == NULL)
		return NULL;
	if((w * h) % 8 == 0)
		self->fields = calloc(1, (w * h) / 8);
	else
		self->fields = calloc(1, (w * h) / 8 + 1);
	if(self->fields == NULL) {
		free(self);
		return NULL;
	}
	self->areas = calloc(1, w * h * sizeof(int));
	if(self->areas == NULL) {
		free(self->fields);
		free(self);
		return NULL;
	}
	self->w = w;
	self->h = h;
	return self;
}

int qrpath_set(
		qrpath_t *self,
		int x,
		int y)
{
	if(x < 0 || x >= self->w)
		return QRPATH_OUT_OF_BOUNDS;
	else if(y < 0 || y >= self->h)
		return QRPATH_OUT_OF_BOUNDS;
	set_bit(self, x, y);
	self->modified = true;
	return QRPATH_SUCCESS;
}

int qrpath_unset(
		qrpath_t *self,
		int x,
		int y)
{
	if(x < 0 || x >= self->w)
		return QRPATH_OUT_OF_BOUNDS;
	else if(y < 0 || y >= self->h)
		return QRPATH_OUT_OF_BOUNDS;
	unset_bit(self, x, y);
	self->modified = true;
	return QRPATH_SUCCESS;
}

int qrpath_isset(
		qrpath_t *self,
		int x,
		int y)
{
	if(x < 0 || x >= self->w)
		return QRPATH_OUT_OF_BOUNDS;
	else if(y < 0 || y >= self->h)
		return QRPATH_OUT_OF_BOUNDS;
	else
		return get_bit(self, x, y) ? QRPATH_SET : QRPATH_UNSET;
}

int qrpath_run_even_odd(
		qrpath_t *self,
		void (*begin)(void *user, int x, int y, int area),
		void (*end)(void *user),
		void (*lineto)(void *user, int x, int y),
		void *user)
{
	int x;
	int y;
	int area;

	if(self->modified)
		update(self);

	area = 1;
	for(y = 0; y < self->h; y++)
		for(x = 0; x < self->w; x++)
			if(get_area(self, x, y) == area) {
				walk_ccw(self, x, y, area, South, begin, end, lineto, user);
				area++;
			}
	area = -1;
	for(y = 0; y < self->h; y++)
		for(x = 0; x < self->w; x++)
			if(get_area(self, x, y) == area) {
				walk_ccw(self, x, y, area, South, begin, end, lineto, user);
				area--;
			}
	return QRPATH_SUCCESS;
}

const int *qrpath_areas(
		qrpath_t *self)
{
	if(self->modified)
		update(self);
	return self->areas;
}

void qrpath_dump(
		qrpath_t *self)
{
	int x;
	int y;
	if(self->modified)
		update(self);
	printf("======================= BITS =======================\n");
	for(y = 0; y < self->h; y++) {
		for(x = 0; x < self->w; x++)
			printf("%c", get_bit(self, x, y) ? 'X' : '-');
		printf("\n");
	}
	printf("====================== AREAS =======================\n");
	for(y = 0; y < self->h; y++) {
		for(x = 0; x < self->w; x++)
			printf("%2d ", get_area(self, x, y));
		printf("\n");
	}
	printf("====================================================\n");
}

