"""Independent python-chess oracle; test-only, not linked into the game.

Legal-move perft does not stop for draws. Game outcomes here use automatic
rules only; python-chess can_claim_* also includes intended moves, whereas
Game.canClaim tests the current position. Compare that current condition
explicitly, then test intended claims separately in C++ regression tests.
"""
import chess
import random
import subprocess

rng = random.Random(20261115)
p = subprocess.Popen(['build/rules_cli'], stdin=subprocess.PIPE,
                     stdout=subprocess.PIPE, text=True, bufsize=1)
count = outcomes = 0


def query_position(board):
    global count
    fen = board.fen(en_passant='fen')
    p.stdin.write(fen + '\n')
    p.stdin.flush()
    actual = p.stdout.readline().strip()
    assert actual == fen, (actual, fen)
    moves = dict(item.split(':', 1) for item in p.stdout.readline().split())
    expected = {m.uci(): board.san(m) for m in board.legal_moves}
    assert moves == expected, (fen, moves, expected)
    count += 1


def query_game(board):
    global outcomes
    moves = ' '.join(m.uci() for m in board.move_stack)
    p.stdin.write('GAME ' + board.root().fen(en_passant='fen') + '|' + moves + '\n')
    p.stdin.flush()
    row = p.stdout.readline().strip()
    assert row != 'INVALID', (board.fen(), moves)
    result, claim, repetitions = map(int, row.split())
    outcome = board.outcome(claim_draw=False)
    expected = 0 if outcome is None else 3 if outcome.winner is None else 1 if outcome.winner else 2
    current_claim = outcome is None and (board.is_repetition(3) or
                                         (board.halfmove_clock >= 100 and any(board.legal_moves)))
    assert result == expected and bool(claim) == current_claim, (board.fen(), row, outcome)
    expected_repetitions = next((n for n in range(5, 0, -1) if board.is_repetition(n)), 1)
    assert repetitions == expected_repetitions, (board.fen(), row, expected_repetitions)
    outcomes += 1


try:
    # Edge cases supplement legal random play, which rarely hits these positions.
    for fen in [
        'r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1',
        '4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 2',
        '4k3/P7/8/8/8/8/8/4K3 w - - 0 1',
        '4k3/8/8/r4pPK/8/8/8/8 w - f6 0 1',
        '4k3/8/8/8/8/8/4r3/R3K3 w - - 0 1',
        '4k3/8/8/8/8/8/8/R3K3 w - - 99 1',
        '4k3/8/8/8/8/8/8/R3K3 w - - 100 1',
        '4k3/8/8/8/8/8/8/R3K3 w - - 150 1',
        '7k/5Q2/6K1/8/8/8/8/8 b - - 0 1',
        '7k/6Q1/6K1/8/8/8/8/8 b - - 150 1',
        '4k3/8/8/8/8/4b3/8/2B1K3 w - - 0 1',
        '4k3/8/8/8/8/3b4/8/2B1K3 w - - 0 1',
    ]:
        board = chess.Board(fen)
        query_position(board)
        query_game(board)
    board = chess.Board()
    for ply in range(17):
        query_game(board)
        if ply < 16:
            board.push_uci(['g1f3', 'g8f6', 'f3g1', 'f6g8'][ply % 4])
    for game in range(20):
        board = chess.Board()
        for ply in range(150):
            query_position(board)
            if ply % 10 == 0 or board.is_game_over():
                query_game(board)
            if board.is_game_over():
                break
            board.push(rng.choice(list(board.legal_moves)))
    print(f'Differential FEN/legal/SAN: {count} positions; '
          f'automatic results/current claims/repetition: {outcomes} histories passed (seed 20261115)')
finally:
    p.stdin.close()
    p.wait(timeout=5)
