#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "catan_game.h"
#include "catan_menu.h"
#include "catan_map.h"

/* Forward declarations needed across sections. */
static void log_add(const char *fmt, ...);
static void redraw(const Game *g);
static int  village_setup_score(const Game *g, int v);
static int  can_place_village(const Game *g, int v, int color);
static int  can_place_village_setup(const Game *g, int v);
static int  can_upgrade_to_city(const Game *g, int v, int color);
static int  can_place_road(const Game *g, int s, int color);
static void print_buildable_villages(const Game *g, const Player *p);
static void print_buildable_roads(const Game *g, const Player *p);
static void print_upgradable_villages(const Game *g, const Player *p);
static void print_setup_village_candidates(const Game *g);

/* ============================================================
 * Static topology data
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

static const int BLOCK_VILLAGES[NUM_BLOCKS][6] = {
	{0,0,0,0,0,0},
	{ 1, 2, 4, 5, 9,10}, { 3, 4, 8, 9,14,15}, { 5, 6,10,11,16,17},
	{ 7, 8,13,14,19,20}, { 9,10,15,16,21,22}, {11,12,17,18,23,24},
	{14,15,20,21,26,27}, {16,17,22,23,28,29}, {19,20,25,26,31,32},
	{21,22,27,28,33,34}, {23,24,29,30,35,36}, {26,27,32,33,38,39},
	{28,29,34,35,40,41}, {31,32,37,38,43,44}, {33,34,39,40,45,46},
	{35,36,41,42,47,48}, {38,39,44,45,49,50}, {40,41,46,47,51,52},
	{45,46,50,51,53,54}
};

/* Harbour data — type 0 = generic 3:1, otherwise specific 2:1 for that type. */
static const Harbour HARBOUR_INIT[NUM_HARBOURS] = {
	{T_WOOL,   { 3,  4}},   /* H1 */
	{T_BRICK,  { 5,  6}},   /* H2 */
	{T_ORE,    { 7, 13}},   /* H3 */
	{0,        {12, 18}},   /* H4 generic */
	{0,        {25, 31}},   /* H5 generic */
	{T_LUMBER, {30, 36}},   /* H6 */
	{T_WHEAT,  {44, 49}},   /* H7 */
	{0,        {47, 52}},   /* H8 generic */
	{0,        {53, 54}}    /* H9 generic */
};

