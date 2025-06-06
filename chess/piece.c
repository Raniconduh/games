#include "piece.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

position
coords_to_pos(short rank, short file){
	position	p = {rank, file};
	return p;
}

SIDE
opposite(SIDE orig) {
	switch (orig) {
	case WHITE:
		return BLACK;
	case BLACK:
		return WHITE;
	default:
		return NONE;
	}
}

uint8_t
check_if_king(position * valid_moves, int move_idx, cell game[SIZE_STD][SIZE_STD],
	      uint8_t game_flags){
	if (move_idx == 0 || valid_moves == NULL)
		return game_flags;
	uint8_t		flags = game_flags;
	if (game[valid_moves[move_idx - 1].rank][valid_moves[move_idx - 1].file].piece != NULL &&
	    game[valid_moves[move_idx - 1].rank][valid_moves[move_idx - 1].file].piece->ident == 'k') {
		switch (game[valid_moves[move_idx - 1].rank][valid_moves[move_idx - 1].file].side) {
		case BLACK:
			flags |= FLAG_CHECK_BLACK;
			break;
		case WHITE:
			flags |= FLAG_CHECK_WHITE;
			break;
		default:
			break;
		}
	}
	return flags;
}

position       *
rook_valid(position pos, cell game[SIZE_STD][SIZE_STD], uint8_t * game_flags)
{
	const int	max_length = (SIZE_STD - 1) * 2;	/* a rook can move ( not
								 * counting other pieces
								 * ) a total of size of
								 * one side - 1 (to
								 * exclude current
								 * position) */
	position       *valid_moves = malloc(sizeof(position) * max_length + sizeof(position));	/* allocate vertical +
												 * horizontal moves,
												 * intentionally left
												 * the +
												 * sizeof(position) to
												 * clarify that's
												 * reserved for the
												 * sentinel */
	int		move_idx = 0;

	/* Line up */
	for (int row = pos.rank + 1; row < SIZE_STD; row++) {
		if (game[row][pos.file].piece != NULL) {
			if (game[row][pos.file].side == opposite(game[pos.rank][pos.file].side)) {
				valid_moves[move_idx++] = coords_to_pos(row, pos.file);
				break;
			}
			break;
		}
		valid_moves[move_idx++] = coords_to_pos(row, pos.file);
	}
	*game_flags = check_if_king(valid_moves, move_idx, game, *game_flags);
	/* Line down */
	for (int row = pos.rank - 1; row >= 0; row--) {
		if (game[row][pos.file].piece != NULL) {
			if (game[row][pos.file].side == opposite(game[pos.rank][pos.file].side)) {
				valid_moves[move_idx++] = coords_to_pos(row, pos.file);
				break;
			}
			break;
		}
		valid_moves[move_idx++] = coords_to_pos(row, pos.file);
	}
	*game_flags = check_if_king(valid_moves, move_idx, game, *game_flags);
	/* Line left */
	for (int col = pos.file - 1; col >= 0; col--) {
		if (game[pos.rank][col].piece != NULL) {
			if (game[pos.rank][col].side == opposite(game[pos.rank][pos.file].side)) {
				valid_moves[move_idx++] = coords_to_pos(pos.rank, col);
				break;
			}
			break;
		}
		valid_moves[move_idx++] = coords_to_pos(pos.rank, col);
	}
	*game_flags = check_if_king(valid_moves, move_idx, game, *game_flags);
	/* Line right */
	for (int col = pos.file + 1; col < SIZE_STD; col++) {
		if (game[pos.rank][col].piece != NULL) {
			if (game[pos.rank][col].side == opposite(game[pos.rank][pos.file].side)) {
				valid_moves[move_idx++] = coords_to_pos(pos.rank, col);
				break;
			}
			break;
		}
		valid_moves[move_idx++] = coords_to_pos(pos.rank, col);
	}
	*game_flags = check_if_king(valid_moves, move_idx, game, *game_flags);
	valid_moves[move_idx] = SENTINEL;
	return valid_moves;
}

position       *
knight_valid(position pos, cell game[SIZE_STD][SIZE_STD], uint8_t * game_flags)
{
	const int	max_length = 8;
	position       *valid_moves = malloc(sizeof(position) * max_length + sizeof(position));
	int		i, j, move_idx = 0;
	for (i = -2; i <= 2; i += 4) {
		for (j = -1; j <= 1; j += 2) {
			position	coords_v = coords_to_pos(pos.rank + i, pos.file + j);
			position	coords_h = coords_to_pos(pos.rank + j, pos.file + i);
			if (!(coords_v.file < 0 || coords_v.rank < 0 ||
			      coords_v.file >= SIZE_STD || coords_v.rank >= SIZE_STD || game[pos.rank + i][pos.file + j].side == game[pos.rank][pos.file].side)) {
				valid_moves[move_idx++] = coords_v;
			}
			if (!(coords_h.file < 0 || coords_h.rank < 0 ||
			      coords_h.file >= SIZE_STD || coords_h.rank >= SIZE_STD || game[pos.rank + j][pos.file + i].side == game[pos.rank][pos.file].side)) {
				valid_moves[move_idx++] = coords_h;
			}
			*game_flags = check_if_king(valid_moves, move_idx, game, *game_flags);
		}
	}
	valid_moves[move_idx] = SENTINEL;
	return valid_moves;
}

