#include "board.h"
#include "piece.h"
#include "fmt_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ERROR -1

#define FREE_AND_FAIL(x)                                                       \
  free(x);                                                                     \
  return -1;

board	       *
init_board(char *fen)
{
	board	       *game;
	game = malloc(sizeof(board));
	if (game == NULL)
		exit(1);
	game->game_flags = FLAG_TURN_WHITE;

	/* init empty cells */
	for (int i = 0; i < SIZE_STD; i++) {
		for (int j = 0; j < 8; j++) {
			game->game[i][j] = (cell) {
				NULL, NONE, 0
			};
		}
	}
	/* game will be imported from FEN notation (standard if omitted) */
	fen_parser(fen, game);

	return game;
}

void
switch_turn(board * game)
{
	game->game_flags ^= FLAG_TURN_WHITE;
	game->game_flags ^= FLAG_TURN_BLACK;
}

void
free_board(board * game)
{
	free(game);
}

/*
 * we will use a position instead of a piece, as we just store all the
 * currently utilized cells (or positions) in the game array. They will just
 * point at the piece they hold, as there is only one instance of each piece.
 * We will just have a shared pointer between all cells for the same piece,
 * for example, the four bishops will just be stored as four entries in the
 * array, with their position as the array indices, and the data they point
 * to is the same *bishop variable.
 */
int
validate_move(position origin, position target, board * game, position * amoves)
{
	cell		origin_cell = game->game[origin.rank][origin.file];
	int		allocated_by_me = 0;
	/* quick check for turn */
	if (((game->game_flags & FLAG_TURN_WHITE) && origin_cell.side != WHITE) ||
	  (game->game_flags & FLAG_TURN_BLACK && origin_cell.side != BLACK))
		return ERROR;

	/* no piece in the selected cell */
	if (origin_cell.piece == NULL)
		return ERROR;
	/* piece is pinned, so can't move */
	if (origin_cell.flags & FLAG_PIN)
		return ERROR;
	/* check if move would not actually move */
	if (origin.file == target.file && origin.rank == target.rank)
		return ERROR;
	/*
	 * current side has an ongoing check so cannot move (TODO: check if
	 * move would un-check)
	 */
	if ((game->game_flags & FLAG_TURN_BLACK && game->game_flags & FLAG_CHECK_BLACK) ||
	    (game->game_flags & FLAG_TURN_WHITE && game->game_flags & FLAG_CHECK_WHITE))
		return ERROR;

	/* assign passed moves array, else calculate it */
	position       *valid_moves = amoves;
	if (valid_moves == NULL) {
		valid_moves = moves(origin_cell.piece, origin, game->game, &game->game_flags);
		allocated_by_me = 1;
	}

	int		found_move = ERROR;
	found_move = is_in(target, valid_moves);
	if (allocated_by_me)
		free(valid_moves);	/* free move list if WE called the
					 * moves() function */
	return found_move;
}

position       *
move_piece(position origin, position target, board * game, position * amoves)
{
	cell		origin_cell = game->game[origin.rank][origin.file];
	cell		target_cell = game->game[target.rank][target.file];
	position       *moved_by_castle = NULL;
	position       *moved_pieces = NULL;

	int		valid = validate_move(origin, target, game, amoves);
	if (valid == ERROR)
		return NULL;
	/* check if promotion is available for pawn */
	if (origin_cell.piece->ident == 'p' &&
	    ((origin_cell.side == BLACK && target.rank == 0)
	   || (origin_cell.side == WHITE && target.rank == SIZE_STD - 1))) {
		char		promoted_sel = 'q';
		/* TODO:		ask for piece input */
		origin_cell.piece = ident_to_piece(promoted_sel);
	}
	/* check if the king is castling, if yes then move the rook too */
	if ((origin_cell.piece->ident == 'k')
	    && (origin_cell.flags & FLAG_FIRSTMOVE)) {
		if (target.file == origin.file + 2) {
			moved_by_castle = move_piece(coords_to_pos(origin.rank, origin.file + 3), coords_to_pos(origin.rank, origin.file + 1), game, NULL);
		} else {
			moved_by_castle = move_piece(coords_to_pos(origin.rank, origin.file - 4), coords_to_pos(origin.rank, origin.file - 1), game, NULL);
		}
		switch_turn(game);
	}

	target_cell.piece = origin_cell.piece;
	origin_cell.piece = NULL;	/* erase the piece from the origin */
	target_cell.side = origin_cell.side;	/* copy piece side */
	origin_cell.side = NONE;
	/* toggle flags */
	/* toggle first move to 0 */
	target_cell.flags = origin_cell.flags & ~(FLAG_FIRSTMOVE);
	origin_cell.flags = 0;


	/* TODO: make use of FLAG_FIRSTMOVE to check for pawn movements */
	game->game[target.rank][target.file] = target_cell;
	game->game[origin.rank][origin.file] = origin_cell;

	switch_turn(game);

	int		i = 0;
	int		malloc_size = 4;	/* origin & end, plus more
						 * space for castles or en
						 * passants */
	moved_pieces = malloc(sizeof(position) * malloc_size + sizeof(position));	/* one more for sentinel */
	moved_pieces[i++] = origin;
	moved_pieces[i++] = target;
	if (moved_by_castle != NULL) {
		moved_pieces[i++] = moved_by_castle[0];
		moved_pieces[i++] = moved_by_castle[1];
	}
	moved_pieces[i] = SENTINEL;
	free(moved_by_castle);
	return moved_pieces;
}

int
is_in(position m, position * moves)
{
	int		i = 0;
	while (moves[i].file != -1 && moves[i].rank != -1) {
		if (m.file == moves[i].file && m.rank == moves[i].rank)
			return 0;
		i++;
	}
	return -1;
}
