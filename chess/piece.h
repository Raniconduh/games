#ifndef PIECE_H
#define PIECE_H
#include <stdint.h>

#define SIZE_STD 8
#define SENTINEL coords_to_pos(-1, -1);
#define FLAG_CHECK_WHITE 0x1
#define FLAG_CHECK_BLACK 0x2
#define FLAG_FIRSTMOVE 0x4	/* for castles and pawn starts */
#define FLAG_PIN       0x8	/* block piece when it would discover a check */

typedef enum {
	BLACK, WHITE, NONE
} SIDE;

typedef struct {
	short		rank;	/* row */
	short		file;	/* column */
} position;

typedef struct {
	char		ident;	/* FEN style piece identification */
	char	       *pretty;
} piece;

typedef struct {
	piece	       *piece;
	SIDE		side;
	uint8_t		flags;
} cell;

position *	moves(piece * piece, position pos, cell game[SIZE_STD][SIZE_STD], uint8_t * game_flags);
extern piece rook, bishop, knight, queen, pawn, king;
position	coords_to_pos(short rank, short file);
piece	       *ident_to_piece(char ident);
#endif
