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

## What's NOT implemented (yet)

This is the minimum playable build. The following standard Catan features
are intentionally omitted:

- Development cards (knight, progress, victory point)
- Harbour trades (only the bank's 4:1 is supported)
- Player-to-player trading
- "Largest army" / "Longest road" bonuses

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
