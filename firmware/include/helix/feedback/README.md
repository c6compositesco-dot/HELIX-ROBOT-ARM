# Feedback section

Required behavior: [feedback and drives](../../../docs/requirements/06_feedback_drives.md) and [model/calibration](../../../docs/requirements/01_model_calibration.md).

Current code represents measurements and validity but does not acquire sensors or close a loop. Implement observation-only motor sensing first, preserving timestamp/reference/source; qualify correction separately and retain output sensing as an independent channel.
