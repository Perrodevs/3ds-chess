#!/usr/bin/env python3
"""Fail when assigned static content gets an untranslated visible literal."""
import ast
import pathlib
import re
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
PATHS = ('source/ai/bots.cpp', 'source/training/academy.cpp',
         'source/training/campaign.cpp', 'source/openings/openings.cpp',
         'source/economy/profile.cpp', 'source/ui/onboarding.cpp')
LITERAL = r'"(?:[^"\\]|\\.)*"'
TOKENS = re.compile(r'//[^\n]*|/\*.*?\*/|' + LITERAL, re.S)


def literals(source):
    return [ast.literal_eval(m.group()) for m in TOKENS.finditer(source)
            if m.group().startswith('"')]


def catalog():
    text = "\n".join((ROOT / ('include/localization/' + name)).read_text()
                     for name in ('app_catalog.inc', 'content_catalog.inc', 'tools_catalog.inc'))
    pairs = re.findall(r'\{\s*(' + LITERAL + r')\s*,\s*(' + LITERAL + r')\s*\}', text)
    return [(ast.literal_eval(es), ast.literal_eval(en)) for es, en in pairs]


def wrapped_lines(text, columns):
    # Mirrors the production word wrap; English uses one byte per glyph.
    lines = []
    while text:
        length = min(len(text), columns)
        if length < len(text):
            space = text.rfind(' ', 0, length + 1)
            if space > 0:
                length = space
        lines.append(text[:length])
        text = text[length:].lstrip(' ')
    return lines


def content_contracts():
    translations = dict(catalog())
    contracts = []

    def fields(path, start, end):
        text = (ROOT / path).read_text()
        begin = text.index(start)
        return literals(text[begin:text.index(end, begin)])

    def add(label, keys, width, lines=1):
        for key in keys:
            contracts.append((label, key, translations.get(key, key), width, lines))

    dialogue = fields(PATHS[0], 'const char *botDialogue(', '    const int bot =')
    bot_header = (ROOT / 'include/ai/bots.hpp').read_text()
    bot_count = int(re.search(r'botCount\s*=\s*(\d+)', bot_header).group(1))
    assert len(dialogue) == bot_count * 9, 'Dialogue extraction lost an event'
    add('dialogue', dialogue, 22, 2)  # Actual game sidebar is two lines, not three.
    help_text = fields(PATHS[5], 'static const char *titles', '    r.text(12, 42')
    assert len(help_text) == 32, 'Expected four titles and 28 help lines'
    add('help_title_scale2', help_text[:4], 24)
    add('help_line', help_text[4:], 49)
    academy = fields(PATHS[1], 'const Lesson catalog', 'bool attacksFrom')
    assert len(academy) == 70, 'Expected ten complete academy records'
    add('academy_name', academy[0::7], 49)
    add('academy_intro', academy[1::7], 49, 3)
    add('academy_objective', academy[2::7], 49, 2)
    add('academy_success', academy[5::7], 49, 2)
    add('academy_error', academy[6::7], 46, 3)
    # The same objectives/errors also appear in the single-line game notice.
    add('academy_notice', academy[2::7] + academy[6::7] +
        ['Esa jugada no es legal. Revisa el movimiento y el jaque.'], 49)
    exercises = fields(PATHS[2], 'const Exercise puzzles', 'uint32_t mask')
    assert len(exercises) == 56, 'Expected twelve exercises and eight chapters'
    add('exercise_name_with_counter', exercises[:48:4], 39)
    add('exercise_description', exercises[1:48:4], 35, 4)
    add('chapter_name_with_counter', exercises[48:], 39)
    openings = fields(PATHS[3], 'const Line catalog', 'const Line unavailable')
    assert len(openings) == 80, 'Expected twenty complete opening records'
    add('opening_name', openings[0::4], 49)
    add('opening_sidebar_name', openings[0::4], 22, 2)
    add('opening_idea', openings[1::4], 49, 3)
    add('opening_goal', openings[2::4], 49, 3)
    add('opening_sidebar_goal', openings[2::4], 22, 5)
    bots = (ROOT / PATHS[0]).read_text()
    metadata = bots[bots.index('const Bot bots'):bots.index('const Bot &botAt')]
    for row in re.findall(r'\{([^{}]+)\}', metadata):
        values = literals(row)
        if len(values) >= 4:
            add('bot_tagline', [values[1]], 40)
            key = 'RANGO: ' + values[2]
            translated = translations.get('RANGO: ', 'RANGO: ') + translations.get(values[2], values[2])
            contracts.append(('bot_strength_with_prefix', key, translated, 23, 1))
            add('bot_range', [values[3]], 23)
    return contracts


class ContentCatalog(unittest.TestCase):
    def test_keys_and_english(self):
        pairs = catalog()
        self.assertGreaterEqual(len(pairs), 1200)
        self.assertEqual(len(pairs), len(dict(pairs)), 'Duplicate source key')
        for es, en in pairs:
            self.assertTrue(es and en)
            self.assertTrue(en.isascii(), (es, en))  # English catalog deliberately stays ASCII; Spanish also has accented glyphs.
            self.assertNotIn('\n', en)

    def test_content_coverage(self):
        translations = dict(catalog())
        bots = (ROOT / PATHS[0]).read_text()
        metadata = bots[bots.index('const Bot bots'):bots.index('const Bot &botAt')]
        # Rival names are identities, not language-dependent labels.
        proper = {ast.literal_eval(m.group(1)) for m in
                  re.finditer(r'\{\s*(' + LITERAL + ')', metadata)}
        unchanged = proper | {'Chess.com', 'Classic 8-bit', 'Neon Blue', 'Forest',
                               'Royal', 'Retro Gold', '3DS CHESS', '3DS', 'CHESS', 'wb'}
        missing = []
        for path in PATHS:
            for text in set(literals((ROOT / path).read_text())):
                if text in translations or text in unchanged:
                    continue
                if not re.search(r'[A-Za-z]', text) or text.endswith('.hpp'):
                    continue
                if text.startswith(('.bak', '.tmp', 'ACADEMY1', 'TRAINING1', 'OPENINGS1')):
                    continue
                if '/' in text and re.fullmatch(r'[prnbqkPRNBQK1-8/]+ [wb] .*', text):
                    continue  # Standard FEN and its side/rights fields must remain byte-identical.
                if re.fullmatch(r'[a-h][1-8][a-h][1-8][qrbn]?(?: [a-h][1-8][a-h][1-8][qrbn]?)*', text):
                    continue  # UCI solution/opening line, never translated.
                missing.append((path, text))
        self.assertEqual([], sorted(set(missing)), 'Visible content missing from English catalog')


    def test_content_layout_contracts(self):
        contracts = content_contracts()
        bot_count = int(re.search(r'botCount\s*=\s*(\d+)', (ROOT / 'include/ai/bots.hpp').read_text()).group(1))
        self.assertGreaterEqual(len(contracts), 235 + 12 * bot_count)
        overflow = []
        for field, source, english, width, maximum_lines in contracts:
            rendered = wrapped_lines(english, width)
            if len(rendered) > maximum_lines:
                overflow.append((field, width, maximum_lines, source, english, rendered))
        self.assertEqual([], overflow, 'English content would be clipped in its actual UI field')


if __name__ == '__main__':
    unittest.main()
