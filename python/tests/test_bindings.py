import random
import threading
import time

import pytest

import bears_chess as bc

STARTPOS = bc.INITIAL_POSITION_FEN
KIWIPETE = bc.PERFT_POSITION_2_FEN
POSITION_3 = bc.PERFT_POSITION_3_FEN
POSITION_4 = bc.PERFT_POSITION_4_FEN
POSITION_5 = bc.PERFT_POSITION_5_FEN
POSITION_6 = bc.PERFT_POSITION_6_FEN

MATE_IN_1 = "6k1/5ppp/8/8/8/8/8/4R2K w - - 0 1"
MATED = "4R1k1/5ppp/8/8/8/8/8/7K b - - 0 1"
STALEMATE = "7k/5Q2/6K1/8/8/8/8/8 b - - 0 1"
CHECK_NOT_MATE = "4R1k1/6pp/8/8/8/8/8/7K b - - 0 1"
PROMOTION = "8/P6k/8/8/8/8/8/K7 w - - 0 1"


def perft(board, depth):
    if depth == 0:
        return 1
    total = 0
    for move in board.legal_moves():
        board.do_move_unchecked(move)
        total += perft(board, depth - 1)
        board.undo_move()
    return total


@pytest.mark.parametrize(
    "fen,depth,expected",
    [
        (STARTPOS, 4, 197281),
        (KIWIPETE, 3, 97862),
        (POSITION_3, 4, 43238),
        (POSITION_4, 3, 9467),
        (POSITION_5, 3, 62379),
        (POSITION_6, 3, 89890),
    ],
)
def test_perft(fen, depth, expected):
    assert perft(bc.Board(fen), depth) == expected


@pytest.mark.parametrize("fen", [STARTPOS, KIWIPETE, POSITION_3, POSITION_4, POSITION_5])
def test_random_walks(fen):
    for seed in range(3):
        rng = random.Random(seed)
        board = bc.Board(fen)
        trail = []
        for _ in range(60):
            moves = board.legal_moves()
            assert len(set(moves)) == len(moves)
            for move in moves:
                assert board.parse_move(move.uci) == move
            if board.is_checkmate() or board.is_stalemate():
                assert not moves
                break
            assert moves
            assert board.is_checkmate() == (board.is_check() and not moves)
            trail.append((board.fen, board.hash))
            board.do_move(rng.choice(moves))
        for fen_before, hash_before in reversed(trail):
            board.undo_move()
            assert board.fen == fen_before
            assert board.hash == hash_before


def test_terminal_states():
    assert bc.Board(MATED).is_checkmate()
    assert not bc.Board(MATED).is_stalemate()
    assert bc.Board(STALEMATE).is_stalemate()
    assert not bc.Board(STALEMATE).is_check()
    assert bc.Board(CHECK_NOT_MATE).is_check()
    assert not bc.Board(CHECK_NOT_MATE).is_checkmate()
    assert not bc.Board(STARTPOS).is_check()


def test_castling_and_ep_moves():
    kiwi = bc.Board(KIWIPETE)
    ucis = {m.uci for m in kiwi.legal_moves()}
    assert "e1g1" in ucis and "e1c1" in ucis

    board = bc.Board()
    for uci in ["e2e4", "d7d5", "e4e5", "f7f5"]:
        board.do_move(board.parse_move(uci))
    ucis = {m.uci for m in board.legal_moves()}
    assert "e5f6" in ucis
    assert "e5d6" not in ucis
    ep = board.parse_move("e5f6")
    assert ep.is_capture


def test_fen_consistency():
    for fen in [STARTPOS, KIWIPETE, POSITION_5]:
        board = bc.Board(fen)
        assert bc.Board(board.fen).fen == board.fen


def test_undo_fails():
    with pytest.raises(RuntimeError):
        bc.Board().undo_move()


def test_parse_move_fails():
    with pytest.raises(ValueError):
        bc.Board().parse_move("e2e5")


def test_do_move_fails():
    board = bc.Board()
    stale = board.parse_move("d2d4")
    board.do_move(stale)
    board.do_move(board.parse_move("e7e5"))
    with pytest.raises(ValueError):
        board.do_move(stale)


def test_move_equality():
    board = bc.Board()
    a = board.parse_move("e2e4")
    b = board.parse_move("e2e4")
    c = board.parse_move("d2d4")
    assert a == b and a != c
    assert len({a, b, c}) == 2
    assert {a: 1}[b] == 1


