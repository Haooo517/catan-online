#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "catan_game.h"
#include "catan_menu.h"
#include "catan_map.h"

/* ============================================================
 * Static topology — derived from print_map geometry.
 * Each entry is (street_id) -> (village_v1, village_v2).
 * ============================================================ */
static const int STREET_V1[NUM_STREETS] = {
	/*  0*/ 0,
	/*  1*/  1, /*  2*/  1, /*  3*/  2, /*  4*/  3, /*  5*/  5,
	/*  6*/  3, /*  7*/  4, /*  8*/  5, /*  9*/  6, /* 10*/  7,
	/* 11*/  9, /* 12*/ 11, /* 13*/  7, /* 14*/  8, /* 15*/  9,
	/* 16*/ 10, /* 17*/ 11, /* 18*/ 12, /* 19*/ 14, /* 20*/ 16,
	/* 21*/ 13, /* 22*/ 14, /* 23*/ 15, /* 24*/ 16, /* 25*/ 17,
	/* 26*/ 18, /* 27*/ 19, /* 28*/ 21, /* 29*/ 23, /* 30*/ 19,
	/* 31*/ 20, /* 32*/ 21, /* 33*/ 22, /* 34*/ 23, /* 35*/ 24,
	/* 36*/ 26, /* 37*/ 28, /* 38*/ 25, /* 39*/ 26, /* 40*/ 27,
	/* 41*/ 28, /* 42*/ 29, /* 43*/ 30, /* 44*/ 31, /* 45*/ 33,
	/* 46*/ 35, /* 47*/ 31, /* 48*/ 32, /* 49*/ 33, /* 50*/ 34,
	/* 51*/ 35, /* 52*/ 36, /* 53*/ 38, /* 54*/ 40, /* 55*/ 37,
	/* 56*/ 38, /* 57*/ 39, /* 58*/ 40, /* 59*/ 41, /* 60*/ 42,
	/* 61*/ 43, /* 62*/ 45, /* 63*/ 47, /* 64*/ 44, /* 65*/ 45,
	/* 66*/ 46, /* 67*/ 47, /* 68*/ 49, /* 69*/ 51, /* 70*/ 50,
	/* 71*/ 51, /* 72*/ 53
};
static const int STREET_V2[NUM_STREETS] = {
	/*  0*/ 0,
	/*  1*/  2, /*  2*/  4, /*  3*/  5, /*  4*/  4, /*  5*/  6,
	/*  6*/  8, /*  7*/  9, /*  8*/ 10, /*  9*/ 11, /* 10*/  8,
	/* 11*/ 10, /* 12*/ 12, /* 13*/ 13, /* 14*/ 14, /* 15*/ 15,
	/* 16*/ 16, /* 17*/ 17, /* 18*/ 18, /* 19*/ 15, /* 20*/ 17,
	/* 21*/ 19, /* 22*/ 20, /* 23*/ 21, /* 24*/ 22, /* 25*/ 23,
	/* 26*/ 24, /* 27*/ 20, /* 28*/ 22, /* 29*/ 24, /* 30*/ 25,
	/* 31*/ 26, /* 32*/ 27, /* 33*/ 28, /* 34*/ 29, /* 35*/ 30,
	/* 36*/ 27, /* 37*/ 29, /* 38*/ 31, /* 39*/ 32, /* 40*/ 33,
	/* 41*/ 34, /* 42*/ 35, /* 43*/ 36, /* 44*/ 32, /* 45*/ 34,
	/* 46*/ 36, /* 47*/ 37, /* 48*/ 38, /* 49*/ 39, /* 50*/ 40,
	/* 51*/ 41, /* 52*/ 42, /* 53*/ 39, /* 54*/ 41, /* 55*/ 43,
	/* 56*/ 44, /* 57*/ 45, /* 58*/ 46, /* 59*/ 47, /* 60*/ 48,
	/* 61*/ 44, /* 62*/ 46, /* 63*/ 48, /* 64*/ 49, /* 65*/ 50,
	/* 66*/ 51, /* 67*/ 52, /* 68*/ 50, /* 69*/ 52, /* 70*/ 53,
	/* 71*/ 54, /* 72*/ 54
};

