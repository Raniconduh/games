#include <time.h>
#include <stdlib.h>
#include <ncurses.h>
#include <stdbool.h>
#include <sys/time.h>


/* BEGIN CONFIG */
#ifndef X
#define X 10
#endif /* X */

#ifndef Y
#define Y 20
#endif /* Y */

#ifndef HIGHLIGHT
#define HIGHLIGHT false
#endif /* HIGHLIGHT */

#ifndef SEED
#define SEED time(NULL)
#endif /* SEED */

#ifndef GRAVITY
#define GRAVITY 750
#endif /* GRAVITY */
/* END CONFIG */

#define TSTOMS(TS) ((TS.tv_sec * 1000000 + TS.tv_nsec / 1000) / 1000)


struct coord {
	int x, y;
};


enum tetromino {
	EMPTY,
	TET_Q, /* SQUARE */
	TET_I,
	TET_L,
	TET_J,
	TET_S,
	TET_Z,
	TET_T,
	LAST,
};


enum color {
	RED = 1,
	GREEN,
	YELLOW,
	BLUE,
	MAGENTA,
	CYAN,
	WHITE
};


enum state {
	RUNNING,
	GAME_OVER,
	QUIT,
};


/* (0,0) is the center of the top row */
struct coord tetrominos[][4] = {
	[TET_Q] = {{0, 0}, {1, 0}, {0, 1}, {1, 1}},
	[TET_I] = {{0, 1}, {0, 0}, {0, 2}, {0, 3}},
	[TET_L] = {{0, 1}, {0, 0}, {0, 2}, {1, 2}},
	[TET_J] = {{0, 1}, {0, 0}, {0, 2}, {-1,2}},
	[TET_S] = {{0, 0}, {1, 0}, {0, 1}, {-1,1}},
	[TET_Z] = {{0, 0}, {-1,0}, {0, 1}, {1, 1}},
	[TET_T] = {{0, 0}, {-1,0}, {1, 0}, {0, 1}},
};


bool valid(int x, int y) {
	return x >= 0 && y >= 0 && x < X && y < Y;
}


void cptogrid(struct coord tet[4], enum tetromino t, enum tetromino grid[X][Y]) {
	for (int i = 0; i < 4; i++) {
		int x = tet[i].x;
		int y = tet[i].y;
		grid[x][y] = t;
	}
}


bool down1(struct coord tet[4], enum tetromino t,  enum tetromino grid[X][Y]) {
	bool active = true;

	for (int i = 0; i < 4; i++) {
		int nx = tet[i].x;
		int ny = tet[i].y + 1;
		if (!valid(nx, ny) || grid[nx][ny]) {
			cptogrid(tet, t, grid);
			active = false;
			break;
		}
	}
	if (active) for (int i = 0; i < 4; i++) tet[i].y++;

	return active;
}


int nlen(int n) {
	int l = 1;
	while ((n /= 10) != 0) l++;
	return l;
}


void draw_grid(enum tetromino grid[X][Y], int level, int score) {
	erase();
	for (int x = 0; x < X; x++) addstr("++++");
	addstr("+\n");
	for (int y = 0; y < Y; y++) {
		addch('+');
		for (int x = 0; x < X; x++) {
			addch(' ');
			int cp = COLOR_PAIR((enum color)grid[x][y]);
			if (grid[x][y] && HIGHLIGHT) cp |= A_REVERSE;
			attron(cp);
			if (!HIGHLIGHT) addch(grid[x][y] ? '#' : ' ');
			else addch(' ');
			attroff(cp);
			addch(' ');
			if (x < X - 1) addch('.');
		}
		addstr("+\n");
	}
	for (int x = 0; x < X; x++) addstr("++++");
	addch('+');

	mvprintw(0, 0, "Level: %d", level);
	mvprintw(0, X * 4 + 1 - 7 - nlen(score), "Score: %d", score);
}


