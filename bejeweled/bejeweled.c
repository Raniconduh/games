#include <time.h>
#include <stdlib.h>
#include <ncurses.h>
#include <stdbool.h>


#ifndef HIGHLIGHT
#define HIGHLIGHT false
#endif /* HIGHLIGHT */

#ifndef X
#define X 16
#endif /* X */

#ifndef Y
#define Y 16
#endif /* Y */

#ifndef SHIFT_DELAY
#define SHIFT_DELAY 25
#endif /* SHIFT_DELAY */

#ifndef MATCH_DELAY
#define MATCH_DELAY 750
#endif /* MATCH_DELAY */

#ifndef BAD_MATCH_DELAY
#define BAD_MATCH_DELAY 750
#endif /* BAD_MATCH_DELAY */

#define SQUARE_C   '#'
#define CIRCLE_C   'O'
#define TRIANGLE_C 'A'
#define RHOMBUS_C  '='
#define DIAMOND_C  'm'
#define HEXAGON_C  '&'


#define POP (1<<9)

#define ISPOP(g) (g & POP)
#define GETGEM(g) (g & 0xFF)

#define GETCOLOR(g) (COLOR_PAIR(g + 1))
#define VALID(x, y) (x < X && y < Y && x >= 0 && y >= 0)


enum gem {
	SQUARE,
	CIRCLE,
	TRIANGLE,
	RHOMBUS,
	DIAMOND,
	HEXAGON,
	LAST_GEM
};


enum {
	RED = 1,
	GREEN,
	YELLOW,
	BLUE,
	MAGENTA,
	CYAN,
};


struct coord {
	int x, y;
};


char gemc[] = {
	[SQUARE]   = SQUARE_C,
	[CIRCLE]   = CIRCLE_C,
	[TRIANGLE] = TRIANGLE_C,
	[RHOMBUS]  = RHOMBUS_C,
	[DIAMOND]  = DIAMOND_C,
	[HEXAGON]  = HEXAGON_C,
};

const struct coord invc = {-1, -1};
enum gem grid[X][Y];
struct coord first = {-1, -1};
int score = 0;


int abs(int n) {
	return n < 0 ? -n : n;
}


int nlen(int n) {
	int len = 0;
	while (n > 0) {
		len++;
		n /= 10;
	}
	return len;
}


void newgem(struct coord c) {
	int x = c.x;
	int y = c.y;

	bool done;
	do {
		done = true;
		grid[x][y] = rand() % LAST_GEM;
		if (VALID(x - 1, y) && grid[x][y] == grid[x - 1][y]) done = false;
		if (VALID(x, y - 1) && grid[x][y] == grid[x][y - 1]) done = false;
	} while (!done);
}


void display(void) {
	erase();
	for (int x = 0; x < X; x++) addstr("+ - ");
	addstr("+\n");
	for (int y = 0; y < Y; y++) {
		for (int x = 0; x < X; x++) {
			addstr("| ");
			int cp = GETCOLOR(GETGEM(grid[x][y]));
			bool reverse = false;
			if (HIGHLIGHT) reverse = true;;
			if (first.x == x && first.y == y) reverse = !reverse;
			if (ISPOP(grid[x][y])) reverse = !reverse;
			if (reverse) cp |= A_REVERSE;
			attron(cp);
			addch(gemc[GETGEM(grid[x][y])]);
			attroff(cp);
			addch(' ');
		}
		addstr("|\n");
		for (int x = 0; x < X; x++) addstr("+ - ");
		addstr("+\n");
	}

	attron(A_REVERSE);
	move(Y*2+1, (X*4+2)/2 - (nlen(score) + 8)/2);
	printw("Score: %d", score);
	attroff(A_REVERSE);
	move(Y*2+1, 0);
	addstr("r to restart");

	refresh();
}


int checkvert(int x, int y, struct coord vc[]) {
	int s = 0;

	int n = y;
	while (VALID(x, n) && grid[x][n] == grid[x][y]) {
		vc[s++] = (struct coord){x, n};
		n++;
	}
	n = y - 1;
	while (VALID(x, n) && grid[x][n] == grid[x][y]) {
		vc[s++] = (struct coord){x, n};
		n--;
	}

	return s;
}


int checkhoriz(int x, int y, struct coord hc[]) {
	int s = 0;

	int n = x;
	while (VALID(n, y) && grid[n][y] == grid[x][y]) {
		hc[s++] = (struct coord){n, y};
		n++;
	}
	n = x - 1;
	while (VALID(n, y) && grid[n][y] == grid[x][y]) {
		hc[s++] = (struct coord){n, y};
		n--;
	}

	return s;
}


