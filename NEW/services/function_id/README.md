# Function ID Service

The Function ID service adapts the packed database and relation-aware scorer in [`../../features/function_id`](../../features/function_id/README.md) to core contracts.

- [`function_id_service.cppm`](function_id_service.cppm) owns immutable database handles, translates canonical instructions, and returns structured candidate/evidence values.
- Database parsing stays read-only and resource-identity-based; runtime scheduling uses the shared worker pool instead of direct `std::async`.
