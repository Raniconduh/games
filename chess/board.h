#ifndef BOARD_H
#define BOARD_H
#include "piece.h"

#define FLAG_CHECK_WHITE 0x1
#define FLAG_CHECK_BLACK 0x2
#define FLAG_TURN_WHITE 0x4
#define FLAG_TURN_BLACK 0x8

typedef struct {
	uint8_t		game_flags;
	position       *last_check_line;
	cell		last_checking_piece;
	cell		game[SIZE_STD][SIZE_STD];
} board;

board	       *init_board();
int		is_in(position m, position * moves);
void		free_board(board * game);
position       *move_piece(position origin, position target, board * game, position * amoves);
int		validate_move(position origin, position target, board * game, position * amoves);
void		switch_turn(board * game);
#endif
