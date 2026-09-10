# SLEIGH Machine-Code-to-P-Code Pipeline

This document is an implementation map for a complete SLEIGH port. It describes
the files involved in turning instruction bytes into assembly text and raw
P-Code, the `.sla` build and load formats, the decoder data structures, and the
boundary between the Java Ghidra front end and the native C++ decompiler.

The existing x86-focused explanation is complementary:
[`X86_SLEIGH_PCODE_RU.md`](X86_SLEIGH_PCODE_RU.md). The repository architecture
overview is [`ARCHITECTURE_OVERVIEW_RU.md`](ARCHITECTURE_OVERVIEW_RU.md).

## 1. Scope and Terminology

SLEIGH is not a table of hard-coded `mov()` functions. A processor language is
specified declaratively. A constructor contains two independent descriptions:

```text
pattern:      which instruction bytes/context values match
constructor:  how operands are printed and which P-Code templates are emitted
```

The complete pipeline is:

```text
.slaspec + included .sinc files
    -> preprocessor -> lexer/parser -> semantic compiler
    -> symbol/pattern/semantic tables -> compressed .sla

.ldefs + .sla + .pspec + .cspec
    -> language object and address/register/context model
    -> byte buffer + address + context
    -> decision tree -> constructor tree -> resolved operand handles
    -> assembly text and/or concrete raw P-Code
```

`.sla` is a compiled language description, not a compiled native decoder. The
runtime still walks its decision tree and semantic templates for each
instruction. The `.sla` file removes source parsing and compilation from the
runtime path.

The word *decoder* is used at two levels:

1. `PackedDecode`/`FormatDecode` decode the serialized `.sla` representation.
2. `DecisionNode`, `Constructor`, and `ParserWalker` decode instruction bytes
   using the already loaded language model.

The second decoder produces both assembly and P-Code. It does not produce
machine-code bytes; assembly printing and semantic emission are two consumers
of the same resolved constructor tree.

## 2. Processor-Language Inputs

### 2.1 Language selection files

The x86 language is registered in
[`Ghidra/Processors/x86/data/languages/x86.ldefs`](../Ghidra/Processors/x86/data/languages/x86.ldefs).
Each `<language>` entry selects:

* a language ID, endian and address size;
* the `.sla` file;
* the processor specification (`.pspec`);
* one or more compiler specifications (`.cspec`).

For x86 the composition roots are:

* [`x86-64.slaspec`](../Ghidra/Processors/x86/data/languages/x86-64.slaspec);
* [`x86.slaspec`](../Ghidra/Processors/x86/data/languages/x86.slaspec);
* [`ia.sinc`](../Ghidra/Processors/x86/data/languages/ia.sinc).

The `.slaspec` files select language properties and include source fragments.
The `.sinc` files contain tokens, registers, constructors, operand subtables,
macros, and instruction semantics. For the x86 family, the complete source
fragment set currently includes:

```text
adx.sinc       avx.sinc          avx2.sinc         avx2_manual.sinc
avx512.sinc    avx512_manual.sinc avx_manual.sinc   bmi1.sinc
bmi2.sinc      cet.sinc          clwb.sinc         cmpccxadd.sinc
fma.sinc       gfni.sinc         ia.sinc           lzcnt.sinc
lockable.sinc  macros.sinc       mpx.sinc          pclmulqdq.sinc
rao.sinc       rdrand.sinc       sgx.sinc          sha.sinc
smx.sinc
```

The exact include graph is authoritative in the two `.slaspec` files. A port
must not infer it from the filename list: conditional preprocessor definitions
can select different fragments and constructor variants.

### 2.2 Processor and compiler specifications

The `.pspec` file supplies processor-specific metadata consumed after `.sla`
loading: registers, aliases, context defaults, default symbols, volatile
ranges, memory blocks, and injected operations. The `.cspec` file describes ABI
and calling convention data. Neither file decodes instruction bytes or defines
ordinary instruction semantics.

Important x86 examples are:

* [`x86-64.pspec`](../Ghidra/Processors/x86/data/languages/x86-64.pspec);
* [`x86-64-win.cspec`](../Ghidra/Processors/x86/data/languages/x86-64-win.cspec);
* [`x86-64-gcc.cspec`](../Ghidra/Processors/x86/data/languages/x86-64-gcc.cspec).

