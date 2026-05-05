#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "catan_map.h"

/* ============================================================
 * Vertex / block label overrides
 *
 * print_map originally drew each vertex as a single coloured `*` — fine
 * visually but useless when the player has to type a vertex id at a
 * prompt. This overlay maps every vertex's anchor cell (and the cell
 * just to its right) to the 2-character label that should appear there:
 *
 *   empty vertex   -> "NN" (zero-padded vertex id, dark-on-light)
 *   own village    -> "PX" (X = player color 1..4, in player colour)
 *   own city       -> "CX"
 *
 * We also stamp each block's id ("01".."19") one row above the dice
 * number so the human knows which block to send the thief to.
 * ============================================================ */
static const int V_ROW[55] = { 0,
	 3,  3,  6,  6,  6,  6,  9,  9,  9,  9,  9,  9,
	12, 12, 12, 12, 12, 12, 15, 15, 15, 15, 15, 15,
	18, 18, 18, 18, 18, 18, 21, 21, 21, 21, 21, 21,
	24, 24, 24, 24, 24, 24, 27, 27, 27, 27, 27, 27,
	30, 30, 30, 30, 33, 33
};
static const int V_COL[55] = { 0,
	28, 33, 20, 25, 36, 41, 12, 17, 28, 33, 44, 49,
	 9, 20, 25, 36, 41, 52, 12, 17, 28, 33, 44, 49,
	 9, 20, 25, 36, 41, 52, 12, 17, 28, 33, 44, 49,
	 9, 20, 25, 36, 41, 52, 12, 17, 28, 33, 44, 49,
	20, 25, 36, 41, 28, 33
};

/* Block centre rows and columns (where the dice token is printed). */
static const int B_ROW[20] = { 0, 6, 9, 9,12,12,12,15,15,18,18,18,21,21,24,24,24,27,27,30 };
static const int B_COL[20] = { 0,30,22,38,14,30,46,22,38,14,30,46,22,38,14,30,46,22,38,30 };

#define MAP_R 38
#define MAP_C 70
static int  s_vertex_at  [MAP_R][MAP_C];   /* 0 or vertex id 1..54 */
static char s_vertex_idx [MAP_R][MAP_C];   /* 0 = first char, 1 = second */
static int  s_block_at   [MAP_R][MAP_C];   /* 0 or block id 1..19 */
static char s_block_idx  [MAP_R][MAP_C];
static int  s_init_done = 0;

static void init_overrides(void) {
	if (s_init_done) return;
	s_init_done = 1;
	memset(s_vertex_at, 0, sizeof(s_vertex_at));
	memset(s_block_at,  0, sizeof(s_block_at));
	for (int v = 1; v <= 54; v++) {
		int r = V_ROW[v], c = V_COL[v];
		s_vertex_at[r][c]      = v; s_vertex_idx[r][c]      = 0;
		s_vertex_at[r][c + 1]  = v; s_vertex_idx[r][c + 1]  = 1;
	}
	for (int b = 1; b <= 19; b++) {
		int r = B_ROW[b] - 1, c = B_COL[b];   /* row above dice token */
		s_block_at[r][c]       = b; s_block_idx[r][c]       = 0;
		s_block_at[r][c + 1]   = b; s_block_idx[r][c + 1]   = 1;
	}
}

/* Vertex code in villages[]:  0=empty, 1..4=village color, 5..8=city color.
 * All vertex labels render on a black background to match the original
 * `*` style (the slot is in the dark margin between hexes). */
static void print_vertex_label(int owner, int vertex_id, int char_idx) {
	char buf[3];
	if (owner == 0) {
		snprintf(buf, sizeof(buf), "%02d", vertex_id);
		printf("\033[40;37m%c\033[m", buf[char_idx]);     /* dim grey id */
		return;
	}
	int color = (owner <= 4) ? owner : (owner - 4);
	int fg;
	switch (color) {
		case 1: fg = 31; break;   /* red */
		case 2: fg = 36; break;   /* cyan */
		case 3: fg = 33; break;   /* yellow */
		case 4: fg = 32; break;   /* green */
		default: fg = 37; break;
	}
	char first  = (owner <= 4) ? 'P' : 'C';
	char second = (char)('0' + color);
	char ch = (char_idx == 0) ? first : second;
	printf("\033[40;%d;1m%c\033[m", fg, ch);
}

/* Block id is painted ON the hex tile, so we keep the tile's bg colour
 * and just put a bold digit on top. dice number stays white-on-white-bg
 * (very legible) one row below — different style on purpose. */
static int hex_bg_for_type(int t) {
	switch (t) {
		case 1: return 45;   /* wool   - magenta */
		case 2: return 42;   /* lumber - green */
		case 3: return 43;   /* wheat  - yellow */
		case 4: return 40;   /* ore    - black */
		case 5: return 41;   /* brick  - red */
		case 6: return 47;   /* desert - white */
		default: return 40;
	}
}

static void print_block_label(int block_type, int block_id, int char_idx) {
	char buf[3];
	snprintf(buf, sizeof(buf), "%02d", block_id);
	int bg = hex_bg_for_type(block_type);
	int fg = (bg == 47) ? 30 : 37;            /* black on desert, else white */
	printf("\033[%d;%d;1m%c\033[m", bg, fg, buf[char_idx]);
}

