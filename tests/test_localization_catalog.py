#!/usr/bin/env python3
"""Static release checks for hand-authored bilingual UI catalog data.

Runtime composition, language persistence, raw user text and screenshots have
separate C++/UI coverage. Long teaching paragraphs deliberately have no line cap.
"""
from pathlib import Path
import json
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
ENTRY = re.compile(r'^\s*\{\s*("(?:\\.|[^"\\])*")\s*,\s*("(?:\\.|[^"\\])*")\s*\}\s*,?\s*$')
FORMAT = re.compile(r'%(?!%)(?:[-+0 #]*\d*(?:\.\d+)?(?:hh|ll|[hljztL])?)([diuoxXfFeEgGaAcsp])')
SUPPORTED = set(chr(i) for i in range(32, 127)) | set('áéíóúüñÁÉÍÓÚÜÑ¿¡\n\t')


def entries():
    found = []
    for file in sorted((ROOT / 'include/localization').glob('*_catalog.inc')):
        for number, line in enumerate(file.read_text(encoding='utf-8').splitlines(), 1):
            if not line.strip() or line.lstrip().startswith('//'):
                continue
            match = ENTRY.fullmatch(line)
            if not match:
                raise AssertionError(f'{file.name}:{number}: invalid catalog initializer')
            found.append((json.loads(match[1]), json.loads(match[2]), f'{file.name}:{number}'))
    return found


class LocalizationCatalog(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.records = entries()
        cls.catalog = {es: en for es, en, _ in cls.records}

    def test_unique_nonempty_keys_and_translations(self):
        self.assertTrue(self.records)
        seen = {}
        for es, en, at in self.records:
            self.assertTrue(es, at)
            self.assertTrue(en, at)
            self.assertNotIn(es, seen, f'{at}: duplicate {es!r}; prior {seen.get(es)}')
            seen[es] = at

    def test_printf_conversion_types_are_preserved(self):
        for es, en, at in self.records:
            self.assertEqual(FORMAT.findall(es), FORMAT.findall(en), f'{at}: printf conversions')
            self.assertEqual(es.count('%%'), en.count('%%'), f'{at}: literal percent')

    def test_font_supports_all_catalog_characters(self):
        for es, en, at in self.records:
            unsupported = set(es + en) - SUPPORTED
            self.assertFalse(unsupported, f'{at}: unsupported glyphs {unsupported!r}')

    def test_known_single_line_tool_labels_fit_their_ui_contract(self):
        # Explicit single-line controls only. Educational paragraphs are wrapped.
        controls = {
            'Editor de posiciones': 47,
            'Guardar posicion...': 47,
            'Turno, enroque y al paso': 47,
            'Editar solucion del puzzle': 47,
            'Crear puzzle desde aqui': 47,
            'Volver al editor': 47,
            'Reclamar tablas si corresponde': 47,
            'Guardar y volver a variantes': 47,
            'Nueva partida (confirmar reemplazo)': 47,
            'Comprobar solucion': 47,
            'Grabar solucion con tablero': 47,
            'No se pudo iniciar la comprobacion': 49,
            'Maximo cinco medias jugadas. START: comprobar': 49,
            'No se exporto: valida posicion o revisa SD': 49,
            'Toca mini-tablero y luego Elegir casilla.': 49,
            'TALLER': 11,
            'POSICION VALIDA': 23,
            'Enroque: rey a': 23,
            'su torre original': 23,
            'Sin entrar en jaque': 23,
            'estadistica clasica': 23,
            'START: OPCIONES': 23,
        }
        for es, capacity in controls.items():
            self.assertIn(es, self.catalog)
            self.assertLessEqual(len(self.catalog[es]), capacity, f'{es!r}: English label overflows')


if __name__ == '__main__':
    unittest.main()
