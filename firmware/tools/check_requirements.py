#!/usr/bin/env python3
"""Check specification structure, not firmware behavior or hardware qualification."""
from __future__ import annotations
import argparse
from collections import Counter
import json
from pathlib import Path
import re
import sys

PHASES = {'R0', 'R1', 'R1-IK', 'R2', 'R3', 'R4', 'R5'}
DOMAINS = {'SYS', 'MOD', 'CAL', 'BSP', 'RT', 'IK', 'MOT', 'STEP', 'SAFE',
           'PWR', 'FB', 'DRV', 'API', 'BUS', 'TASK', 'TOOL', 'SIM', 'OBS', 'REL', 'SEC'}
HEAD = re.compile(r'^### ([A-Z]{2,5}-[0-9]{3}): (.+)$', re.M)
META = re.compile(r'^Phase: ([^;\n]+); Issue: #([0-9]+); Origin: (user|derived)$', re.M)
SOURCE = re.compile(r'(?<![A-Za-z0-9])S([0-9]{2})(?:-S([0-9]{2}))?(?![0-9])')
PARAM = re.compile(r'\bP-[A-Z][A-Z0-9]*(?:-[A-Z0-9]+)*\b')
LINK = re.compile(r'\[[^\]\n]*\]\(([^)\s]+)\)')


def audit(root: Path, required_domains: set[str] | None = None) -> tuple[list[dict], list[str]]:
    root = root.resolve()
    errors: list[str] = []
    records: list[dict] = []
    required = DOMAINS if required_domains is None else required_domains

    def registry(filename: str, key: str, id_pattern: str) -> dict[str, dict]:
        try:
            data = json.loads((root / filename).read_text(encoding='utf-8'))
            rows = data[key]
            if not isinstance(rows, list):
                raise ValueError(f'{key} must be a list')
        except (OSError, ValueError, KeyError, TypeError) as exc:
            errors.append(f'{filename}: {exc}')
            return {}
        result: dict[str, dict] = {}
        for row in rows:
            if not isinstance(row, dict) or not isinstance(row.get('id'), str):
                errors.append(f'{filename}: invalid record')
                continue
            identifier = row['id']
            if not re.fullmatch(id_pattern, identifier) or identifier in result:
                errors.append(f'{filename}: invalid/duplicate ID {identifier}')
                continue
            result[identifier] = row
            if key == 'sources':
                for field in ('title', 'url', 'kind', 'basis', 'caveat'):
                    if not isinstance(row.get(field), str) or not row[field].strip():
                        errors.append(f'{identifier}: missing source field {field}')
                if not str(row.get('url', '')).startswith('https://'):
                    errors.append(f'{identifier}: source URL must be HTTPS')
            else:
                if row.get('status') not in {'proposed', 'unresolved'}:
                    errors.append(f'{identifier}: invalid parameter status')
                if row.get('status') == 'unresolved' and row.get('value') is not None:
                    errors.append(f'{identifier}: unresolved parameter has a value')
                if row.get('status') == 'proposed' and row.get('value') is None:
                    errors.append(f'{identifier}: proposed parameter has no value')
                if row.get('phase') not in PHASES:
                    errors.append(f'{identifier}: invalid parameter phase')
                if type(row.get('issue')) is not int or not 2 <= row['issue'] <= 11:
                    errors.append(f'{identifier}: invalid owning issue')
                for field in ('unit', 'acceptance'):
                    if not isinstance(row.get(field), str) or not row[field].strip():
                        errors.append(f'{identifier}: missing parameter field {field}')
        return result

    sources = registry('sources.json', 'sources', r'S[0-9]{2}')
    parameters = registry('parameters.json', 'parameters', r'P-[A-Z][A-Z0-9-]*')
    seen: set[str] = set()
    domains: set[str] = set()
    for path in sorted(root.glob('*.md')):
        text = path.read_text(encoding='utf-8')
        for match in SOURCE.finditer(text):
            start, end = int(match[1]), int(match[2] or match[1])
            if end < start:
                errors.append(f'{path.name}: reversed source range {match[0]}')
            for number in range(start, end + 1):
                if f'S{number:02d}' not in sources:
                    errors.append(f'{path.name}: unknown source S{number:02d}')
        for identifier in set(PARAM.findall(text)):
            if identifier not in parameters:
                errors.append(f'{path.name}: unknown parameter {identifier}')
        for target in LINK.findall(text):
            if '://' in target or target.startswith(('#', 'mailto:')):
                continue
            local = target.split('#', 1)[0]
            if local and not (path.parent / local).exists():
                errors.append(f'{path.name}: missing local link {target}')
        for line in text.splitlines():
            if re.match(r'^### [A-Z]{2,5}-[0-9]', line) and not HEAD.fullmatch(line):
                errors.append(f'{path.name}: malformed requirement heading {line}')
        matches = list(HEAD.finditer(text))
        for index, match in enumerate(matches):
            identifier, title = match[1], match[2]
            if identifier in seen:
                errors.append(f'{path.name}: duplicate requirement {identifier}')
            seen.add(identifier)
            domain = identifier.split('-', 1)[0]
            domains.add(domain)
            if domain not in DOMAINS:
                errors.append(f'{identifier}: unknown domain')
            block = text[match.end():matches[index + 1].start() if index + 1 < len(matches) else len(text)]
            metadata = META.findall(block)
            req = re.findall(r'^Requirement: (.+)$', block, re.M)
            acceptance = re.findall(r'^Acceptance: (.+)$', block, re.M)
            if len(metadata) != 1 or len(req) != 1 or len(acceptance) != 1:
                errors.append(f'{identifier}: require exactly one metadata, Requirement and Acceptance line')
                continue
            phase, issue, origin = metadata[0]
            if phase not in PHASES:
                errors.append(f'{identifier}: invalid phase {phase}')
            if not 2 <= int(issue) <= 11:
                errors.append(f'{identifier}: unknown owning issue #{issue}')
            if ' shall ' not in f' {req[0]} ':
                errors.append(f'{identifier}: requirement lacks shall')
            if len(req[0]) < 40 or len(acceptance[0]) < 40:
                errors.append(f'{identifier}: insufficient requirement/acceptance text')
            records.append({'id': identifier, 'title': title, 'phase': phase,
                            'issue': int(issue), 'origin': origin,
                            'requirement': req[0], 'acceptance': acceptance[0],
                            'source': path.name, 'line': text[:match.start()].count('\n') + 1,
                            'status': 'specified'})
    for domain in sorted(required - domains):
        errors.append(f'missing requirement domain {domain}')
    if not records:
        errors.append('no requirements found')
    return records, errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[1] / 'docs' / 'requirements')
    parser.add_argument('--export', type=Path)
    args = parser.parse_args()
    try:
        records, errors = audit(args.root)
    except (OSError, ValueError) as exc:
        print(f'ERROR: {exc}', file=sys.stderr)
        return 1
    if errors:
        for error in errors:
            print(f'ERROR: {error}', file=sys.stderr)
        return 1
    counts = Counter(item['phase'] for item in records)
    print(f'PASS: {len(records)} specified requirements across {len({r["id"].split("-")[0] for r in records})} domains; {dict(sorted(counts.items()))}')
    print('Structure/reference checks only; no firmware behavior or hardware qualification is established.')
    if args.export:
        args.export.parent.mkdir(parents=True, exist_ok=True)
        args.export.write_text(json.dumps({'schema_version': 1, 'requirements': records}, indent=2) + '\n', encoding='utf-8')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
