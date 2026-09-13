# Function Body

Ports the flow and body behavior used by `CreateFunctionCmd`, `FollowFlow`,
`SimpleBlockModel`, and `BasicBlockModel` from the corresponding Ghidra Java
sources under `Ghidra/Features/Base` and `Ghidra/Framework/SoftwareModeling`.

## Contract

- Required state: decoded instructions, flow references, and function entries.
- Consumes: `function_added`, `code_added`, and `flow_changed`.
- Produces: function body address sets, basic blocks, CFG successors/predecessors, and `function_changed`.
- Priority: `400`.
- Consumers: stack, references, propagation, and no-return repair.

Calls are excluded from body traversal but retain call and fall-through
semantics. `Function::instruction_starts` preserves traversal units while
`Function::body` and `body_ranges` expose the complete byte AddressSet. Direct
and conditional jumps become CFG edges, returns terminate blocks, and block
starts include entries, flow destinations, and post-terminator fall-through.
Shared code is represented by shared ranges instead of silently deleting an
existing body. Unresolved indirect flow remains unresolved rather than being
guessed.

Golden body evidence: [`../test_data/subroutine_references/`](../test_data/subroutine_references/).