void print_map( int blocks[], int villages[], int streets[], int indexs[] ){
	init_overrides();
	//print gameboard
	for(int i = 1; i <= 37; i++){
		//player section 1
		for(int j = 1; j <= 20; j++){
			//p1 (white text — was muted-red which read as pink against the
			//     terminal background)
			if(i == 1 && j == 10)
				printf("\033[40;37;1mP\033[m");
			else if(i == 1 && j == 11)
				printf("\033[40;37;1m1\033[m");
			else if(i >= 2 && i <= 11 && j >= 2 && j <= 19)
				printf("\033[47m \033[m");
				
			//com2
			else if(i == 37 && j == 9)
				printf("\033[40;33;1mC\033[m");
			else if(i == 37 && j == 10)
				printf("\033[40;33;1mO\033[m");
			else if(i == 37 && j == 11)
				printf("\033[40;33;1mM\033[m");
			else if(i == 37 && j == 12)
				printf("\033[40;33;1m2\033[m");
			else if(i >= 27 && i <= 36 && j >= 2 && j <= 19)
				printf("\033[47m \033[m");
			
			//hint
			else if(i >= 14 && i <= 24 && j >= 3 && j <= 18){
			
				//title
				if(i == 15 && j >= 3 && j <= 18){
					if(j == 9)
						printf("\033[40;37;1mH\033[m");
					else if(j == 10)
						printf("\033[40;37;1mI\033[m");
					else if(j == 11)
						printf("\033[40;37;1mN\033[m");
					else if(j == 12)
						printf("\033[40;37;1mT\033[m");
					else if(j == 8 || j == 13)
						printf("\033[40;37;1m \033[m");
					else
						printf("\033[47;1m \033[m");
				
				//wool
				}else if(i == 17 && j >= 3 && j <= 18){
					if(j >= 6 && j <= 7)
						printf("\033[45m \033[m");
					else if(j == 10)
						printf("\033[47;35;1mW\033[m");
					else if(j == 11)
						printf("\033[47;35;1mO\033[m");
					else if(j == 12)
						printf("\033[47;35;1mO\033[m");
					else if(j == 13)
						printf("\033[47;35;1mL\033[m");
					else
						printf("\033[47m \033[m");
				
				//brick
				}else if(i == 18 && j >= 3 && j <= 18){
					if(j >= 6 && j <= 7)
						printf("\033[41m \033[m");
					else if(j == 10)
						printf("\033[47;31;1mB\033[m");
					else if(j == 11)
						printf("\033[47;31;1mR\033[m");
					else if(j == 12)
						printf("\033[47;31;1mI\033[m");
					else if(j == 13)
						printf("\033[47;31;1mC\033[m");
					else if(j == 14)
						printf("\033[47;31;1mK\033[m");
					else
						printf("\033[47m \033[m");
						
				//iron
				}else if(i == 19 && j >= 3 && j <= 18){
					if(j >= 6 && j <= 7)
						printf("\033[40m \033[m");
					else if(j == 10)
						printf("\033[47;30;1mI\033[m");
					else if(j == 11)
						printf("\033[47;30;1mR\033[m");
					else if(j == 12)
						printf("\033[47;30;1mO\033[m");
					else if(j == 13)
						printf("\033[47;30;1mN\033[m");
					else
						printf("\033[47m \033[m");
						
				//wheat
				}else if(i == 20 && j >= 3 && j <= 18){
					if(j >= 6 && j <= 7)
						printf("\033[43m \033[m");
					else if(j == 10)
						printf("\033[47;33;1mW\033[m");
					else if(j == 11)
						printf("\033[47;33;1mH\033[m");
					else if(j == 12)
						printf("\033[47;33;1mE\033[m");
					else if(j == 13)
						printf("\033[47;33;1mA\033[m");
					else if(j == 14)
						printf("\033[47;33;1mT\033[m");
					else
						printf("\033[47m \033[m");
						
				//plank
				}else if(i == 21 && j >= 3 && j <= 18){
					if(j >= 6 && j <= 7)
						printf("\033[42m \033[m");
					else if(j == 10)
						printf("\033[47;32;1mP\033[m");
					else if(j == 11)
						printf("\033[47;32;1mL\033[m");
					else if(j == 12)
						printf("\033[47;32;1mA\033[m");
					else if(j == 13)
						printf("\033[47;32;1mN\033[m");
					else if(j == 14)
						printf("\033[47;32;1mK\033[m");
					else
						printf("\033[47m \033[m");
						
				//any
				}else if(i == 22 && j >= 3 && j <= 18){
					if(j >= 6 && j <= 7)
						printf("\033[44m \033[m");
					else if(j == 10)
						printf("\033[47;34;1mA\033[m");
					else if(j == 11)
						printf("\033[47;34;1mN\033[m");
					else if(j == 12)
						printf("\033[47;34;1mY\033[m");
					else if(j == 13)
						printf("\033[47;34;1m/\033[m");
					else
						printf("\033[47m \033[m");
				}else if(i == 23 && j >= 3 && j <= 18){
					if(j == 10)
						printf("\033[47;34;1mD\033[m");
					else if(j == 11)
						printf("\033[47;34;1mE\033[m");
					else if(j == 12)
						printf("\033[47;34;1mS\033[m");
					else if(j == 13)
						printf("\033[47;34;1mE\033[m");
					else if(j == 14)
						printf("\033[47;34;1mR\033[m");
					else if(j == 15)
						printf("\033[47;34;1mT\033[m");
					else
						printf("\033[47m \033[m");
						
				//white background
				}else
					printf("\033[47m \033[m");
			
			//black background
			}else
				printf("\033[40m \033[m");
		}
		
		//map
		for(int j = 1; j <= 60; j++){

			//labelled-vertex / block-id overlay (intercepts ahead of the
			//original village `*` dispatch and the per-block dice number)
			if (s_vertex_at[i][j]) {
				int v = s_vertex_at[i][j];
				print_vertex_label(villages[v], v, s_vertex_idx[i][j]);
				continue;
			}
			if (s_block_at[i][j]) {
				int b = s_block_at[i][j];
				print_block_label(blocks[b], b, s_block_idx[i][j]);
				continue;
			}

			//villages

			if(i == 3 && j == 28) //village 1
				print_village(villages[1]);
			else if(i == 3 && j == 33) //village 2
				print_village(villages[2]);
			else if(i == 6 && j == 20) //village 3
				print_village(villages[3]);
			else if(i == 6 && j == 25) //village 4
				print_village(villages[4]);
			else if(i == 6 && j == 36) //village 5
				print_village(villages[5]);
			else if(i == 6 && j == 41) //village 6
				print_village(villages[6]);
			else if(i == 9 && j == 12) //village 7
				print_village(villages[7]);
			else if(i == 9 && j == 17) //village 8
				print_village(villages[8]);
			else if(i == 9 && j == 28) //village 9
				print_village(villages[9]);
			else if(i == 9 && j == 33) //village 10
				print_village(villages[10]);
			else if(i == 9 && j == 44) //village 11
				print_village(villages[11]);
			else if(i == 9 && j == 49) //village 12
				print_village(villages[12]);
			else if(i == 12 && j == 9) //village 13
				print_village(villages[13]);
			else if(i == 12 && j == 20) //village 14
				print_village(villages[14]);
			else if(i == 12 && j == 25) //village 15
				print_village(villages[15]);
			else if(i == 12 && j == 36) //village 16
				print_village(villages[16]);
			else if(i == 12 && j == 41) //village 17
				print_village(villages[17]);
			else if(i == 12 && j == 52) //village 18
				print_village(villages[18]);
			else if(i == 15 && j == 12) //village 19
				print_village(villages[19]);
			else if(i == 15 && j == 17) //village 20
				print_village(villages[20]);
			else if(i == 15 && j == 28) //village 21
				print_village(villages[21]);
			else if(i == 15 && j == 33) //village 22
				print_village(villages[22]);
			else if(i == 15 && j == 44) //village 23
				print_village(villages[23]);
			else if(i == 15 && j == 49) //village 24
				print_village(villages[24]);
			else if(i == 18 && j == 9) //village 25
				print_village(villages[25]);
			else if(i == 18 && j == 20) //village 26
				print_village(villages[26]);
			else if(i == 18 && j == 25) //village 27
				print_village(villages[27]);
			else if(i == 18 && j == 36) //village 28
				print_village(villages[28]);
			else if(i == 18 && j == 41) //village 29
				print_village(villages[29]);
			else if(i == 18 && j == 52) //village 30
				print_village(villages[30]);
			else if(i == 21 && j == 12) //village 31
				print_village(villages[31]);
			else if(i == 21 && j == 17) //village 32
				print_village(villages[32]);
			else if(i == 21 && j == 28) //village 33
				print_village(villages[33]);
			else if(i == 21 && j == 33) //village 34
				print_village(villages[34]);
			else if(i == 21 && j == 44) //village 35
				print_village(villages[35]);
			else if(i == 21 && j == 49) //village 36
				print_village(villages[36]);
			else if(i == 24 && j == 9) //village 37
				print_village(villages[37]);
			else if(i == 24 && j == 20) //village 38
				print_village(villages[38]);
			else if(i == 24 && j == 25) //village 39
				print_village(villages[39]);
			else if(i == 24 && j == 36) //village 40
				print_village(villages[40]);
			else if(i == 24 && j == 41) //village 41
				print_village(villages[41]);
			else if(i == 24 && j == 52) //village 42
				print_village(villages[42]);
			else if(i == 27 && j == 12) //village 43
				print_village(villages[43]);
			else if(i == 27 && j == 17) //village 44
				print_village(villages[44]);
			else if(i == 27 && j == 28) //village 45
				print_village(villages[45]);
			else if(i == 27 && j == 33) //village 46
				print_village(villages[46]);
			else if(i == 27 && j == 44) //village 47
				print_village(villages[47]);
			else if(i == 27 && j == 49) //village 48
				print_village(villages[48]);
			else if(i == 30 && j == 20) //village 49
				print_village(villages[49]);
			else if(i == 30 && j == 25) //village 50
				print_village(villages[50]);
			else if(i == 30 && j == 36) //village 51
				print_village(villages[51]);
			else if(i == 30 && j == 41) //village 52
				print_village(villages[52]);
			else if(i == 33 && j == 28) //village 53
				print_village(villages[53]);
			else if(i == 33 && j == 33) //village 54
				print_village(villages[54]);
			
			//streets
			
			else if(i == 3 && j >= 29 && j <= 32) //street 1
				print_street1(streets[1]);
			else if((i == 4 && j == 27) || (i == 5 && j == 26)) //street 2
				print_street2(streets[2]);
			else if((i == 4 && j == 34) || (i == 5 && j == 35)) //street 3
				print_street3(streets[3]);
			else if(i == 6 && j >= 21 && j <= 24) //street 4
				print_street1(streets[4]);
			else if(i == 6 && j >= 37 && j <= 40) //street 5
				print_street1(streets[5]);
			else if((i == 7 && j == 19) || (i == 8 && j == 18)) //street 6
				print_street2(streets[6]);
			else if((i == 7 && j == 26) || (i == 8 && j == 27)) //street 7
				print_street3(streets[7]);
			else if((i == 7 && j == 35) || (i == 8 && j == 34)) //street 8
				print_street2(streets[8]);
			else if((i == 7 && j == 42) || (i == 8 && j == 43)) //street 9
				print_street3(streets[9]);
			else if(i == 9 && j >= 13 && j <= 16) //street 10
				print_street1(streets[10]);
			else if(i == 9 && j >= 29 && j <= 32) //street 11
				print_street1(streets[11]);
			else if(i == 9 && j >= 45 && j <= 48) //street 12
				print_street1(streets[12]);
			else if((i == 10 && j == 11) || (i == 11 && j == 10)) //street 13
				print_street2(streets[13]);
			else if((i == 10 && j == 18) || (i == 11 && j == 19)) //street 14
				print_street3(streets[14]);
			else if((i == 10 && j == 27) || (i == 11 && j == 26)) //street 15
				print_street2(streets[15]);
			else if((i == 10 && j == 34) || (i == 11 && j == 35)) //street 16
				print_street3(streets[16]);
			else if((i == 10 && j == 43) || (i == 11 && j == 42)) //street 17
				print_street2(streets[17]);
			else if((i == 10 && j == 50) || (i == 11 && j == 51)) //street 18
				print_street3(streets[18]);
			else if(i == 12 && j >= 21 && j <= 24) //street 19
				print_street1(streets[19]);
			else if(i == 12 && j >= 37 && j <= 40) //street 20
				print_street1(streets[20]);
			else if((i == 13 && j == 10) || (i == 14 && j == 11)) //street 21
				print_street3(streets[21]);
			else if((i == 13 && j == 19) || (i == 14 && j == 18)) //street 22
				print_street2(streets[22]);
			else if((i == 13 && j == 26) || (i == 14 && j == 27)) //street 23
				print_street3(streets[23]);
			else if((i == 13 && j == 35) || (i == 14 && j == 34)) //street 24
				print_street2(streets[24]);
			else if((i == 13 && j == 42) || (i == 14 && j == 43)) //street 25
				print_street3(streets[25]);
			else if((i == 13 && j == 51) || (i == 14 && j == 50)) //street 26
				print_street2(streets[26]);
			else if(i == 15 && j >= 13 && j <= 16) //street 27
				print_street1(streets[27]);
			else if(i == 15 && j >= 29 && j <= 32) //street 28
				print_street1(streets[28]);
			else if(i == 15 && j >= 45 && j <= 48) //street 29
				print_street1(streets[29]);
			else if((i == 16 && j == 11) || (i == 17 && j == 10)) //street 30
				print_street2(streets[30]);
			else if((i == 16 && j == 18) || (i == 17 && j == 19)) //street 31
				print_street3(streets[31]);
			else if((i == 16 && j == 27) || (i == 17 && j == 26)) //street 32
				print_street2(streets[32]);
			else if((i == 16 && j == 34) || (i == 17 && j == 35)) //street 33
				print_street3(streets[33]);
			else if((i == 16 && j == 43) || (i == 17 && j == 42)) //street 34
				print_street2(streets[34]);
			else if((i == 16 && j == 50) || (i == 17 && j == 51)) //street 35
				print_street3(streets[35]);
			else if(i == 18 && j >= 21 && j <= 24) //street 36
				print_street1(streets[36]);
			else if(i == 18 && j >= 37 && j <= 40) //street 37
				print_street1(streets[37]);
			else if((i == 19 && j == 10) || (i == 20 && j == 11)) //street 38
				print_street3(streets[38]);
			else if((i == 19 && j == 19) || (i == 20 && j == 18)) //street 39
				print_street2(streets[39]);
			else if((i == 19 && j == 26) || (i == 20 && j == 27)) //street 40
				print_street3(streets[40]);
			else if((i == 19 && j == 35) || (i == 20 && j == 34)) //street 41
				print_street2(streets[41]);
			else if((i == 19 && j == 42) || (i == 20 && j == 43)) //street 42
				print_street3(streets[42]);
			else if((i == 19 && j == 51) || (i == 20 && j == 50)) //street 43
				print_street2(streets[43]);
			else if(i == 21 && j >= 13 && j <= 16) //street 44
				print_street1(streets[44]);
			else if(i == 21 && j >= 29 && j <= 32) //street 45
				print_street1(streets[45]);
			else if(i == 21 && j >= 45 && j <= 48) //street 46
				print_street1(streets[46]);
			else if((i == 22 && j == 11) || (i == 23 && j == 10)) //street 47
				print_street2(streets[47]);
			else if((i == 22 && j == 18) || (i == 23 && j == 19)) //street 48
				print_street3(streets[48]);
			else if((i == 22 && j == 27) || (i == 23 && j == 26)) //street 49
				print_street2(streets[49]);
			else if((i == 22 && j == 34) || (i == 23 && j == 35)) //street 50
				print_street3(streets[50]);
			else if((i == 22 && j == 43) || (i == 23 && j == 42)) //street 51
				print_street2(streets[51]);
			else if((i == 22 && j == 50) || (i == 23 && j == 51)) //street 52
				print_street3(streets[52]);
			else if(i == 24 && j >= 21 && j <= 24) //street 53
				print_street1(streets[53]);
			else if(i == 24 && j >= 37 && j <= 40) //street 54
				print_street1(streets[54]);
			else if((i == 25 && j == 10) || (i == 26 && j == 11)) //street 55
				print_street3(streets[55]);
			else if((i == 25 && j == 19) || (i == 26 && j == 18)) //street 56
				print_street2(streets[56]);
			else if((i == 25 && j == 26) || (i == 26 && j == 27)) //street 57
				print_street3(streets[57]);
			else if((i == 25 && j == 35) || (i == 26 && j == 34)) //street 58
				print_street2(streets[58]);
			else if((i == 25 && j == 42) || (i == 26 && j == 43)) //street 59
				print_street3(streets[59]);
			else if((i == 25 && j == 51) || (i == 26 && j == 50)) //street 60
				print_street2(streets[60]);
			else if(i == 27 && j >= 13 && j <= 16) //street 61
				print_street1(streets[61]);
			else if(i == 27 && j >= 29 && j <= 32) //street 62
				print_street1(streets[62]);
			else if(i == 27 && j >= 45 && j <= 48) //street 63
				print_street1(streets[63]);
			else if((i == 28 && j == 18) || (i == 29 && j == 19)) //street 64
				print_street3(streets[64]);
			else if((i == 28 && j == 27) || (i == 29 && j == 26)) //street 65
				print_street2(streets[65]);
			else if((i == 28 && j == 34) || (i == 29 && j == 35)) //street 66
				print_street3(streets[66]);
			else if((i == 28 && j == 43) || (i == 29 && j == 42)) //street 67
				print_street2(streets[67]);
			else if(i == 30 && j >= 21 && j <= 24) //street 68
				print_street1(streets[68]);
			else if(i == 30 && j >= 37 && j <= 40) //street 69
				print_street1(streets[69]);
			else if((i == 31 && j == 26) || (i == 32 && j == 27)) //street 70
				print_street3(streets[70]);
			else if((i == 31 && j == 35) || (i == 32 && j == 34)) //street 71
				print_street2(streets[71]);
			else if(i == 33 && j >= 29 && j <= 32) //street 72
				print_street1(streets[72]);
			
			//blocks
			
			else if(((i == 4 || i == 8) && j >= 28 && j <= 33) || ((i == 5 || i == 7) && j >= 27 && j <= 34) || (i == 6 && j >= 26 && j <= 35)){ //block 1
				if(i == 6 && j == 30) printf("\033[47;30m%02d\033[m", indexs[1]);
				else if(i != 6 || j != 31) print_block(blocks[1]);
			}
			
			else if(((i == 7 || i == 11) && j >= 20 && j <= 25) || ((i == 8 || i == 10) && j >= 19 && j <= 26) || (i == 9 && j >= 18 && j <= 27)){ //block 2
				if(i == 9 && j == 22) printf("\033[47;30m%02d\033[m", indexs[2]);
				else if(i != 9 || j != 23) print_block(blocks[2]);
			}
			
			else if(((i == 7 || i == 11) && j >= 36 && j <= 41) || ((i == 8 || i == 10) && j >= 35 && j <= 42) || (i == 9 && j >= 34 && j <= 43)){ //block 3
				if(i == 9 && j == 38) printf("\033[47;30m%02d\033[m", indexs[3]);
				else if(i != 9 || j != 39) print_block(blocks[3]);
			}
			
			else if(((i == 10 || i == 14) && j >= 12 && j <= 17) || ((i == 11 || i == 13) && j >= 11 && j <= 18) || (i == 12 && j >= 10 && j <= 19)){ //block 4
				if(i == 12 && j == 14) printf("\033[47;30m%02d\033[m", indexs[4]);
				else if(i != 12 || j != 15) print_block(blocks[4]);
			}
			
			else if(((i == 10 || i == 14) && j >= 28 && j <= 33) || ((i == 11 || i == 13) && j >= 27 && j <= 34) || (i == 12 && j >= 26 && j <= 35)){ //block 5
				if(i == 12 && j == 30) printf("\033[47;30m%02d\033[m", indexs[5]);
				else if(i != 12 || j != 31) print_block(blocks[5]);
			}
			
			else if(((i == 10 || i == 14) && j >= 44 && j <= 49) || ((i == 11 || i == 13) && j >= 43 && j <= 50) || (i == 12 && j >= 42 && j <= 51)){ //block 6
				if(i == 12 && j == 46) printf("\033[47;30m%02d\033[m", indexs[6]);
				else if(i != 12 || j != 47) print_block(blocks[6]);
			}
			
			else if(((i == 13 || i == 17) && j >= 20 && j <= 25) || ((i == 14 || i == 16) && j >= 19 && j <= 26) || (i == 15 && j >= 18 && j <= 27)){ //block 7
				if(i == 15 && j == 22) printf("\033[47;30m%02d\033[m", indexs[7]);
				else if(i != 15 || j != 23) print_block(blocks[7]);
			}
			
			else if(((i == 13 || i == 17) && j >= 36 && j <= 41) || ((i == 14 || i == 16) && j >= 35 && j <= 42) || (i == 15 && j >= 34 && j <= 44)){ //block 8
				if(i == 15 && j == 38) printf("\033[47;30m%02d\033[m", indexs[8]);
				else if(i != 15 || j != 39) print_block(blocks[8]);
			}
			
			else if(((i == 16 || i == 20) && j >= 12 && j <= 17) || ((i == 17 || i == 19) && j >= 11 && j <= 18) || (i == 18 && j >= 10 && j <= 19)){ //block 9
				if(i == 18 && j == 14) printf("\033[47;30m%02d\033[m", indexs[9]);
				else if(i != 18 || j != 15) print_block(blocks[9]);
			}
			
			else if(((i == 16 || i == 20) && j >= 28 && j <= 33) || ((i == 17 || i == 19) && j >= 27 && j <= 34) || (i == 18 && j >= 26 && j <= 35)){ //block 10
				if(i == 18 && j == 30) printf("\033[47;30;1mTF\033[m");
				else if(i != 18 || j != 31) print_block(blocks[10]);
			}
			
			else if(((i == 16 || i == 20) && j >= 44 && j <= 49) || ((i == 17 || i == 19) && j >= 43 && j <= 50) || (i == 18 && j >= 42 && j <= 51)){ //block 11
				if(i == 18 && j == 46) printf("\033[47;30m%02d\033[m", indexs[11]);
				else if(i != 18 || j != 47) print_block(blocks[11]);
			}
			
			else if(((i == 19 || i == 23) && j >= 20 && j <= 25) || ((i == 20 || i == 22) && j >= 19 && j <= 26) || (i == 21 && j >= 18 && j <= 27)){ //block 12
				if(i == 21 && j == 22) printf("\033[47;30m%02d\033[m", indexs[12]);
				else if(i != 21 || j != 23) print_block(blocks[12]);
			}
			
			else if(((i == 19 || i == 23) && j >= 36 && j <= 41) || ((i == 20 || i == 22) && j >= 35 && j <= 42) || (i == 21 && j >= 34 && j <= 43)){ //block 13
				if(i == 21 && j == 38) printf("\033[47;30m%02d\033[m", indexs[13]);
				else if(i != 21 || j != 39) print_block(blocks[13]);
			}
			
			else if(((i == 22 || i == 26) && j >= 12 && j <= 17) || ((i == 23 || i == 25) && j >= 11 && j <= 18) || (i == 24 && j >= 10 && j <= 19)){ //block 14
				if(i == 24 && j == 14) printf("\033[47;30m%02d\033[m", indexs[14]);
				else if(i != 24 || j != 15) print_block(blocks[14]);
			}
			
			else if(((i == 22 || i == 26) && j >= 28 && j <= 33) || ((i == 23 || i == 25) && j >= 27 && j <= 34) || (i == 24 && j >= 26 && j <= 35)){ //block 15
				if(i == 24 && j == 30) printf("\033[47;30m%02d\033[m", indexs[15]);
				else if(i != 24 || j != 31) print_block(blocks[15]);
			}
			
			else if(((i == 22 || i == 26) && j >= 44 && j <= 49) || ((i == 23 || i == 25) && j >= 43 && j <= 50) || (i == 24 && j >= 42 && j <= 51)){ //block 16
				if(i == 24 && j == 46) printf("\033[47;30m%02d\033[m", indexs[16]);
				else if(i != 24 || j != 47) print_block(blocks[16]);
			}
			
			else if(((i == 25 || i == 29) && j >= 20 && j <= 25) || ((i == 26 || i == 28) && j >= 19 && j <= 26) || (i == 27 && j >= 18 && j <= 27)){ //block 17
				if(i == 27 && j == 22) printf("\033[47;30m%02d\033[m", indexs[17]);
				else if(i != 27 || j != 23) print_block(blocks[17]);
			}
			
			else if(((i == 25 || i == 29) && j >= 36 && j <= 41) || ((i == 26 || i == 28) && j >= 35 && j <= 42) || (i == 27 && j >= 34 && j <= 43)){ //block 18
				if(i == 27 && j == 38) printf("\033[47;30m%02d\033[m", indexs[18]);
				else if(i != 27 || j != 39) print_block(blocks[18]);
			}
			
			else if(((i == 28 || i == 32) && j >= 28 && j <= 33) || ((i == 29 || i == 31) && j >= 27 && j <= 34) || (i == 30 && j >= 26 && j <= 35)){ //block 19
				if(i == 30 && j == 30) printf("\033[47;30m%02d\033[m", indexs[19]);
				else if(i != 30 || j != 31) print_block(blocks[19]);
			}
			
			//harbor 1
			else if(i >= 3 && i <= 5 && j >= 20 && j <= 25){
				if(i == 4 && j == 21)
					printf("\033[45;37;1m2\033[m");
				else if(i == 4 && j == 22)
					printf("\033[45;37;1m-\033[m");
				else if(i == 4 && j == 23)
					printf("\033[45;37;1m>\033[m");
				else if(i == 4 && j == 24)
					printf("\033[45;37;1m1\033[m");
				else
					printf("\033[45m \033[m");
			}
			
			//harbor 2
			else if(i >= 3 && i <= 5 && j >= 36 && j <= 41){
				if(i == 4 && j == 37)
					printf("\033[41;37;1m2\033[m");
				else if(i == 4 && j == 38)
					printf("\033[41;37;1m-\033[m");
				else if(i == 4 && j == 39)
					printf("\033[41;37;1m>\033[m");
				else if(i == 4 && j == 40)
					printf("\033[41;37;1m1\033[m");
				else
					printf("\033[41m \033[m");
			}
			
			//harbor 3
			else if(i >= 9 && i <= 11 && j >= 5 && j <= 11){
				if(i == 10 && j == 6)
					printf("\033[40;37;1m2\033[m");
				else if(i == 10 && j == 7)
					printf("\033[40;37;1m-\033[m");
				else if(i == 10 && j == 8)
					printf("\033[40;37;1m>\033[m");
				else if(i == 10 && j == 9)
					printf("\033[40;37;1m1\033[m");
				else if(i == 9 && j <= 16)
					printf("\033[40m \033[m");
				else if(i == 10 && j <= 15)
					printf("\033[40m \033[m");
				else if(i == 11 && j <= 14)
					printf("\033[40m \033[m");
			}
			
			//harbor 4
			else if(i >= 9 && i <= 11 && j >= 50 && j <= 56){
				if(i == 10 && j == 52)
					printf("\033[44;37;1m3\033[m");
				else if(i == 10 && j == 53)
					printf("\033[44;37;1m-\033[m");
				else if(i == 10 && j == 54)
					printf("\033[44;37;1m>\033[m");
				else if(i == 10 && j == 55)
					printf("\033[44;37;1m1\033[m");
				else if(i == 9 && j >= 50)
					printf("\033[44m \033[m");
				else if(i == 10 && j >= 51)
					printf("\033[44m \033[m");
				else if(i == 11 && j >= 52)
					printf("\033[44m \033[m");
			}
			
			//harbor 5
			else if(i >= 19 && i <= 21 && j >= 5 && j <= 11){
				if(i == 20 && j == 6)
					printf("\033[44;37;1m3\033[m");
				else if(i == 20 && j == 7)
					printf("\033[44;37;1m-\033[m");
				else if(i == 20 && j == 8)
					printf("\033[44;37;1m>\033[m");
				else if(i == 20 && j == 9)
					printf("\033[44;37;1m1\033[m");
				else if(i == 19 && j <= 14)
					printf("\033[44m \033[m");
				else if(i == 20 && j <= 15)
					printf("\033[44m \033[m");
				else if(i == 21 && j <= 16)
					printf("\033[44m \033[m");
			}
			
			//harbor 6
			else if(i >= 19 && i <= 21 && j >= 50 && j <= 56){
				if(i == 20 && j == 52)
					printf("\033[42;37;1m2\033[m");
				else if(i == 20 && j == 53)
					printf("\033[42;37;1m-\033[m");
				else if(i == 20 && j == 54)
					printf("\033[42;37;1m>\033[m");
				else if(i == 20 && j == 55)
					printf("\033[42;37;1m1\033[m");
				else if(i == 19 && j >= 52)
					printf("\033[42m \033[m");
				else if(i == 20 && j >= 51)
					printf("\033[42m \033[m");
				else if(i == 21 && j >= 50)
					printf("\033[42m \033[m");
			}
			
			//harbor 7
			else if(i >= 28 && i <= 30 && j >= 13 && j <= 19){
				if(i == 29 && j == 14)
					printf("\033[43;37;1m2\033[m");
				else if(i == 29 && j == 15)
					printf("\033[43;37;1m-\033[m");
				else if(i == 29 && j == 16)
					printf("\033[43;37;1m>\033[m");
				else if(i == 29 && j == 17)
					printf("\033[43;37;1m1\033[m");
				else if(i == 28 && j >= 13 && j <= 17)
					printf("\033[43m \033[m");
				else if(i == 29 && j >= 13 && j <= 18)
					printf("\033[43m \033[m");
				else if(i == 30 && j >= 13 && j <= 19)
					printf("\033[43m \033[m");
				else
					printf("\033[46m \033[m");
			}
			
			//harbor 8
			else if(i >= 28 && i <= 30 && j >= 42 && j <= 48){
				if(i == 29 && j == 44)
					printf("\033[44;37;1m3\033[m");
				else if(i == 29 && j == 45)
					printf("\033[44;37;1m-\033[m");
				else if(i == 29 && j == 46)
					printf("\033[44;37;1m>\033[m");
				else if(i == 29 && j == 47)
					printf("\033[44;37;1m1\033[m");
				else if(i == 28 && j >= 44 && j <= 48)
					printf("\033[44;37;1m \033[m");
				else if(i == 29 && j >= 43 && j <= 48)
					printf("\033[44;37;1m \033[m");
				else if(i == 30 && j >= 42 && j <= 48)
					printf("\033[44;37;1m \033[m");
				else
					printf("\033[46m \033[m");
			}
			
			//harbor 9
			else if(i >= 34 && i <= 36 && j >= 28 && j <= 33){
				if(i == 35 && j == 29)
					printf("\033[44;37;1m3\033[m");
				else if(i == 35 && j == 30)
					printf("\033[44;37;1m-\033[m");
				else if(i == 35 && j == 31)
					printf("\033[44;37;1m>\033[m");
				else if(i == 35 && j == 32)
					printf("\033[44;37;1m1\033[m");
				else
					printf("\033[44m \033[m");
			}
			
			//blue background
			else
				printf("\033[46m \033[m");
		}
		
		//player section 2
		for(int j = 1; j <= 20; j++){
			//com1
			if(i == 1 && j == 9)
				printf("\033[40;36;1mC\033[m");
			else if(i == 1 && j == 10)
				printf("\033[40;36;1mO\033[m");
			else if(i == 1 && j == 11)
				printf("\033[40;36;1mM\033[m");
			else if(i == 1 && j == 12)
				printf("\033[40;36;1m1\033[m");
			else if(i >= 2 && i <= 11 && j >= 2 && j <= 19)
				printf("\033[47m \033[m");
				
			//com3
			else if(i == 37 && j == 9)
				printf("\033[40;32;1mC\033[m");
			else if(i == 37 && j == 10)
				printf("\033[40;32;1mO\033[m");
			else if(i == 37 && j == 11)
				printf("\033[40;32;1mM\033[m");
			else if(i == 37 && j == 12)
				printf("\033[40;32;1m3\033[m");
			else if(i >= 27 && i <= 36 && j >= 2 && j <= 19)
				printf("\033[47m \033[m");
			
			//table
			else if(i >= 13 && i <= 25 && j >= 3 && j <= 18){
			
				//title
				if(i == 14 && j >= 3 && j <= 18){
					if(j == 6)
						printf("\033[40;37;1mB\033[m");
					else if(j == 7)
						printf("\033[40;37;1mU\033[m");
					else if(j == 8)
						printf("\033[40;37;1mI\033[m");
					else if(j == 9)
						printf("\033[40;37;1mL\033[m");
					else if(j == 10)
						printf("\033[40;37;1mD\033[m");
					else if(j == 11)
						printf("\033[40;37;1m \033[m");
					else if(j == 12)
						printf("\033[40;37;1mC\033[m");
					else if(j == 13)
						printf("\033[40;37;1mO\033[m");
					else if(j == 14)
						printf("\033[40;37;1mS\033[m");
					else if(j == 15)
						printf("\033[40;37;1mT\033[m");
					else if(j == 5 || j == 16)
						printf("\033[40;37;1m \033[m");
					else
						printf("\033[47m \033[m");
						
				//road
				}else if(i == 16 && j >= 3 && j <= 18){
					if(j == 5)
						printf("\033[42;37;1m1\033[m");
					else if(j == 6)
						printf("\033[41;37;1m1\033[m");
					else if(j == 10)
						printf("\033[47;30;1mR\033[m");
					else if(j == 11)
						printf("\033[47;30;1mO\033[m");
					else if(j == 12)
						printf("\033[47;30;1mA\033[m");
					else if(j == 13)
						printf("\033[47;30;1mD\033[m");
					else
						printf("\033[47m \033[m");
						
				}else if(i == 17 && j >= 3 && j <= 18){
					if(j == 10)
						printf("\033[47;30;1m(\033[m");
					else if(j == 11)
						printf("\033[47;30;1mP\033[m");
					else if(j == 12)
						printf("\033[47;30;1mT\033[m");
					else if(j == 13)
						printf("\033[47;31;1m+\033[m");
					else if(j == 14)
						printf("\033[47;31;1m0\033[m");
					else if(j == 15)
						printf("\033[47;30;1m)\033[m");
					else
						printf("\033[47m \033[m");
				
				//village
				}else if(i == 18 && j >= 3 && j <= 18){
					if(j == 5)
						printf("\033[42;37;1m1\033[m");
					else if(j == 6)
						printf("\033[41;37;1m1\033[m");
					else if(j == 7)
						printf("\033[43;37;1m1\033[m");
					else if(j == 8)
						printf("\033[45;37;1m1\033[m");
					else if(j == 10)
						printf("\033[47;30;1mV\033[m");
					else if(j == 11)
						printf("\033[47;30;1mI\033[m");
					else if(j == 12)
						printf("\033[47;30;1mL\033[m");
					else if(j == 13)
						printf("\033[47;30;1mL\033[m");
					else if(j == 14)
						printf("\033[47;30;1mA\033[m");
					else if(j == 15)
						printf("\033[47;30;1mG\033[m");
					else if(j == 16)
						printf("\033[47;30;1mE\033[m");
					else
						printf("\033[47m \033[m");
				
				}else if(i == 19 && j >= 3 && j <= 18){
					if(j == 10)
						printf("\033[47;30;1m(\033[m");
					else if(j == 11)
						printf("\033[47;30;1mP\033[m");
					else if(j == 12)
						printf("\033[47;30;1mT\033[m");
					else if(j == 13)
						printf("\033[47;31;1m+\033[m");
					else if(j == 14)
						printf("\033[47;31;1m1\033[m");
					else if(j == 15)
						printf("\033[47;30;1m)\033[m");
					else
						printf("\033[47m \033[m");
				
				//city
				}else if(i == 20 && j >= 3 && j <= 18){
					if(j == 5)
						printf("\033[43;37;1m2\033[m");
					else if(j == 6)
						printf("\033[40;37;1m3\033[m");
					else if(j == 10)
						printf("\033[47;30;1mC\033[m");
					else if(j == 11)
						printf("\033[47;30;1mI\033[m");
					else if(j == 12)
						printf("\033[47;30;1mT\033[m");
					else if(j == 13)
						printf("\033[47;30;1mY\033[m");
					else
						printf("\033[47m \033[m");
				
				}else if(i == 21 && j >= 3 && j <= 18){
					if(j == 10)
						printf("\033[47;30;1m(\033[m");
					else if(j == 11)
						printf("\033[47;30;1mP\033[m");
					else if(j == 12)
						printf("\033[47;30;1mT\033[m");
					else if(j == 13)
						printf("\033[47;31;1m+\033[m");
					else if(j == 14)
						printf("\033[47;31;1m2\033[m");
					else if(j == 15)
						printf("\033[47;30;1m)\033[m");
					else
						printf("\033[47m \033[m");
				
				//develop
				}else if(i == 22 && j >= 3 && j <= 18){
					if(j == 5)
						printf("\033[43;37;1m1\033[m");
					else if(j == 6)
						printf("\033[45;37;1m1\033[m");
					else if(j == 7)
						printf("\033[40;37;1m1\033[m");
					else if(j == 10)
						printf("\033[47;30;1mD\033[m");
					else if(j == 11)
						printf("\033[47;30;1mE\033[m");
					else if(j == 12)
						printf("\033[47;30;1mV\033[m");
					else if(j == 13)
						printf("\033[47;30;1mE\033[m");
					else if(j == 14)
						printf("\033[47;30;1mL\033[m");
					else if(j == 15)
						printf("\033[47;30;1mO\033[m");
					else if(j == 16)
						printf("\033[47;30;1mP\033[m");
					else
						printf("\033[47m \033[m");
						
				}else if(i == 23 && j >= 3 && j <= 18){
					if(j == 10)
						printf("\033[47;30;1mC\033[m");
					else if(j == 11)
						printf("\033[47;30;1mA\033[m");
					else if(j == 12)
						printf("\033[47;30;1mR\033[m");
					else if(j == 13)
						printf("\033[47;30;1mD\033[m");
					else
						printf("\033[47m \033[m");
						
				}else if(i == 24 && j >= 3 && j <= 18){
					if(j == 10)
						printf("\033[47;30;1m(\033[m");
					else if(j == 11)
						printf("\033[47;30;1mP\033[m");
					else if(j == 12)
						printf("\033[47;30;1mT\033[m");
					else if(j == 13)
						printf("\033[47;31;1m+\033[m");
					else if(j == 14)
						printf("\033[47;31;1m?\033[m");
					else if(j == 15)
						printf("\033[47;30;1m)\033[m");
					else
						printf("\033[47m \033[m");
				
				}else
					printf("\033[47m \033[m");
			
			//black background
			}else
				printf("\033[40m \033[m");
		}
		printf("\n");
	}
	
	return;
}

