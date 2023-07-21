#include <string.h>
#include <ncurses.h>
#include <stdbool.h>


/* BEGIN CONFIG */
#define Y 16
#define X 16

#define DELAY 250

#define LIVE_C '#'
#define DEAD_C ' '
/* END CONFIG */


#define ARRLEN(a) (sizeof(a)/sizeof(*a))

#define LIVE 1<<0
#define DEAD 1<<1
#define TODIE 1<<2
#define TOLIVE 1<<3


enum {
	GREEN = 1,
};


struct {
	 int x, y;
} around[8] = {
	{.x = -1, .y = -1}, {.x = 0, .y = -1}, {.x = +1, .y = -1},
	{.x = -1, .y = 0},                     {.x = +1, .y = 0},
	{.x = -1, .y = +1}, {.x = 0, .y = +1}, {.x = +1, .y = +1}
};


void display(const char grid[X][Y], bool edit) {
	for (int x = 0; x < X; x++) addstr("+ - ");
	addstr("+\n");
	for (int y = 0; y < Y; y++) {
		for (int x = 0; x < X; x++) {
			addstr("| ");
			char c = grid[x][y];
			int cp = 0;
			if (edit) cp = COLOR_PAIR(GREEN);
			attron(cp);
			addch(c & LIVE ? LIVE_C : DEAD_C);
			attroff(cp);
			addch(' ');
		}
		addstr("|\n");
		for (int x = 0; x < X; x++) addstr("+ - ");
		addstr("+\n");
	}
}


int main() {
	initscr();
	noecho();
	curs_set(0);
	keypad(stdscr, TRUE);

	mousemask(BUTTON1_CLICKED, NULL);

	use_default_colors();
	start_color();
	init_pair(GREEN, COLOR_GREEN, -1);

	char grid[X][Y];
	memset(grid, DEAD, sizeof(grid));
	char initial_grid[X][Y];
	memset(initial_grid, DEAD, sizeof(grid));

	clear();

	bool edit_initial = true;
	while (true) {
		// initialize the board
		while (true) {
			erase();
			display(grid, true);

			move(0, (X*4+2)/2 - 5);
			attron(A_REVERSE);
			addstr("BOARD EDIT");
			move(Y*2, (X*4+2)/2 - 21);
			addstr("Enter to start | r to restart | q to quit");
			attroff(A_REVERSE);
			refresh();

			int c = getch();
			if (c == KEY_ENTER || c == '\n') break;
			if (c == 'q') goto terminate;
			if (c == 'r') {
				memset(grid, DEAD, sizeof(grid));
				continue;
			}
			if (c != KEY_MOUSE) continue;
			MEVENT e;
			if (getmouse(&e) != OK) continue;
			
			if (e.x % 4 == 0 || e.y % 2 == 0) continue;
			int x = (e.x - 2) / 4;
			int y = (e.y - 1) / 2;
			if ((e.x - 1) % 4 == 0) x++;
			if (x > X || y > Y) continue;

			if (grid[x][y] == LIVE) grid[x][y] = DEAD;
			else grid[x][y] = LIVE;
		}

		if (edit_initial) memcpy(initial_grid, grid, sizeof(grid));

		timeout(0);

		while (true) {
			// determine new cell states
			for (int x = 0; x < X; x++) {
				for (int y = 0; y < Y; y++) {
					int neighbors = 0;
					for (int i = 0; i < ARRLEN(around); i++) {
						int nx = x + around[i].x;
						int ny = y + around[i].y;
						if (nx < 0 || ny < 0 || nx >= X || ny >= Y) continue;
						if (grid[nx][ny] & LIVE) neighbors++;
					}

					if (neighbors == 3) grid[x][y] |= TOLIVE;
					else if (neighbors == 2 && grid[x][y] & LIVE) grid[x][y] |= TOLIVE;
					else grid[x][y] |= TODIE;
				}
			}

			// update all cells
			for (int x = 0; x < X; x++) {
				for (int y = 0; y < Y; y++) {
					if (grid[x][y] & TOLIVE) grid[x][y] = LIVE;
					else grid[x][y] = DEAD;
				}
			}

			erase();
			display(grid, false);

			move(Y*2, (X*4+2)/2 - 23);
			attron(A_REVERSE);
			addstr("q to quit | i to edit initial | e to edit now");
			attroff(A_REVERSE);
			refresh();

			int c = getch();
			if (c != ERR) {
				if (c == 'q') goto terminate;
				if (c == 'i') {
					edit_initial = true;
					break;
				}
				if (c == 'e') {
					edit_initial = false;
					break;
				}
			}

			napms(DELAY);
		}

		timeout(-1);
		if (edit_initial) memcpy(grid, initial_grid, sizeof(grid));
	}

	terminate:
	echo();
	curs_set(1);
	endwin();
}
