# Instructions for coding agents and contributors

This file applies to the entire repository. More specific `AGENTS.md` files may be added later for individual subsystems; when present, their local rules supplement this file rather than replacing the product and architecture constraints below.

## Required context

Before material design or implementation work, read:

1. [README.md](README.md)
2. [Product vision](docs/product/vision.md)
3. [Architecture](ARCHITECTURE.md)
4. [Roadmap](docs/roadmap/roadmap.md)

Do not infer implemented capability from directory names, generated schemas, README technology lists or deployment scaffolding. Inspect actual targets, dependencies and runtime code.

## Product direction

- ARQ is intended to be a modern front-office trading and risk platform for client-facing financial institutions.
- The first production vertical is FX spot, followed by FX forwards.
- ARQ will be the authoritative system of record for trades and will own positions derived from trade and lifecycle events.
- The initial production client is the web application through the C# gateway and bindings.
- Customer and institution extensions will normally be compile-time modules selected into a customer distribution.
- Recompilation is acceptable when it preserves strong typing and reduces runtime complexity.
- Independently installable runtime financial plugins, a dynamically typed universal market and microservices for small calculation operations are not current goals.

## Architectural constraints

- Keep financial-domain APIs strongly typed.
- Deterministic, performance-sensitive calculations should be local in-process calls over explicit immutable reference-data, market and model snapshots.
- Do not introduce network calls into calculation hot paths for discount factors, FX conversions, interpolation or similar primitives.
- Central authoritative services own business commands, validation, concurrency and lifecycle transitions.
- Kafka is for durable business facts and rebuildable durable streams.
- NATS is for ephemeral notification and low-latency fan-out; correctness must not depend on receiving every NATS message.
- Redis is a rebuildable current-state projection, not the authoritative business store.
- ClickHouse is for durable history, audit and analytical projections, not transactional command ownership.
- Operational caches and projections do not need to be event-sourced, but they must expose or retain a source watermark and be safely rebuildable.
- Web gateways should query services/read projections and use NATS for notification. Do not make each gateway replica reconstruct authoritative state from Kafka.
- Live and EOD workflows should share financial types and calculation libraries without being forced through the same runtime path.

## Domain and temporal rules

- Initial trade lifecycle: `Draft`, `Pending`, `Active`, `Cancelled`, `Matured`.
- Activation requires four-eyes approval by a user other than the submitter.
- State transitions are explicit commands and durable events, not arbitrary field mutations.
- Positions are deterministic projections of authoritative active trades and lifecycle events; do not make them independently editable.
- Distinguish business-effective time from source, recorded/ingestion and processing time.
- Backdated corrections create a new recorded version with a past effective time; they do not rewrite prior facts.
- Official valuations must identify the exact trade/position version, reference snapshot, market snapshot revision and model/configuration version used.
- EOD is governed by a London business calendar; the cut time should be configurable.

## Modules and dependencies

- Stable platform foundations must not depend on asset-class or institution modules.
- Asset-class modules may depend on stable platform and financial-foundation APIs.
- Institution modules may depend on documented public platform and selected asset-module APIs.
- Avoid circular package dependencies and avoid hoisting interfaces into core without a demonstrated stable cross-module abstraction.
- A customer build may generate a closed compile-time set of supported products, reference types and market types.
- Prefer application/service composition roots over global ownership of module-specific factories.

## Schemas and generated code

- Canonical definitions live under `codegen/definitions/` and schema-supporting source files; generated outputs must not be edited directly.
- Every persisted or transmitted field must have a permanent explicit identity. Protobuf field numbers must never depend on declaration order.
- Never reuse removed Protobuf field numbers or names; reserve them.
- Additive fields must be optional or have deterministic compatibility defaults.
- Renames retain wire identity. Type or semantic changes require a new field/schema version and, where needed, an explicit upcaster.
- Database changes must be expressed as forward-only migrations. Do not rely solely on regenerated fresh-install DDL.
- Upcasters belong at durable read boundaries so current domain and calculation code does not accumulate historical-version branches.
- Preserve original durable payloads when required for audit or reconstruction.
- Schema changes require compatibility tests, generated-output verification and rolling-deployment consideration.
- Code generation must depend on generator code, all definitions and all relevant templates; never allow stale generated outputs because only template timestamps were checked.

## Messaging, ordering and recovery

- Durable consumers must define ordering, idempotency, duplicate and late-arrival behaviour.
- Consumers of transactional Kafka output that depend on committed facts must use `read_committed` semantics.
- State-plus-watermark updates must be atomic or have a recovery protocol that cannot advertise state beyond its contents.
- Offsets and versions must advance monotonically.
- Durable events need event ID, aggregate ID/version, schema version, correlation, causation, actor/source and relevant temporal fields.
- Command acknowledgement, command completion and ephemeral UI notification are different concepts. Do not make an ephemeral reply the only evidence that a durable command succeeded.
- DLQ handling must include an explicit replay and correction path.