/* Block -> 6 villages (top-l, top-r, mid-l, mid-r, bot-l, bot-r). */
static const int BLOCK_VILLAGES[NUM_BLOCKS][6] = {
	{0,0,0,0,0,0},                      /* 0 unused */
	{ 1, 2, 4, 5, 9,10}, /*  1 */
	{ 3, 4, 8, 9,14,15}, /*  2 */
	{ 5, 6,10,11,16,17}, /*  3 */
	{ 7, 8,13,14,19,20}, /*  4 */
	{ 9,10,15,16,21,22}, /*  5 */
	{11,12,17,18,23,24}, /*  6 */
	{14,15,20,21,26,27}, /*  7 */
	{16,17,22,23,28,29}, /*  8 */
	{19,20,25,26,31,32}, /*  9 */
	{21,22,27,28,33,34}, /* 10 (desert) */
	{23,24,29,30,35,36}, /* 11 */
	{26,27,32,33,38,39}, /* 12 */
	{28,29,34,35,40,41}, /* 13 */
	{31,32,37,38,43,44}, /* 14 */
	{33,34,39,40,45,46}, /* 15 */
	{35,36,41,42,47,48}, /* 16 */
	{38,39,44,45,49,50}, /* 17 */
	{40,41,46,47,51,52}, /* 18 */
	{45,46,50,51,53,54}  /* 19 */
};

/* ============================================================
 * Topology builder
 * ============================================================ */
static void build_topology(BoardTopology *t) {
	memcpy(t->street_v1, STREET_V1, sizeof(STREET_V1));
	memcpy(t->street_v2, STREET_V2, sizeof(STREET_V2));

	for (int v = 0; v < NUM_VILLAGES; v++) {
		t->village_streets_count[v] = 0;
		t->village_blocks_count[v]  = 0;
	}

	for (int s = 1; s < NUM_STREETS; s++) {
		int v1 = t->street_v1[s];
		int v2 = t->street_v2[s];
		if (v1 == 0 || v2 == 0) continue;
		t->village_streets[v1][t->village_streets_count[v1]++] = s;
		t->village_streets[v2][t->village_streets_count[v2]++] = s;
	}

	for (int b = 1; b < NUM_BLOCKS; b++) {
		for (int k = 0; k < 6; k++) {
			int v = BLOCK_VILLAGES[b][k];
			t->village_blocks[v][t->village_blocks_count[v]++] = b;
		}
	}
}

/* ============================================================
 * Naming helpers
 * ============================================================ */
const char *resource_name(int r) {
	switch (r) {
		case R_WOOL:   return "wool";
		case R_BRICK:  return "brick";
		case R_ORE:    return "ore";
		case R_WHEAT:  return "wheat";
		case R_LUMBER: return "lumber";
		default:       return "?";
	}
}
const char *type_name(int t) {
	switch (t) {
		case T_WOOL:   return "wool";
		case T_LUMBER: return "lumber";
		case T_WHEAT:  return "wheat";
		case T_ORE:    return "ore";
		case T_BRICK:  return "brick";
		case T_DESERT: return "desert";
		default:       return "?";
	}
}
int type_to_resource(int t) {
	switch (t) {
		case T_WOOL:   return R_WOOL;
		case T_LUMBER: return R_LUMBER;
		case T_WHEAT:  return R_WHEAT;
		case T_ORE:    return R_ORE;
		case T_BRICK:  return R_BRICK;
		default:       return -1;
	}
}

/* ============================================================
 * Game init
 * ============================================================ */
static const char *PLAYER_NAMES[NUM_PLAYERS] = { "P1 (you)", "COM1", "COM2", "COM3" };

