#include "piece.h"
#include "board.h"
#include <regex.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void
fen_parser(char *fen_string, board * game)
{
	const char     *fen_pattern = "[rRnNbBkKqQpP0-9]{1,8}/[rRnNbBkKqQpP0-9]{1,8}/[rRnNbBkKqQpP0-9]{1,8}/[rRnNbBkKqQpP0-9]{1,8}/[rRnNbBkKqQpP0-9]{1,8}/[rRnNbBkKqQpP0-9]{1,8}/[rRnNbBkKqQpP0-9]{1,8}/[rRnNbBkKqQpP0-9]{1,8}/";
	regex_t		fen_match;
	regmatch_t	pmatch[10];
	int		res_comp = regcomp(&fen_match, fen_pattern, REG_EXTENDED | REG_NOSUB);
	if (res_comp != 0) {
		printf("Fatal error composing regex\n");
		exit(1);
	}
	int		result = regexec(&fen_match, fen_string, 10, pmatch, 0);
	if (result != 0) {
		printf("Provided FEN is invalid\n");
		exit(1);
	}
	regfree(&fen_match);
	int		row = 0, col = 0, j = 0;
	for (int i = 0; fen_string[i] != 0; i++) {
		if (fen_string[i] == '/') {
			col = 0;
			row++;
			continue;
		}
		if (fen_string[i] >= 49 && fen_string[i] <= 57) {
			int		count = fen_string[i] - 48;	/* turn number to int */
			while (count - j > 0) {
				game->game[row][col++] = (cell) {
					NULL, NONE, 0
				};
				j++;
			}
			j = 0;
			/* lowercase letters (white) */
		} else if (fen_string[i] >= 0x61 && fen_string[i] <= 0x7a) {
			game->game[row][col++] = (cell) {
				ident_to_piece(fen_string[i]), WHITE, FLAG_FIRSTMOVE
			};
			/* uppercase letters (black) */
		} else if (fen_string[i] >= 0x41 && fen_string[i] <= 0x5a) {
			game->game[row][col++] = (cell) {
				ident_to_piece(fen_string[i]), BLACK, FLAG_FIRSTMOVE
			};
		} else {
			continue;
		}
	}
}
