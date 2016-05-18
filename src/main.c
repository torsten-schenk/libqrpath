#include <qrpath/qrpath.h>

enum {
	Width = 6,
	Height = 6
};

static const char *Pattern = 
	"  XXX "
	"  X X "
	"  X X "
	"  XXX "
	"   X  "
	"    XX";

static void begin(
		void *user,
		int x,
		int y,
		int area)
{
	printf("BEGIN: %d %d  %d\n", x, y, area);
}

static void end(
		void *user)
{
	printf("END\n");
}

static void lineto(
		void *user,
		int x,
		int y)
{
	printf("LINETO: %d %d\n", x, y);
}

int main()
{
	int x;
	int y;
	qrpath_t *path = qrpath_new(Width, Height);
	for(y = 0; y < Height; y++)
		for(x = 0; x < Width; x++)
			if(Pattern[y * Width + x] == 'X')
				qrpath_set(path, x, y);
	qrpath_dump(path);
	qrpath_run_even_odd(path, begin, end, lineto, NULL);
	return 0;
}