void map_generator( int blocks[] ){
	for(int i = 0; i < 500; i++){
		int r1 = rand() % 19 + 1;
		int r2 = rand() % 19 + 1;
		int temp = blocks[r1];
		blocks[r1] = blocks[r2];
		blocks[r2] = temp;
	}
	for(int i = 1; i < 20; i++){
		if(blocks[i] == 6){
			int temp = blocks[i];
			blocks[i] = blocks[10];
			blocks[10] = temp;
		}
	}
	
	return;
}

//print *
void print_village( int x ){
	switch(x){
		case 0:
			printf("\033[40;37;1m*\033[m");
			break;
		case 1:
			printf("\033[40;31;1m*\033[m");
			break;
		case 2:
			printf("\033[40;36;1m*\033[m");
			break;
		case 3:
			printf("\033[40;33;1m*\033[m");
			break;
		case 4:
			printf("\033[40;32;1m*\033[m");
			break;
		case 5:
			printf("\033[40;31;1m#\033[m");
			break;
		case 6:
			printf("\033[40;36;1m#\033[m");
			break;
		case 7:
			printf("\033[40;33;1m#\033[m");
			break;
		case 8:
			printf("\033[40;32;1m#\033[m");
			break;
		default:
			break;
	}
	
	return;
}

//print -
void print_street1( int x ){
	switch(x){
		case 0:
			printf("\033[40;37;1m-\033[m");
			break;
		case 1:
			printf("\033[40;31;1m-\033[m");
			break;
		case 2:
			printf("\033[40;36;1m-\033[m");
			break;
		case 3:
			printf("\033[40;33;1m-\033[m");
			break;
		case 4:
			printf("\033[40;32;1m-\033[m");
			break;
		default:
			break;
	}
	
	return;
}

