#include <ncurses.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

/* BEGIN CONFIG */
#ifndef X
#define X 64
#endif /* X */

#ifndef Y
#define Y 32
#endif /* Y */

#ifndef SEED
#include <time.h>
#define SEED time(NULL)
#endif /* SEED */

#ifndef DELAY
#define DELAY 100
#endif /* DELAY */

#ifndef PATH_BENDS
#define PATH_BENDS 3
#endif /* PATH_BENDS */

#ifndef ATTACK_ANIMATION_DELAY
#define ATTACK_ANIMATION_DELAY 35
#endif /* ATTACK_ANIMATION_DELAY */

#ifndef MAX_ENEMIES
#define MAX_ENEMIES 256
#endif /* MAX_ENEMIES */

#ifndef MAX_TURRETS
#define MAX_TURRETS 128
#endif /* MAX_TURRETS */

#ifndef STARTING_CASH
#define STARTING_CASH 120
#endif /* STARTING_CASH */

#ifndef STARTING_LIVES
#define STARTING_LIVES 25
#endif /* STARTING_LIVES */

#ifndef STARTING_ROUND
#define STARTING_ROUND 1
#endif /* STARTING_ROUND */
/* END CONFIG */

#define ARRLEN(a) (sizeof(a)/sizeof(*a))

#define BIN1(a) (a & 1)
#define BIN2(a,b) ((BIN1(a) << 1 ) | BIN1(b))
#define BIN4(a,b,c,d) ((BIN2(a,b) << 2) | BIN2(c,d))
#define BIN8(a,b,c,d,e,f,g,h) ((BIN4(a,b,c,d) << 4) | BIN4(e,f,g,h))

#define CELL_PATH_MASK BIN8(0,0,0,0,1,1,0,0)
#define INT_TO_CELL_PATH(i) (((i) & (CELL_PATH_MASK >> 2)) << 2)
#define CELL_PATH_TO_INT(c) (((c) & CELL_PATH_MASK) >> 2)

#define CELL_TURRET_MASK BIN8(1,1,1,1,0,0,0,0)
#define INT_TO_CELL_TURRET(i) (((i) & (CELL_TURRET_MASK >> 4)) << 4)
#define CELL_TURRET_TO_INT(c) (((c) & CELL_TURRET_MASK) >> 4)

#define SHOP_STARTX (X + 2)
#define SHOP_STARTY (1)

#define SHOP_ID_TO_X(id) (SHOP_STARTX)
#define SHOP_ID_TO_Y(id) ((id) * 2 + SHOP_STARTY + 2)

#define Y_TO_SHOP_ID(y) (((y) - SHOP_STARTY) % 2 == 1 ?     \
                         (-1) :                             \
                         (((y) - SHOP_STARTY - 2) / 2))


enum color {
	RED = 1,
	GREEN,
	YELLOW,
	BLUE,
	MAGENTA,
	CYAN,
};

enum state {
	RUNNING,
	PAUSED,
	GAME_OVER,
	QUIT,
};

enum cell {
	CELL_EMPTY  = 0,
	CELL_PATH   = 1 << 0,
	CELL_TURRET = 1 << 1,
};

enum cell_path {
	CELL_PATH_UP,
	CELL_PATH_DOWN,
	CELL_PATH_LEFT,
	CELL_PATH_RIGHT,
};


struct enemies {
	struct enemy {
		int x;
		int y;
		int count;
		int ticks;
	} enemies[MAX_ENEMIES];

	int idx;
	int last_round;
	int spawned;
	int killed;
	int to_spawn;

	int spawnx;
	int spawny;
};


