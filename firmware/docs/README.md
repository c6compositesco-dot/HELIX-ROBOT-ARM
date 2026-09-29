# Firmware engineering documentation

Start with the [requirements baseline](requirements/README.md): product intent, section map, phased capabilities and traceable acceptance criteria. The [methods record](requirements/METHODS.md) separates mature implementation techniques from newer research candidates and unresolved hardware questions.

Use [integration gaps](requirements/INTEGRATION_GAPS.md) to see what the current code does not yet implement, [qualification](requirements/QUALIFICATION.md) for evidence gates, and the [parameter register](requirements/parameters.json) for proposed versus unresolved values. Sources are recorded with limitations in [sources.json](requirements/sources.json).

The earlier [implementation outline](IMPLEMENTATION.md) and [foundation validation](VALIDATION.md) remain historical context. New requirements do not imply that their functionality has been implemented or tested. The work branch remains non-flashable and the existing root binary is preserved.

Run:

```sh
python3 -m unittest discover -s firmware/tools -p test_requirements_checker.py -v
python3 firmware/tools/check_requirements.py --export /tmp/helix-requirements.json
```

The export is a specification registry with status `specified`, not a completed-test report. GitHub issues #2-#11 own the implementation/evidence workstreams.