## Testing expectations

- Prefer deterministic unit tests for financial calculations and snapshot semantics.
- Service and projection changes require tests for restart, replay, duplicates, ordering and failure windows in proportion to their risk.
- Schema changes require old-payload/new-reader compatibility fixtures.
- Storage adapters require contract or integration tests against the actual supported database where practical.
- Do not require external infrastructure for ordinary calculation-library unit tests.
- A test passing against a mock does not prove cross-system transaction or recovery behaviour.

## Working practices

- Preserve unrelated working-tree changes. Never discard or overwrite user changes.
- Use `rg`/`rg --files` for repository searches where available.
- Inspect CMake/package dependencies rather than inferring package relationships from folders.
- Follow existing formatting and naming unless a deliberate migration is in scope.
- Do not add infrastructure, messaging systems or abstraction layers without identifying the concrete product or operational problem they solve.
- Update the relevant documentation when changing product scope, architectural boundaries, schema rules or roadmap phase completion.
- Clearly distinguish implemented functionality, scaffolding, documentation intent and inference in reviews and change descriptions.

## Human learning and participation

Development in this repository should preserve opportunities for the repository
owner to practise programming, debugging, API design and technical design. Do
not assume that every implementation request authorises completing all
intellectually valuable work on the owner's behalf.

For a material coding or design task, consider whether there is a meaningful
part that the owner could undertake. This may be a bounded implementation slice,
the design of an API or abstraction, or a proposal for a larger architectural
problem. Good learning work exercises reasoning about C++, financial-domain
behaviour, APIs, tests, debugging, concurrency or architecture.

Bounded implementation slices should normally be completable in roughly 15-45
minutes and leave the repository in a safe state while unfinished. API and
architectural design exercises may be larger or more open-ended; when useful,
divide them into stages such as problem framing, proposal, critique and
refinement rather than reducing them to a small coding exercise.

When a suitable implementation or design exercise exists:

1. Investigate enough of the repository to define the problem accurately.
2. Explain the relevant context and why the exercise is useful.
3. Give the owner:
   - the intended behaviour or problem to solve;
   - relevant files and existing examples;
   - architectural, domain and compatibility constraints;
   - clear acceptance or evaluation criteria;
   - the command or tests that will verify implementation work, where applicable.
4. Do not provide the complete implementation or pre-empt the central design
   decision unless requested. Begin with directional hints and reveal more
   detail progressively if the owner becomes stuck.
5. For an API or architectural exercise, first ask the owner to propose a design
   and explain its responsibilities, boundaries and trade-offs. Then evaluate it
   against the repository's product direction, architecture, dependency rules,
   failure modes and likely evolution. Challenge weak points constructively and
   compare alternatives only after the owner has had a genuine opportunity to
   reason about the problem.
6. After the owner implements or proposes the work, review it constructively,
   run or recommend appropriate verification, explain any defects or trade-offs,
   and help complete the surrounding integration or refinement.

Prefer reserving for the owner work involving:

- financial or temporal domain logic;
- API, interface, type and abstraction design;
- architectural boundaries and larger design problems where the owner can
  develop and defend a proposal;
- contained bug diagnosis;
- lifecycle and state-transition behaviour;
- deterministic unit or regression tests;
- algorithms and data-structure choices;
- comprehension of an unfamiliar subsystem.

Agents should normally perform mechanical or low-learning-value work, including:

- generated output and schema propagation;
- repetitive edits and broad renames;
- formatting and routine build wiring;
- dependency and environment setup;
- exhaustive repository searches;
- routine documentation synchronization;
- test execution and collection of diagnostic evidence.

Do not manufacture an exercise when the task is trivial, urgent, primarily
mechanical, unsafe to divide, or when a partial implementation would create a
misleading or hazardous state. Never delegate generated-file edits or actions
that require unavailable context, credentials or operational authority.

The owner's explicit instruction controls the collaboration mode:

- "Take this end-to-end" or "implement this for me" means the agent may complete
  the implementation autonomously.
- "Pair with me" means investigate and work interactively, pausing at meaningful
  decisions, design problems and implementation slices.
- "Give me an exercise" means define a bounded implementation or design task and
  withhold the solution until asked.
- "Review this" means critique and explain without rewriting it unless asked.

If the requested mode is unclear, preserve one suitable learning opportunity
while continuing independently with investigation, mechanical work and
verification. This may be an implementation slice, an API design or a larger
architectural proposal. Do not repeatedly ask whether the owner wants to
participate.