The Java loader reads the processor specification in
[`SleighLanguage.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/app/plugin/processors/sleigh/SleighLanguage.java),
especially `readInitialDescription()`, `readRemainingSpecification()`, and
`read()`. Compiler specifications are selected by the language provider and are
used by later analysis/decompilation stages.

Language discovery is performed by
[`SleighLanguageProvider.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/app/plugin/processors/sleigh/SleighLanguageProvider.java).
It scans `.ldefs` resources, creates `SleighLanguageDescription` records, and
lazily constructs `SleighLanguage` instances when a `LanguageID` is requested.
This provider is the application registry front door, not part of instruction
pattern matching.

## 3. Building a `.sla` File

### 3.1 Build orchestration

Processor modules declare SLEIGH compilation options in their `build.gradle`
files. Shared processor build logic generates the compiler argument file and
invokes the SLEIGH compiler for processor language sources. The generated
`sleighArgs.txt` configuration is the authoritative record of the effective
compiler arguments for a build.

At runtime, stale language files may also be compiled. The orchestration is in
[`SleighLanguageFile.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/app/plugin/processors/sleigh/SleighLanguageFile.java):

* `needsCompilation()` checks absence, timestamp staleness, and format version;
* `isSlaFileStale()` uses `SleighPreprocessor.scanForTimestamp()` so included
  `.sinc` files participate in invalidation;
* `withLock()` prevents concurrent writers, including Windows JVM writers;
* `compileSlaFile()` launches the compiler from the installation directory.

The command-line launch class is
[`SleighCompileLauncher.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/pcodeCPort/slgh_compile/SleighCompileLauncher.java).
It parses options in `SleighCompileOptions`, invokes `SleighCompile`, and
supports compiling one file or all `.slaspec` files in a directory.

### 3.2 Compiler phases

The central implementation is
[`SleighCompile.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/pcodeCPort/slgh_compile/SleighCompile.java).
`run_compilation()` performs the following phases:

1. `SleighPreprocessor` expands `@include` files and preprocessor definitions.
2. `SleighLexer` converts the preprocessed character stream into tokens.
3. `SleighParser` builds the ANTLR parse tree.
4. `SleighCompiler.root()` converts the parse tree to compiler-side symbols,
   patterns, constructors, and semantic templates.
5. `process()` performs post-processing, consistency checks, cross references,
   decision-tree construction, and semantic normalization.
6. `SlaFormat.buildEncoder()` writes the resulting language model.

The grammar and generated parser sources are:

* [`SleighLexer.g`](../Ghidra/Framework/SoftwareModeling/src/main/antlr/ghidra/sleigh/grammar/SleighLexer.g);
* [`SleighParser.g`](../Ghidra/Framework/SoftwareModeling/src/main/antlr/ghidra/sleigh/grammar/SleighParser.g);
* [`SleighCompiler.g`](../Ghidra/Framework/SoftwareModeling/src/main/antlr/ghidra/sleigh/grammar/SleighCompiler.g);
* [`SleighPreprocessor.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/sleigh/grammar/SleighPreprocessor.java);
* [`AbstractSleighCompiler.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/sleigh/grammar/AbstractSleighCompiler.java).

The compiler-side model is intentionally separate from the runtime model. The
main compiler packages are:

* `pcodeCPort/slgh_compile`: parsing, semantic compilation, post-processing,
  consistency checking, and output;
* `pcodeCPort/slghsymbol`: source symbols and constructor tables;
* `pcodeCPort/slghpattern`: instruction/context pattern representation;
* `pcodeCPort/slghpatexpress`: pattern expressions and field arithmetic;
* `pcodeCPort/semantics`: `ConstTpl`, `VarnodeTpl`, `OpTpl`, and `ConstructTpl`;
* `pcodeCPort/sleighbase`: shared serialized SLEIGH model and address spaces.

These packages are the Java port of the original native compiler concepts. They
are not the final concrete `PcodeOp` objects for a particular instruction.

### 3.3 `.sla` physical format

[`SlaFormat.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/pcode/utils/SlaFormat.java)
defines the format header and serialization entry points. A normal `.sla` file
is:

```text
bytes 0..2: ASCII "sla"
byte  3:    format version
remaining:  DEFLATE-compressed PackedEncode stream
```