struct {
	char * name;
	char symbol;
	int cost;    // cost to purchase
	int radius;  // attack radius
	int rsplash; // slash damage radius
	int damage;  // damage per attack
	int dsplash; // splash damage
	int stack;   // amount of turrets per tile (-1 for attacking turrets)
	int ticks;   // ticks between attacks
	int n_upgrades; // number of upgrades
	struct {
		int cost;
		int radius;
		int rsplash;
		int damage;
		int dsplash;
		int ticks;
	} upgrades[4];
} turrets[] = {
/*    Name             Sym  Cost  Rad RSplsh Dmg DSplsh Stack  Tick nUpgr*/
	{"Spikes",         'M',  25,   0,   0,    1,   0,    10,     1,   0,},
	{"Gunner",         '%', 100,   3,   0,    1,   0,    -1,     5,   3, {
	                       { 25,   4,   0,    1,   0,            5     },
					       { 50,   4,   0,    2,   0,            5     },
					       { 75,   4,   0,    3,   0,            4     },
	}},
	{"Sniper",         '|', 125,  25,   0,    1,   0,    -1,    40,   4, {
	                       {100,  25,   0,    2,   0,           32     },
						   {125,  25,   0,    3,   0,           27     },
						   {200,  25,   0,    5,   0,           20     },
						   {400,  25,   0,    5,   0,           10     },
	}},
	{"Bomb Lobber",    '&', 165,   3,   1,    1,   1,    -1,    10,   3, {
	                       { 50,   3,   2,    1,   1,           10     },
					       {100,   4,   2,    1,   2,           10     },
					       {150,   4,   2,    1,   2,            6     },
	}},
	{"Machine Gunner", '$', 400,   2,   0,    1,   0,    -1,     3,   4, {
	                       {200,   3,   0,    1,   0,            3     },
						   {400,   4,   0,    2,   0,            2     },
						   {800,   5,   0,    4,   0,            2     },
						   {8000,  5,   0,    4,   0,            1     },
	}},
};

struct turrets {
	struct spawned_turret {
		int x;
		int y;
		int id;
		int radius;
		int rsplash;
		int damage;
		int dsplash;
		int stack;
		int ticks;
		int level; // upgrade level
		int kills;
	} spawned[MAX_TURRETS];

	int idx;
} spawned_turrets;


unsigned long ticks = 0;


void enemies_push(struct enemies * enemies, int x, int y, int count, int ticks) {
	int idx = enemies->idx;
	enemies->enemies[idx].x = x;
	enemies->enemies[idx].y = y;
	enemies->enemies[idx].count = count;
	enemies->enemies[idx].ticks = ticks;
	enemies->spawned += count;
	enemies->idx++;
}


void enemies_pop(struct enemies * enemies, int i) {
	enemies->idx--;
	memmove(&enemies->enemies[i], &enemies->enemies[i + 1],
	        (MAX_ENEMIES - (i + 1)) * sizeof(*enemies->enemies));
}


void turrets_push(
	struct turrets * spawned_turrets, char grid[X][Y], int x, int y, int id
) {
	int idx = spawned_turrets->idx;
	spawned_turrets->spawned[idx].x = x;
	spawned_turrets->spawned[idx].y = y;
	spawned_turrets->spawned[idx].id = id;
	spawned_turrets->spawned[idx].radius = turrets[id].radius;
	spawned_turrets->spawned[idx].rsplash = turrets[id].rsplash;
	spawned_turrets->spawned[idx].damage = turrets[id].damage;
	spawned_turrets->spawned[idx].dsplash = turrets[id].dsplash;
	spawned_turrets->spawned[idx].stack = turrets[id].stack;
	spawned_turrets->spawned[idx].ticks = turrets[id].ticks;
	spawned_turrets->spawned[idx].level = 0;
	spawned_turrets->spawned[idx].kills = 0;
	spawned_turrets->idx++;

	grid[x][y] |= CELL_TURRET | INT_TO_CELL_TURRET(id);
}

void turrets_pop(struct turrets * spawned, char grid[X][Y], int x, int y, int i) {
	spawned->idx--;
	memmove(&spawned->spawned[i], &spawned->spawned[i + 1],
	        (MAX_TURRETS - (i + 1)) * sizeof(*spawned->spawned));

	grid[x][y] &= ~CELL_TURRET | INT_TO_CELL_TURRET(0xFF);
}


int rand_range(int lo, int hi) {
	return rand() % (hi - lo) + lo;
}


void generate_path(char grid[X][Y], struct enemies * enemies) {
	int spawnx = 0;
	int spawny = rand() % Y;
	enemies->spawnx = spawnx;
	enemies->spawny = spawny;

	int lastx = spawnx;
	int lasty = spawny;
	for (int i = 0; i < PATH_BENDS; i++) {
		int bendx = rand_range(lastx + 1, (i + 1) * X / PATH_BENDS);
		int bendy = rand_range(0, Y);

		// horizontal run
		for (int x = lastx; x < bendx; x++) {
			grid[x][lasty] |= CELL_PATH | INT_TO_CELL_PATH(CELL_PATH_RIGHT);
		}

		// vertical connection
		if (lasty != bendy) {
			if (lasty < bendy) for (int y = lasty; y < bendy; y++) {
				grid[bendx][y] |= CELL_PATH | INT_TO_CELL_PATH(CELL_PATH_DOWN);
			} else for (int y = lasty; y > bendy; y--) {
				grid[bendx][y] |= CELL_PATH | INT_TO_CELL_PATH(CELL_PATH_UP);
			}
		}

		lastx = bendx;
		lasty = bendy;
	}

	// connect to edge
	for (int x = lastx; x < X; x++) {
		grid[x][lasty] |= CELL_PATH | INT_TO_CELL_PATH(CELL_PATH_RIGHT);
	}
}


