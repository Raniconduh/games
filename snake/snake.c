#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <stdbool.h>
#include <sys/time.h>


/* BEGIN CONFIG */
#ifndef Y
#define Y 16
#endif /* Y */

#ifndef X
#define X 32
#endif /* X */

#ifndef DELAY
// time between movements (ms)
#define DELAY 100
#endif /* DELAY */

#ifndef WALLS
// enable or disable solid walls
#define WALLS false
#endif /* WALLS */

#ifndef SEED
#include <time.h>
#define SEED time(NULL)
#endif /* SEED */
/* END CONFIG */


#if WALLS
#define WALLC '#'
#else
#define WALLC ' '
#endif


enum rstate {
	RUNNING,
	WALL_CRASH,
	SELF_HIT,
	QUIT,
};


enum color {
	RED = 1,
	MAGENTA,
	GREEN,
	CYAN,
};


enum direction {
	DIR_UP,
	DIR_DOWN,
	DIR_LEFT,
	DIR_RIGHT,
};


struct node {
	enum direction dir;

	bool pivot;
	int x;
	int y;

	struct node * next;
	struct node * prev;
};


struct snake {
	struct node * nodes;
	struct node * last;

	int length;
};


struct apple {
	int x;
	int y;
};


int nlen(int n) {
	int len = 0;
	while (n > 0) {
		len++;
		n /= 10;
	}
	return len;
}