`PackedEncode` writes elements and attributes using the shared p-code encoder
protocol. `SleighBase.encode()` serializes the `<sleigh>` model, including:

* format properties, alignment, unique-space allocation, and delay-slot data;
* source-file index mappings;
* address spaces;
* the SLEIGH symbol table and its decision/constructor tree.

The encoder implementation is
[`PackedEncode.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/pcode/PackedEncode.java);
the shared XML-like encoder abstraction is `Encoder.java`. Debug compilation
can instead use `XmlEncode`, but that is diagnostic output, not the production
`.sla` representation.

## 4. Loading `.sla` and Constructing the Runtime Model

`SleighLanguage.initialize()` is the Java runtime entry point. Its high-level
order is:

1. acquire the language-file lock and compile if necessary;
2. create a `PackedDecode` using `SlaFormat.buildDecoder()`;
3. call `SleighLanguage.decode()`;
4. load register and processor-spec metadata;
5. construct address factories, context caches, register aliases, and runtime
   lookup structures.

`SlaFormat.buildDecoder()` validates the `sla` header, inflates the payload into
`PackedDecode`, and returns a decoder positioned at the serialized model.
`SleighLanguage.decode()` restores format fields, source indexes, address spaces,
the symbol table, and sets `root` to the global `instruction` subtable's
`DecisionNode`.

The runtime serialization classes are:

* [`PackedDecode.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/pcode/PackedDecode.java);
* [`Decoder.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/pcode/Decoder.java);
* [`DecoderException.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/pcode/DecoderException.java);
* [`SleighBase.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/pcodeCPort/sleighbase/SleighBase.java);
* [`SymbolTable.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/app/plugin/processors/sleigh/symbol/SymbolTable.java);
* [`AddrSpace.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/pcodeCPort/space/AddrSpace.java);
* [`Translate.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/pcodeCPort/translate/Translate.java).

The native loader has the same logical stages. The architecture factory is
[`sleigh_arch.cc`](../Ghidra/Features/Decompiler/src/decompile/cpp/sleigh_arch.cc),
where `SleighArchitecture::buildTranslator()` creates a native `Sleigh`
translator from the configured document store. Native format and model loading
are then performed by `Sleigh::initialize()` and `SleighBase::decode()`.

The loaded runtime symbol packages mirror the compiler concepts:

```text
symbol/       Symbol, Constructor, SubtableSymbol, OperandSymbol,
              DecisionNode, ContextSymbol, UseropSymbol, ...
pattern/      PatternBlock, InstructionPattern, ContextPattern, ...
expression/   TokenField, ContextField, OperandValue, arithmetic expressions
template/     ConstTpl, VarnodeTpl, HandleTpl, OpTpl, ConstructTpl
```

The runtime `symbol`, `pattern`, `expression`, and `template` packages are the
files that must be preserved when porting the decoder. The compiler-side
`pcodeCPort` packages create and serialize equivalent information; they are not
interchangeable with the runtime packages without an explicit design decision.

## 5. Decoding One Instruction

### 5.1 Input state

`SleighLanguage.parse(MemBuffer, ProcessorContext, boolean)` creates a
`SleighParserContext` containing the instruction address, byte source, endian,
processor context, and references to the loaded language. `ParserWalker` then
maintains the current constructor state, operand breadcrumb path, fixed handles,
and parser context while traversing the constructor tree.

The relevant files are:

* [`SleighLanguage.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/app/plugin/processors/sleigh/SleighLanguage.java);
* [`SleighParserContext.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/app/plugin/processors/sleigh/SleighParserContext.java);
* [`ParserWalker.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/app/plugin/processors/sleigh/ParserWalker.java);
* [`ConstructState.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/app/plugin/processors/sleigh/ConstructState.java);
* [`FixedHandle.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/app/plugin/processors/sleigh/FixedHandle.java);
* [`ContextCache.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/app/plugin/processors/sleigh/ContextCache.java).

### 5.2 Pattern matching and constructor resolution

The root `DecisionNode` reads fields from the instruction byte stream and
context. Each node selects a child using a mask/value or context expression;
the terminal node yields a `Constructor`. `DecisionNode.resolve()` is the hot
path for this selection.

A constructor can recursively resolve operand subtables. The constructor's
context changes are applied, then each operand is pushed onto the
`ParserWalker`. `SubtableSymbol.resolve()` and the pattern classes resolve
ModR/M, SIB, opcode extensions, instruction-size modes, prefixes, and other
architecture-specific alternatives.

