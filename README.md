# Catan (Terminal Edition)

A single-player terminal version of *Settlers of Catan*, written in C, with
ANSI-coloured ASCII board rendering. You play one human seat against three
greedy AI opponents.

```
                                   *----*
                                  /      \
                                 /        \
                          *----*    08    *----*
                         /      \        /      \
                        /        \      /        \
                 *----*    06    *----*    05    *----*
                /      \        /      \        /      \
              ...
```

## Build

Requires a C11 compiler.

### Linux / macOS / WSL

```bash
make
./catan
```

### Windows (MinGW / MSYS2)

```bash
mingw32-make
catan.exe
```

### Windows (cl.exe / MSVC)

```cmd
cl /W3 /O2 catan.c catan_map.c catan_menu.c catan_game.c /Fe:catan.exe
```

ANSI colour escapes are enabled automatically on Windows 10+ via
`ENABLE_VIRTUAL_TERMINAL_PROCESSING`.

## Play

1. Run `./catan`, pick **1) Start The Game**.
2. **Setup phase**: each player places 2 villages and 2 connecting roads in
   snake order. Vertices are numbered 1–54, edges 1–72.
3. **Each turn**:
   - The current player rolls two dice. Tiles whose number matches give
     resources to every adjacent village (×1) or city (×2), unless the thief
     is on that tile.
   - On a **7**, every player with more than 7 cards discards half (rounded
     down). The roller moves the thief to a new tile.
   - You can then **Build**, **Trade with the bank (4:1)**, or **End turn**.
4. First player to reach **10 victory points** wins.

### Build costs

| Item    | Cost                                | Points |
|---------|-------------------------------------|--------|
| Road    | 1 lumber + 1 brick                  | —      |
| Village | 1 lumber + 1 brick + 1 wheat + 1 wool | 1   |
| City    | 2 wheat + 3 ore (upgrades a village) | 2 (replaces the 1 from village) |

## Trading

- **Bank**: spend 4 of any one resource for 1 of another.
- **Generic harbour (3:1)**: if you have a settlement at one of the two
  harbour vertices, you trade 3-for-1 instead.
- **Specific harbour (2:1)**: if the harbour matches a resource type, you
  trade 2-of-that-resource for 1 of any other.
- **Player-to-player**: propose `give X get Y`. AI accepts when the deal is
  fair (give ≥ get) and not unreasonably large.

## Development cards

Cost: 1 ore + 1 wool + 1 wheat. Card type drawn at random from the deck of
25 (14 Knight, 5 Victory Point, 2 Year of Plenty, 2 Monopoly, 2 Road
Building). Cards bought this turn are locked until next turn (except VP,
which is hidden but counts immediately).

| Card | Effect |
|------|--------|
| Knight | Move the thief, steal one random card from a chosen victim. Counts toward Largest Army. |
| Year of Plenty | Take any 2 resources from the bank. |
| Monopoly | Name a resource — every other player gives you all of it they have. |
| Road Building | Place 2 free roads. |
| Victory Point | +1 hidden victory point. |

## Bonus points

- **Largest Army (+2)**: first player to *play* 3 Knight cards. Transfers
  to anyone with strictly more played knights.
- **Longest Road (+2)**: first player whose continuous chain of roads is 5+
  segments. Transfers when another player surpasses the holder. An
  opponent settlement on a vertex breaks the chain at that vertex.

## Files

| File             | Purpose |
|------------------|---------|
| `catan.c`        | Top-level main loop and start menu |
| `catan_map.c/h`  | ANSI board renderer + map randomiser |
| `catan_menu.c/h` | Cross-platform key input + text menus |
| `catan_game.c/h` | Game state, board topology, rules, AI |
| `Makefile`       | Build rules |

## License

MIT — see [LICENSE](LICENSE).