char grid_getc(char grid[X][Y], int x, int y) {
	char c = grid[x][y];
	char out = ' ';
	if (c & CELL_TURRET) out = turrets[CELL_TURRET_TO_INT(c)].symbol;
    else if (c & CELL_PATH) switch(CELL_PATH_TO_INT(c)) {
		case CELL_PATH_UP:    out = '^'; break;
		case CELL_PATH_DOWN:  out = 'v'; break;
		case CELL_PATH_LEFT:  out = '<'; break;
		case CELL_PATH_RIGHT: out = '>'; break;
	}
	return out;
}


int grid_getcolor(char grid[X][Y], int x, int y) {
	char c = grid[x][y];
	int out = 0;
	if (c & CELL_TURRET) out = A_UNDERLINE;
	else if (c & CELL_PATH) out = A_DIM;
	return out;
}


void draw_grid(char grid[X][Y]) {
	addch('+');
	for (int x = 0; x < X; x++) addch('-');
	addch('+');
	addch('\n');
	for (int y = 0; y < Y; y++) {
		addch('|');
		for (int x = 0; x < X; x++) {
			int cp = grid_getcolor(grid, x, y);
			attron(cp);
			addch(grid_getc(grid, x, y));
			attroff(cp);
		}
		addstr("|\n");
	}
	addch('+');
	for (int x = 0; x < X; x++) addch('-');
	addch('+');
	addch('\n');
}



int get_shop_str(char * s, int n, int i) {
	return snprintf(s, n, "%s: $%d", turrets[i].name,
	                turrets[i].cost);
}


int yx_to_shop_id(int y, int x) {
	int id = Y_TO_SHOP_ID(y);
	if (id < 0) return -1;
	if (id >= ARRLEN(turrets)) return -1;

	int len = get_shop_str(NULL, 0, id);
	int off = x - SHOP_ID_TO_X(id);
	if (off < 0 || off >= len) return -1;
	return id;
}


void draw_shop(int cash) {
	int y = SHOP_STARTY;
	int x = SHOP_STARTX;
	mvaddstr(y, x,   "Shop:");
	mvaddstr(++y, x, "-----");

	char s[25];
	for (int i = 0; i < ARRLEN(turrets); i++) {
		x = SHOP_ID_TO_X(i);
		y = SHOP_ID_TO_Y(i);
		move(y, x);
		int len = get_shop_str(s, ARRLEN(s), i);
		if (cash >= turrets[i].cost) attron(A_REVERSE);
		addstr(s);
		if (cash >= turrets[i].cost) attroff(A_REVERSE);
	}
}


void erase_shop(void) {
	int y = SHOP_STARTY;
	int x = SHOP_STARTX;
	move(y, x);
	clrtoeol();
	move(++y, x);
	clrtoeol();

	for (int i = 0; i < ARRLEN(turrets); i++) {
		x = SHOP_ID_TO_X(i);
		y = SHOP_ID_TO_Y(i);
		move(y, x);
		clrtoeol();
	}
}


void draw_radius(char grid[X][Y], int x, int y, int r) {
	attron(A_REVERSE);
	for (int rx = -r + 1; rx < r; rx++) {
		for (int ry = -r + 1; ry < r; ry++) {
			if (rx*rx + ry*ry > r*r) continue;
			int dx = rx + x;
			int dy = ry + y;
			if (dx < 0) dx = 0;
			else if (dx >= X) dx = X - 1;
			if (dy < 0) dy = 0;
			else if (dy >= Y) dy = Y - 1;
			move(dy + 1, dx + 1);
			addch(grid_getc(grid, dx, dy));
		}
	}
	attroff(A_REVERSE);
}


