#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
static void enable_ansi(void) {
	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD mode = 0;
	if (h && GetConsoleMode(h, &mode))
		SetConsoleMode(h, mode | 0x0004 /* ENABLE_VIRTUAL_TERMINAL_PROCESSING */);
	SetConsoleOutputCP(65001);
}
#else
static void enable_ansi(void) {}
#endif

#include "catan_map.h"
#include "catan_menu.h"
#include "catan_game.h"

/* Resource type per hex (indexes 1..19 used). map_generator shuffles this. */
static int blocks[NUM_BLOCKS] = {
	0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 6, 3, 3, 3, 4, 4, 4, 5, 5, 5
};

/* Number tokens per hex (indexes 1..19 used; index 10 is desert -> 0). */
static int indexs[NUM_BLOCKS] = {
	0, 2, 6, 5, 3, 4, 11, 5, 9, 8, 0, 8, 6, 10, 10, 3, 4, 9, 11, 12
};

/* Owner state — referenced by print_map. */
static int villages[NUM_VILLAGES] = {0};
static int streets[NUM_STREETS]   = {0};

int main(void) {
	enable_ansi();
	srand((unsigned)time(NULL));

	/* Start menu loop. */
	while (1) {
		int choice = start_menu();
		printf("\n");
		if (choice == 1) break;
		if (choice == 2) {
			rule_menu();
			getch();
			printf("\n");
			continue;
		}
		if (choice == 3) {
			printf("See you next time ~\n");
			return 0;
		}
	}

	printf("Game Start!\n\n");

	map_generator(blocks);

	Game g;
	game_init(&g, blocks, indexs, villages, streets);

	run_setup_phase(&g);

	int turn = 0;
	while (1) {
		int pi = turn % NUM_PLAYERS;
		run_player_turn(&g, pi);

		int w = game_winner(&g);
		if (w >= 0) {
			print_map(blocks, villages, streets, indexs);
			print_status(&g);
			printf("\n*** %s wins with %d points! ***\n",
				g.players[w].name, g.players[w].points);
			break;
		}
		turn++;
	}
	return 0;
}