`InstructionPattern.isMatch()` compares instruction bits. `ContextPattern` and
the expression classes compare or calculate context fields. `TokenField` reads
arbitrary instruction bit fields; `ContextField` reads fields maintained in the
processor context. These values are not yet P-Code values: they are inputs to
operand construction and constructor selection.

### 5.3 Concrete operand handles

An operand symbol resolves to a `FixedHandle`. A handle identifies an address
space, offset, size, and optionally an offset space for indirect memory. This is
where a template such as `m32` becomes a concrete register, constant, memory
address, or unique temporary for the current instruction.

`ConstTpl.fix()`, `ConstTpl.fixSpace()`, `ConstTpl.fillinOffset()`, and
`HandleTpl.fix()` substitute dynamic values such as `inst_start`, `inst_next`,
context fields, operand values, and unique-space offsets. The resulting handles
are consumed by both assembly printing and P-Code emission.

### 5.4 Assembly output

The selected constructor stores print pieces and operand references. The
`Constructor.printMnemonic()`, `print()`, and `printList()` methods use the
same `ParserWalker` state to produce the instruction text. Therefore the
decoder's output is not only P-Code:

```text
bytes -> constructor tree -> resolved operands ->
                         -> print pieces -> assembly text
                         -> semantic templates -> raw P-Code
```

Assembly APIs include `InstructionPrototype.getMnemonic()`, operand-list APIs,
and `SleighInstructionPrototype` methods. The native equivalents are
`Sleigh::printAssembly()` and `AssemblyEmit` in

## 6. P-Code Template Expansion

### 6.1 Template model

Compiled semantics are not immediately concrete operations. They are templates:

* `ConstTpl` represents real constants, dynamic jump values, address spaces,
  handles, and runtime-resolved offsets;
* `VarnodeTpl` represents a space/offset/size triple, possibly dynamic;
* `OpTpl` represents an opcode with template inputs and an optional output;
* `ConstructTpl` is an ordered list of operations, with named sections for
  `build`, `delay`, and related directives;
* `HandleTpl` describes how an operand's fixed handle is filled in.

The Java runtime definitions are in
[`ghidra/app/plugin/processors/sleigh/template`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/app/plugin/processors/sleigh/template).
The corresponding compiler-side definitions are in
[`ghidra/pcodeCPort/semantics`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/pcodeCPort/semantics).

### 6.2 Emitter algorithm

`SleighInstructionPrototype.getPcode()` creates a `PcodeEmitObjects`, walks the
selected constructor's `ConstructTpl`, resolves dynamic templates, and returns
`PcodeOp[]`. `getPcodePacked()` performs the same walk with `PcodeEmitPacked`
and writes the compact transport representation.

The shared algorithm is in

[`PcodeEmit.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/app/plugin/processors/sleigh/PcodeEmit.java)
implements the shared algorithm:

1. `build()` recursively expands constructor templates and `build` directives.
2. `dump()` converts each resolved `OpTpl` into a concrete operation.
3. labels are recorded while encountered;
4. `resolveRelatives()` converts label references into instruction-relative
   addresses;
5. delay slots and cross-builds may recursively emit additional instructions;
6. `resolveFinalFallthrough()` adds a final branch when a flow override requires
   it.

`PcodeEmitObjects` materializes Java `PcodeOp` and `VarnodeData` objects.
`PcodeEmitPacked` serializes the result through `PatchEncoder`, using
`PackedEncode`/`PackedBytes`-compatible transport. P-Code overrides, call/branch
flow overrides, injections, labels, and delay slots are handled in the emitter,
not in the x86 `.sinc` instruction rule alone.

For language-wide analysis and regression tooling, the traversal adapters are:

* [`SleighLanguages.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/app/plugin/languages/sleigh/SleighLanguages.java);
* [`SleighConstructorTraversal.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/app/plugin/languages/sleigh/SleighConstructorTraversal.java);
* [`SleighSubtableTraversal.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/app/plugin/languages/sleigh/SleighSubtableTraversal.java);
* [`SleighPcodeTraversal.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/app/plugin/languages/sleigh/SleighPcodeTraversal.java).

They enumerate constructors and `OpTpl` records without decoding a particular
byte buffer. They are useful for coverage reports and differential tests, but
are not in the hot path of `parse()`.

### 6.3 Raw P-Code data model

At the Java API level, the central objects are:

* [`PcodeOp.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/pcode/PcodeOp.java): opcode, output, inputs, sequence number;
* [`Varnode.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/pcode/Varnode.java): address-space offset and byte size;
* [`PcodeOpAST.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/pcode/PcodeOpAST.java): analysis-owned operation node;
* [`VarnodeAST.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/pcode/VarnodeAST.java): analysis-owned variable node;
* [`SequenceNumber.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/pcode/SequenceNumber.java): instruction address and operation order;
* [`PcodeBlockBasic.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/pcode/PcodeBlockBasic.java): basic-block organization;
* [`PcodeSyntaxTree.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/pcode/PcodeSyntaxTree.java): operation and varnode graph container.