int try_purchase(char grid[X][Y], int id, int cash, struct turrets * spawned_turrets) {
	int cost = turrets[id].cost;
	int radius = turrets[id].radius;
	if (cost > cash) return -1;
	timeout(-1);

	bool done = false;
	while (!done) {
		move(Y + 2, X + 2 - 10);
		addstr("q to abort");
		move(Y + 3, X + 2 - 14);
		addstr("Click to place");
		refresh();

		switch (getch()) {
			case 'q':
				cost = 0;
				done = true;
				break;
			case KEY_MOUSE:;
				MEVENT e;
				if (getmouse(&e) != OK) break;
				int x = e.x - 1;
				int y = e.y - 1;
				if (x < 0 || x >= X || y < 0 || y >= Y) break;
				// turrets with a nonnegative stack must be placed on paths
				// other turrets can only by placed on empty cells
				if ((grid[x][y] & CELL_TURRET) ||
				       (turrets[id].stack < 0 && (grid[x][y] & CELL_PATH))) {
					break;
				}
				if (turrets[id].stack > 0 && !(grid[x][y] & CELL_PATH)) break;

				// draw radius
				draw_radius(grid, x, y, radius);
				move(e.y, e.x);
				addch(turrets[id].symbol);
				move(Y + 3, X + 2 - 15);
				addstr("Enter to accept");
				refresh();
				if (getch() != '\n') {
					cost = 0;
					done = true;
					break;
				}

				turrets_push(spawned_turrets, grid, x, y, id);
				done = true;
				break;
		}
	}

	timeout(DELAY);
	return cost;
}


int try_upgrade(int id, int cash, struct turrets * spawned_turrets, char grid[X][Y]) {
	int cost = 0;
	timeout(-1);

	struct spawned_turret * st = &spawned_turrets->spawned[id];

	erase_shop();
	int y = SHOP_STARTY;
	int x = SHOP_STARTX;
	move(y, x);
	addstr(turrets[st->id].name);
	addch(':');
	move(++y, x);
	int nlen = strlen(turrets[st->id].name);
	for (int i = 0; i < nlen + 1; i++) addch('-');

	move(SHOP_ID_TO_Y(0), SHOP_ID_TO_X(0));
	attron(A_REVERSE);
	addstr("Turret Info");
	attroff(A_REVERSE);

	int tid = st->id;
	int max_level = turrets[tid].n_upgrades;
	int cur_level = st->level;
	int up_idx = cur_level;
	// sell for 75% of purchasing cost
	int selling = 0;
	selling += turrets[tid].cost;
	for (int i = 0; i < cur_level; i++) {
		selling += turrets[tid].upgrades[i].cost;
	}
	if (turrets[tid].stack > 0) {
		selling *= st->stack;
		selling /= turrets[tid].stack;
	}
	selling *= 3;
	selling /= 4;
	move(SHOP_ID_TO_Y(1), SHOP_ID_TO_X(1));
	attron(A_REVERSE);
	printw("Sell: -$%d", selling);
	attroff(A_REVERSE);

	if (cur_level < max_level) {
		cost = turrets[tid].upgrades[up_idx].cost;
		bool affordable = true;
		if (cash < cost) affordable = false;
		move(SHOP_ID_TO_Y(2), SHOP_ID_TO_X(2));
		if (affordable) attron(A_REVERSE);
		printw("Upgrade: $%d", cost);
		if (affordable) attroff(A_REVERSE);


	}

	draw_radius(grid, st->x, st->y, st->radius);

	bool done = false;
	while (!done) {
		move(Y + 2, X + 2 - 12);
		addstr("Any to abort");
		// clear the pause/resume prompt
		move(Y + 3, X + 2 - 14);
		clrtoeol();
		refresh();

		switch (getch()) {
			case KEY_MOUSE:;
				MEVENT e;
				if (getmouse(&e) != OK) break;
				int x = e.x - 1;
				int y = e.y - 1;
				int sid = yx_to_shop_id(e.y, e.x);
				// turret info
				if (sid == 0) {
					int kills = st->kills;
					int ticks = st->ticks;
					int rad = st->radius;
					int dmg = st->damage;
					int rsp = st->rsplash;
					int dsp = st->dsplash;
					int stk = st->stack;
					int py, px;
					py = SHOP_STARTY;
					px = SHOP_STARTX;
					erase_shop();
					move(py, px);
					addstr(turrets[st->id].name);
					addch(':');
					move(++py, px);
					for (int i = 0; i < nlen + 1; i++) addch('-');

					mvprintw(++py, px, "Kills: %d", kills); 
					mvprintw(++py, px, "Level: %d", cur_level);
					mvprintw(++py, px, "Attack Ticks: %d", ticks);
					mvprintw(++py, px, "Radius: %d", rad);
					mvprintw(++py, px, "Damage: %d", dmg);
					if (rsp > 0 && dsp > 0) {
						mvprintw(++py, px, "Splash Radius: %d", rsp);
						mvprintw(++py, px, "Splash Damage: %d", dsp);
					}
					if (stk > 0) {
						mvprintw(++py, px, "Stacked: %d", stk);
					}

					move(Y + 2, X + 2 - 13);
					addstr("Any to finish");
					refresh();
					getch();
					done = true;
					cost = 0;
					break;
				// sell turret
				} else if (sid == 1) {
					cost = -selling;
					done = true;
				// upgrade turret
				} else if (sid == 2) {
					if (cash < cost) break;
					st->level++;
					st->radius = turrets[tid].upgrades[up_idx].radius;
					st->rsplash = turrets[tid].upgrades[up_idx].rsplash;
					st->damage = turrets[tid].upgrades[up_idx].damage;
					st->dsplash = turrets[tid].upgrades[up_idx].dsplash;
					st->ticks = turrets[tid].upgrades[up_idx].ticks;
					done = true;
					break;
				}
				break;
			default:
				cost = 0;
				done = true;
				break;
		}
	}

	timeout(DELAY);
	return cost;
}