void game_init(Game *g, int blocks[], int indexs[], int villages[], int streets[]) {
	g->blocks   = blocks;
	g->indexs   = indexs;
	g->villages = villages;
	g->streets  = streets;
	g->last_roll = 0;

	for (int i = 0; i < NUM_VILLAGES; i++) villages[i] = 0;
	for (int i = 0; i < NUM_STREETS;  i++) streets[i]  = 0;

	/* Thief starts on the desert (block whose type == T_DESERT). */
	g->thief_block = 10;
	for (int b = 1; b < NUM_BLOCKS; b++) {
		if (blocks[b] == T_DESERT) { g->thief_block = b; break; }
	}

	for (int i = 0; i < NUM_PLAYERS; i++) {
		Player *p = &g->players[i];
		for (int r = 0; r < NUM_RESOURCES; r++) p->resources[r] = 0;
		p->villages_left = 5;
		p->cities_left   = 4;
		p->roads_left    = 15;
		p->points        = 0;
		p->color         = i + 1;       /* 1..4 used by villages[]/streets[] cell value */
		p->is_human      = (i == 0);
		p->name          = PLAYER_NAMES[i];
	}

	build_topology(&g->topo);
}

/* ============================================================
 * Adjacency / placement validation
 * ============================================================ */
static int village_owner_color(const Game *g, int v) {
	int x = g->villages[v];
	if (x == 0) return 0;
	if (x <= 4) return x;       /* village */
	return x - 4;               /* city: 5..8 -> color 1..4 */
}

static int village_is_city(const Game *g, int v) {
	return g->villages[v] >= 5;
}

/* Two villages are "adjacent" if they share a street. */
static int village_has_neighboring_settlement(const Game *g, int v) {
	for (int k = 0; k < g->topo.village_streets_count[v]; k++) {
		int s = g->topo.village_streets[v][k];
		int other = (g->topo.street_v1[s] == v) ? g->topo.street_v2[s] : g->topo.street_v1[s];
		if (g->villages[other] != 0) return 1;
	}
	return 0;
}

/* Village placement during normal play: empty + no neighboring settlement
 * + connected to a road of the same color. */
static int can_place_village(const Game *g, int v, int color) {
	if (v < 1 || v >= NUM_VILLAGES) return 0;
	if (g->villages[v] != 0) return 0;
	if (village_has_neighboring_settlement(g, v)) return 0;
	for (int k = 0; k < g->topo.village_streets_count[v]; k++) {
		int s = g->topo.village_streets[v][k];
		if (g->streets[s] == color) return 1;
	}
	return 0;
}

/* Setup-phase village placement: empty + no neighboring settlement
 * (no road requirement). */
static int can_place_village_setup(const Game *g, int v) {
	if (v < 1 || v >= NUM_VILLAGES) return 0;
	if (g->villages[v] != 0) return 0;
	if (village_has_neighboring_settlement(g, v)) return 0;
	return 1;
}

static int can_upgrade_to_city(const Game *g, int v, int color) {
	if (v < 1 || v >= NUM_VILLAGES) return 0;
	if (village_is_city(g, v)) return 0;
	if (village_owner_color(g, v) != color) return 0;
	return 1;
}

/* Road placement during normal play: empty + at least one endpoint either
 * owns a village/city of the same color OR is connected to a same-color road
 * (the endpoint must not have a settlement of another color blocking us). */
static int can_place_road(const Game *g, int s, int color) {
	if (s < 1 || s >= NUM_STREETS) return 0;
	if (g->streets[s] != 0) return 0;
	int v1 = g->topo.street_v1[s];
	int v2 = g->topo.street_v2[s];
	int endpoints[2] = { v1, v2 };
	for (int e = 0; e < 2; e++) {
		int v = endpoints[e];
		int owner = village_owner_color(g, v);
		if (owner == color) return 1;          /* connected to our own settlement */
		if (owner == 0) {
			/* check connecting roads through this empty vertex */
			for (int k = 0; k < g->topo.village_streets_count[v]; k++) {
				int s2 = g->topo.village_streets[v][k];
				if (s2 == s) continue;
				if (g->streets[s2] == color) return 1;
			}
		}
	}
	return 0;
}