The native decompiler's raw representation is:

* [`op.hh`](../Ghidra/Features/Decompiler/src/decompile/cpp/op.hh) and
  [`op.cc`](../Ghidra/Features/Decompiler/src/decompile/cpp/op.cc): `PcodeOpRaw`,
  `PcodeOp`, and operation behavior;
* [`varnode.hh`](../Ghidra/Features/Decompiler/src/decompile/cpp/varnode.hh)
  and `varnode.cc`: `VarnodeData`, `Varnode`, and SSA/heritage metadata;
* [`space.hh`](../Ghidra/Features/Decompiler/src/decompile/cpp/space.hh):
  address spaces (`ram`, `register`, `unique`, `const`, and others);
* [`opcodes.hh`](../Ghidra/Features/Decompiler/src/decompile/cpp/opcodes.hh):
  the P-Code opcode enumeration;
* [`pcoderaw.hh`](../Ghidra/Features/Decompiler/src/decompile/cpp/pcoderaw.hh):
  XML/raw P-Code stream decoding.

An individual raw operation is conceptually:

```text
PcodeOp {
    opcode: OpCode,
    output: optional VarnodeData,
    inputs: vector<VarnodeData>,
    instruction_address: Address,
    instruction_size: integer
}

VarnodeData {
    space: AddrSpace*,
    offset: integer,
    size: integer
}
```

`LOAD` and `STORE` use an explicit address-space input. `unique` is the virtual
temporary space; `const` is used for constants; `register` identifies processor
register storage. Size is in bytes and is semantically significant.

## 7. Native C++ Runtime Path

The native decompiler already contains a full SLEIGH runtime. It is not merely
a P-Code consumer. The central files are:

* [`sleigh.hh`](../Ghidra/Features/Decompiler/src/decompile/cpp/sleigh.hh) and
  [`sleigh.cc`](../Ghidra/Features/Decompiler/src/decompile/cpp/sleigh.cc):
  `Sleigh`, `SleighBuilder`, `PcodeCacher`, `DisassemblyCache`;
* [`sleighbase.hh`](../Ghidra/Features/Decompiler/src/decompile/cpp/sleighbase.hh)
  and `sleighbase.cc`: serialized SLEIGH model, symbols, spaces, and root;
* [`slaformat.hh`](../Ghidra/Features/Decompiler/src/decompile/cpp/slaformat.hh)
  and `slaformat.cc`: `.sla` header, compression, and packed format;
* [`slghsymbol.hh`](../Ghidra/Features/Decompiler/src/decompile/cpp/slghsymbol.hh)
  and `slghsymbol.cc`: constructors, subtables, symbols, and decision data;
* [`slghpattern.hh`](../Ghidra/Features/Decompiler/src/decompile/cpp/slghpattern.hh)
  and `slghpattern.cc`: instruction/context patterns;
* [`slghpatexpress.hh`](../Ghidra/Features/Decompiler/src/decompile/cpp/slghpatexpress.hh)
  and `slghpatexpress.cc`: pattern expressions;
* [`semantics.hh`](../Ghidra/Features/Decompiler/src/decompile/cpp/semantics.hh)
  and `semantics.cc`: semantic templates and P-Code builder base;
* [`context.hh`](../Ghidra/Features/Decompiler/src/decompile/cpp/context.hh)
  and `context.cc`: parser context and instruction/context byte access;
* [`translate.hh`](../Ghidra/Features/Decompiler/src/decompile/cpp/translate.hh)
  and `translate.cc`: generic translation interface and P-Code stream support.