position       *
bishop_valid(position pos, cell game[SIZE_STD][SIZE_STD], uint8_t * game_flags)
{
	/*
	 * we can approximate, in the worst case the bishop will take
	 * (board's diagonal length * 2) - 3
	 */
	const int	max_length = SIZE_STD * 2;
	position       *valid_moves = malloc(sizeof(position) * max_length + sizeof(position));	/* allocate vertical +
												 * horizontal moves +
												 * sentinel */
	int		i = 1, move_idx = 0;

	/* first diagonal: top right */
	while (pos.rank + i < SIZE_STD && pos.file + i < SIZE_STD) {
		if (game[pos.rank + i][pos.file + i].piece != NULL) {
			if (game[pos.rank + i][pos.file + i].side == opposite(game[pos.rank][pos.file].side)) {
				valid_moves[move_idx++] = coords_to_pos(pos.rank + i, pos.file + i);
				break;
			}
			break;
		}
		valid_moves[move_idx++] = coords_to_pos(pos.rank + i, pos.file + i);
		i++;
	}
	*game_flags = check_if_king(valid_moves, move_idx, game, *game_flags);
	/* second diagonal: top left */
	i = 1;
	while (pos.rank + i < SIZE_STD && pos.file - i >= 0) {
		if (game[pos.rank + i][pos.file - i].piece != NULL) {
			if (game[pos.rank + i][pos.file - i].side == opposite(game[pos.rank][pos.file].side)) {
				valid_moves[move_idx++] = coords_to_pos(pos.rank + i, pos.file - i);
				break;
			}
			break;
		}
		valid_moves[move_idx++] = coords_to_pos(pos.rank + i, pos.file - i);
		i++;
	}
	*game_flags = check_if_king(valid_moves, move_idx, game, *game_flags);
	/* third diagonal: bottom left */
	i = 1;
	while (pos.rank - i >= 0 && pos.file - i >= 0) {
		if (game[pos.rank - i][pos.file - i].piece != NULL) {
			if (game[pos.rank - i][pos.file - i].side == opposite(game[pos.rank][pos.file].side)) {
				valid_moves[move_idx++] = coords_to_pos(pos.rank - i, pos.file - i);
				break;
			}
			break;
		}
		valid_moves[move_idx++] = coords_to_pos(pos.rank - i, pos.file - i);
		i++;
	}
	*game_flags = check_if_king(valid_moves, move_idx, game, *game_flags);
	/* fourth diagonal: bottom right */
	i = 1;
	while (pos.rank - i >= 0 && pos.file + i < SIZE_STD) {
		if (game[pos.rank - i][pos.file + i].piece != NULL) {
			if (game[pos.rank - i][pos.file + i].side == opposite(game[pos.rank][pos.file].side)) {
				valid_moves[move_idx++] = coords_to_pos(pos.rank - i, pos.file + i);
				break;
			}
			break;
		}
		valid_moves[move_idx++] = coords_to_pos(pos.rank - i, pos.file + i);
		i++;
	}
	*game_flags = check_if_king(valid_moves, move_idx, game, *game_flags);
	valid_moves[move_idx] = SENTINEL;
	return valid_moves;
}

position       *
queen_valid(position pos, cell game[SIZE_STD][SIZE_STD], uint8_t * game_flags)
{
	position       *bishop_moves = bishop_valid(pos, game, game_flags);
	position       *rook_moves = rook_valid(pos, game, game_flags);
	int		bishop_moves_len, rook_moves_len;
	for (bishop_moves_len = 0; bishop_moves[bishop_moves_len].file != -1; bishop_moves_len++)
		;
	for (rook_moves_len = 0; rook_moves[rook_moves_len].file != -1; rook_moves_len++)
		;
	int		total_len = bishop_moves_len + rook_moves_len;
	position       *valid_moves = malloc(sizeof(position) * total_len + sizeof(position));
	memcpy(valid_moves, bishop_moves, bishop_moves_len * sizeof(position));
	memcpy(valid_moves + bishop_moves_len, rook_moves, rook_moves_len * sizeof(position));
	free(bishop_moves);
	free(rook_moves);
	valid_moves[bishop_moves_len + rook_moves_len] = SENTINEL;
	return valid_moves;
}

