# Ghidra Port Evidence

Project lifecycle behavior follows the observable contracts of Ghidra `Program`, `DomainFile`, `ProgramManager`, `AutoAnalysisManager`, and loader initialization. The C++ implementation separates those concerns into [`project_session.cppm`](project_session.cppm), event history, SQLite projection, and service adapters.

The project session intentionally does not expose Java database handles or native parser ownership. Input loading emits replayable memory/listing/function state events, while revisions and resource identities provide deterministic re-open behavior.
