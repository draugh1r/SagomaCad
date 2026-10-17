"""Check build-time palette compilation without touching imported masters."""
import sys
import unittest
from pathlib import Path
import xml.etree.ElementTree as ET
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from palette_to_json import knockouts, NS

class KnockoutTests(unittest.TestCase):
    def test_only_previous_artwork_is_masked(self):
        svg = '<svg xmlns="http://www.w3.org/2000/svg"><path id="before" fill="#328bd3"/><circle id="erase" fill="#ffffff"/><path id="after" fill="#236ead"/></svg>'
        root = ET.fromstring(knockouts(svg, '#ffffff'))
        group = root.find(f'{{{NS}}}g')
        self.assertEqual([n.get('id') for n in group], ['before'])
        self.assertEqual(root[-1].get('id'), 'after')
        mask = root.find(f'{{{NS}}}defs/{{{NS}}}mask')
        self.assertEqual(mask[-1].get('fill'), 'black')
        self.assertNotIn('#ffffff', ET.tostring(root, encoding='unicode'))

    def test_existing_coverage_masks_keep_black_and_white(self):
        svg = '<svg xmlns="http://www.w3.org/2000/svg"><defs><mask id="original"><path fill="white"/><circle fill="black"/></mask></defs><path fill="#ffffff" stroke="#236ead"/></svg>'
        result = knockouts(svg, '#ffffff')
        self.assertIn('fill="white"', result)
        self.assertIn('fill="black"', result)
        self.assertIn('stroke="#236ead"', result)

if __name__ == '__main__':
    unittest.main()
