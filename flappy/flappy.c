#include <time.h>
#include <stdlib.h>
#include <ncurses.h>
#include <stdbool.h>


#define X COLS
#define Y LINES

#define N_PIPES (X / 30)
#define OPENING (Y / 8)
#define JUMP (Y / 14)


enum {
	YELLOW = 1,
	GREEN,
};


enum state {
	GAME_OVER = 0,
	RUNNING = 1,
};


struct coord {
	int x, y;
};


int nlen(int n) {
	int len = 1;
	while (n > 0) {
		len++;
		n /= 10;
	}
	return len;
}


int rrand(int hi, int lo) {
	return rand() % (hi - lo + 1) + lo;
}


int main() {
	srand(time(NULL));

	initscr();
	noecho();
	curs_set(0);

	timeout(0);

	use_default_colors();
	start_color();
	init_pair(YELLOW, COLOR_YELLOW, -1);
	init_pair(GREEN, COLOR_GREEN, -1);

	while (true) {
		struct coord bird = {X / (N_PIPES+1), (Y - 1) / 2};
		int last_movement = 0;

		struct coord pipes[N_PIPES];
		for (int i = 0; i < N_PIPES; i++) {
			pipes[i] = (struct coord){X / (N_PIPES+1) + ((X / (N_PIPES+1)) * (i + 1)), rrand(Y-OPENING/2, OPENING/2)};
		}
		int curpipe = 0;

		int score = 0;

		enum state state = RUNNING;
		while (state == RUNNING) {
			erase();

			move(bird.y, bird.x);
			attron(COLOR_PAIR(YELLOW));
			addch('@');
			attroff(COLOR_PAIR(YELLOW));

			attron(COLOR_PAIR(GREEN));
			for (int i = 0; i < N_PIPES; i++) {
				// create the top half of the pipe
				for (int y = 0; y < pipes[i].y - OPENING / 2; y++) {
					for (int x = -1; x <= 1; x++) {
						move(y, pipes[i].x + x);
						addch('#');
					}
				}

				// bottom half
				for (int y = pipes[i].y + OPENING / 2; y < Y; y++) {
					for (int x = -1; x <= 1; x++) {
						move(y, pipes[i].x + x);
						addch('#');
					}
				}
			}
			attroff(COLOR_PAIR(GREEN));

			move(0, 0);
			printw("Score: %d", score);

			refresh();

			// check bird position
			if (bird.y >= Y) {
				state = GAME_OVER;
				break;
			}

			int px, py;
			px = pipes[curpipe].x;
			py = pipes[curpipe].y;
			if (bird.x >= px - 1 && bird.x <= px + 1 && (bird.y < py - OPENING / 2 || bird.y > py + OPENING / 2)) {
				state = GAME_OVER;
				break;
			} else if (bird.x == pipes[curpipe].x + 1) {
				score++;

				pipes[curpipe] = (struct coord){X / (N_PIPES+1) + ((X / (N_PIPES+1)) * N_PIPES), rrand(Y-OPENING/2, OPENING/2)};

				curpipe++;
				if (curpipe >= N_PIPES) curpipe = 0;
			}

			switch (getch()) {
				case ' ':
					last_movement = 0;
					if (bird.y == 0) break;
					bird.y -= JUMP;

					if (bird.y < 0) bird.y = 0;
					break;
				case 'q':
				case KEY_RESIZE:
					goto terminate;
					break;
				default:
					break;
			}


			// move everything now
			bird.y += ++last_movement / 3;

			for (int i = 0; i < N_PIPES; i++) {
				pipes[i].x--;
			}

			napms(100);
		}

		timeout(-1);
		switch (state) {
			case GAME_OVER:
				attron(A_REVERSE);
				move(Y / 2, X / 2 - 5);
				addstr("Game Over!");
				move(Y / 2 + 1, (X - nlen(score)) / 2 - 6);
				printw("Final Score: %d", score);
				attroff(A_REVERSE);
				refresh();

				if (getch() == 'q') goto terminate;
				break;
			default: break;
		}
		timeout(0);
	}

	terminate:
	echo();
	curs_set(1);
	endwin();
}