def test_move_properties():
    board = bc.Board(PROMOTION)
    move = board.parse_move("a7a8q")
    assert move.is_promotion and move.promotion == "q"
    assert not move.is_capture
    assert move.uci == "a7a8q"
    assert (move.from_sq, move.to_sq) == (48, 56)


def test_search_mate_in_1():
    result = bc.Search(16).go(bc.Board(MATE_IN_1), depth=4)
    assert result.pvs[0].moves[0].uci == "e1e8"
    assert result.pvs[0].score > 20000


def test_search_stalemate():
    result = bc.Search(16).go(bc.Board(STALEMATE), depth=4)
    assert result.pvs == []


def test_search_promotion():
    result = bc.Search(16).go(bc.Board(PROMOTION), depth=6)
    assert result.pvs[0].moves[0].uci == "a7a8q"


def test_report_callback():
    seen = []
    search = bc.Search(16, on_report=seen.append)
    result = search.go(bc.Board(), depth=5)
    assert len(seen) == 5
    assert [m.uci for m in seen[-1].pvs[0].moves] == [m.uci for m in result.pvs[0].moves]
    assert seen[-1].elapsed_ms >= 0
    assert 0 <= seen[-1].hashfull <= 1000
    assert len(seen[-1].pvs[0].moves) == 5


def test_search_stop_doesnt_latch():
    search = bc.Search(16)
    search.stop()
    assert search.go(bc.Board(), depth=5).depth == 5


def test_search_stop_threaded():
    search = bc.Search(16)
    holder = {}
    thread = threading.Thread(
        target=lambda: holder.update(result=search.go(bc.Board(), movetime_ms=10_000))
    )
    thread.start()
    time.sleep(0.3)
    started = time.time()
    search.stop()
    thread.join(timeout=3)
    assert not thread.is_alive()
    assert time.time() - started < 1.5
    assert holder["result"].nodes > 0


def test_search_go_failure():
    search = bc.Search(16)
    thread = threading.Thread(
        target=lambda: search.go(bc.Board(), movetime_ms=2_000)
    )
    thread.start()
    time.sleep(0.2)
    with pytest.raises(RuntimeError):
        search.go(bc.Board(), depth=3)
    search.stop()
    thread.join(timeout=3)
    assert not thread.is_alive()


def test_mcts_repetition_scores_draw():
    board = bc.Board("4k3/8/8/8/8/8/8/3QK3 w - - 10 20")
    for uci in ["e1e2", "e8e7", "e2e1", "e7e8"] * 2:
        board.do_move(board.parse_move(uci))
    result = bc.UniformMCTS().go(board, 300)
    repeating = next(s for s in result.moves if s.move.uci == "e1e2")
    assert repeating.visits > 0
    assert repeating.q == 0.0
    assert all(s.q > 0.5 for s in result.moves if s.move.uci != "e1e2" and s.visits)


def test_policy_index_is_distinct_and_mirrored():
    for fen in bc.PERFT_FENS:
        board = bc.Board(fen)
        indices = [bc.policy_index(board, move) for move in board.legal_moves()]
        assert all(0 <= index < bc.POLICY_SIZE for index in indices)
        assert len(set(indices)) == len(indices)

    white = bc.Board("4k3/8/8/8/8/8/4P3/4K3 w - - 0 1")
    black = bc.Board("4k3/4p3/8/8/8/8/8/4K3 b - - 0 1")
    assert bc.policy_index(white, white.parse_move("e2e4")) == bc.policy_index(black, black.parse_move("e7e5"))


def test_encode_board_start_position():
    planes = bc.encode_board(bc.Board())
    assert planes.shape == (bc.INPUT_PLANES, 8, 8)
    assert planes.dtype.name == "float32"
    own_pawns = planes[5]
    opponent_pawns = planes[11]
    assert own_pawns[1].sum() == 8 and own_pawns.sum() == 8
    assert opponent_pawns[6].sum() == 8 and opponent_pawns.sum() == 8
    assert planes[4][0][4] == 1 and planes[10][7][4] == 1
    assert all(planes[p].sum() == 64 for p in range(12, 16))
    assert planes[16].sum() == 0 and planes[17].sum() == 0