void draw_enemies(struct enemies * enemies) {
	for (int i = 0; i < enemies->idx; i++) {
		if (enemies->enemies[i].count == 0) continue;
		int cp = 0;
		switch (enemies->enemies[i].count) {
			case 1: cp = COLOR_PAIR(RED); break;
			case 2: cp = COLOR_PAIR(CYAN); break;
			case 3: cp = COLOR_PAIR(GREEN); break;
			case 4: cp = COLOR_PAIR(YELLOW); break;
			default: cp = COLOR_PAIR(MAGENTA); break;
		}
		int x = enemies->enemies[i].x;
		int y = enemies->enemies[i].y;
		move(1 + y, 1 + x);
		attron(cp);
		addch('@');
		attroff(cp);
	}
}


int get_spawn_rate(int round) {
	if (round <= 10) return 5;
	if (round <= 35) return rand_range(2,4);
	return rand_range(1, 4);
}


int get_speed(int round) {
	if (round <= 10) return 5;
	if (round <= 35) return rand_range(3, 5);
	return rand_range(1, 3);
}


int get_stack(int round) {
	int lower_rounds[] = {
		1, 1, 1, 2, 2,
	};
	int mid_rounds[] = {
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		2, 2, 2, 2, 2, 2, 2, 2, 2,
		2, 2, 2, 2, 2, 3, 3, 3, 3,
		3, 3, 3, 3, 3, 3, 4, 4, 4,
	};
	int mid_rounds2[] = {
		2, 2, 2, 3, 3, 3, 3, 3, 3,
		4, 4, 4, 4, 4, 4, 4, 5, 5,
		5, 5, 6, 6, 6, 7, 7, 8, 9
	};
	if (round <= 10) return lower_rounds[rand_range(0, ARRLEN(lower_rounds))];
	if (round <= 35) return mid_rounds[rand_range(0, ARRLEN(mid_rounds))];
	if (round <= 60) return mid_rounds2[rand_range(0, ARRLEN(mid_rounds2))];
	return rand_range(5, 20);
}


int get_to_spawn(int round) {
	int ret = 0;
	if (round <= 3) ret = (round + 1) * 4 + rand_range(0, 2);
	else if (round <= 20) ret = round * 3 + rand_range(0, 5);
	else if (round <= 40) ret = 2 * MAX_ENEMIES / 3 + rand_range(-5, 5);
	else ret =  7 * MAX_ENEMIES / 8 + rand_range(-5, 5);
	return ret > MAX_ENEMIES - 20 ? MAX_ENEMIES - 20 : ret;
}


