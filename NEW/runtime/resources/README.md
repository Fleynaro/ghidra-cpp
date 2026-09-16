# Runtime Resources

Resource management validates exact analyzer/compiler/SLA identities before service execution.

- [`resource_manager.cppm`](resource_manager.cppm) implements the core resource-manager contract.
- [`resource_lease.cppm`](resource_lease.cppm) holds an immutable resource-set identity for one operation.

The manager does not copy or rewrite `.sla`/`.fidb` resources. Project history records the identity so replay never silently uses a different resource version.
