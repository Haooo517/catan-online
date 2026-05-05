#pragma once

#define NUM_PLAYERS   4
#define NUM_BLOCKS    20   /* indices 1..19 used */
#define NUM_VILLAGES  55   /* indices 1..54 used */
#define NUM_STREETS   73   /* indices 1..72 used */
#define NUM_RESOURCES 5
#define NUM_HARBOURS  9
#define NUM_DEV_TYPES 5
#define DECK_SIZE     25

/* Resource indices into Player.resources[] */
enum {
	R_WOOL   = 0,
	R_BRICK  = 1,
	R_ORE    = 2,
	R_WHEAT  = 3,
	R_LUMBER = 4
};

/* Block type values */
enum {
	T_WOOL   = 1,
	T_LUMBER = 2,
	T_WHEAT  = 3,
	T_ORE    = 4,
	T_BRICK  = 5,
	T_DESERT = 6
};

/* Development card types */
enum {
	DEV_KNIGHT = 0,
	DEV_VP     = 1,
	DEV_YOP    = 2,   /* year of plenty */
	DEV_MONO   = 3,   /* monopoly */
	DEV_ROAD   = 4    /* road building */
};

/* Harbour type: 0 = generic 3:1; otherwise matches resource enum +1
 * (1=wool, 2=lumber, 3=wheat, 4=ore, 5=brick). */
typedef struct {
	int type;
	int villages[2];
} Harbour;

typedef struct {
	int  resources[NUM_RESOURCES];
	int  villages_left;
	int  cities_left;
	int  roads_left;
	int  points;            /* including longest road / largest army / hidden VP cards */

	int  dev_hand[NUM_DEV_TYPES];
	int  dev_locked[NUM_DEV_TYPES];   /* bought this turn — can't play yet */
	int  knights_played;
	int  has_largest_army;
	int  has_longest_road;
	int  longest_road_length;
	int  played_dev_this_turn;        /* enforces 1 dev card per turn */

	int  color;
	int  is_human;
	const char *name;
} Player;

typedef struct {
	int street_v1[NUM_STREETS];
	int street_v2[NUM_STREETS];
	int village_streets[NUM_VILLAGES][3];
	int village_streets_count[NUM_VILLAGES];
	int village_blocks[NUM_VILLAGES][3];
	int village_blocks_count[NUM_VILLAGES];
} BoardTopology;

typedef struct {
	int      *blocks;
	int      *indexs;
	int      *villages;
	int      *streets;
	int       thief_block;
	int       last_roll;

	Harbour   harbours[NUM_HARBOURS];

	int       dev_deck[NUM_DEV_TYPES];
	int       deck_remaining;

	int       largest_army_holder;   /* -1 if none */
	int       largest_army_count;    /* current threshold (>=3) */
	int       longest_road_holder;
	int       longest_road_length;   /* current threshold (>=5) */

	Player    players[NUM_PLAYERS];
	BoardTopology topo;
} Game;

/* lifecycle */
void game_init(Game *g, int blocks[], int indexs[], int villages[], int streets[]);

/* setup & turns */
void run_setup_phase(Game *g);
void run_player_turn(Game *g, int player_idx);
int  game_winner(const Game *g);

/* helpers */
void print_status(const Game *g);
const char *resource_name(int r);
const char *type_name(int t);
const char *dev_name(int d);
int  type_to_resource(int t);

/* dice */
int  roll_dice(void);
void distribute_resources(Game *g, int roll);