`Sleigh::initialize(DocumentStorage&)` obtains the serialized `<sleigh>` data,
uses `sla::FormatDecode`, and restores the model. `Sleigh::resolve()` walks the
root decision tree; `resolveHandles()` makes operands concrete.

`Sleigh::printAssembly()` calls constructor printing through `AssemblyEmit`.
`Sleigh::oneInstruction()` creates a `SleighBuilder`, builds the constructor
template, and sends concrete operations to a `PcodeEmit`. The native builder
also handles dynamic pointers, unique allocation, labels, cross-builds, and
delay slots.

This is the native end-to-end path:

```text
LoadImage + ContextDatabase + .sla
    -> Sleigh::obtainContext()
    -> Sleigh::resolve()
    -> Sleigh::resolveHandles()
    -> ParserWalker / Constructor
    -> printAssembly(AssemblyEmit)
       or oneInstruction(PcodeEmit)
    -> SleighBuilder -> PcodeCacher -> PcodeOpRaw/Varnode
```

`LoadImage` and `ContextDatabase` are required abstractions for C++ integration:

* [`loadimage.hh`](../Ghidra/Features/Decompiler/src/decompile/cpp/loadimage.hh)
  supplies instruction bytes and mapped program data;
* [`context.hh`](../Ghidra/Features/Decompiler/src/decompile/cpp/context.hh)
  supplies context-register values and parser context state;
* [`ghidra_translate.hh`](../Ghidra/Features/Decompiler/src/decompile/cpp/ghidra_translate.hh)
  adapts the native translator to Ghidra's XML/callback architecture.

## 8. Java-to-Native Boundary

In the normal Ghidra decompiler process, Java owns the program database and
instruction language. [`DecompileCallback.java`](../Ghidra/Features/Decompiler/src/main/java/ghidra/app/decompiler/DecompileCallback.java)
handles a native query for instruction P-Code and calls:

```text
Instruction.getPrototype().getPcodePacked(...)
```

The packed result is transported through the decompiler protocol and consumed
by native code. `DecompileProcess.java`, `DecompInterface.java`, `PatchEncoder`,
`PackedEncode`, and the native marshal/decoder classes are the transport layer.
The native decompiler therefore usually does not decode the original x86 bytes
for this callback; Java has already selected the constructor and emitted P-Code.

Separately, the native `Sleigh` class can load the same `.sla` and decode bytes
itself. This is used by native tools, architecture setup, emulation helpers, and
code paths that need a translator directly. A C++ rewrite must explicitly choose
whether it is replacing:

1. Java language loading and per-instruction emission;
2. the native SLEIGH runtime;
3. both, while preserving the packed callback protocol.

The two paths must agree on `.sla` format, address-space IDs, opcode numbers,
unique-space layout, register offsets, context semantics, delay-slot behavior,
and P-Code transport encoding.

## 9. Complete File Inventory by Responsibility

This is the practical porting inventory. Files not listed are consumers of the
result, not part of instruction decoding.

| Responsibility | Files/directories |
|---|---|
| x86 language source | `Ghidra/Processors/x86/data/languages/*.slaspec`, `*.sinc`, `x86.ldefs`, `*.pspec`, `*.cspec` |
| build integration | processor `build.gradle` files and generated `sleighArgs.txt` configuration |
| compiler CLI | `pcodeCPort/slgh_compile/SleighCompileLauncher.java`, `SleighCompileOptions.java` |
| compiler pipeline | `SleighCompile.java`, `SleighPreprocessor.java`, ANTLR grammar files, `AbstractSleighCompiler.java` |
| compiler model | `pcodeCPort/slgh_compile`, `slghsymbol`, `slghpattern`, `slghpatexpress`, `semantics`, `sleighbase` |
| `.sla` Java format | `pcode/utils/SlaFormat.java`, `program/model/pcode/PackedEncode.java`, `PackedDecode.java`, `Encoder.java`, `Decoder.java` |
| Java language lifecycle | `SleighLanguageFile.java`, `SleighLanguage.java`, `SleighLanguageDescription.java`, `SleighLanguageProvider.java` |
| Java decode runtime | `SleighParserContext.java`, `ParserWalker.java`, `ContextCache.java`, `DecisionNode.java`, `Constructor.java`, symbol/pattern/expression/template packages |
| Java P-Code emission | `PcodeEmit.java`, `PcodeEmitObjects.java`, `PcodeEmitPacked.java`, `VarnodeData.java`, `UniqueLayout.java` |
| Java P-Code model | `program/model/pcode/PcodeOp.java`, `Varnode.java`, `SequenceNumber.java`, `PcodeOpAST.java`, `VarnodeAST.java`, block classes |
| language-wide traversal | `app/plugin/languages/sleigh/SleighLanguages.java`, `SleighConstructorTraversal.java`, `SleighSubtableTraversal.java`, `SleighPcodeTraversal.java` |
| Java/native callback | `DecompileCallback.java`, `DecompileProcess.java`, `DecompInterface.java`, `PatchEncoder.java`, `PackedEncode.java` |
| native `.sla` runtime | `sleighbase.*`, `sleigh.*`, `slaformat.*`, `slghsymbol.*`, `slghpattern.*`, `slghpatexpress.*`, `semantics.*`, `context.*` |
| native translation interface | `translate.*`, `loadimage.*`, `ghidra_translate.*`, `ghidra_arch.*` |
| native P-Code model | `op.*`, `varnode.*`, `space.*`, `opcodes.*`, `pcoderaw.*`, `emulate.*` |

