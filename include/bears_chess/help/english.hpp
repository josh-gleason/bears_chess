#pragma once

namespace bears_chess {

constexpr const char* HELP = R"(
Bear's Chess Engine CLI Help
----------------------------

Type 'help <command>' for detailed information about a specific command.

Non-UCI Commands:
  help                     Show this help message or detailed command help
  display | d              Display the current board position
  perft                    Run performance test to specified depth
  q | quit | exit          Quit the program

UCI Commands:
  uci                      Switch to UCI mode
  debug                    Turn debug mode on or off
  isready                  Ping the engine to check if it's ready
  setoption                Set engine option
  register                 Register the engine
  ucinewgame               Start a new game
  position                 Set up a position
  go                       Start calculating
  stop                     Stop calculating
  ponderhit                The opponent has played the expected move
  quit                     Quit the program

For more information about the UCI protocol, see the UCI specification.
)";

constexpr const char* HELP_UNKNOWN = R"(
Unknown command. Type 'help' without arguments to see all available commands.
)";

constexpr const char* HELP_HELP = R"(
Command: help
=============

Brief:
  Show help information

Description:
  Display general help or detailed help for a specific command.

Syntax:
  help [command]

Examples:
  help
  help uci
  help position

Notes:
  - If no command is specified, displays a general overview of all available
    commands.
  - Command names are case-sensitive.
)";

constexpr const char* HELP_DISPLAY = R"(
Command: display
================

Brief:
  Display the current board position

Description:
  Shows the current chess position in a human-readable format. The board is
  displayed from white's perspective with rank and file labels.

Syntax:
  display
  d

Examples:
  display
  d

Notes:
  - The display shows piece placement, side to move, castling rights, and en
    passant square if available.
  - Alias: 'd' can be used as a shortcut.
)";

constexpr const char* HELP_PERFT = R"(
Command: perft
==============

Brief:
  Run performance test

Description:
  Performs a performance test (perft) by generating and counting all legal
  moves to a specified depth. This is useful for debugging move generation and
  verifying correctness.

Syntax:
  perft <depth> [moves] [stats]

Examples:
  perft 5
  perft 4 moves
  perft 6 stats
  perft 5 moves stats

Notes:
  - Depth must be a positive integer.
  - The 'moves' flag shows the number of nodes for each legal move from the
    current position.
  - The 'stats' flag shows detailed statistics including captures, castling,
    promotions, en passant, and checks.
  - Higher depths can take significantly longer to compute.
)";

constexpr const char* HELP_QUIT = R"(
Command: quit
=============

Brief:
  Quit the program

Description:
  Exits the chess engine cleanly, terminating all threads and releasing
  resources.

Syntax:
  quit
  q
  exit

Examples:
  quit
  q
  exit

Notes:
  - All three forms (quit, q, exit) perform the same action.
  - The engine will stop any ongoing search before exiting.
)";

constexpr const char* HELP_UCI = R"(
Command: uci
============

Brief:
  Initialize UCI mode

Description:
  Tell the engine to use the Universal Chess Interface (UCI) protocol. This
  should be the first command sent when using the engine in UCI mode. The
  engine responds with its identity and available options, then sends 'uciok'
  to confirm it's ready.

Syntax:
  uci

Examples:
  uci

Notes:
  - This command is typically sent once at engine startup by a UCI-compatible
    GUI.
  - After sending 'uci', the engine will respond with:
    - 'id name <name>' to identify itself
    - 'id author <author>' to identify the author
    - 'option' commands for each configurable parameter
    - 'uciok' to signal completion
  - According to the UCI protocol, this enables UCI mode and the engine should
    not calculate until receiving a 'go' command.
)";

constexpr const char* HELP_DEBUG = R"(
Command: debug
==============

Brief:
  Enable or disable debug mode

Description:
  Switch the debug mode of the engine on or off. In debug mode, the engine
  sends additional information to help with debugging, such as the commands it
  receives and internal state information.

Syntax:
  debug [on|off]

Examples:
  debug on
  debug off

Notes:
  - Debug mode is off by default.
  - This command can be sent at any time, even while the engine is thinking.
  - Additional info is sent using the UCI 'info string' command.
  - Debug output helps GUI developers and engine developers diagnose issues.
)";

constexpr const char* HELP_ISREADY = R"(
Command: isready
================

Brief:
  Synchronize with the engine

Description:
  Used to synchronize the engine with the GUI. The engine must respond with
  'readyok' when it has processed all previous commands and is ready to accept
  new commands. This is used to wait for time-consuming operations to complete.

Syntax:
  isready

Examples:
  isready

Notes:
  - This command must always be answered with 'readyok'.
  - Can be sent at any time, even when the engine is searching.
  - The engine should respond immediately, even during search.
  - Typical use cases:
    - After 'uci' command to wait for engine initialization
    - After 'setoption' commands that may take time (e.g., loading tablebases)
    - After 'ucinewgame' to ensure the engine is ready for a new game
    - To check if the engine is still alive/responsive
)";

constexpr const char* HELP_SETOPTION = R"(
Command: setoption
==================

Brief:
  Set an engine option

Description:
  Change internal parameters of the engine. Each engine may support different
  options. The available options are announced by the engine after the 'uci'
  command using 'option' messages.

Syntax:
  setoption name <id> [value <x>]

Examples:
  setoption name Hash value 128
  setoption name Threads value 4
  setoption name Ponder value true
  setoption name Clear Hash

Notes:
  - Option names and values are case-insensitive and may contain spaces.
  - The 'value' parameter is optional for button-type options.
  - This command should only be sent when the engine is not searching.
  - Common UCI option types:
    - check: boolean options (true/false)
    - spin: integer options with min/max range
    - combo: selection from predefined values
    - button: trigger actions (no value needed)
    - string: text values
  - Standard UCI options:
    - Hash: hash table size in MB
    - Threads: number of search threads
    - Ponder: enable/disable pondering
    - MultiPV: number of principal variations to search
)";