void shift(struct coord s[], int n) {
	for (int i = 0; i < n; i++) {
		struct coord sc = s[i];
		for (int n = sc.y; n > 0; n--) {
			grid[sc.x][n] = grid[sc.x][n - 1];
			display();
			napms(SHIFT_DELAY);
		}

		newgem((struct coord){sc.x, 0});
	}
}


void makepop(struct coord s[], int n) {
	for (int i = 0; i < n; i++) {
		struct coord c = s[i];
		grid[c.x][c.y] |= POP;
	}
}


void sort(struct coord s[], int n) {
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n - 1; j++) {
			if (s[j].y > s[j + 1].y) {
				struct coord t = s[j];
				s[j] = s[j + 1];
				s[j + 1] = t;
			}
		}
	}
}


int main(void) {
	srand(time(NULL));

	initscr();
	noecho();
	curs_set(0);
	keypad(stdscr, TRUE);

	mousemask(BUTTON1_CLICKED, NULL);

	use_default_colors();
	start_color();
	init_pair(RED, COLOR_RED, -1);
	init_pair(GREEN, COLOR_GREEN, -1);
	init_pair(YELLOW, COLOR_YELLOW, -1);
	init_pair(BLUE, COLOR_BLUE, -1);
	init_pair(MAGENTA, COLOR_MAGENTA, -1);
	init_pair(CYAN, COLOR_CYAN, -1);

	restart:
	while (true) {
		// generate a grid where each element is unique from its direct neighbors
		for (int x = 0; x < X; x++) {
			for (int y = 0; y < Y; y++) {
				newgem((struct coord){x, y});
			}
		}

		first = invc;

		while (true) {
			display();


			switch (getch()) {
				case 'r':
					goto restart;
				case KEY_MOUSE:;
					MEVENT e;
					if (getmouse(&e) != OK) break;

					// get and VALIDate x and y coords
					if (e.x % 4 == 0 || e.y % 2 == 0) break;
					int x = (e.x - 2) / 4;
					int y = (e.y - 1) / 2;
					if ((e.x - 1) % 4 == 0) x++;
					if (!VALID(x, y)) break;

					if (first.x == -1 && first.y == -1) {
						first.x = x;
						first.y = y;
						break;
					}

					if (first.x == x && first.y == y) {
						first = invc;
						break;
					}

					if (abs(first.x - x) + abs(first.y - y) > 1) {
						first = invc;
						break;
					}

					// swap the gems and see what happens
					enum gem t = grid[first.x][first.y];
					grid[first.x][first.y] = grid[x][y];
					grid[x][y] = t;

					struct coord vc[Y];
					int v = checkvert(x, y, vc);
					struct coord hc[X];
					int h = checkhoriz(x, y, hc);

					if (v >= 3) {
						first = invc;
						sort(vc, v);
						makepop(vc, v);
						display();
						napms(MATCH_DELAY);
						shift(vc, v);

						score += 1000;
						v -= 3;
						score += 2000*v;
					} else if (h >= 3) {
						first = invc;
						makepop(hc, h);
						display();
						napms(MATCH_DELAY);
						shift(hc, h);

						score += 1000;
						h -= 3;
						score += 2000*h;
					} else {
						// no matches
						struct coord c = first;
						first.x = x;
						first.y = y;
						display();
						napms(BAD_MATCH_DELAY);	
						first = c;
						enum gem t = grid[first.x][first.y];
						grid[first.x][first.y] = grid[x][y];
						grid[x][y] = t;

						first = invc;
					}

					// hacky, ugly way to clear all gems
					int nc;
					do {
						nc = 0;

						for (int nx = 0; nx < X; nx++) {
							for (int ny = 0; ny < Y; ny++) {
								struct coord vc[Y];
								int v = checkvert(nx, ny, vc);
								struct coord hc[X];
								int h = checkhoriz(nx, ny, hc);
								if (v >= 3) {
									nc += v;

									sort(vc, v);
									makepop(vc, v);
									display();
									napms(750);
									shift(vc, v);

									score += 1000;
									v -= 3;
									score += 2000*v;
								} else if (h >= 3) {
									nc += h;

									makepop(hc, h);
									display();
									napms(750);
									shift(hc, h);

									score += 1000;
									h -= 3;
									score += 2000*h;
								}
							}
						}
					} while (nc != 0);
					break;
				case 'q':
					goto terminate;
			}
		}
	}

	terminate:
	echo();
	curs_set(1);
	endwin();
}