The wildcard in the x86 row means every language source file included by the
two root `.slaspec` files, not arbitrary files in the processor module.

## 10. Porting Invariants and Recommended Order

The following invariants are more important than class names:

* constructor selection must be deterministic for the same bytes, address, and
  context;
* instruction and context fields use the language's declared endian and bit
  numbering rules;
* an address space is identified consistently across `.sla`, handles, P-Code,
  register maps, and the native process;
* Varnode size is bytes, while many pattern values are bit fields;
* `unique` offsets must not collide and must obey the serialized allocation mask;
* labels and relative branches are unresolved during template traversal and are
  finalized only after all emitted operations are known;
* delay-slot instructions may extend the emitted sequence and fallthrough;
* assembly text and P-Code are generated from the same resolved parse tree;
* `.pspec` metadata and `.cspec` ABI rules are not substitutes for `.sinc`
  instruction semantics;
* raw P-Code is not AST/high-level P-Code. SSA, type recovery, heritage, and
  C printing happen after translation.

Recommended implementation order for a C++ rewrite:

1. shared byte/stream, compression, encoder/decoder, and `.sla` validation;
2. address spaces, varnode data, constants, unique layout, and opcode IDs;
3. symbol table, patterns, pattern expressions, constructors, and decision tree;
4. parser context, context cache, walker, and fixed-handle resolution;
5. assembly printing;
6. semantic templates and a concrete `PcodeEmit`;
7. labels, relative flow, delay slots, cross-builds, injections, and overrides;
8. packed Java/native transport compatibility;
9. differential tests against Java `getPcode()` and native `oneInstruction()`.

Useful regression points are
[`SleighCompileRegressionTest.java`](../Ghidra/Test/IntegrationTest/src/test.slow/java/ghidra/pcodeCPort/slgh_compile/regression/SleighCompileRegressionTest.java),
[`SleighCompileOptionsTest.java`](../Ghidra/Framework/SoftwareModeling/src/test.slow/java/ghidra/pcodeCPort/slgh_compile/SleighCompileOptionsTest.java),
and the processor SLEIGH tests under
[`Ghidra/Framework/SoftwareModeling/src/test`](../Ghidra/Framework/SoftwareModeling/src/test).

## 11. x86 Example in One Trace

For `89 51 10` in x86-64 mode:

```text
x86.ldefs
  -> x86-64.sla
  -> root instruction DecisionNode
  -> opcode/modrm/size pattern
  -> MOV m32,Reg32 constructor in ia.sinc
  -> m32/addressing subtable resolves RCX + signed 0x10
  -> Reg32 resolves EDX
  -> print: MOV dword ptr [RCX + 0x10],EDX
  -> semantic template: m32 = Reg32
  -> concrete P-Code:
       unique = INT_ADD RCX, 0x10
       STORE ram, unique, EDX:4
```

The exact temporary offset and textual formatting are implementation details;
the operation, space, address expression, and access size are the semantic
contract. This distinction is essential when validating a rewritten module:
compare normalized P-Code semantics and metadata, not only printed strings.
