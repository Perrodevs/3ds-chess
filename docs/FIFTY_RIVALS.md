# 50 Rivals — 1.3.0 RC1

Base: 1.2.0 RC2, commit750c05b. Work branch: feature/50-rivals.
No merge, push, stable release or physical test has been performed.

## Catalog

Existing IDs0–40 are unchanged, including Elon40. New IDs:

|ID|Opponent|Engine level index|Style|
|--|--|--|--|
|41|Garry Kasparov|5|Tactical|
|42|Mikhail Tal|5|Aggressive|
|43|Bobby Fischer|5|Positional|
|44|Ada Wong|3|Defensive|
|45|Albert Wesker|4|Aggressive|
|46|Enderman|2|Balanced|
|47|GLaDOS|4|Tactical|
|48|Linus Torvalds|3|Defensive|
|49|NULL|5|Balanced default; lightweight adaptation|

These are fictional interpretations, not authentic player simulations or measured Elo.
Nine events per new opponent are translated into Spanish and English. Nine optional
rivalry lines use the existing unobtrusive tournament footer, with the actual speaker.
Ten pages retain five selectable slots each. Existing campaign/progression and
random tournament entrant pools remain IDs0–39 to preserve their progression.

## Art and music

The36 new expressions are original32x32 code drawings in
include/graphics/rival_sprites.hpp, rendered with existing rectangle primitives.
No downloaded photographic or franchise portraits were added. These drawings do
not establish permission to distribute the represented franchise characters.
Existing atlas40 and original Elon code art remain intact. No additional textures,
image decoding buffers or sound buffers are allocated by the expansion.
The existing procedural music function accepts the new stable IDs and produces
original per-opponent motifs; no franchise recording or melody was imported.
Auditory quality, render speed and visibility on actual3DS remain unverified.

## Compatibility

Archive writer3 stores50 opponent counters. Readers1 and2 retain their historical
40 and41 rows; new counters start at zero. This is forward migration: older game
builds cannot read writer3. Preserve RC2 save backups when testing this branch.
Profile writer5 reads versions1–4; product13 and achievement7 are new.
Character identifiers and existing rewards retain their meanings. Older builds
cannot read new profile/archive writers; preserve a backup for rollback.

## Verification so far

Baseline make -j3 test passed43 host programs before changes.
Catalog test:50 identities,36 distinct expression drawings,81 translated events,
9 legal bounded12-ply self-play traces;5 distinct traces,64800 nodes in129ms on
this Mac (one sample, not target hardware performance).
Existing storage/economy, style comparison and50 selector tests passed.
The first full regression run exposed the existing Elon footer assertion after
speaker labeling changed. Restoring the short Elon label and using short names
for other speakers fixes the footer width and preserves the prior text. Targeted
Elon dialogue, localization, archive1/2 migration and space UI/secret checks passed
subsequently. Full post-integration regression subsequently passed46 host programs; ARMv6K
compilation passed. Final artifacts and verification are listed in FIFTY_RIVALS_COMPLETE.md.

NULL progression, copied adaptive worker inputs, two cosmetic result scenes and
its earned-only board/achievement are implemented and host tested. See
NULL_ADAPTATION.md for exact historical-data limits and migration policy.
Emulator observation is partial; physical validation remains pending. This branch
has not been merged or published.