position       *
king_valid(position pos, cell game[SIZE_STD][SIZE_STD], uint8_t * game_flags)
{
	int		move_idx = 0;
	const int	max_length = 8;
	position       *valid_moves = malloc(sizeof(position) * max_length + sizeof(position));
	for (int i = -1; i <= 1; i++) {
		for (int j = -1; j <= 1; j++) {
			if (i == 0 && j == 0)
				continue;
			if (pos.rank + i >= 0 && pos.file + j >= 0 &&
			pos.rank + i < SIZE_STD && pos.file + j < SIZE_STD &&
			    (game[pos.rank + i][pos.file + j].side != game[pos.rank][pos.file].side)) {
				valid_moves[move_idx++] = coords_to_pos(pos.rank + i, pos.file + j);
			}
		}
	}
	/* check if castling is possible */
	if ((game[pos.rank][pos.file].flags & FLAG_FIRSTMOVE)
	    && (game[pos.rank][pos.file - 4].piece->ident = 'r')
	    && (game[pos.rank][pos.file - 4].flags & FLAG_FIRSTMOVE)
	    && (game[pos.rank][pos.file - 3].piece == NULL)
	    && (game[pos.rank][pos.file - 2].piece == NULL)
	    && (game[pos.rank][pos.file - 1].piece == NULL)) {
		valid_moves[move_idx++] = coords_to_pos(pos.rank, pos.file - 2);
	}
	if ((game[pos.rank][pos.file].flags & FLAG_FIRSTMOVE)
	    && (game[pos.rank][pos.file + 3].piece->ident = 'r')
	    && (game[pos.rank][pos.file + 3].flags & FLAG_FIRSTMOVE)
	    && (game[pos.rank][pos.file + 2].piece == NULL)
	    && (game[pos.rank][pos.file + 1].piece == NULL)) {
		valid_moves[move_idx++] = coords_to_pos(pos.rank, pos.file + 2);
	}
	valid_moves[move_idx] = SENTINEL;
	return valid_moves;
}

position       *
pawn_valid(position pos, cell game[SIZE_STD][SIZE_STD], uint8_t * game_flags)
{
	int		max_length = 4;
	position       *valid_moves =
	malloc(sizeof(position) * max_length + sizeof(position));
	int		move_idx = 0;
	int		side_sign = 1;
	if (game[pos.rank][pos.file].side == BLACK)
		side_sign = -1;
	if (game[pos.rank + side_sign][pos.file].piece == NULL
	    && pos.rank + side_sign >= 0
	    && pos.rank + side_sign < SIZE_STD) {
		valid_moves[move_idx++] = coords_to_pos(pos.rank + side_sign, pos.file);
	}
	if (game[pos.rank][pos.file].flags & FLAG_FIRSTMOVE
	    && game[pos.rank + 2 * side_sign][pos.file].piece == NULL
	    && game[pos.rank + 1 * side_sign][pos.file].piece == NULL) {
		valid_moves[move_idx++] = coords_to_pos(pos.rank + 2 * side_sign, pos.file);
	}
	if (game[pos.rank + side_sign][pos.file + side_sign].piece != NULL
	    && game[pos.rank + side_sign][pos.file + side_sign].side == opposite(game[pos.rank][pos.file].side)) {
		valid_moves[move_idx++] = coords_to_pos(pos.rank + side_sign, pos.file + side_sign);
	}
	*game_flags = check_if_king(valid_moves, move_idx, game, *game_flags);

	if (game[pos.rank + side_sign][pos.file - side_sign].piece != NULL
	    && game[pos.rank + side_sign][pos.file - side_sign].side == opposite(game[pos.rank][pos.file].side)) {
		valid_moves[move_idx++] = coords_to_pos(pos.rank + side_sign, pos.file - side_sign);
	}
	*game_flags = check_if_king(valid_moves, move_idx, game, *game_flags);

	valid_moves[move_idx] = SENTINEL;
	return valid_moves;
}

position *
moves(piece * piece, position pos, cell game[SIZE_STD][SIZE_STD], uint8_t * game_flags)
{
	switch (piece->ident) {
	case 'r':
		return rook_valid(pos, game, game_flags);
		break;
	case 'b':
		return bishop_valid(pos, game, game_flags);
		break;
	case 'n':
		return knight_valid(pos, game, game_flags);
		break;
	case 'q':
		return queen_valid(pos, game, game_flags);
		break;
	case 'k':
		return king_valid(pos, game, game_flags);
		break;
	case 'p':
		return pawn_valid(pos, game, game_flags);
		break;
	default:
		return NULL;
	}
}

piece	       *
ident_to_piece(char ident)
{
	switch (tolower(ident)) {
	case 'r':
		return &rook;
	case 'n':
		return &knight;
	case 'b':
		return &bishop;
	case 'q':
		return &queen;
	case 'k':
		return &king;
	case 'p':
		return &pawn;
	default:
		return NULL;
	}
}

piece		rook = {'r', "♖"};
piece		knight = {'n', "♘"};
piece		bishop = {'b', "♗"};
piece		queen = {'q', "♕"};
piece		king = {'k', "♔"};
piece		pawn = {'p', "♙"};

/*
 * piece		rook = {'r', "R"}; piece		knight = {'n', "N"};
 * piece		bishop = {'b', "B"}; piece		queen = {'q', "Q"};
 * piece		king = {'k', "K"}; piece		pawn = {'p', "P"};
 */
