# Project Runtime

Project runtime composes core contracts, services, event history, projections, and workers into one real lifecycle.

- [`project_config.cppm`](project_config.cppm) defines project paths, artifacts, resources, and load options.
- [`project_state.cppm`](project_state.cppm) defines lifecycle/result values.
- [`project_session.cppm`](project_session.cppm) opens/replays `events.log`, loads the PE fixture, emits memory/listing/function events, and queues decompilation.
- [`project_manager.cppm`](project_manager.cppm) owns project sessions.
- [`runtime_core.cppm`](runtime_core.cppm) owns one shared worker pool and event bus.

The public native facade is [`../api/project_facade.cppm`](../api/project_facade.cppm). Commands are routed by [`../dispatcher/command_dispatcher.cppm`](../dispatcher/command_dispatcher.cppm).
