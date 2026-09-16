# Function ID Service

The Function ID service owns the packed database and relation-aware scorer under [`src`](src) and adapts them to core contracts.

- [`function_id_service.cppm`](function_id_service.cppm) owns immutable database handles, translates canonical instructions, and returns structured candidate/evidence values.
- The hashing algorithm reuses canonical [`core::OperandObject`](../../core/domain/operand.cppm) facts; FID-specific instruction masks and skip/relocation state remain local algorithm inputs. Database lookup flows through `IFunctionIdDatabase::query` rather than exposing the native database object.
- Database parsing stays read-only and resource-identity-based; runtime scheduling uses the shared worker pool instead of direct `std::async`.
