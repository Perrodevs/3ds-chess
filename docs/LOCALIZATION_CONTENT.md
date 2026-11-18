# English content translations

`include/localization/content_catalog.inc` supplies exact Spanish-source/English
pairs to the presentation catalog. It contains 637 unique entries after merging shared keys into the central app catalog:
all 369 rival dialogue events, additional compact reaction strings, descriptive
rival metadata, ten interactive academy lessons (introductions, goals, feedback),
eight original puzzles, four special challenges, eight campaign chapter names,
twenty opening lines' names and teaching text, shop labels, and all four help pages.
The central app catalog owns the shared 41-rival title string.

The English text was authored per sentence, including chess terminology (rank/file,
fork, pin, discovered attack, smothered mate, en passant) and concise character tone.
It does not change ratings, promise authentic playing styles, or alter objectives.
All English output uses the existing pixel font's ASCII character set. Source keys
retain their exact existing bytes; Spanish text is not rewritten or reserialized.

Rival personal names, product names already in English, Chess.com, NVIDIA/CUDA/VRAM,
FEN, UCI moves, save-format identifiers, filenames, and clock values are deliberately
unchanged. They are identities or machine data, not untranslated Spanish prose.
No online translation service or runtime network request is used.

The central renderer must translate a complete string before word wrapping or
truncation. In particular, `botLine` constructs short strings for expressions:
translate its full dialogue before computing the compact version. Centered help
labels must likewise use translated text before measuring their width. The catalog
alone cannot translate a fragment already cut off upstream.

`python3 tests/test_content_catalog.py` checks all string literals in the six owned
content modules, against all three merged catalogs, with explicit exemptions for machine data and proper names. It
also rejects duplicate source keys, empty output, and unsupported English glyphs.
This is a source-coverage regression test, not proof that every screen fits. Screen
layout and actual language preference integration are verified separately.

Shared keys are deduplicated with priority app, then content, then tools. English
output and source-key coverage are checked across that merged catalog, rather
than requiring duplicate entries in each include file. Elon's nine fictional
short lines and descriptive metadata are translated with their exact UTF-8 keys.


## Dynamic content layout contracts

The coverage test now checks 727 concrete field instances against the actual
word-wrap and glyph limits used by the UI, including reuse in single-line notices.
Seven English academy strings originally exceeded the 49-glyph notice limit;
they were rewritten concisely without changing Spanish keys or lesson conditions.
A further dialogue wording correction reads "Look at checks first."

Measured maxima with the current catalog:

| Content | Max English glyphs | Wrapped lines / capacity |
| --- | ---: | --- |
| 369 dialogue events | 36 | 2 / 2 at 22 columns |
| Four help titles | 18 | 1 / 1 at 24 columns, scale 2 |
| 28 help lines | 43 | 1 / 1 at 49 columns |
| Academy names | 22 | 1 / 1 at 49 columns |
| Academy introductions | 75 | 2 / 3 at 49 columns |
| Academy objectives | 46 | 1 / 2 at 49 columns |
| Academy success feedback | 70 | 2 / 2 at 49 columns |
| Academy notices (objectives/errors) | 49 | 1 / 1 at 49 columns |
| Original exercise descriptions | 42 | 2 / 4 at 35 columns |
| Opening names | 38 | 1 / 1 at 49; 2 / 2 at 22 |
| Opening ideas | 61 | 2 / 3 at 49 columns |
| Opening goals | 75 | 2 / 3 at 49; 4 / 5 at 22 |
| Rival taglines | 23 | 1 / 1 at 40 columns |
| Rival strength with label | 13 | 1 / 1 at 23 columns |
| Rival reference range | 23 | 1 / 1 at 23 columns |

The dialogue constraint deliberately uses the actual sidebar's two lines, rather
than assuming three. The test fails if any English word-wrap leaves unrendered
content. Results in `build/localization/content-layout.json` describe source/UI
contracts, not a claim of physical console testing or pixel-level screenshot QA.
