#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "catan_menu.h"

#ifdef _WIN32
#include <conio.h>
char getch(void) {
	return (char)_getch();
}
#else
#include <termios.h>
#include <unistd.h>
static struct termios old_io, current_io;

static void initTermios(int echo) {
	tcgetattr(0, &old_io);
	current_io = old_io;
	current_io.c_lflag &= ~ICANON;
	if (echo) current_io.c_lflag |= ECHO;
	else      current_io.c_lflag &= ~ECHO;
	tcsetattr(0, TCSANOW, &current_io);
}

static void resetTermios(void) {
	tcsetattr(0, TCSANOW, &old_io);
}

char getch(void) {
	char ch;
	initTermios(0);
	ch = getchar();
	resetTermios();
	return ch;
}
#endif

int read_int(const char *prompt, int min, int max) {
	int v = 0;
	char buf[64];
	while (1) {
		printf("%s", prompt);
		fflush(stdout);
		if (!fgets(buf, sizeof(buf), stdin)) {
			fprintf(stderr, "\n[input closed — exiting]\n");
			exit(0);
		}
		if (sscanf(buf, "%d", &v) == 1 && v >= min && v <= max) return v;
		printf("  -> please enter a number between %d and %d\n", min, max);
	}
}

int start_menu(){
	printf("\nWelcome to the best game ever - \033[31mCatan\033[m\n");
	printf("Enter the number on the left of each option to choose what to do\n");
	printf("1) Start The Game\n");
	printf("2) Game Rule\n");
	printf("3) Exit\n");
	return read_int("Your choice: ", 1, 3);
}

void rule_menu(){
	printf("Game Rule:\n");
	printf("  \033[1m-How to win?\033[m\n");
	printf("    Be the first person to reach 10 points\n\n");
	printf("  \033[1m-How to play?\033[m (minimal version)\n");
	printf("  -Setup phase\n");
	printf("    Each player places 2 villages and 2 roads (snake order).\n");
	printf("    Each village must connect to its own road.\n");
	printf("    Two villages cannot share an edge (must be at least one road apart).\n");
	printf("    After placing the second village, you immediately receive resources\n");
	printf("    from the three adjacent hex tiles.\n\n");
	printf("  -Each turn\n");
	printf("    1) Roll two dice. Every village/city next to a tile whose number\n");
	printf("       matches the roll produces 1 (village) or 2 (city) resources.\n");
	printf("    2) On a 7, every player with more than 7 cards discards half (rounded down),\n");
	printf("       and the roller moves the thief to a new tile.\n");
	printf("       Tiles under the thief produce no resource.\n");
	printf("    3) Build (and/or trade with bank 4-for-1), then end your turn.\n\n");
	printf("  -Build cost\n");
	printf("    Road    : 1 lumber + 1 brick\n");
	printf("    Village : 1 lumber + 1 brick + 1 wheat + 1 wool   (1 point)\n");
	printf("    City    : 2 wheat  + 3 ore  (upgrade a village)   (2 points)\n");

	printf("\n  Press any key to return\n");
}

int main_menu(){
	printf("What do you want to do now?\n");
	printf("1) Build\n");
	printf("2) Trade with bank (4:1)\n");
	printf("3) End turn\n");
	return read_int("Your choice: ", 1, 3);
}

int build_menu(){
	printf("What do you want to build?\n");
	printf("1) Road    (1 lumber + 1 brick)\n");
	printf("2) Village (1 lumber + 1 brick + 1 wheat + 1 wool)\n");
	printf("3) City    (2 wheat + 3 ore, upgrade a village)\n");
	printf("4) Cancel\n");
	return read_int("Your choice: ", 1, 4);
}

int trade_menu(){
	printf("Trade 4 of one resource for 1 of another (with the bank).\n");
	printf("Resource codes: 1) Wool  2) Brick  3) Ore  4) Wheat  5) Lumber  6) Cancel\n");
	return read_int("Resource to GIVE (4 cards) [1-6]: ", 1, 6);
}

int pause_menu(){
	printf("GAME PAUSED\n");
	printf("1) Resume\n");
	printf("2) Exit\n");
	return read_int("Your choice: ", 1, 2);
}