/* Setup-phase road placement: must touch the village just placed. */
static int can_place_road_setup(const Game *g, int s, int v_just_placed) {
	if (s < 1 || s >= NUM_STREETS) return 0;
	if (g->streets[s] != 0) return 0;
	int v1 = g->topo.street_v1[s];
	int v2 = g->topo.street_v2[s];
	return (v1 == v_just_placed || v2 == v_just_placed);
}

/* ============================================================
 * Build-cost helpers
 * ============================================================ */
static int can_afford_road(const Player *p) {
	return p->resources[R_LUMBER] >= 1 && p->resources[R_BRICK] >= 1;
}
static int can_afford_village(const Player *p) {
	return p->resources[R_LUMBER] >= 1 && p->resources[R_BRICK] >= 1
	    && p->resources[R_WHEAT]  >= 1 && p->resources[R_WOOL]  >= 1;
}
static int can_afford_city(const Player *p) {
	return p->resources[R_WHEAT] >= 2 && p->resources[R_ORE] >= 3;
}

static void pay_road(Player *p)    { p->resources[R_LUMBER]--; p->resources[R_BRICK]--; }
static void pay_village(Player *p) { p->resources[R_LUMBER]--; p->resources[R_BRICK]--; p->resources[R_WHEAT]--; p->resources[R_WOOL]--; }
static void pay_city(Player *p)    { p->resources[R_WHEAT] -= 2; p->resources[R_ORE] -= 3; }

/* ============================================================
 * Dice & resource distribution
 * ============================================================ */
int roll_dice(void) {
	int a = (rand() % 6) + 1;
	int b = (rand() % 6) + 1;
	return a + b;
}

void distribute_resources(Game *g, int roll) {
	for (int b = 1; b < NUM_BLOCKS; b++) {
		if (g->indexs[b] != roll) continue;
		if (b == g->thief_block) continue;
		int res = type_to_resource(g->blocks[b]);
		if (res < 0) continue;
		for (int k = 0; k < 6; k++) {
			int v     = BLOCK_VILLAGES[b][k];
			int color = village_owner_color(g, v);
			if (color == 0) continue;
			int amount = village_is_city(g, v) ? 2 : 1;
			g->players[color - 1].resources[res] += amount;
		}
	}
}

static int total_cards(const Player *p) {
	int s = 0;
	for (int r = 0; r < NUM_RESOURCES; r++) s += p->resources[r];
	return s;
}

/* On a 7: every player with >7 cards discards floor(n/2). For simplicity
 * the human gets prompted; AI discards from largest stockpile first.
 * Then the roller (or AI) moves the thief to a non-thief block. */
static void apply_thief(Game *g, int roller_idx) {
	for (int i = 0; i < NUM_PLAYERS; i++) {
		Player *p = &g->players[i];
		int total = total_cards(p);
		if (total <= 7) continue;
		int discard = total / 2;
		if (p->is_human) {
			printf("\n[Thief] You have %d cards and must discard %d.\n", total, discard);
			while (discard > 0) {
				printf("  Resources: ");
				for (int r = 0; r < NUM_RESOURCES; r++)
					printf("%s=%d  ", resource_name(r), p->resources[r]);
				printf("\n");
				int sel = read_int("  Discard which? 1)Wool 2)Brick 3)Ore 4)Wheat 5)Lumber: ", 1, 5) - 1;
				if (p->resources[sel] <= 0) { printf("    none of those.\n"); continue; }
				p->resources[sel]--;
				discard--;
			}
		} else {
			printf("[Thief] %s discards %d cards.\n", p->name, discard);
			while (discard > 0) {
				int big = 0;
				for (int r = 1; r < NUM_RESOURCES; r++)
					if (p->resources[r] > p->resources[big]) big = r;
				if (p->resources[big] <= 0) break;
				p->resources[big]--;
				discard--;
			}
		}
	}

	Player *roller = &g->players[roller_idx];
	int target = g->thief_block;
	if (roller->is_human) {
		printf("\n[Thief] Move the thief. Current block: %d.\n", g->thief_block);
		while (1) {
			target = read_int("  Move thief to block (1-19, different from current): ", 1, 19);
			if (target != g->thief_block) break;
			printf("    must be different from current.\n");
		}
	} else {
		/* AI: pick a block with most opposing settlements (and not desert). */
		int best = -1, best_score = -1;
		for (int b = 1; b < NUM_BLOCKS; b++) {
			if (b == g->thief_block) continue;
			int score = 0;
			for (int k = 0; k < 6; k++) {
				int v = BLOCK_VILLAGES[b][k];
				int c = village_owner_color(g, v);
				if (c != 0 && c != roller->color) score += village_is_city(g, v) ? 2 : 1;
			}
			if (score > best_score) { best_score = score; best = b; }
		}
		if (best > 0) target = best;
		printf("[Thief] %s moves the thief to block %d.\n", roller->name, target);
	}
	g->thief_block = target;
}

