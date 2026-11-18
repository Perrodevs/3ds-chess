#!/usr/bin/env python3
"""Offline python-chess reference comparison; requires existing host test executable."""
import chess, random, subprocess
from pathlib import Path
rng=random.Random(960)
cases=[]
def add(seed,b):
    if not b.is_valid(): return
    back=chess.Board.from_chess960_pos(seed)
    king=back.king(chess.WHITE)%8
    rights=0
    for color,shift in [(chess.WHITE,0),(chess.BLACK,2)]:
        for rook in b.pieces(chess.ROOK,color):
            if b.castling_rights & chess.BB_SQUARES[rook]: rights |= 1 << (shift+(0 if rook%8>king else 1))
    fields=b.fen(en_passant='fen').split();fields[2]='-'
    expected={}
    for m in b.legal_moves:
        next=b.copy();next.push(m);nf=next.fen(en_passant='fen').split();nf[2]='-'
        expected[b.uci(m,chess960=True)]=' '.join(nf)
    cases.append((seed,rights,' '.join(fields),expected))
for seed in range(960):
    b=chess.Board.from_chess960_pos(seed);add(seed,b)
    for ply in range(12):
        moves=list(b.legal_moves)
        if not moves:break
        b.push(rng.choice(moves))
        if ply in (2,5,8,11):add(seed,b)
    initial=chess.Board.from_chess960_pos(seed)
    b=chess.Board(None,chess960=True)
    for square in initial.pieces(chess.ROOK,chess.WHITE):b.set_piece_at(square,chess.Piece(chess.ROOK,chess.WHITE))
    b.set_piece_at(initial.king(chess.WHITE),chess.Piece(chess.KING,chess.WHITE))
    for k in range(56,64):
        if not b.is_attacked_by(chess.WHITE,k):b.set_piece_at(k,chess.Piece(chess.KING,chess.BLACK));break
    b.castling_rights=b.rooks & b.occupied_co[chess.WHITE]
    add(seed,b)
    # Attackers/blockers test both transit attacks and rook shielding.
    for _ in range(3):
        altered=b.copy()
        for _ in range(rng.randrange(1,4)):
            square=rng.randrange(8,56)
            altered.set_piece_at(square,chess.Piece(rng.choice([chess.ROOK,chess.BISHOP,chess.KNIGHT,chess.QUEEN]),rng.choice([chess.WHITE,chess.BLACK])))
        add(seed,altered)
root=Path(__file__).resolve().parents[1]
text=''.join(f'{seed} {rights} {fen}\n' for seed,rights,fen,_ in cases)
p=subprocess.run([str(root/'build/test_variants'),'--cli','--states'],input=text,text=True,capture_output=True,check=True)
lines=p.stdout.splitlines();assert len(lines)==len(cases)
for index,((seed,rights,fen,expected),line) in enumerate(zip(cases,lines)):
    actual=dict(item.split(':',1) for item in line.split(';') if item)
    assert actual==expected,(index,seed,rights,fen,'missing',set(expected)-set(actual),'extra',set(actual)-set(expected))
print(f'Chess960 differential: {len(cases)} valid positions across all 960 seeds match python-chess legal moves and resulting board/FEN clocks (including UCI960 castling).')
