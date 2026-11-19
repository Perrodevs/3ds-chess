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

Before three samples the model returns Balanced. Prepared openings additionally
require at least three observed e4/d4/c4 starts with the human playing White;
Black-only histories or absent first moves never become assumed e4 observations. Later,
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
paths were tested independently. The App loads/saves this sidecar per active profile. Profile-generation retirement
also retires null.dat. A valid lastId reserves future session IDs after an older
profile restore; a corrupt sidecar disables NULL while preserving normal games.

Host state sizeof=56 bytes. Normal and Address/UndefinedBehaviorSanitizer tests
passed: valid/forged/unpaid/stale histories, archive reconstruction, absent history,
4096 cap, zero-ply timeout, legal varied openings, actual advanced Career progression,
one-time announcements/scenes, truncation/corruption and six injected write faults.
ARMv6K object-symbol sizing confirms56 bytes for State and104 bytes for the
complete EndScene (including64 cosmetic board bytes). Comparing archived RC2
headers with the same target compiler: App6040→6448 (+408 inline bytes),
EndScene32→104. These are type sizes, not peak memory. Existing texture and audio
buffer sizes and worker/main stacks are unchanged. No physical RAM measurement.

The live worker receives a copied Style and optional legal opening move, never a
reference to mutable model data. No new search or larger budget is introduced.
Manual style override remains available; prepared openings use only current legal
history and the profile's verified prior observations. Enderman uses seeded
variation among existing reasonable openings at its normal level.

NULL's two180-frame scenes are gated by the sidecar's newly recorded eligible
human result; A/B skip and reduced motion preserve the result. The eight-half-move
early resignation/agreed-draw exclusion also applies to NULL scenes/rewards.
The first eligible human win grants product13 (NULL TERMINAL BOARD) at zero cost,
and achievement bit7 (ERROR404) under the existing25-coin achievement rule.
Normal win/draw/loss and level formulas are unchanged. Later NULL wins grant the
ordinary competitive reward only. The product cannot be bought before earning it.
The trophy room appends display index19; its existing0–18 IDs are unchanged.
Profile writer5 reads1–4 with old inventory/achievement bounds; writer5 adds the
new bits. Older builds cannot read the new profile/archive writers: retain backups.

ASan/UBSan live UI tests passed for lock/B, settlement/reload/dedup, palette preview,
legacy4 reading, all frames of both scenes in both languages and100 prepared-worker
cancellations. Full post-integration regression results are reported separately.
Emulator: selector10/10, lock/B and English NULL victory scene observed. Mac locked
before the rematch interaction; human victory scene and final full visual QA remain
pending. All six original emulator saves were restored with identical SHA-256.