int spawn_enemies(struct enemies * enemies, char grid[X][Y], int round) {
	if (enemies->last_round != round) {
		enemies->spawned = 0;
		enemies->killed = 0;
		enemies->last_round = round;
		enemies->to_spawn = get_to_spawn(round);
	}

	int deaths = 0;
	
	// advance all enemies
	// iterate backward so that popping doesn't mess up the iteration
	for (int i = enemies->idx - 1; i >= 0; i--) {
		if (enemies->enemies[i].count == 0) continue;
		if (ticks % enemies->enemies[i].ticks != 0) continue;
		int x = enemies->enemies[i].x;
		int y = enemies->enemies[i].y;
		int c = grid[x][y];
		if (c & CELL_PATH) switch (CELL_PATH_TO_INT(c)) {
			case CELL_PATH_UP:    y--; break;
			case CELL_PATH_DOWN:  y++; break;
			case CELL_PATH_LEFT:  x--; break;
			case CELL_PATH_RIGHT: x++; break;
		}
		enemies->enemies[i].x = x;
		enemies->enemies[i].y = y;
		if (x >= X || y >= Y) {
			enemies->killed += enemies->enemies[i].count;
			deaths += enemies->enemies[i].count;
			enemies_pop(enemies, i);
		}
	}

	if (enemies->spawned >= enemies->to_spawn) return deaths;

	int spawn_rate = get_spawn_rate(round);
	int speed = get_speed(round);
	int stack = get_stack(round);
	if (ticks % spawn_rate == 0) {
		enemies_push(enemies, enemies->spawnx, enemies->spawny, stack, speed);
	}

	return deaths;
}


int find_nearest_enemy(struct enemies * enemies, int x, int y, int rad) {
	if (enemies->idx == 0) return -1;

	int nearestx = X + 1;
	int nearesty = Y + 1;
	int nearestid = -1;

	for (int i = 0; i < enemies->idx; i++) {
		int ex = enemies->enemies[i].x - x;
		int ey = enemies->enemies[i].y - y;
		if (ex*ex + ey*ey > rad*rad) continue;
		if (ex*ex + ey*ey < nearestx*nearestx + nearesty*nearesty) {
			nearestx = ex;
			nearesty = ey;
			nearestid = i;
		}
	}

	return nearestid;
}


int attack_enemy(struct enemies * enemies, int id, int dmg) {
	int kills = enemies->enemies[id].count;
	enemies->enemies[id].count -= dmg;
	if (dmg >= kills) {
		enemies_pop(enemies, id);
	} else kills = dmg;
	enemies->killed += kills;
	return kills;
}


int splash_enemies(struct enemies * enemies, int x, int y, int rad, int dmg) {
	int kills = 0;
	for (int i = 0; i < enemies->idx; i++) {
		int ex = enemies->enemies[i].x - x;
		int ey = enemies->enemies[i].y - y;
		if (ex*ex + ey*ey > rad*rad) continue;

		kills += attack_enemy(enemies, i, dmg);
	}

	return kills;
}


int run_turrets(struct turrets * spawned, struct enemies * enemies, char grid[X][Y]) {
	int kills = 0;

	for (int i = 0; i < spawned->idx; i++) {
		int tid = spawned->spawned[i].id;
		int tx = spawned->spawned[i].x;
		int ty = spawned->spawned[i].y;
		int radius = spawned->spawned[i].radius;
		int damage = spawned->spawned[i].damage;
		int rsplash = spawned->spawned[i].rsplash;
		int dsplash = spawned->spawned[i].dsplash;

		if (ticks % spawned->spawned[i].ticks != 0) continue;

		int nearest = find_nearest_enemy(enemies, tx, ty, radius);
		if (nearest < 0) continue;

		int nx = enemies->enemies[nearest].x;
		int ny = enemies->enemies[nearest].y;

		// damage animation
		move(ny + 1, nx + 1);
		attron(A_REVERSE);
		addch(grid_getc(grid, nx, ny));
		refresh();
		napms(ATTACK_ANIMATION_DELAY);
		attroff(A_REVERSE);
		refresh();

		int just_killed = attack_enemy(enemies, nearest, damage);
		if (rsplash > 0 && dsplash > 0)
			just_killed += splash_enemies(enemies, nx, ny, rsplash, dsplash);

		kills += just_killed;
		if (spawned->spawned[i].stack > 0) {
			spawned->spawned[i].stack -= just_killed;
			if (spawned->spawned[i].stack <= 0) {
				turrets_pop(spawned, grid, tx, ty, i);
			}
		}

		spawned->spawned[i].kills += just_killed;
	}

	return kills;
}