/* Standard Catan dev deck. */
static const int INITIAL_DECK[NUM_DEV_TYPES] = {
	14, /* knight */
	 5, /* victory point */
	 2, /* year of plenty */
	 2, /* monopoly */
	 2  /* road building */
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
		int v1 = t->street_v1[s], v2 = t->street_v2[s];
		if (v1 == 0 || v2 == 0) continue;
		t->village_streets[v1][t->village_streets_count[v1]++] = s;
		t->village_streets[v2][t->village_streets_count[v2]++] = s;
	}
	for (int b = 1; b < NUM_BLOCKS; b++)
		for (int k = 0; k < 6; k++) {
			int v = BLOCK_VILLAGES[b][k];
			t->village_blocks[v][t->village_blocks_count[v]++] = b;
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
const char *dev_name(int d) {
	switch (d) {
		case DEV_KNIGHT: return "Knight";
		case DEV_VP:     return "Victory Point";
		case DEV_YOP:    return "Year of Plenty";
		case DEV_MONO:   return "Monopoly";
		case DEV_ROAD:   return "Road Building";
		default:         return "?";
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
 * Log buffer + redraw helper
 *
 * The terminal embed (xterm.js) and a real terminal both lose the map as
 * the game scrolls. Instead, every prompt we'd otherwise print directly
 * goes through redraw(), which clears the screen and re-renders the map,
 * the status panel, and the most recent N events from a small ring buffer.
 * Game-flow messages call log_add(); transient feedback (validation
 * errors, sub-menus) still uses printf and washes away on next redraw.
 * ============================================================ */
#define LOG_LINES      8
#define LOG_LINE_WIDTH 100

static char g_log[LOG_LINES][LOG_LINE_WIDTH];
static int  g_log_pos = 0;

static void log_add(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(g_log[g_log_pos], LOG_LINE_WIDTH, fmt, ap);
	va_end(ap);
	g_log_pos = (g_log_pos + 1) % LOG_LINES;
}

static void redraw(const Game *g) {
	/* Cursor home + clear screen below. xterm.js handles this; on a real
	 * terminal it works as long as ANSI VT processing is on. */
	printf("\033[H\033[2J");
	print_map(g->blocks, g->villages, g->streets, g->indexs);
	print_status(g);
	printf("\n--- recent events ---\n");
	int empty = 1;
	for (int i = 0; i < LOG_LINES; i++) {
		int idx = (g_log_pos + i) % LOG_LINES;
		if (g_log[idx][0]) {
			printf("  %s\n", g_log[idx]);
			empty = 0;
		}
	}
	if (empty) printf("  (no events yet)\n");
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

	g->thief_block = 10;
	for (int b = 1; b < NUM_BLOCKS; b++)
		if (blocks[b] == T_DESERT) { g->thief_block = b; break; }

	memcpy(g->harbours, HARBOUR_INIT, sizeof(HARBOUR_INIT));

	memcpy(g->dev_deck, INITIAL_DECK, sizeof(INITIAL_DECK));
	g->deck_remaining = 0;
	for (int t = 0; t < NUM_DEV_TYPES; t++) g->deck_remaining += g->dev_deck[t];

	g->largest_army_holder  = -1;
	g->largest_army_count   = 2;   /* must reach 3 to claim */
	g->longest_road_holder  = -1;
	g->longest_road_length  = 4;   /* must reach 5 to claim */

	for (int i = 0; i < NUM_PLAYERS; i++) {
		Player *p = &g->players[i];
		memset(p, 0, sizeof(*p));
		p->villages_left = 5;
		p->cities_left   = 4;
		p->roads_left    = 15;
		p->color         = i + 1;
		p->is_human      = (i == 0);
		p->name          = PLAYER_NAMES[i];
	}

	build_topology(&g->topo);
}

/* ============================================================
 * Owner / adjacency helpers
 * ============================================================ */
static int village_owner_color(const Game *g, int v) {
	int x = g->villages[v];
	if (x == 0) return 0;
	if (x <= 4) return x;
	return x - 4;
}
static int village_is_city(const Game *g, int v) { return g->villages[v] >= 5; }

static int village_has_neighboring_settlement(const Game *g, int v) {
	for (int k = 0; k < g->topo.village_streets_count[v]; k++) {
		int s = g->topo.village_streets[v][k];
		int other = (g->topo.street_v1[s] == v) ? g->topo.street_v2[s] : g->topo.street_v1[s];
		if (g->villages[other] != 0) return 1;
	}
	return 0;
}

static int can_place_village(const Game *g, int v, int color) {
	if (v < 1 || v >= NUM_VILLAGES) return 0;
	if (g->villages[v] != 0) return 0;
	if (village_has_neighboring_settlement(g, v)) return 0;
	for (int k = 0; k < g->topo.village_streets_count[v]; k++)
		if (g->streets[g->topo.village_streets[v][k]] == color) return 1;
	return 0;
}
static int can_place_village_setup(const Game *g, int v) {
	if (v < 1 || v >= NUM_VILLAGES) return 0;
	if (g->villages[v] != 0) return 0;
	return !village_has_neighboring_settlement(g, v);
}
static int can_upgrade_to_city(const Game *g, int v, int color) {
	if (v < 1 || v >= NUM_VILLAGES) return 0;
	if (village_is_city(g, v)) return 0;
	return village_owner_color(g, v) == color;
}
static int can_place_road(const Game *g, int s, int color) {
	if (s < 1 || s >= NUM_STREETS) return 0;
	if (g->streets[s] != 0) return 0;
	int ends[2] = { g->topo.street_v1[s], g->topo.street_v2[s] };
	for (int e = 0; e < 2; e++) {
		int v = ends[e];
		int owner = village_owner_color(g, v);
		if (owner == color) return 1;
		if (owner == 0) {
			for (int k = 0; k < g->topo.village_streets_count[v]; k++) {
				int s2 = g->topo.village_streets[v][k];
				if (s2 != s && g->streets[s2] == color) return 1;
			}
		}
	}
	return 0;
}
static int can_place_road_setup(const Game *g, int s, int v_just_placed) {
	if (s < 1 || s >= NUM_STREETS) return 0;
	if (g->streets[s] != 0) return 0;
	int v1 = g->topo.street_v1[s], v2 = g->topo.street_v2[s];
	return (v1 == v_just_placed || v2 == v_just_placed);
}

/* ============================================================
 * Build cost
 * ============================================================ */
static int can_afford_road(const Player *p)    { return p->resources[R_LUMBER] >= 1 && p->resources[R_BRICK] >= 1; }
static int can_afford_village(const Player *p) { return p->resources[R_LUMBER] >= 1 && p->resources[R_BRICK] >= 1
                                                       && p->resources[R_WHEAT]  >= 1 && p->resources[R_WOOL]  >= 1; }
static int can_afford_city(const Player *p)    { return p->resources[R_WHEAT] >= 2 && p->resources[R_ORE] >= 3; }
static int can_afford_dev_card(const Player *p){ return p->resources[R_WHEAT] >= 1 && p->resources[R_ORE] >= 1 && p->resources[R_WOOL] >= 1; }

static void pay_road(Player *p)     { p->resources[R_LUMBER]--; p->resources[R_BRICK]--; }
static void pay_village(Player *p)  { p->resources[R_LUMBER]--; p->resources[R_BRICK]--; p->resources[R_WHEAT]--; p->resources[R_WOOL]--; }
static void pay_city(Player *p)     { p->resources[R_WHEAT] -= 2; p->resources[R_ORE] -= 3; }
static void pay_dev_card(Player *p) { p->resources[R_WHEAT]--; p->resources[R_ORE]--; p->resources[R_WOOL]--; }

/* ============================================================
 * Trade ratio (best harbour the player owns for resource r)
 * ============================================================ */
static int player_owns_village(const Game *g, int color, int v) {
	return village_owner_color(g, v) == color;
}
static int trade_ratio(const Game *g, const Player *p, int r) {
	int best = 4;
	for (int h = 0; h < NUM_HARBOURS; h++) {
		const Harbour *H = &g->harbours[h];
		int owns = player_owns_village(g, p->color, H->villages[0]) ||
		           player_owns_village(g, p->color, H->villages[1]);
		if (!owns) continue;
		if (H->type == 0) { if (best > 3) best = 3; }
		else if (type_to_resource(H->type) == r) { if (best > 2) best = 2; }
	}
	return best;
}

/* ============================================================
 * Dice & resource distribution
 * ============================================================ */
int roll_dice(void) { return ((rand() % 6) + 1) + ((rand() % 6) + 1); }

void distribute_resources(Game *g, int roll) {
	for (int b = 1; b < NUM_BLOCKS; b++) {
		if (g->indexs[b] != roll || b == g->thief_block) continue;
		int res = type_to_resource(g->blocks[b]);
		if (res < 0) continue;
		for (int k = 0; k < 6; k++) {
			int v = BLOCK_VILLAGES[b][k];
			int color = village_owner_color(g, v);
			if (color == 0) continue;
			g->players[color - 1].resources[res] += village_is_city(g, v) ? 2 : 1;
		}
	}
}

static int total_cards(const Player *p) {
	int s = 0;
	for (int r = 0; r < NUM_RESOURCES; r++) s += p->resources[r];
	return s;
}

/* ============================================================
 * Thief & stealing
 * ============================================================ */
/* Pick a random resource from victim's hand and transfer to thief_player.
 * Does nothing if victim has no cards. */
static void steal_random(Player *thief, Player *victim) {
	int total = total_cards(victim);
	if (total <= 0) {
		log_add("%s steals from %s — nothing to take", thief->name, victim->name);
		return;
	}
	int pick = rand() % total;
	for (int r = 0; r < NUM_RESOURCES; r++) {
		if (pick < victim->resources[r]) {
			victim->resources[r]--;
			thief->resources[r]++;
			if (thief->is_human || victim->is_human)
				log_add("%s steals 1 %s from %s", thief->name, resource_name(r), victim->name);
			else
				log_add("%s steals 1 card from %s", thief->name, victim->name);
			return;
		}
		pick -= victim->resources[r];
	}
}

/* Players with settlements adjacent to a block (excluding the thief player). */
static int victims_at_block(const Game *g, int block, int self_color, int out[NUM_PLAYERS]) {
	int seen[NUM_PLAYERS+1] = {0};
	int n = 0;
	for (int k = 0; k < 6; k++) {
		int v = BLOCK_VILLAGES[block][k];
		int c = village_owner_color(g, v);
		if (c == 0 || c == self_color) continue;
		if (seen[c]) continue;
		seen[c] = 1;
		out[n++] = c - 1;
	}
	return n;
}

static int human_pick_thief_block(int current) {
	while (1) {
		int t = read_int("  Move thief to block (1-19, different from current): ", 1, 19);
		if (t != current) return t;
		printf("    must be different from current.\n");
	}
}

static int ai_pick_thief_block(const Game *g, int self_color) {
	int best = -1, best_score = -1;
	for (int b = 1; b < NUM_BLOCKS; b++) {
		if (b == g->thief_block) continue;
		int score = 0;
		for (int k = 0; k < 6; k++) {
			int v = BLOCK_VILLAGES[b][k];
			int c = village_owner_color(g, v);
			if (c != 0 && c != self_color) score += village_is_city(g, v) ? 2 : 1;
		}
		if (score > best_score) { best_score = score; best = b; }
	}
	return best > 0 ? best : (g->thief_block % 19) + 1;
}

/* Move the thief and steal from one neighbouring opponent. */
static void move_thief_and_steal(Game *g, int actor_idx) {
	Player *actor = &g->players[actor_idx];
	int target;
	if (actor->is_human) {
		redraw(g);
		printf("\n  thief currently on block %d. blocks 1..19 listed across the map.\n", g->thief_block);
		target = human_pick_thief_block(g->thief_block);
	} else {
		target = ai_pick_thief_block(g, actor->color);
	}
	g->thief_block = target;
	log_add("%s moves the thief to block %d", actor->name, target);

	int victim_idxs[NUM_PLAYERS];
	int nv = victims_at_block(g, target, actor->color, victim_idxs);
	if (nv == 0) {
		log_add("  no opponents on block %d — no steal", target);
		return;
	}
	int victim;
	if (actor->is_human) {
		printf("\n  victims on block %d: ", target);
		for (int i = 0; i < nv; i++) printf("[%d]%s  ", i + 1, g->players[victim_idxs[i]].name);
		printf("\n");
		int sel = read_int("  steal from which? ", 1, nv);
		victim = victim_idxs[sel - 1];
	} else {
		victim = victim_idxs[rand() % nv];
	}
	steal_random(actor, &g->players[victim]);
}

/* Discard half if >7 cards, then move thief. */
static void apply_thief_on_seven(Game *g, int roller_idx) {
	for (int i = 0; i < NUM_PLAYERS; i++) {
		Player *p = &g->players[i];
		int total = total_cards(p);
		if (total <= 7) continue;
		int discard = total / 2;
		if (p->is_human) {
			redraw(g);
			printf("\n[thief] you have %d cards — must discard %d.\n", total, discard);
			while (discard > 0) {
				printf("  resources: ");
				for (int r = 0; r < NUM_RESOURCES; r++) printf("%s=%d  ", resource_name(r), p->resources[r]);
				printf("  (%d more to discard)\n", discard);
				int sel = read_int("  discard which? 1)wool 2)brick 3)ore 4)wheat 5)lumber: ", 1, 5) - 1;
				if (p->resources[sel] <= 0) { printf("    none of those\n"); continue; }
				p->resources[sel]--;
				discard--;
			}
			log_add("you discarded %d cards", total / 2);
		} else {
			log_add("%s discards %d cards", p->name, discard);
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
	move_thief_and_steal(g, roller_idx);
}

/* ============================================================
 * Setup phase
 * ============================================================ */
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

/* ---- buildable-location helpers (printed inline, not logged) ---- */
static void list_hex_neighbours(const Game *g, int v) {
	int n = g->topo.village_blocks_count[v];
	for (int k = 0; k < n; k++) {
		int b = g->topo.village_blocks[v][k];
		if (g->blocks[b] == T_DESERT) printf("desert");
		else                          printf("%s[%d]", type_name(g->blocks[b]), g->indexs[b]);
		if (k < n - 1) printf(", ");
	}
}

static void print_buildable_villages(const Game *g, const Player *p) {
	printf("\n  valid vertices for new village:\n   ");
	int count = 0, on_line = 0;
	for (int v = 1; v < NUM_VILLAGES; v++) {
		if (!can_place_village(g, v, p->color)) continue;
		if (on_line >= 6) { printf("\n   "); on_line = 0; }
		printf(" v%-2d", v);
		on_line++; count++;
	}
	if (count == 0) printf(" (none — extend a road first)");
	printf("\n");
}

static void print_buildable_roads(const Game *g, const Player *p) {
	printf("\n  valid edges for new road:\n");
	int count = 0;
	for (int s = 1; s < NUM_STREETS; s++) {
		if (!can_place_road(g, s, p->color)) continue;
		int v1 = g->topo.street_v1[s], v2 = g->topo.street_v2[s];
		if (count % 3 == 0) printf("   ");
		printf(" e%-2d (v%d-v%d)", s, v1, v2);
		if (count % 3 == 2) printf("\n");
		count++;
	}
	if (count == 0) printf("    (none)\n");
	else if (count % 3 != 0) printf("\n");
}

static void print_upgradable_villages(const Game *g, const Player *p) {
	printf("\n  your villages (pick one to upgrade to city):\n");
	int count = 0;
	for (int v = 1; v < NUM_VILLAGES; v++) {
		if (!can_upgrade_to_city(g, v, p->color)) continue;
		printf("    v%-2d  ", v);
		list_hex_neighbours(g, v);
		printf("\n");
		count++;
	}
	if (count == 0) printf("    (none)\n");
}

static void print_setup_village_candidates(const Game *g) {
	typedef struct { int v; int score; } Cand;
	Cand cs[NUM_VILLAGES];
	int n = 0;
	for (int v = 1; v < NUM_VILLAGES; v++) {
		if (!can_place_village_setup(g, v)) continue;
		cs[n].v = v;
		cs[n].score = village_setup_score(g, v);
		n++;
	}
	for (int i = 1; i < n; i++) {
		Cand x = cs[i]; int j = i;
		while (j > 0 && cs[j-1].score < x.score) { cs[j] = cs[j-1]; j--; }
		cs[j] = x;
	}
	int show = n > 16 ? 16 : n;
	printf("\n  best vertices by pip count:\n");
	for (int i = 0; i < show; i++) {
		printf("    v%-2d  pips=%2d  ", cs[i].v, cs[i].score);
		list_hex_neighbours(g, cs[i].v);
		printf("\n");
	}
	if (n > show) printf("    ... (%d more valid)\n", n - show);
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
	print_setup_village_candidates(g);
	while (1) {
		int v = read_int("  place village at vertex (1-54): ", 1, 54);
		if (can_place_village_setup(g, v)) return v;
		printf("    invalid (occupied or adjacent to another settlement)\n");
	}
}
static int human_pick_setup_road(const Game *g, int v) {
	printf("\n  adjacent edges to v%d:", v);
	for (int k = 0; k < g->topo.village_streets_count[v]; k++) {
		int s = g->topo.village_streets[v][k];
		int o = (g->topo.street_v1[s] == v) ? g->topo.street_v2[s] : g->topo.street_v1[s];
		printf("  e%d (-> v%d)", s, o);
	}
	printf("\n");
	while (1) {
		int s = read_int("  place road at edge (1-72): ", 1, 72);
		if (can_place_road_setup(g, s, v)) return s;
		printf("    invalid (must touch your new village)\n");
	}
}
static void give_initial_resources(Game *g, int color, int v) {
	Player *p = &g->players[color - 1];
	for (int k = 0; k < g->topo.village_blocks_count[v]; k++) {
		int res = type_to_resource(g->blocks[g->topo.village_blocks[v][k]]);
		if (res >= 0) p->resources[res]++;
	}
}

/* Forward declarations for recompute helpers used after each build. */
static void update_longest_road(Game *g);
static void update_largest_army(Game *g);

void run_setup_phase(Game *g) {
	int order_fwd[NUM_PLAYERS] = {0,1,2,3};
	int order_rev[NUM_PLAYERS] = {3,2,1,0};
	for (int round = 0; round < 2; round++) {
		const int *order = (round == 0) ? order_fwd : order_rev;
		for (int idx = 0; idx < NUM_PLAYERS; idx++) {
			int pi    = order[idx];
			Player *p = &g->players[pi];
			redraw(g);
			printf("\n[setup round %d] %s — place a village and a road.\n", round + 1, p->name);
			int v, s;
			if (p->is_human) {
				v = human_pick_setup_village(g);
				g->villages[v] = p->color;
				p->villages_left--;
				redraw(g);
				printf("\n[setup round %d] %s — now place a road touching v%d.\n", round + 1, p->name, v);
				s = human_pick_setup_road(g, v);
			} else {
				v = ai_pick_setup_village(g);
				g->villages[v] = p->color;
				p->villages_left--;
				s = ai_pick_setup_road(g, v);
			}
			g->streets[s] = p->color;
			p->roads_left--;
			p->points++;
			if (round == 1) give_initial_resources(g, p->color, v);
			log_add("[setup R%d] %s places village v%d, road e%d", round + 1, p->name, v, s);
		}
	}
	update_longest_road(g);
}

/* ============================================================
 * Build actions
 * ============================================================ */
static int do_build_road(Game *g, Player *p, int s) {
	if (!can_place_road(g, s, p->color) || p->roads_left <= 0 || !can_afford_road(p)) return 0;
	pay_road(p);
	g->streets[s] = p->color;
	p->roads_left--;
	update_longest_road(g);
	return 1;
}
static int do_build_village(Game *g, Player *p, int v) {
	if (!can_place_village(g, v, p->color) || p->villages_left <= 0 || !can_afford_village(p)) return 0;
	pay_village(p);
	g->villages[v] = p->color;
	p->villages_left--;
	p->points++;
	update_longest_road(g);   /* might break opponents' chains */
	return 1;
}
static int do_build_city(Game *g, Player *p, int v) {
	if (!can_upgrade_to_city(g, v, p->color) || p->cities_left <= 0 || !can_afford_city(p)) return 0;
	pay_city(p);
	g->villages[v] = p->color + 4;
	p->cities_left--;
	p->villages_left++;
	p->points++;
	return 1;
}

/* Free road (used by road-building dev card and setup). */
static int do_build_free_road(Game *g, Player *p, int s) {
	if (!can_place_road(g, s, p->color) || p->roads_left <= 0) return 0;
	g->streets[s] = p->color;
	p->roads_left--;
	update_longest_road(g);
	return 1;
}

/* ============================================================
 * Longest road / largest army announcements
 * (the actual builds happen in do_build_*; longest road is recomputed
 *  by update_longest_road. Bonus award/transfer messages go to the log.)
 * ============================================================ */
static int dfs_longest_path(const Game *g, int color, int v, char *road_used) {
	int owner = village_owner_color(g, v);
	if (owner != 0 && owner != color) return 0;   /* blocked */
	int best = 0;
	for (int k = 0; k < g->topo.village_streets_count[v]; k++) {
		int s = g->topo.village_streets[v][k];
		if (road_used[s]) continue;
		if (g->streets[s] != color) continue;
		int other = (g->topo.street_v1[s] == v) ? g->topo.street_v2[s] : g->topo.street_v1[s];
		road_used[s] = 1;
		int len = 1 + dfs_longest_path(g, color, other, road_used);
		road_used[s] = 0;
		if (len > best) best = len;
	}
	return best;
}
static int compute_longest_road(const Game *g, int color) {
	char road_used[NUM_STREETS] = {0};
	char tried[NUM_VILLAGES] = {0};
	int best = 0;
	for (int s = 1; s < NUM_STREETS; s++) {
		if (g->streets[s] != color) continue;
		int v1 = g->topo.street_v1[s], v2 = g->topo.street_v2[s];
		if (!tried[v1]) { tried[v1] = 1; int len = dfs_longest_path(g, color, v1, road_used); if (len > best) best = len; }
		if (!tried[v2]) { tried[v2] = 1; int len = dfs_longest_path(g, color, v2, road_used); if (len > best) best = len; }
	}
	return best;
}
static void update_longest_road(Game *g) {
	int max_len = 0, max_holder = -1;
	for (int i = 0; i < NUM_PLAYERS; i++) {
		int len = compute_longest_road(g, g->players[i].color);
		g->players[i].longest_road_length = len;
		if (len > max_len) { max_len = len; max_holder = i; }
	}
	int curr = g->longest_road_holder;
	int curr_len = g->longest_road_length;
	if (max_len < 5) {
		if (curr >= 0 && g->players[curr].longest_road_length < 5) {
			log_add("%s loses Longest Road", g->players[curr].name);
			g->players[curr].has_longest_road = 0;
			g->players[curr].points -= 2;
			g->longest_road_holder = -1;
			g->longest_road_length = 4;
		}
		return;
	}
	if (curr < 0) {
		Player *p = &g->players[max_holder];
		p->has_longest_road = 1;
		p->points += 2;
		g->longest_road_holder = max_holder;
		g->longest_road_length = max_len;
		log_add("%s gains Longest Road (+2)", p->name);
	} else {
		if (g->players[curr].longest_road_length >= max_len) {
			g->longest_road_length = g->players[curr].longest_road_length;
			return;
		}
		if (max_len > g->players[curr].longest_road_length) {
			log_add("Longest Road: %s -> %s", g->players[curr].name, g->players[max_holder].name);
			g->players[curr].has_longest_road = 0;
			g->players[curr].points -= 2;
			g->players[max_holder].has_longest_road = 1;
			g->players[max_holder].points += 2;
			g->longest_road_holder = max_holder;
			g->longest_road_length = max_len;
		}
		(void)curr_len;
	}
}

/* ============================================================
 * Largest army
 * ============================================================ */
static void update_largest_army(Game *g) {
	int max = 0, holder = -1;
	for (int i = 0; i < NUM_PLAYERS; i++)
		if (g->players[i].knights_played > max) { max = g->players[i].knights_played; holder = i; }
	if (max < 3) return;
	int curr = g->largest_army_holder;
	if (curr < 0) {
		g->players[holder].has_largest_army = 1;
		g->players[holder].points += 2;
		g->largest_army_holder = holder;
		g->largest_army_count  = max;
		log_add("%s gains Largest Army (+2)", g->players[holder].name);
	} else if (max > g->players[curr].knights_played) {
		log_add("Largest Army: %s -> %s", g->players[curr].name, g->players[holder].name);
		g->players[curr].has_largest_army = 0;
		g->players[curr].points -= 2;
		g->players[holder].has_largest_army = 1;
		g->players[holder].points += 2;
		g->largest_army_holder = holder;
		g->largest_army_count  = max;
	}
}

/* ============================================================
 * Development cards
 * ============================================================ */
static int draw_random_dev(Game *g) {
	if (g->deck_remaining <= 0) return -1;
	int pick = rand() % g->deck_remaining;
	for (int t = 0; t < NUM_DEV_TYPES; t++) {
		if (pick < g->dev_deck[t]) {
			g->dev_deck[t]--;
			g->deck_remaining--;
			return t;
		}
		pick -= g->dev_deck[t];
	}
	return -1;
}

static int buy_dev_card(Game *g, Player *p) {
	if (g->deck_remaining <= 0) { printf("  deck is empty.\n"); return 0; }
	if (!can_afford_dev_card(p)) { printf("  not enough resources.\n"); return 0; }
	pay_dev_card(p);
	int t = draw_random_dev(g);
	if (t < 0) return 0;
	if (t == DEV_VP) {
		p->dev_hand[DEV_VP]++;
		p->points++;
		log_add("%s drew a Victory Point card", p->name);
	} else {
		p->dev_locked[t]++;
		if (p->is_human) log_add("you drew a %s card", dev_name(t));
		else             log_add("%s drew a development card", p->name);
	}
	return 1;
}

static void play_knight(Game *g, int actor_idx) {
	Player *p = &g->players[actor_idx];
	log_add("%s plays Knight!", p->name);
	p->knights_played++;
	move_thief_and_steal(g, actor_idx);
	update_largest_army(g);
}

static void play_year_of_plenty(Game *g, Player *p) {
	(void)g;
	int r1, r2;
	if (p->is_human) {
		r1 = read_int("  YoP - first resource [1=Wool 2=Brick 3=Ore 4=Wheat 5=Lumber]: ", 1, 5) - 1;
		r2 = read_int("  YoP - second resource [1=Wool 2=Brick 3=Ore 4=Wheat 5=Lumber]: ", 1, 5) - 1;
	} else {
		/* AI picks resources that bring it closest to a build. */
		int needed[NUM_RESOURCES] = {0};
		needed[R_LUMBER] = needed[R_BRICK] = 1;
		if (!can_afford_city(p)) { needed[R_WHEAT] = 1; needed[R_ORE] = 1; }
		needed[R_WHEAT] = 1; needed[R_WOOL] = 1;
		r1 = R_WHEAT; r2 = R_ORE;
		for (int r = 0; r < NUM_RESOURCES; r++) if (needed[r] && p->resources[r] == 0) { r1 = r; break; }
		for (int r = 0; r < NUM_RESOURCES; r++) if (needed[r] && r != r1 && p->resources[r] <= 1) { r2 = r; break; }
	}
	p->resources[r1]++;
	p->resources[r2]++;
	log_add("%s plays Year of Plenty (+1 %s, +1 %s)", p->name, resource_name(r1), resource_name(r2));
}

static void play_monopoly(Game *g, int actor_idx) {
	Player *p = &g->players[actor_idx];
	int r;
	if (p->is_human) {
		r = read_int("  Monopoly - which resource? [1=Wool 2=Brick 3=Ore 4=Wheat 5=Lumber]: ", 1, 5) - 1;
	} else {
		/* AI picks the resource with highest opponent total. */
		int totals[NUM_RESOURCES] = {0};
		for (int i = 0; i < NUM_PLAYERS; i++)
			if (i != actor_idx)
				for (int rr = 0; rr < NUM_RESOURCES; rr++) totals[rr] += g->players[i].resources[rr];
		r = 0;
		for (int rr = 1; rr < NUM_RESOURCES; rr++) if (totals[rr] > totals[r]) r = rr;
	}
	int taken = 0;
	for (int i = 0; i < NUM_PLAYERS; i++) {
		if (i == actor_idx) continue;
		taken += g->players[i].resources[r];
		g->players[i].resources[r] = 0;
	}
	p->resources[r] += taken;
	log_add("%s plays Monopoly on %s (takes %d)", p->name, resource_name(r), taken);
}

static void play_road_building(Game *g, int actor_idx) {
	Player *p = &g->players[actor_idx];
	log_add("%s plays Road Building (2 free roads)", p->name);
	for (int n = 0; n < 2; n++) {
		if (p->roads_left <= 0) break;
		if (p->is_human) {
			redraw(g);
			printf("\n  Free road #%d (of 2):\n", n + 1);
			print_buildable_roads(g, p);
			while (1) {
				int s = read_int("    edge (1-72, 0 to skip): ", 0, 72);
				if (s == 0) break;
				if (do_build_free_road(g, p, s)) {
					log_add("you build a free road at e%d", s);
					break;
				}
				printf("    invalid placement\n");
			}
		} else {
			int placed = 0;
			for (int s = 1; s < NUM_STREETS; s++) {
				if (do_build_free_road(g, p, s)) {
					log_add("%s places free road at e%d", p->name, s);
					placed = 1;
					break;
				}
			}
			if (!placed) break;
		}
	}
}

/* ============================================================
 * Player-to-player trade
 * ============================================================ */
static int ai_evaluates_offer(const Player *self, int give_r, int give_n, int get_r, int get_n) {
	/* AI accepts if: it has enough to give, the deal is fair-or-better, and
	 * the ask isn't huge. */
	if (self->resources[give_r] < give_n) return 0;
	if (give_n > 4) return 0;
	if (get_n > give_n) return 0;
	/* Preference: AI accepts when it has surplus of give_r (>=2 will remain) or
	 * needs get_r for an obvious build. */
	int remaining = self->resources[give_r] - give_n;
	if (remaining >= 2) return 1;
	if (get_r == R_WHEAT || get_r == R_ORE) return 1;
	return 0;
}

static void human_trade_player(Game *g, Player *p) {
	printf("  Trade with [1=COM1 2=COM2 3=COM3 4=Cancel]: ");
	int ch = read_int("", 1, 4);
	if (ch == 4) return;
	int target_idx = ch;
	Player *t = &g->players[target_idx];
	int give_r = read_int("  Resource to GIVE [1=Wool 2=Brick 3=Ore 4=Wheat 5=Lumber]: ", 1, 5) - 1;
	int give_n = read_int("  How many to give: ", 1, 9);
	if (p->resources[give_r] < give_n) { printf("  not enough.\n"); return; }
	int get_r  = read_int("  Resource to GET  [1=Wool 2=Brick 3=Ore 4=Wheat 5=Lumber]: ", 1, 5) - 1;
	int get_n  = read_int("  How many to get: ", 1, 9);
	if (give_r == get_r) { printf("  must be different.\n"); return; }
	if (t->resources[get_r] < get_n) { printf("  %s only has %d %s.\n", t->name, t->resources[get_r], resource_name(get_r)); return; }
	if (ai_evaluates_offer(&g->players[target_idx], get_r, get_n, give_r, give_n)) {
		p->resources[give_r] -= give_n; p->resources[get_r] += get_n;
		t->resources[get_r]  -= get_n;  t->resources[give_r] += give_n;
		log_add("trade with %s: you -%d %s, +%d %s", t->name, give_n, resource_name(give_r), get_n, resource_name(get_r));
	} else {
		log_add("%s declined your trade", t->name);
	}
}

/* ============================================================
 * Turn start: unlock dev cards bought last turn
 * ============================================================ */
static void unlock_dev_cards(Player *p) {
	for (int t = 0; t < NUM_DEV_TYPES; t++) {
		p->dev_hand[t] += p->dev_locked[t];
		p->dev_locked[t] = 0;
	}
	p->played_dev_this_turn = 0;
}

/* ============================================================
 * Trade with bank/harbour (human + AI)
 * ============================================================ */
static int do_bank_trade(Game *g, Player *p, int give_r, int get_r) {
	int ratio = trade_ratio(g, p, give_r);
	if (p->resources[give_r] < ratio) return 0;
	p->resources[give_r] -= ratio;
	p->resources[get_r]  += 1;
	return ratio;
}

static void human_trade_bank(Game *g, Player *p) {
	printf("  Your trade ratios: ");
	for (int r = 0; r < NUM_RESOURCES; r++) printf("%s=%d:1 ", resource_name(r), trade_ratio(g, p, r));
	printf("\n");
	int give = trade_menu();
	if (give == 6) return;
	give -= 1;
	int ratio = trade_ratio(g, p, give);
	if (p->resources[give] < ratio) { printf("  need %d %s.\n", ratio, resource_name(give)); return; }
	int get = read_int("  Resource to GET [1=Wool 2=Brick 3=Ore 4=Wheat 5=Lumber]: ", 1, 5) - 1;
	if (get == give) { printf("  must be different.\n"); return; }
	int r = do_bank_trade(g, p, give, get);
	log_add("you trade %d %s -> 1 %s (bank)", r, resource_name(give), resource_name(get));
}

/* ============================================================
 * Human turn
 * ============================================================ */
static void human_play_dev(Game *g, Player *p, int player_idx) {
	if (p->played_dev_this_turn) { printf("  already played a dev card this turn.\n"); return; }
	int playable = p->dev_hand[DEV_KNIGHT] + p->dev_hand[DEV_YOP] + p->dev_hand[DEV_MONO] + p->dev_hand[DEV_ROAD];
	if (playable == 0) { printf("  no playable dev cards.\n"); return; }
	printf("  Hand: Knight=%d  YoP=%d  Mono=%d  Road=%d  (VP=%d hidden)\n",
		p->dev_hand[DEV_KNIGHT], p->dev_hand[DEV_YOP], p->dev_hand[DEV_MONO], p->dev_hand[DEV_ROAD], p->dev_hand[DEV_VP]);
	int ch = read_int("  Play [1=Knight 2=YoP 3=Mono 4=Road 5=Cancel]: ", 1, 5);
	if (ch == 5) return;
	int t = (ch == 1) ? DEV_KNIGHT : (ch == 2) ? DEV_YOP : (ch == 3) ? DEV_MONO : DEV_ROAD;
	if (p->dev_hand[t] <= 0) { printf("  none of those.\n"); return; }
	p->dev_hand[t]--;
	p->played_dev_this_turn = 1;
	switch (t) {
		case DEV_KNIGHT: play_knight(g, player_idx);          break;
		case DEV_YOP:    play_year_of_plenty(g, p);            break;
		case DEV_MONO:   play_monopoly(g, player_idx);         break;
		case DEV_ROAD:   play_road_building(g, player_idx);    break;
	}
}

static void human_take_turn(Game *g, Player *p, int player_idx) {
	while (1) {
		redraw(g);
		int ch = main_menu();
		printf("\n");
		if (ch == 6) return;
		if (ch == 1) {
			int what = build_menu();
			if (what == 4) continue;
			if (what == 1) {
				if (!can_afford_road(p) || p->roads_left <= 0) {
					log_add("can't build road (need 1 lumber + 1 brick and pieces left)");
					continue;
				}
				print_buildable_roads(g, p);
				int s = read_int("  road at edge (1-72): ", 1, 72);
				if (do_build_road(g, p, s)) log_add("you build a road at e%d", s);
				else                         log_add("invalid road placement at e%d", s);
			} else if (what == 2) {
				if (!can_afford_village(p) || p->villages_left <= 0) {
					log_add("can't build village (need 1 each lumber/brick/wheat/wool)");
					continue;
				}
				print_buildable_villages(g, p);
				int v = read_int("  village at vertex (1-54): ", 1, 54);
				if (do_build_village(g, p, v)) log_add("you build a village at v%d", v);
				else                            log_add("invalid village placement at v%d", v);
			} else if (what == 3) {
				if (!can_afford_city(p) || p->cities_left <= 0) {
					log_add("can't build city (need 2 wheat + 3 ore)");
					continue;
				}
				print_upgradable_villages(g, p);
				int v = read_int("  upgrade vertex (1-54): ", 1, 54);
				if (do_build_city(g, p, v)) log_add("you upgrade v%d to a city", v);
				else                         log_add("invalid city upgrade at v%d", v);
			}
			continue;
		}
		if (ch == 2) { buy_dev_card(g, p);                continue; }
		if (ch == 3) { human_play_dev(g, p, player_idx);  continue; }
		if (ch == 4) { human_trade_bank(g, p);            continue; }
		if (ch == 5) { human_trade_player(g, p);          continue; }
	}
}

/* ============================================================
 * AI turn
 * ============================================================ */
static int ai_try_build(Game *g, Player *p) {
	if (can_afford_city(p) && p->cities_left > 0) {
		for (int v = 1; v < NUM_VILLAGES; v++)
			if (can_upgrade_to_city(g, v, p->color)) {
				do_build_city(g, p, v);
				log_add("%s upgrades v%d to a city", p->name, v);
				return 1;
			}
	}
	if (can_afford_village(p) && p->villages_left > 0) {
		int best = 0, best_score = -1;
		for (int v = 1; v < NUM_VILLAGES; v++) {
			if (!can_place_village(g, v, p->color)) continue;
			int s = village_setup_score(g, v);
			if (s > best_score) { best_score = s; best = v; }
		}
		if (best > 0) {
			do_build_village(g, p, best);
			log_add("%s builds a village at v%d", p->name, best);
			return 1;
		}
	}
	if (can_afford_road(p) && p->roads_left > 0) {
		int best_s = 0, best_len = -1;
		for (int s = 1; s < NUM_STREETS; s++) {
			if (!can_place_road(g, s, p->color)) continue;
			g->streets[s] = p->color;
			int len = compute_longest_road(g, p->color);
			g->streets[s] = 0;
			if (len > best_len) { best_len = len; best_s = s; }
		}
		if (best_s > 0) {
			do_build_road(g, p, best_s);
			log_add("%s builds a road at e%d", p->name, best_s);
			return 1;
		}
	}
	return 0;
}

static int ai_try_buy_dev(Game *g, Player *p) {
	if (g->deck_remaining <= 0) return 0;
	if (!can_afford_dev_card(p)) return 0;
	/* Only buy when not better off building something. */
	if (can_afford_road(p) || can_afford_village(p) || can_afford_city(p)) return 0;
	return buy_dev_card(g, p);
}

static int ai_try_play_dev(Game *g, Player *p, int player_idx) {
	if (p->played_dev_this_turn) return 0;
	if (p->dev_hand[DEV_KNIGHT] > 0) {
		/* Play knight if it would grant or extend largest army, or if a
		 * juicy block to thief exists. */
		int target = ai_pick_thief_block(g, p->color);
		int has_victims = 0;
		int vs[NUM_PLAYERS];
		if (victims_at_block(g, target, p->color, vs) > 0) has_victims = 1;
		int boosts_army = (p->knights_played + 1 > g->largest_army_count) ||
		                  (g->largest_army_holder == player_idx);
		if (has_victims || boosts_army) {
			p->dev_hand[DEV_KNIGHT]--;
			p->played_dev_this_turn = 1;
			play_knight(g, player_idx);
			return 1;
		}
	}
	if (p->dev_hand[DEV_MONO] > 0) {
		int totals[NUM_RESOURCES] = {0};
		for (int i = 0; i < NUM_PLAYERS; i++)
			if (i != player_idx)
				for (int r = 0; r < NUM_RESOURCES; r++) totals[r] += g->players[i].resources[r];
		int max = 0;
		for (int r = 1; r < NUM_RESOURCES; r++) if (totals[r] > totals[max]) max = r;
		if (totals[max] >= 4) {
			p->dev_hand[DEV_MONO]--;
			p->played_dev_this_turn = 1;
			play_monopoly(g, player_idx);
			return 1;
		}
	}
	if (p->dev_hand[DEV_YOP] > 0 && !(can_afford_road(p) || can_afford_village(p) || can_afford_city(p))) {
		p->dev_hand[DEV_YOP]--;
		p->played_dev_this_turn = 1;
		play_year_of_plenty(g, p);
		return 1;
	}
	if (p->dev_hand[DEV_ROAD] > 0 && p->roads_left >= 2) {
		p->dev_hand[DEV_ROAD]--;
		p->played_dev_this_turn = 1;
		play_road_building(g, player_idx);
		return 1;
	}
	return 0;
}

static int ai_try_trade(Game *g, Player *p) {
	if (can_afford_road(p) || can_afford_village(p) || can_afford_city(p)) return 0;
	for (int give = 0; give < NUM_RESOURCES; give++) {
		int ratio = trade_ratio(g, p, give);
		if (p->resources[give] < ratio) continue;
		for (int get = 0; get < NUM_RESOURCES; get++) {
			if (get == give) continue;
			p->resources[give] -= ratio;
			p->resources[get]  += 1;
			if (can_afford_road(p) || can_afford_village(p) || can_afford_city(p)) {
				log_add("%s trades %d %s -> 1 %s (bank)", p->name, ratio, resource_name(give), resource_name(get));
				return 1;
			}
			p->resources[give] += ratio;
			p->resources[get]  -= 1;
		}
	}
	return 0;
}

static void ai_take_turn(Game *g, Player *p, int player_idx) {
	/* Try playing a knight before rolling? Skip — we already rolled. */
	while (1) {
		if (ai_try_play_dev(g, p, player_idx)) continue;
		if (ai_try_build(g, p))                continue;
		if (ai_try_trade(g, p))                continue;
		if (ai_try_buy_dev(g, p))              continue;
		break;
	}
}

/* ============================================================
 * Top-level turn
 * ============================================================ */
void run_player_turn(Game *g, int player_idx) {
	Player *p = &g->players[player_idx];
	unlock_dev_cards(p);
	log_add("=== %s's turn ===", p->name);

	if (p->is_human) {
		redraw(g);
		printf("\nPress any key to roll the dice...");
		fflush(stdout);
		getch();
	}
	int roll = roll_dice();
	g->last_roll = roll;
	log_add("[dice] %s rolled %d", p->name, roll);
	if (roll == 7) apply_thief_on_seven(g, player_idx);
	else           distribute_resources(g, roll);

	if (p->is_human) human_take_turn(g, p, player_idx);
	else             ai_take_turn(g, p, player_idx);
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
		printf("  %s%s\033[m  pts=%d  cards=%d  knights=%d  road=%d%s%s",
			col, p->name, p->points, total, p->knights_played,
			p->longest_road_length,
			p->has_longest_road ? " [LR]" : "",
			p->has_largest_army ? " [LA]" : "");
		if (p->is_human) {
			printf("\n    res[");
			for (int r = 0; r < NUM_RESOURCES; r++)
				printf("%s%s=%d", (r == 0 ? "" : " "), resource_name(r), p->resources[r]);
			printf("]   dev[K=%d V=%d Y=%d M=%d R=%d  +locked=%d]",
				p->dev_hand[DEV_KNIGHT], p->dev_hand[DEV_VP], p->dev_hand[DEV_YOP],
				p->dev_hand[DEV_MONO], p->dev_hand[DEV_ROAD],
				p->dev_locked[DEV_KNIGHT] + p->dev_locked[DEV_YOP] +
				p->dev_locked[DEV_MONO] + p->dev_locked[DEV_ROAD]);
		}
		printf("\n");
	}
	printf("  thief on block %d.  last roll: %d.  deck=%d.\n", g->thief_block, g->last_roll, g->deck_remaining);
}

int game_winner(const Game *g) {
	for (int i = 0; i < NUM_PLAYERS; i++)
		if (g->players[i].points >= 10) return i;
	return -1;
}