int main(void) {
	srand(SEED);

	initscr();
	noecho();
	curs_set(0);
	keypad(stdscr, TRUE);
	timeout(0);

	use_default_colors();
	start_color();
	init_pair(RED, COLOR_RED, -1);
	init_pair(GREEN, COLOR_GREEN, -1);
	init_pair(YELLOW, COLOR_YELLOW, -1);
	init_pair(BLUE, COLOR_BLUE, -1);
	init_pair(MAGENTA, COLOR_MAGENTA, -1);
	init_pair(CYAN, COLOR_CYAN, -1);
	init_pair(WHITE, COLOR_WHITE, -1);

	while (true) {
		enum tetromino grid[X][Y] = {0};

		int level = 0;
		int score = 0;
		int total_cleared = 0;

		bool active = false;
		struct coord cur_tet[4];
		enum tetromino tet_type;

		int reftime = 0;
		{
			struct timespec ts;
			clock_gettime(CLOCK_MONOTONIC, &ts);
			reftime = TSTOMS(ts);
		}

		enum state done = RUNNING;
		while (!done) {
			if (!active) {
				int t = rand() % ((LAST-1) - (EMPTY+1) + 1) + (EMPTY+1);
				tet_type = t;
				for (int i = 0; i < 4; i++) {
					int y = tetrominos[t][i].y;
					int x = tetrominos[t][i].x + X / 2;
					if (grid[x][y]) {
						done = GAME_OVER;
						goto finished;
					}
					cur_tet[i].x = x;
					cur_tet[i].y = y;
				}
				active = true;
			}

			draw_grid(grid, level, score);

			// draw current tetromino
			int cp = COLOR_PAIR((enum color)tet_type);
			if (HIGHLIGHT) cp |= A_REVERSE;
			attron(cp);
			for (int i = 0; i < 4; i++) {
				int x = cur_tet[i].x * 4 + 2;
				int y = cur_tet[i].y + 1;
				if (!HIGHLIGHT) mvaddch(y, x, '@');
				else mvaddch(y, x, ' ');
			}
			attroff(cp);

			// draw tetromino shadow
			int x_lo = X;
			int x_hi = -1;
			for (int i = 0; i < 4; i++) {
				int x = cur_tet[i].x;
				if (x > x_hi) x_hi = x;
				if (x < x_lo) x_lo = x;
			}

			x_lo = x_lo * 4 + 2;
			x_hi = x_hi * 4 + 2;

			for (int i = x_lo; i <= x_hi; i++) {
				int y = Y + 1;
				mvaddch(y, i, '_');
			}

			refresh();

			switch (getch()) {
				case KEY_DOWN:
					active = down1(cur_tet, tet_type, grid);
					if (active) score++;
					break;
				case KEY_LEFT:
					{
					bool can_move = true;
					for (int i = 0; i < 4; i++) {
						int x = cur_tet[i].x - 1;
						int y = cur_tet[i].y;
						if (!valid(x, y) || grid[x][y]) {
							can_move = false;
							break;
						}
					}
					if (!can_move) break;

					for (int i = 0; i < 4; i++) cur_tet[i].x--;
					}
					break;
				case KEY_RIGHT:
					{
					bool can_move = true;
					for (int i = 0; i < 4; i++) {
						int x = cur_tet[i].x + 1;
						int y = cur_tet[i].y;
						if (!valid(x, y) || grid[x][y]) {
							can_move = false;
							break;
						}
					}
					if (!can_move) break;

					for (int i = 0; i < 4; i++) cur_tet[i].x++;
					}
					break;
				case KEY_UP:
					// squares don't really rotate
					if (tet_type == TET_Q) break;
					/* x' = y_c - y + x_c
					 * y' = x - x_c + y_c
					 */
					bool can_rotate = true;
					struct coord ncoords[4];
					int x_c = cur_tet[0].x;
					int y_c = cur_tet[0].y;

					for (int i = 0; i < 4; i++) {
						int x = y_c - cur_tet[i].y + x_c;
						int y = cur_tet[i].x - x_c + y_c;
						ncoords[i] = (struct coord){x, y};

						if (!valid(ncoords[i].x, ncoords[i].y) || grid[x][y]) {
							can_rotate = false;
							break;
						}
					}
					if (!can_rotate) break;

					for (int i = 0; i < 4; i++) {
						cur_tet[i] = ncoords[i];
					}
					break;
				case ' ':
					while ((active = down1(cur_tet, tet_type, grid)))
						score++;
					break;
				case 'q':
					done = QUIT;
					goto finished;
				case ERR:
				deafult: break;
			}

			struct timespec ts;
			clock_gettime(CLOCK_MONOTONIC, &ts);
			int dt = TSTOMS(ts) - reftime;
			if (dt >= GRAVITY - ((GRAVITY/10) * level)) {
				reftime = TSTOMS(ts);
				active = down1(cur_tet, tet_type, grid);
			}

			if (!active) {
				int first_row = -1;

				int rows_cleared = 0;
				for (int y = Y - 1; y >= 0; y--) {
					bool full_row = true;
					for (int x = 0; x < X; x++) {
						if (!grid[x][y]) {
							full_row = false;
							break;
						}
					}

					if (full_row) {
						draw_grid(grid, level, score);

						if (first_row < 0) first_row = y;

						attron(COLOR_PAIR(GREEN) | A_REVERSE);
						for (int x = 1; x < 4 * X; x++) {
							mvaddch(y + 1, x, '#');	
						}
						attroff(COLOR_PAIR(GREEN) | A_REVERSE);

						for (int x = 0; x < X; x++) {
							grid[x][y] = 0;
						}

						refresh();
						napms(1000);
						rows_cleared++;
						total_cleared++;

						if (total_cleared >= 10) {
							level++;
							total_cleared -= 10;
						}
					}
				}

				if (rows_cleared > 0) {
					for (int y = first_row - rows_cleared; y >= 0; y--) {
						for (int x = 0; x < X; x++) {
							grid[x][y + rows_cleared] = grid[x][y];
						}
					}
				}

				switch (rows_cleared) {
					case 1:
						score += 40 * (level + 1);
						break;
					case 2:
						score += 100 * (level + 1);
						break;
					case 3:
						score += 300 * (level + 1);
						break;
					case 4:
						score += 1200 * (level + 1);
						break;
				}
			}
		}

		finished:
		switch (done) {
			case GAME_OVER:
				move((Y + 1) / 2, (X * 4 + 2) / 2 - 5);
				if (!HIGHLIGHT) attron(A_REVERSE);
				addstr("Game over");
				move((Y + 1) / 2 + 1, (X * 4 + 2) / 2 - 13);
				addstr("Press any key to continue");
				if (!HIGHLIGHT) attroff(A_REVERSE);
				refresh();
				timeout(-1);
				getch();
				timeout(0);
				break;
			case QUIT:
				goto terminate;
		}
	}

	terminate:
	keypad(stdscr, FALSE);
	curs_set(1);
	echo();
	endwin();
}
