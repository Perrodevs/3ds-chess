# NULL adaptive backend

State stores verified settled competitive game counts, colors, first White move
(e4/d4/c4/other), human captures, castled games and early minor-piece development.
Only standard initial positions count; practice/local and early resignation or
agreed draws do not. Canonical storage replay must agree with the input result,
position, move and repetition histories. paidId must equal the terminal session ID.
Monotonically processed IDs prevent duplicates. No historical aggregate alone
proves a win or an opening; initialization can replay retained validated archives.
No fabricated old statistics are added. The first4096 eligible games form the
bounded model; later IDs and unlock wins still advance. No network or neural net.

Before three samples the model returns Balanced and no prepared opening. Later,
capture frequency favors Defensive, low early development Aggressive, otherwise
Positional. The dominant historical first move influences a3:1 seeded selection
between legal ordinary openings. This is a small configurable preference, not a
perfect response, prediction of future moves, extra search, or measured strength.
Per-game processing occurs at settlement; the intended worker interface will use
only a copied style and legal prepared move, without sharing mutable model data.

Permanent unlock bits: competitive Byte2, Elon40 and GLaDOS47 wins; validated
Career bestDivision>=3 (Diamante,36 points) and at least one Career win. Career
progress does not become a competitive training sample. NULL49 decisive results
produce scene events only after unlock; the first victory marker is idempotent.

NULL1 checked text is capped at2048 bytes, with range/derived invariant checks,
FNV integrity, temporary write/flush/fsync/validate/rename, and valid backup
recovery. An unrecoverable corrupt file is preserved and refuses overwrite.
This does not promise absolute power-loss resistance. Three real profile-generation
paths were tested independently. The live App integration is not finished yet.

Host state sizeof=56 bytes. Normal and Address/UndefinedBehaviorSanitizer tests
passed: valid/forged/unpaid/stale histories, archive reconstruction, absent history,
4096 cap, zero-ply timeout, legal varied openings, actual advanced Career progression,
one-time announcements/scenes, truncation/corruption and six injected write faults.
Target memory and live adaptation/cinematics/rewards remain pending.
