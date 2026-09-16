# Replay Tests

[`replay_tests.cppm`](replay_tests.cppm) verifies that committed envelopes can rebuild the current software projection after the in-memory model is discarded, and that the SQLite checkpoint follows the applied event sequence.
