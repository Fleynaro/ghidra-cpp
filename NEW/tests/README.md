# Architecture Tests

These tests exercise the complete runtime architecture rather than isolated legacy features.

- [`integration`](integration/README.md) runs the native facade against the analyzer executable fixture.
- [`replay`](replay/README.md) validates append-only recovery, projection rebuild, and durable checkpoint behavior.

Focused core, runtime, service, and project tests remain next to their implementation modules.