/* ============================================================
 * Setup phase
 * ============================================================ */

/* Heuristic value for placing a settlement at v: sum of pip counts of the
 * adjacent non-desert hexes (6-|7-n|, 0 for desert / 0). */
static int pip_count(int n) {
	if (n <= 0 || n == 7) return 0;
	int d = (n > 7) ? n - 7 : 7 - n;
	return 6 - d;
}
static int village_setup_score(const Game *g, int v) {
	int s = 0;
	for (int k = 0; k < g->topo.village_blocks_count[v]; k++) {
		int b = g->topo.village_blocks[v][k];
		if (g->blocks[b] == T_DESERT) continue;
		s += pip_count(g->indexs[b]);
	}
	return s;
}

static int ai_pick_setup_village(const Game *g) {
	int best = 0, best_score = -1;
	for (int v = 1; v < NUM_VILLAGES; v++) {
		if (!can_place_village_setup(g, v)) continue;
		int s = village_setup_score(g, v);
		if (s > best_score) { best_score = s; best = v; }
	}
	return best;
}

static int ai_pick_setup_road(const Game *g, int v) {
	for (int k = 0; k < g->topo.village_streets_count[v]; k++) {
		int s = g->topo.village_streets[v][k];
		if (can_place_road_setup(g, s, v)) return s;
	}
	return 0;
}

static int human_pick_setup_village(const Game *g) {
	while (1) {
		int v = read_int("  Place village at vertex (1-54): ", 1, 54);
		if (can_place_village_setup(g, v)) return v;
		printf("    invalid (occupied or adjacent to another settlement).\n");
	}
}
static int human_pick_setup_road(const Game *g, int v) {
	printf("  Adjacent edges to vertex %d:", v);
	for (int k = 0; k < g->topo.village_streets_count[v]; k++)
		printf(" %d", g->topo.village_streets[v][k]);
	printf("\n");
	while (1) {
		int s = read_int("  Place road at edge (1-72): ", 1, 72);
		if (can_place_road_setup(g, s, v)) return s;
		printf("    invalid (must touch your new village).\n");
	}
}

static void give_initial_resources(Game *g, int color, int v) {
	Player *p = &g->players[color - 1];
	for (int k = 0; k < g->topo.village_blocks_count[v]; k++) {
		int b   = g->topo.village_blocks[v][k];
		int res = type_to_resource(g->blocks[b]);
		if (res >= 0) p->resources[res]++;
	}
}

void run_setup_phase(Game *g) {
	int order_fwd[NUM_PLAYERS] = {0,1,2,3};
	int order_rev[NUM_PLAYERS] = {3,2,1,0};

	for (int round = 0; round < 2; round++) {
		const int *order = (round == 0) ? order_fwd : order_rev;
		for (int idx = 0; idx < NUM_PLAYERS; idx++) {
			int pi    = order[idx];
			Player *p = &g->players[pi];
			print_map(g->blocks, g->villages, g->streets, g->indexs);
			printf("\n[Setup round %d] %s (color %d) — place a village and a road.\n",
				round + 1, p->name, p->color);

			int v, s;
			if (p->is_human) {
				v = human_pick_setup_village(g);
				g->villages[v] = p->color;
				p->villages_left--;
				print_map(g->blocks, g->villages, g->streets, g->indexs);
				s = human_pick_setup_road(g, v);
			} else {
				v = ai_pick_setup_village(g);
				g->villages[v] = p->color;
				p->villages_left--;
				s = ai_pick_setup_road(g, v);
				printf("  %s places village at %d and road at %d.\n", p->name, v, s);
			}
			g->streets[s] = p->color;
			p->roads_left--;
			p->points++;

			/* On the second placement, give starting resources. */
			if (round == 1) give_initial_resources(g, p->color, v);
		}
	}
}