constexpr const char* HELP_REGISTER = R"(
Command: register
=================

Brief:
  Register the engine

Description:
  Attempt to register the engine with a name and/or code, or indicate that
  registration will be done later. Used for engines that require registration
  to enable all features.

Syntax:
  register later
  register name <x> code <y>

Examples:
  register later
  register name John code 4359874324

Notes:
  - This command is only relevant for engines that require registration.
  - Most open-source engines do not use this feature.
  - If an engine sends 'registration error' at startup, the GUI should send
    this command.
  - The engine responds with 'registration checking' followed by 'registration
    ok' or 'registration error'.
  - Unlike copy protection errors, the engine should still function (possibly
    with limited features) if registration fails.
)";

constexpr const char* HELP_UCINEWGAME = R"(
Command: ucinewgame
===================

Brief:
  Start a new game

Description:
  Inform the engine that the next search will be from a different game. This
  allows the engine to reset internal state, clear hash tables selectively, or
  perform other new-game initialization.

Syntax:
  ucinewgame

Examples:
  ucinewgame

Notes:
  - Should be sent before the first 'position' command of a new game.
  - The engine may take some time to process this, so 'isready' should be sent
    afterward to wait for completion.
  - This can be sent for:
    - A new game the engine should play
    - A new game to analyze
    - The next position from a test suite
  - Not all GUIs support this command, so engines should not rely on receiving
    it.
  - If not sent before the first position, the engine should not expect it at
    all for that GUI.
)";

constexpr const char* HELP_POSITION = R"(
Command: position
=================

Brief:
  Set up a chess position

Description:
  Set up the position on the internal board and optionally play a sequence of
  moves from that position. This command always precedes a 'go' command to
  define what position to search.

Syntax:
  position [startpos | fen <fenstring>] [moves <move1> ... <movei>]

Examples:
  position startpos
  position startpos moves e2e4 e7e5
  position fen rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
  position fen 8/8/8/4k3/8/8/4K3/8 w - - 0 1 moves e2e3

Notes:
  - Moves are in long algebraic notation (e.g., e2e4, e7e8q for promotion).
  - Castling moves are encoded as king moves: e1g1 (white short), e1c1 (white
    long).
  - If this position is from a different game than the last one, 'ucinewgame'
    should be sent first.
  - The FEN string must include all six fields (position, side to move,
    castling, en passant, halfmove clock, fullmove number).
  - There is no separate 'new' command in UCI - use 'position startpos'
    instead.
)";

constexpr const char* HELP_GO = R"(
Command: go
===========

Brief:
  Start calculating

Description:
  Begin searching on the current position (set up with the 'position' command).
  Various parameters control the search behavior, time management, and search
  limits.

Syntax:
  go [searchmoves <move1> ... <movei>] [ponder] [wtime <x>] [btime <x>]
     [winc <x>] [binc <x>] [movestogo <x>] [depth <x>] [nodes <x>]
     [mate <x>] [movetime <x>] [infinite]

Examples:
  go infinite
  go depth 10
  go movetime 5000
  go wtime 60000 btime 60000 winc 1000 binc 1000
  go wtime 120000 btime 120000 movestogo 40
  go nodes 1000000
  go mate 3
  go searchmoves e2e4 d2d4
  go ponder

Notes:
  - All parameters are optional. If omitted, they don't influence the search.
  - Parameters:
    - searchmoves: restrict search to only these moves
    - ponder: search in pondering mode (don't stop even if mate found)
    - wtime/btime: time remaining for white/black in milliseconds
    - winc/binc: time increment per move for white/black in milliseconds
    - movestogo: moves until next time control (if not sent with time, it's
      sudden death)
    - depth: search only this many plies
    - nodes: search only this many nodes
    - mate: search for mate in this many moves
    - movetime: search for exactly this many milliseconds
    - infinite: search until 'stop' command
  - The engine must always send 'bestmove' when it stops, even in ponder mode.
  - Before 'bestmove', the engine should send a final 'info' with complete
    statistics.
)";

constexpr const char* HELP_STOP = R"(
Command: stop
=============

Brief:
  Stop calculating

Description:
  Stop the current search as soon as possible. The engine must respond with
  'bestmove' (and optionally 'ponder') to indicate the best move found so far.

Syntax:
  stop

Examples:
  stop

Notes:
  - The engine should stop as quickly as possible but may take a moment to
    finish the current iteration.
  - After stopping, the engine must send 'bestmove' even if the search was
    incomplete.
  - This applies even in ponder mode - 'bestmove' is required after every 'go'
    command.
  - If the engine receives 'stop' when not calculating, it should ignore the
    command.
  - The final 'bestmove' should be preceded by a final 'info' command with
    search statistics.
)";

constexpr const char* HELP_PONDERHIT = R"(
Command: ponderhit
==================

Brief:
  Opponent played expected move

Description:
  Inform the engine that the user has played the expected move that the engine
  was pondering on. The engine should switch from pondering to normal search
  mode and continue searching.

Syntax:
  ponderhit

Examples:
  ponderhit

Notes:
  - This is only sent if the engine was told to ponder (via 'go ponder').
  - The engine should continue searching but switch from ponder mode to normal
    mode.
  - The pondered move is the last move in the preceding 'position' command.
  - If the opponent plays a different move, the GUI sends 'stop' instead, then
    a new 'position' and 'go'.
  - After 'ponderhit', the engine continues until it finishes naturally or
    receives 'stop'.
)";

} // namespace bears_chess
