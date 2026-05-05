#pragma once

#define NUM_PLAYERS   4
#define NUM_BLOCKS    20   /* indices 1..19 used */
#define NUM_VILLAGES  55   /* indices 1..54 used */
#define NUM_STREETS   73   /* indices 1..72 used */
#define NUM_RESOURCES 5

/* Resource indices into Player.resources[] */
enum {
	R_WOOL   = 0,
	R_BRICK  = 1,
	R_ORE    = 2,
	R_WHEAT  = 3,
	R_LUMBER = 4
};

/* Block type values (match catan.c blocks[] and print_block colors) */
enum {
	T_WOOL   = 1,
	T_LUMBER = 2,
	T_WHEAT  = 3,
	T_ORE    = 4,
	T_BRICK  = 5,
	T_DESERT = 6
};

typedef struct {
	int  resources[NUM_RESOURCES];
	int  villages_left;
	int  cities_left;
	int  roads_left;
	int  points;
	int  color;       /* 1..4 — used for villages[] / streets[] cell value */
	int  is_human;
	const char *name;
} Player;

typedef struct {
	int street_v1[NUM_STREETS];   /* street -> village endpoint 1 */
	int street_v2[NUM_STREETS];   /* street -> village endpoint 2 */
	int village_streets[NUM_VILLAGES][3];
	int village_streets_count[NUM_VILLAGES];
	int village_blocks[NUM_VILLAGES][3];
	int village_blocks_count[NUM_VILLAGES];
} BoardTopology;

typedef struct {
	int      *blocks;        /* hex resource type, 1..19 (catan.c owns the array) */
	int      *indexs;        /* hex number token, 1..19 */
	int      *villages;      /* village owner code, 1..54  (0=empty, 1..4=village, 5..8=city) */
	int      *streets;       /* road owner code,    1..72  (0=empty, 1..4=color) */
	int       thief_block;   /* current thief block index, 1..19 */
	int       last_roll;
	Player    players[NUM_PLAYERS];
	BoardTopology topo;
} Game;

/* lifecycle */
void game_init(Game *g, int blocks[], int indexs[], int villages[], int streets[]);

/* setup */
void run_setup_phase(Game *g);

/* turns */
void run_player_turn(Game *g, int player_idx);
int  game_winner(const Game *g);  /* returns 0..3 if someone has >= 10 points, else -1 */

/* helpers used by main / status display */
void print_status(const Game *g);
const char *resource_name(int r);
const char *type_name(int t);
int  type_to_resource(int t);

/* dice */
int  roll_dice(void);   /* returns 2..12 */
void distribute_resources(Game *g, int roll);
