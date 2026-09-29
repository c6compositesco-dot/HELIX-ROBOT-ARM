import json
from pathlib import Path
import tempfile
import unittest
from check_requirements import audit

VALID = '''# Example
Sources: [S01]. Parameter: P-TICK.
### SYS-001: Example contract
Phase: R1; Issue: #2; Origin: derived
Requirement: The controller shall reject invalid configuration before any motion is admitted.
Acceptance: Inject an invalid configuration and verify refusal without actuator side effects.
'''

class CheckerTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.doc = self.root / 'README.md'
        self.doc.write_text(VALID)
        self.source = {'sources': [{'id': 'S01', 'title': 'Primary', 'url': 'https://example.org', 'kind': 'example', 'basis': 'Test', 'caveat': 'Fixture only'}]}
        self.params = {'parameters': [{'id': 'P-TICK', 'status': 'proposed', 'value': 1000, 'phase': 'R1', 'issue': 3, 'unit': 'Hz', 'acceptance': 'Measure on target.'}]}
        self.write_data()
    def write_data(self):
        (self.root / 'sources.json').write_text(json.dumps(self.source))
        (self.root / 'parameters.json').write_text(json.dumps(self.params))
    def run_audit(self):
        return audit(self.root, required_domains={'SYS'})
    def mutate(self, old, new):
        self.doc.write_text(VALID.replace(old, new))
    def test_valid_and_not_verified(self):
        records, errors = self.run_audit()
        self.assertEqual([], errors)
        self.assertEqual('specified', records[0]['status'])
        self.assertEqual(3, records[0]['line'])
    def test_missing_acceptance(self):
        self.mutate('Acceptance:', 'Observation:')
        self.assertTrue(self.run_audit()[1])
    def test_duplicate_requirement(self):
        self.doc.write_text(VALID + '\n' + VALID)
        self.assertTrue(any('duplicate requirement' in e for e in self.run_audit()[1]))
    def test_unknown_phase(self):
        self.mutate('Phase: R1;', 'Phase: R99;')
        self.assertTrue(self.run_audit()[1])
    def test_unknown_issue(self):
        self.mutate('Issue: #2;', 'Issue: #200;')
        self.assertTrue(self.run_audit()[1])
    def test_unknown_source(self):
        self.mutate('[S01]', '[S99]')
        self.assertTrue(self.run_audit()[1])
    def test_source_range(self):
        self.mutate('[S01]', '[S01-S03]')
        self.assertTrue(any('S02' in e for e in self.run_audit()[1]))
    def test_comma_separated_sources(self):
        self.mutate('[S01]', '[S01, S99]')
        self.assertTrue(any('S99' in e for e in self.run_audit()[1]))
    def test_reversed_source_range(self):
        self.mutate('[S01]', '[S03-S01]')
        self.assertTrue(any('reversed source range' in e for e in self.run_audit()[1]))
    def test_unknown_parameter(self):
        self.mutate('P-TICK', 'P-MISSING')
        self.assertTrue(self.run_audit()[1])
    def test_missing_link(self):
        self.doc.write_text(VALID + '\n[missing](missing.md)\n')
        self.assertTrue(self.run_audit()[1])
    def test_bad_registry(self):
        (self.root / 'sources.json').write_text('{broken')
        self.assertTrue(self.run_audit()[1])
    def test_duplicate_source(self):
        self.source['sources'] *= 2
        self.write_data()
        self.assertTrue(self.run_audit()[1])
    def test_unresolved_parameter_value(self):
        self.params['parameters'][0]['status'] = 'unresolved'
        self.write_data()
        self.assertTrue(self.run_audit()[1])
    def test_missing_domain(self):
        self.assertTrue(audit(self.root, required_domains={'SYS', 'IK'})[1])
    def test_bad_heading(self):
        self.mutate('SYS-001:', 'SYS-01:')
        self.assertTrue(self.run_audit()[1])
    def test_missing_shall(self):
        self.mutate(' shall ', ' may ')
        self.assertTrue(self.run_audit()[1])
    def test_origin_must_be_known(self):
        self.mutate('Origin: derived', 'Origin: guessed')
        self.assertTrue(self.run_audit()[1])
    def test_non_https_source(self):
        self.source['sources'][0]['url'] = 'http://example.org'
        self.write_data()
        self.assertTrue(self.run_audit()[1])

if __name__ == '__main__':
    unittest.main()