//print /
void print_street2( int x ){
	switch(x){
		case 0:
			printf("\033[40;37;1m/\033[m");
			break;
		case 1:
			printf("\033[40;31;1m/\033[m");
			break;
		case 2:
			printf("\033[40;36;1m/\033[m");
			break;
		case 3:
			printf("\033[40;33;1m/\033[m");
			break;
		case 4:
			printf("\033[40;32;1m/\033[m");
			break;
		default:
			break;
	}
	
	return;
}

//print counter /
void print_street3( int x ){
	switch(x){
		case 0:
			printf("\033[40;37;1m\\\033[m");
			break;
		case 1:
			printf("\033[40;31;1m\\\033[m");
			break;
		case 2:
			printf("\033[40;36;1m\\\033[m");
			break;
		case 3:
			printf("\033[40;33;1m\\\033[m");
			break;
		case 4:
			printf("\033[40;32;1m\\\033[m");
			break;
		default:
			break;
	}
	
	return;
}

//print space
void print_block( int x ){
	switch(x){
		case 0:
			printf("\033[47m \033[m");
			break;
		case 1:
			printf("\033[45m \033[m");
			break;
		case 2:
			printf("\033[42m \033[m");
			break;
		case 3:
			printf("\033[43m \033[m");
			break;
		case 4:
			printf("\033[40m \033[m");
			break;
		case 5:
			printf("\033[41m \033[m");
			break;
		case 6:
			printf("\033[44m \033[m");
			break;
		default:
			break;
	}
	
	return;
}