bool no_enemies(struct enemies * enemies) {
	return enemies->killed >= enemies->to_spawn;
}


int main(void) {
	srand(SEED);

	initscr();
	noecho();
	curs_set(0);
	keypad(stdscr, TRUE);
	timeout(DELAY);

	mousemask(BUTTON1_CLICKED, NULL);

	use_default_colors();
	start_color();
	init_pair(RED, COLOR_RED, -1);
	init_pair(GREEN, COLOR_GREEN, -1);
	init_pair(YELLOW, COLOR_YELLOW, -1);
	init_pair(BLUE, COLOR_BLUE, -1);
	init_pair(MAGENTA, COLOR_MAGENTA, -1);
	init_pair(CYAN, COLOR_CYAN, -1);

	while (true) {
		char grid[X][Y] = {};
		struct enemies enemies = {};
		struct turrets spawned_turrets = {};

		int cash = STARTING_CASH;
		int lives = STARTING_LIVES;
		int round = STARTING_ROUND;
		int score = 0;

		generate_path(grid, &enemies);

		bool paused = true;
		int done = RUNNING;
		while (!done) {
			if (!paused) {
				int deaths = spawn_enemies(&enemies, grid, round);
				lives -= deaths;

				if (no_enemies(&enemies)) {
					round++;
				}

				if (lives < 0) {
					done = GAME_OVER;
				}

				int killed = run_turrets(&spawned_turrets, &enemies, grid);
				if (killed >= 0) {
					cash += killed;
					score += killed;
				}
			}

			erase();
			draw_grid(grid);
			draw_enemies(&enemies);
			draw_shop(cash);
			move(Y + 2, 0);
			printw("Round: %d\n", round);
			printw("Lives: %d\n", lives);
			printw("Cash: %d\n", cash);
			printw("Score: %d\n", score);
			move(Y + 2, X + 2 - 9);
			addstr("q to quit");
			move(Y + 3, X + 2 - 14);
			if (paused) addstr(" Any to resume");
			else addstr("Space to pause");
			refresh();

			switch (getch()) {
				case 'q': done = QUIT; break;
				case ' ':
					paused = !paused;
					break;
				case KEY_MOUSE:;
					MEVENT e;
					if (getmouse(&e) != OK) break;

					int id = yx_to_shop_id(e.y, e.x);
					if (id >= 0) {
						int cost = try_purchase(grid, id, cash, &spawned_turrets);
						if (cost < 0) {
							move(Y/2 + 1, X/2 + 1 - 9);
							attron(A_REVERSE);
							addstr("Insufficient Funds");
							attroff(A_REVERSE);
							refresh();
							napms(500);
						} else {
							cash -= cost;
						}
					} else {
						int tid = -1;
						int x, y;
						for (int i = 0; i < spawned_turrets.idx; i++) {
							x = spawned_turrets.spawned[i].x;
							y = spawned_turrets.spawned[i].y;
							if (x != e.x - 1 || y != e.y - 1) continue;
							
							tid = i;
							break;
						}

						if (tid >= 0) {
							int cost = try_upgrade(tid, cash, &spawned_turrets, grid);
							cash -= cost;
							// turret was sold, remove it
							if (cost < 0) {
								turrets_pop(&spawned_turrets, grid, x, y, tid);
							}
						}
					}
					break;
				case ERR: break;
				default:
					if (paused) paused = false;
					break;
			}

			ticks++;
		}

		switch (done) {
			case QUIT: goto terminate;
			case GAME_OVER:
				move(Y/2 + 1, X/2 + 1 - 12);
				addch(' ');
				attron(A_REVERSE);
				addstr("You ran out of lives :(");
				attroff(A_REVERSE);
				addch(' ');
				move(Y/2 + 2, X/2 + 1 - 12);
				addch(' ');
				attron(A_REVERSE);
				printw("Final Score: %9d ", score);
				attroff(A_REVERSE);
				addch(' ');
				break;
		}

		move(Y/2 + 3, X/2 + 1 - 14);
		addch(' ');
		attron(A_REVERSE);
		addstr("Any to play again, q to quit");
		attroff(A_REVERSE);
		addch(' ');
		timeout(-1);
		if (getch() == 'q') goto terminate;
		timeout(DELAY);
	}

	terminate:
	keypad(stdscr, FALSE);
	curs_set(1);
	echo();
	endwin();
}