/* ============================================================
 * Build actions (return 1 on success, 0 on failure)
 * ============================================================ */
static int do_build_road(Game *g, Player *p, int s) {
	if (!can_place_road(g, s, p->color)) return 0;
	if (p->roads_left <= 0) return 0;
	if (!can_afford_road(p)) return 0;
	pay_road(p);
	g->streets[s] = p->color;
	p->roads_left--;
	return 1;
}
static int do_build_village(Game *g, Player *p, int v) {
	if (!can_place_village(g, v, p->color)) return 0;
	if (p->villages_left <= 0) return 0;
	if (!can_afford_village(p)) return 0;
	pay_village(p);
	g->villages[v] = p->color;
	p->villages_left--;
	p->points++;
	return 1;
}
static int do_build_city(Game *g, Player *p, int v) {
	if (!can_upgrade_to_city(g, v, p->color)) return 0;
	if (p->cities_left <= 0) return 0;
	if (!can_afford_city(p)) return 0;
	pay_city(p);
	g->villages[v] = p->color + 4;   /* 5..8 = city */
	p->cities_left--;
	p->villages_left++;              /* village piece returns to supply */
	p->points++;                     /* city = 2 points (already had 1) */
	return 1;
}

/* ============================================================
 * AI turn
 * ============================================================ */

/* Try to build the most valuable thing possible. Returns 1 if anything
 * was built. */
static int ai_try_build(Game *g, Player *p) {
	/* Upgrade to city. */
	if (can_afford_city(p) && p->cities_left > 0) {
		for (int v = 1; v < NUM_VILLAGES; v++) {
			if (can_upgrade_to_city(g, v, p->color)) {
				do_build_city(g, p, v);
				printf("  %s upgrades vertex %d to a city.\n", p->name, v);
				return 1;
			}
		}
	}
	/* New village. */
	if (can_afford_village(p) && p->villages_left > 0) {
		int best = 0, best_score = -1;
		for (int v = 1; v < NUM_VILLAGES; v++) {
			if (!can_place_village(g, v, p->color)) continue;
			int s = village_setup_score(g, v);
			if (s > best_score) { best_score = s; best = v; }
		}
		if (best > 0) {
			do_build_village(g, p, best);
			printf("  %s builds a village at %d.\n", p->name, best);
			return 1;
		}
	}
	/* Road. */
	if (can_afford_road(p) && p->roads_left > 0) {
		for (int s = 1; s < NUM_STREETS; s++) {
			if (can_place_road(g, s, p->color)) {
				do_build_road(g, p, s);
				printf("  %s builds a road at %d.\n", p->name, s);
				return 1;
			}
		}
	}
	return 0;
}

/* AI bank trade: only trade when (a) we currently can't afford anything,
 * and (b) the trade directly unlocks a build action. */
static int ai_try_trade(Game *g, Player *p) {
	(void)g;
	if (can_afford_road(p) || can_afford_village(p) || can_afford_city(p)) return 0;
	for (int give = 0; give < NUM_RESOURCES; give++) {
		if (p->resources[give] < 4) continue;
		for (int get = 0; get < NUM_RESOURCES; get++) {
			if (get == give) continue;
			p->resources[give] -= 4;
			p->resources[get]  += 1;
			if (can_afford_road(p) || can_afford_village(p) || can_afford_city(p)) {
				printf("  %s trades 4 %s -> 1 %s with the bank.\n",
					p->name, resource_name(give), resource_name(get));
				return 1;
			}
			p->resources[give] += 4;
			p->resources[get]  -= 1;
		}
	}
	return 0;
}

