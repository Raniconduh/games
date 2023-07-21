#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <ncurses.h>
#include <string.h>


/* BEGIN CONFIG */
#ifndef Y
#define Y 16
#endif /* Y */

#ifndef X
#define X 16
#endif /* X */

#ifndef MINES
// number of mines
#define MINES 35
#endif /* MINES */

#ifndef HIGHLIGHT_SQUARES
// reverse the highlighting of squares
#define HIGHLIGHT_SQUARES true
#endif /* HIGHLIGHT_SQUARES */
/* END CONFIG */


#define ARRLEN(a) (sizeof(a)/sizeof(*a))


enum color {
	RED = 1,
	GREEN,
	CYAN,
	BLUE,
	MAGENTA,
	BLACK,
};


enum state {
	RUNNING,
	QUIT,
	GAME_OVER,
	WIN,
};


struct square {
	bool revealed;
	bool mine;
	bool flagged;
	int neighbors;
};


struct {
	 int x, y;
} around[8] = {
	{.x = -1, .y = -1}, {.x = 0, .y = -1}, {.x = +1, .y = -1},
	{.x = -1, .y = 0},                     {.x = +1, .y = 0},
	{.x = -1, .y = +1}, {.x = 0, .y = +1}, {.x = +1, .y = +1}
};


struct square grid[X][Y] = {0};
int total_revealed = 0;
int total_flags = 0;


void global_reset(void) {
	memset(grid, 0, sizeof(grid));
	total_revealed = 0;
	total_flags = 0;
}


bool won(void) {
	return total_revealed >= X*Y - MINES;
}


int count_mines(int x, int y) {
	int n = 0;

	for (int i = 0; i < ARRLEN(around); i++) {
		int nx = x + around[i].x;
		int ny = y + around[i].y;
		if (nx < 0 || ny < 0 || nx >= X || ny >= Y) continue;
		if (grid[nx][ny].mine) n++;
	}

	return n;
}


void reveal(int x, int y) {
	if (grid[x][y].revealed) return;
	total_revealed++;
	grid[x][y].revealed = true;
}


void reveal_blank(int x, int y) {
	struct coord {
		int x, y;
	} coords[8];
	for (int i = 0; i < ARRLEN(coords); i++) coords[i] = (struct coord){-1, -1};
	int n = 0;

	for (int i = 0; i < ARRLEN(around); i++) {
		int nx = x + around[i].x;
		int ny = y + around[i].y;
		if (nx < 0 || ny < 0 || nx >= X || ny >= Y) continue;
		struct square s = grid[nx][ny];
		if (s.mine) continue;
		if (s.neighbors == 0 && !s.revealed) {
			reveal(nx, ny);

			coords[n].x = nx;
			coords[n].y = ny;
			n++;
		} else if (s.neighbors != 0) reveal(nx, ny);
	}

	for (int i = 0; i < n; i++) {
		reveal_blank(coords[i].x, coords[i].y);
	}
}


void create_grid(int x, int y) {
	struct {
		int x, y;
	} exclude[9] = {0};
	int n = 0;

	exclude[n].x = x;
	exclude[n].y = y;
	n++;

	// squares around the grid that mines cannot spawn in
	for (int i = 0; i < ARRLEN(around); i++) {
		int nx, ny;
		nx = x + around[i].x;
		ny = y + around[i].y;

		if (nx < 0 || ny < 0 || nx >= X || ny >= Y) continue;
		else {
			exclude[n].x = nx;
			exclude[n].y = ny;
			n++;
		}
	}

	bool done = false;
	int tries = 0;
	do {
		for (int i = 0; i < MINES; i++) {
			int mx, my;
			bool done = false;
			do {
				mx = rand() % X;
				my = rand() % Y;

				bool illegal = false;
				for (int i = 0; i < n; i++) {
					if (exclude[i].x == mx && exclude[i].y == my) {
						illegal = true;
						break;
					}
				}

				if (illegal) continue;

				if (!grid[mx][my].mine) {
					grid[mx][my].mine = true;
					done = true;
				}
			} while (!done);
		}


		for (int x = 0; x < X; x++) {
			for (int y = 0; y < Y; y++) {
				grid[x][y].neighbors = count_mines(x, y);
			}
		}

		// prevent insta-wins
		reveal_blank(x, y);
		if (!won()) done = true;
		else global_reset();
		tries++;

	} while (tries < 10 && !done);
}


int nlen(int n) {
	int len = 0;
	while (n > 0) {
		len++;
		n /= 10;
	}
	return len;
}


