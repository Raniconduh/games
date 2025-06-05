#include "board.h"
#include "piece.h"
#include <stdio.h>
#include <curses.h>
#include <stdlib.h>
#include <locale.h>
#include <math.h>

#define WHITE_CELL 1
#define BLUE_CELL 2
#define CHECK_CELL 3

int
main()
{
	board	       *game_board = init_board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR/");
	if (game_board == NULL)
		printf("error initializing board...\n");
	setlocale(LC_ALL, "");
	initscr();
	noecho();
	cbreak();
	start_color();

	init_pair(WHITE_CELL, COLOR_BLACK, COLOR_CYAN);
	init_pair(BLUE_CELL, COLOR_WHITE, COLOR_BLACK);
	init_pair(CHECK_CELL, COLOR_WHITE, COLOR_RED);

	int		min_size = fmin(COLS, LINES);
	int		cell_size = min_size / SIZE_STD;
	int		padding = (COLS - cell_size * SIZE_STD * 2) / 2;

	WINDOW	       *win = newwin(cell_size * SIZE_STD, cell_size * SIZE_STD * 2, 0, padding);
	WINDOW	       *ui_board[SIZE_STD][SIZE_STD];

	keypad(win, 1);
	box(win, 0, 0);
	touchwin(win);
	int		input = 0;
	int		sel_row = 0, sel_col = 0;
	int		selected = 0;
	position	origin;
	position	target;
	position       *amoves = NULL;
	for (int row = 0; row < SIZE_STD; row++) {
		for (int col = 0; col < SIZE_STD; col++) {
			ui_board[row][col] = subwin(win, cell_size, cell_size * 2, row * cell_size, padding + cell_size * col * 2);
			//box(ui_board[row][col], 0, 0);
			wbkgd(ui_board[row][col], COLOR_PAIR(((row + col) % 2 == 0) ? BLUE_CELL : WHITE_CELL));
			touchwin(win);
			if (game_board->game[row][col].piece == NULL)
				continue;
			wmove(ui_board[row][col], cell_size / 2, cell_size / 2);
			waddstr(ui_board[row][col], game_board->game[row][col].piece->pretty);
		}
	}
	while (input != 'q') {
		int		x, y;
		getparyx(ui_board[sel_row][sel_col], y, x);
		wmove(win, y + cell_size / 2, x + cell_size / 2);
		switch (input = wgetch(win)) {
		case KEY_UP:
			sel_row = fmin(sel_row + 1, SIZE_STD - 1);
			break;
		case KEY_DOWN:
			sel_row = fmax(0, sel_row - 1);
			break;
		case KEY_LEFT:
			sel_col = fmax(0, sel_col - 1);
			break;
		case KEY_RIGHT:
			sel_col = fmin(SIZE_STD - 1, sel_col + 1);
			break;
		case 'c':
			if (selected) {
				target = coords_to_pos(sel_row, sel_col);
				int		valid = validate_move(origin, target, game_board, amoves);

				/* clean all the hints */
				for (int i = 0; amoves[i].rank != -1; i++) {
					wmove(ui_board[amoves[i].rank][amoves[i].file], cell_size / 2, cell_size / 2);
					waddstr(ui_board[amoves[i].rank][amoves[i].file],
						game_board->game[amoves[i].rank][amoves[i].file].piece == NULL ? " " :
						game_board->game[amoves[i].rank][amoves[i].file].piece->pretty);

					wrefresh(ui_board[amoves[i].rank][amoves[i].file]);
				}

				if (valid == -1) {
					free(amoves);
					amoves = NULL;
					selected = 0;
					break;
				}

				/* move the piece */
				position       *to_refresh = move_piece(origin, target, game_board, amoves);

				/*
				 * clean the possible moves as we do not need
				 * them anymore after validation
				 */
				free(amoves);
				amoves = NULL;
				selected = 0;

				/*
				 * calculate moves a second time to find
				 * following checks
				 */
				free(moves(game_board->game[target.rank][target.file].piece, target,
				game_board->game, &game_board->game_flags));
				/*
				 * we do not actually need those moves, so
				 * that's why we just free them afterwards,
				 * but it will update any checks it finds if
				 * that piece were to move
				 */
				int		i = 0;
				while (to_refresh != NULL && to_refresh[i].rank != -1 && to_refresh[i].file != -1) {
					char	       *sym = game_board->game[to_refresh[i].rank][to_refresh[i].file].piece == NULL ?
					" " :
					game_board->game[to_refresh[i].rank][to_refresh[i].file].piece->pretty;
					wmove(ui_board[to_refresh[i].rank][to_refresh[i].file], cell_size / 2, cell_size / 2);
					waddstr(ui_board[to_refresh[i].rank][to_refresh[i].file], sym);
					wrefresh(ui_board[to_refresh[i].rank][to_refresh[i].file]);
					i++;
				}
				free(to_refresh);
				/*
				 * /\* fill original cell with an empty piece
				 * *\/
				 */
				/*
				 * wmove(ui_board[origin.rank][origin.file],
				 * cell_size / 2, cell_size / 2);
				 */
				/*
				 * waddstr(ui_board[origin.rank][origin.file],
				 * " ");
				 */

				/*
				 * /\* fill target cell with the new piece
				 * symbol *\/
				 */
				/*
				 * wmove(ui_board[target.rank][target.file],
				 * cell_size / 2, cell_size / 2);
				 */
				/*
				 * waddstr(ui_board[target.rank][target.file],
				 * game_board->game[target.rank][target.file].piece->pretty);
				 */

				/* /\* refresh both subwindows *\/ */
				/*
				 * wrefresh(ui_board[origin.rank][origin.file]);
				 */
				/*
				 * wrefresh(ui_board[target.rank][target.file]);
				 */

			} else {
				origin = coords_to_pos(sel_row, sel_col);
				/* if selected piece cell is empty, ignore it */
				if (game_board->game[sel_row][sel_col].piece == NULL)
					break;

				/*
				 * generate all possible valid moves for
				 * chosen piece
				 */
				amoves = moves(game_board->game[sel_row][sel_col].piece, coords_to_pos(sel_row, sel_col),
				 game_board->game, &game_board->game_flags);
				/* if no moves have been returned, ignore */
				if (amoves == NULL)
					break;

				/* draw all possible moves as hints */
				for (int i = 0; amoves[i].rank != -1; i++) {
					wmove(ui_board[amoves[i].rank][amoves[i].file], cell_size / 2, cell_size / 2);
					/*
					 * draw an "x" if there would be a
					 * capturing move, else draw a "o"
					 */
					if (game_board->game[amoves[i].rank][amoves[i].file].piece != NULL) {
						waddstr(ui_board[amoves[i].rank][amoves[i].file], "x");
					} else {
						waddstr(ui_board[amoves[i].rank][amoves[i].file], "o");
					}
					/* refresh hint subwindow */
					wrefresh(ui_board[amoves[i].rank][amoves[i].file]);
				}
				selected = 1;
			}
		default:
			continue;
		}
		wrefresh(win);
	}
	endwin();
	free_board(game_board);
	return 0;
}