int main() {
	srand(SEED);

	initscr();
	noecho();
	keypad(stdscr, true);
	curs_set(0);
	// the snake should move once every DELAY ms,
	// no matter if input is recieved
	timeout(DELAY);

	use_default_colors();
	start_color();
	init_pair(RED, COLOR_RED, -1);
	init_pair(MAGENTA, COLOR_MAGENTA, -1);
	init_pair(GREEN, COLOR_GREEN, -1);
	init_pair(CYAN, COLOR_CYAN, -1);

	while (true) {
		struct snake s = {
			.nodes = &(struct node){
				.dir = DIR_RIGHT,
				.pivot = false,
				.x = X / 2,
				.y = Y / 2,
				.next = NULL,
				.prev = NULL,
			},
			.last = NULL,
			.length = 1,
		};
		s.last = s.nodes;

		struct apple a = {
			.x = rand() % X,
			.y = rand() % Y,
		};

		int score = 1;

		enum rstate state = RUNNING;
		while (true) {
			// ate an apple!
			if (s.nodes->x == a.x && s.nodes->y == a.y) {
				score++;

				struct node * l = s.last;
				// add a new node
				s.length++;
				l->next = malloc(sizeof(struct node));
				struct node * nw = l->next;
				nw->dir = l->dir;
				nw->pivot = false;

				switch (l->dir) {
					case DIR_UP:
						nw->x = l->x;
						nw->y = l->y + 1;
						break;
					case DIR_DOWN:
						nw->x = l->x;
						nw->y = l->y - 1;
						break;
					case DIR_LEFT:
						nw->x = l->x + 1;
						nw->y = l->y;
						break;
					case DIR_RIGHT:
						nw->x = l->x - 1;
						nw->y = l->y;
						break;
				}

				nw->prev = l;
				nw->next = NULL;

				s.last = l->next;

				a.x = rand() % X;
				a.y = rand() % Y;
			}

			// move the snake
			for (struct node * n = s.last; n; n = n->prev) {
				switch (n->dir) {
					case DIR_UP:
						n->y--;
						break;
					case DIR_DOWN:
						n->y++;
						break;
					case DIR_LEFT:
						n->x--;
						break;
					case DIR_RIGHT:
						n->x++;
						break;
				}

				// teleport the snake in no-walls mode
				if (!WALLS) {
					if (n->x < 0) n->x = X - 1;
					else if (n->x >= X) n->x = 0;
					
					if (n->y < 0) n->y = Y - 1;
					else if (n->y >= Y) n->y = 0;
				}

				// set the direction of the next node to that of the previous one
				if (n->prev && n->prev->pivot) {
					n->dir = n->prev->dir;
					n->prev->pivot = false;
					n->pivot = true;
				}
			}

			// handle wall crash
			if (WALLS && (s.nodes->x < 0 || s.nodes->x >= X || s.nodes->y < 0 || s.nodes->y >= Y)) {
				state = WALL_CRASH;
				goto done;
			}

			// handle bumping into self
			for (struct node * n = s.nodes->next; n; n = n->next) {
				if (s.nodes->x == n->x && s.nodes->y == n->y) {
					state = SELF_HIT;
					goto done;
				}
			}

			erase();
			// draw the border
			attron(A_REVERSE | A_DIM | COLOR_PAIR(MAGENTA));
			for (int i = 0; i <= X + 1; i++) { // top
				move(0, i);
				addch(WALLC);
			}
			for (int i = 1; i < Y + 1; i++) { // middle
				move(i, 0);
				addch(WALLC);
				move(i, X + 1);
				addch(WALLC);
				
			}
			for (int i = 0; i <= X + 1; i++) { // bottom
				attron(A_REVERSE);
				move(Y + 1, i);
				addch(WALLC);
			}
			attroff(A_REVERSE | A_DIM | COLOR_PAIR(MAGENTA));
			// write the score
			move(0, X / 2 - 3 - nlen(score) / 2);
			attron(A_UNDERLINE);
			printw("Score: %d", score);
			attroff(A_UNDERLINE);
			// draw the snake
			move(s.nodes->y + 1, s.nodes->x + 1);
			// first the head
			attron(COLOR_PAIR(CYAN));
			switch (s.nodes->dir) {
				case DIR_UP:
					addch('^');
					break;
				case DIR_DOWN:
					addch('v');
					break;
				case DIR_LEFT:
					addch('<');
					break;
				case DIR_RIGHT:
					addch('>');
					break;
			}
			attroff(COLOR_PAIR(CYAN));
			// then the body
			attron(COLOR_PAIR(GREEN));
			for (struct node * n = s.nodes->next; n; n = n->next) {
				move(n->y + 1, n->x + 1);
				addch('*');
			}
			attroff(COLOR_PAIR(GREEN));
			// draw the apple
			move(a.y + 1, a.x + 1);
			attron(COLOR_PAIR(RED));
			addch('@');
			attroff(COLOR_PAIR(RED));
			refresh();

			// handle input
			struct timeval start;
			gettimeofday(&start, NULL);

			int c = getch();
			switch (c) {
				case KEY_UP:
					if (s.length > 1 && s.nodes->dir == DIR_DOWN) break;
					s.nodes->dir = DIR_UP;
					s.nodes->pivot = true;
					break;
				case KEY_DOWN:
					if (s.length > 1 && s.nodes->dir == DIR_UP) break;
					s.nodes->dir = DIR_DOWN;
					s.nodes->pivot = true;
					break;
					break;
				case KEY_LEFT:
					if (s.length > 1 && s.nodes->dir == DIR_RIGHT) break;
					s.nodes->dir = DIR_LEFT;
					s.nodes->pivot = true;
					break;
				case KEY_RIGHT:
					if (s.length > 1 && s.nodes->dir == DIR_LEFT) break;
					s.nodes->dir = DIR_RIGHT;
					s.nodes->pivot = true;
					break;
				case ' ':
					timeout(-1);
					move(Y / 2, X / 2 - 2);
					attron(A_REVERSE);
					addstr("Paused");
					attroff(A_REVERSE);
					int c;
					while ((c = getch()) != ' ') {
						if (c == 'q') {
							state = QUIT;
							goto done;
						}
					}
					timeout(DELAY);
					break;
				case 'q':
					state = QUIT;
					goto done;
			}

			struct timeval end;
			gettimeofday(&end, NULL);

			unsigned delta_ms = (end.tv_usec - start.tv_usec) * 1000;
			if (delta_ms < DELAY) {
				napms(delta_ms);
			}
		}

		done:
		switch (state) {
			case QUIT:
				goto terminate;
			case WALL_CRASH:
				move(Y / 2, X / 2 - 12);
				attron(A_REVERSE);
				addstr("You crashed into the wall");
				attroff(A_REVERSE);
				refresh();
				napms(1500);
				break;
			case SELF_HIT:
				move(Y / 2, X / 2 - 10);
				attron(A_REVERSE);
				addstr("You ran into yourself");
				attroff(A_REVERSE);
				refresh();
				napms(1500);
				break;
			default: break;
		}

		for (struct node * n = s.nodes->next; n; n = n->next) {
			free(n);
		}
	}

	terminate:
	curs_set(1);
	keypad(stdscr, false);
	echo();
	endwin();
}