static void ai_take_turn(Game *g, Player *p) {
	while (1) {
		if (ai_try_build(g, p)) continue;
		if (ai_try_trade(g, p)) continue;
		break;
	}
}

/* ============================================================
 * Human action loop
 * ============================================================ */
static void human_take_turn(Game *g, Player *p) {
	while (1) {
		print_status(g);
		int choice = main_menu();
		printf("\n");
		if (choice == 3) return;     /* end turn */
		if (choice == 1) {           /* build */
			int what = build_menu();
			if (what == 4) continue;
			if (what == 1) {
				if (!can_afford_road(p))     { printf("  not enough resources.\n"); continue; }
				if (p->roads_left <= 0)      { printf("  out of road pieces.\n");   continue; }
				int s = read_int("  Road at edge (1-72): ", 1, 72);
				if (!do_build_road(g, p, s))  printf("  invalid placement.\n");
			} else if (what == 2) {
				if (!can_afford_village(p))   { printf("  not enough resources.\n"); continue; }
				if (p->villages_left <= 0)    { printf("  out of village pieces.\n"); continue; }
				int v = read_int("  Village at vertex (1-54): ", 1, 54);
				if (!do_build_village(g, p, v)) printf("  invalid placement.\n");
			} else if (what == 3) {
				if (!can_afford_city(p))      { printf("  not enough resources.\n"); continue; }
				if (p->cities_left <= 0)      { printf("  out of city pieces.\n"); continue; }
				int v = read_int("  Upgrade vertex (1-54): ", 1, 54);
				if (!do_build_city(g, p, v))  printf("  invalid (must own a village there).\n");
			}
			continue;
		}
		if (choice == 2) {           /* bank trade */
			int give = trade_menu();
			if (give == 6) continue;
			give -= 1;
			if (p->resources[give] < 4) { printf("  need 4 of that resource.\n"); continue; }
			int get = read_int("  Resource to GET [1)Wool 2)Brick 3)Ore 4)Wheat 5)Lumber]: ", 1, 5) - 1;
			if (get == give) { printf("  must be different.\n"); continue; }
			p->resources[give] -= 4;
			p->resources[get]  += 1;
			continue;
		}
	}
}

/* ============================================================
 * Top-level turn
 * ============================================================ */
void run_player_turn(Game *g, int player_idx) {
	Player *p = &g->players[player_idx];
	print_map(g->blocks, g->villages, g->streets, g->indexs);
	printf("\n=== %s's turn ===\n", p->name);

	if (p->is_human) {
		printf("Press any key to roll the dice...");
		fflush(stdout);
		getch();
		printf("\n");
	}
	int roll = roll_dice();
	g->last_roll = roll;
	printf("[Dice] %s rolled %d.\n", p->name, roll);
	if (roll == 7) {
		apply_thief(g, player_idx);
	} else {
		distribute_resources(g, roll);
	}

	if (p->is_human) human_take_turn(g, p);
	else             ai_take_turn(g, p);
}

/* ============================================================
 * Status display
 * ============================================================ */
void print_status(const Game *g) {
	printf("\n--- status ---\n");
	for (int i = 0; i < NUM_PLAYERS; i++) {
		const Player *p = &g->players[i];
		const char *col;
		switch (p->color) {
			case 1: col = "\033[31m"; break;
			case 2: col = "\033[36m"; break;
			case 3: col = "\033[33m"; break;
			case 4: col = "\033[32m"; break;
			default: col = "";
		}
		int total = total_cards(p);
		printf("  %s%s\033[m  pts=%d  cards=%d", col, p->name, p->points, total);
		if (p->is_human) {
			printf("  [");
			for (int r = 0; r < NUM_RESOURCES; r++) {
				printf("%s%s=%d", (r == 0 ? "" : " "), resource_name(r), p->resources[r]);
			}
			printf("]");
		}
		printf("\n");
	}
	printf("  thief on block %d.  last roll: %d.\n", g->thief_block, g->last_roll);
}

int game_winner(const Game *g) {
	for (int i = 0; i < NUM_PLAYERS; i++)
		if (g->players[i].points >= 10) return i;
	return -1;
}