int main(void) {
	srand(time(NULL));

	initscr();
	noecho();
	curs_set(0);
	keypad(stdscr, TRUE);

	mousemask(ALL_MOUSE_EVENTS, NULL);

	use_default_colors();
	start_color();
	init_pair(RED, COLOR_RED, -1);
	init_pair(GREEN, COLOR_GREEN, -1);
	init_pair(CYAN, COLOR_CYAN, -1);
	init_pair(BLUE, COLOR_BLUE, -1);
	init_pair(MAGENTA, COLOR_MAGENTA, -1);
	init_pair(BLACK, COLOR_WHITE, COLOR_BLACK);


	while (true) {
		bool grid_exists = false;
		bool flag_mode = false;

		clear();

		time_t start_time;

		enum state done = RUNNING;
		bool display_once_more = false;
		while (!done || display_once_more) {
			erase();
			// draw the board
			for (int x = 0; x < X; x++) addstr("+ - ");
			addstr("+\n");

			for (int y = 0; y < Y; y++) {
				for (int x = 0; x < X; x++) {
					addstr("| ");
					struct square s = grid[x][y];
					if (done == GAME_OVER && s.mine) {
						attron(COLOR_PAIR(RED) | A_REVERSE);
						addch('*');
						attroff(COLOR_PAIR(RED) | A_REVERSE);
					} else if (s.flagged) {
						int cp = COLOR_PAIR(RED);
						char flagc = '!';
						if (!HIGHLIGHT_SQUARES) cp |= A_REVERSE;
						if (done == GAME_OVER) {
							cp = COLOR_PAIR(CYAN) | A_REVERSE;
							flagc = '?';
						}
						attron(cp);
						addch(flagc);
						attroff(cp);
					} else if (s.revealed) {
						int cp = 0;
						switch (s.neighbors) {
						case 0: cp = 0; break;
						case 1: cp = COLOR_PAIR(CYAN); break;
						case 2: cp = COLOR_PAIR(MAGENTA); break;
						case 3: cp = COLOR_PAIR(BLACK); break;
						case 4: cp = COLOR_PAIR(BLUE); break;
						case 5: cp = COLOR_PAIR(CYAN); break;
						case 6: cp = COLOR_PAIR(MAGENTA); break;
						case 7: cp = COLOR_PAIR(BLACK); break;
						case 8: cp = COLOR_PAIR(BLUE); break;
						}
						if (HIGHLIGHT_SQUARES && cp) cp |= A_REVERSE;
						if (done == GAME_OVER) cp = 0;
						attron(cp);
						addch(s.neighbors > 0 ? '0' + s.neighbors : ' ');
						attroff(cp);
					} else {
						int cp = COLOR_PAIR(GREEN);
						if (done == GAME_OVER) cp = 0;
						attron(cp);
						addch('#');
						attroff(cp);
					}
					addch(' ');
				}
				addstr("|\n");

				for (int x = 0; x < X; x++) addstr("+ - ");
				addstr("+\n");
			}

			int y, x;
			getyx(stdscr, y, x);
			move(y, (X*4+2)/2 - (24+nlen(MINES)+nlen(total_flags))/2);
			printw("MINES: %d   FLAGS PLACED: %d\n", MINES, total_flags);
			move(y+1, (X*4+2)/2 - 16);
			printw("'f' to toggle flag mode: now %s\n", flag_mode ? "ON" : "OFF");

			refresh();

			if (display_once_more) break;
			
			int c = getch();
			switch (c) {
			case KEY_MOUSE:;
				MEVENT event;
				if (getmouse(&event) != OK) break;
				// ignore clicks on grid patterning
				if (event.x % 4 == 0) break;
				if (event.y % 2 == 0) break;
				// convert to grid indices
				int x = (event.x - 2) / 4;
				int y = (event.y - 1) / 2;

				// fix clicks on left space in each cell
				if ((event.x - 1) % 4 == 0) x++;

				if (x > X || y > Y) break;

				if (!flag_mode && event.bstate & BUTTON1_CLICKED) {
					if (!grid_exists) {
						create_grid(x, y);
						start_time = time(NULL);
						grid_exists = true;
					}

					// clicking on a flag should do nothing
					if (grid[x][y].flagged) break;
					if (grid[x][y].mine) {
						done = GAME_OVER;
						display_once_more = true;
						break;
					}

					// don't waste time if the square is already revealed
					if (grid[x][y].revealed) break;
					reveal(x, y);

					if (grid[x][y].neighbors == 0) {
						reveal_blank(x, y);
					}

					if (won()) {
						done = WIN;
						display_once_more = true;
					}
				} else if (flag_mode || (!grid[x][y].revealed && event.bstate & BUTTON3_CLICKED)) {
					grid[x][y].flagged = !grid[x][y].flagged;
					if (grid[x][y].flagged) total_flags++;
					else total_flags--;
				}
				break;
			case 'f':
				flag_mode = !flag_mode;
				break;
			case 'q':
			case 27:
			   done = QUIT;
			   break;
			default:
			   break;
			}
		}

		switch (done) {
			case GAME_OVER:
				move(0, (X*4+2) / 2 - 3);
				attron(A_REVERSE | COLOR_PAIR(RED));
				addstr("BOOM!");
				attroff(A_REVERSE | COLOR_PAIR(RED));
				break;
			case WIN:
				move((Y*2+1)/2, (X*4+2) / 2 - 16);
				attron(A_REVERSE);
				addstr("All the mines have been found :)");
				attroff(A_REVERSE);
				break;
			case QUIT:
				goto terminate;
			default: break;
		}


		time_t end_time = time(NULL);

		move(Y*2+1, 0);
		clrtoeol();
		move(Y*2+1, (X*4+2)/2 - 14);
		attron(A_REVERSE);
		addstr("Any to play again, q to quit");
		attroff(A_REVERSE);
		move(Y*2+2, 0);
		clrtoeol();
		move(Y*2+2, (X*4+2)/2 - (4 + nlen(end_time - start_time)/2));
		printw("TIME: %ds", end_time - start_time);
		refresh();
		if (getch() == 'q') goto terminate;

		global_reset();
	}

	terminate:
	echo();
	curs_set(1);
	endwin();
}
