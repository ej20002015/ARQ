# ARQ consolidated roadmap

This roadmap is optimized for a small team and evolves the existing repository rather than replacing it. Phases describe architectural sequence and exit conditions; individual tasks and defects should be tracked in the issue tracker.

## Phase 0 — Repair current correctness defects

Stabilise the existing reference-data and market-data foundations before extending them.

1. Fix the ClickHouse `MktName`/`MarketName` mismatch.
2. Configure authoritative Kafka consumers with `read_committed`.
3. Give transactional producers stable identities and verify restart/fencing behaviour.
4. Make Redis market-state and offset updates atomic.
5. Fix market tombstones.
6. Reconcile snapshot updates per partition without offset regression.
7. Make late or equal-as-of market updates deterministic.
8. Fix code-generation invalidation so definition changes regenerate every affected artifact.
9. Add integration tests for duplicates, aborted transactions, restarts, rebalances and projection replay.
10. Provide a coherent live-update or invalidation path for reference-data read caches.

**Exit condition:** Existing reference and market pipelines can be replayed repeatedly and converge on the same state.

## Phase 1 — Establish schema evolution and compatibility

Complete this before introducing durable trade schemas.

### Compatibility rules

- Every persisted entity and field has a permanent stable identifier.
- Protobuf field numbers are explicit in canonical definitions and never derived from declaration order.
- Existing field numbers and removed field names are never reused.
- Removed Protobuf fields become reserved.
- Renames retain their wire identity.
- New fields are optional or have deterministic compatibility defaults.
- Type or semantic changes require a new field/schema version and an explicit converter/upcaster where necessary.
- Changes to keys or identity require a new entity/event version.
- Events are immutable once published.
- Database migrations are versioned and forward-only.
- Supported old and new service versions can coexist during rolling deployment.

### Generator outputs

Extend canonical definitions and code generation to produce:

- C++ domain types and serialization code.
- C# bindings and client metadata.
- Protobuf schemas with stable numbering and reservations.
- Forward-only ClickHouse migrations rather than only fresh-install DDL.
- Topic/schema manifests.
- Compatibility reports.
- Mechanical defaulting and safe conversion code.
- Upcaster registration scaffolding.
- Old-payload/new-reader compatibility tests.
- Golden binary fixtures for important historical versions.

The generator may create mechanical conversions for safe renames, defaults and widening. It must not invent semantic conversions; those must be explicitly implemented and reviewed.

### CI gates

CI should reject:

- Changed or reused field numbers.
- Incompatible required fields.
- Type changes without approved conversion.
- Missing database migrations.
- Generated output that differs from canonical definitions.
- Failure to read supported historical fixtures.
- Rolling-deployment incompatibility outside the declared support policy.

**Exit condition:** A developer can perform and test a representative additive change, rename, deprecation and semantic version change using documented procedures.

## Phase 2 — Correct package boundaries and shared semantics

1. Remove the `ARQCore`/`ARQMarket` dependency cycle.
2. Keep stable platform foundations free of generated asset/customer types.
3. Introduce compile-time distribution composition for selected modules.
4. Define a common durable event envelope containing event ID, aggregate ID/version, schema version, correlation, causation, actor, source and relevant timestamps.
5. Standardise business-effective, source, recorded/ingestion and processing times.
6. Define immutable reference and market snapshot identity/revision.
7. Define correction, cancellation, replay and idempotency behaviour.
8. Add consumer lag, projection watermark, DLQ count, command latency and snapshot age metrics.

**Exit condition:** Platform foundations can support long-lived trade facts without depending on one asset class or one schema version.

## Phase 3 — Build FX reference and market foundations

Add the minimum domain required for FX spot and forwards:

1. Currencies and currency conventions.
2. Currency pairs and quotation conventions.
3. London and relevant currency calendars.
4. Settlement conventions and spot-date calculation.
5. Users, desks, books and legal entities.
6. Counterparties and client accounts.
7. Market-data sources and provenance.
8. FX spot rates.
9. Forward points or discount curves, according to the selected initial pricing convention.
10. Versioned reference and market snapshots.
11. Manual correction workflow with effective and recorded times.
12. Original-as-known and latest-corrected historical queries.
13. Durable official EOD market retention.

All additions must exercise the schema-evolution process from Phase 1.

**Exit condition:** ARQ can construct a reproducible typed FX market for a supported valuation time and distinguish original from corrected historical knowledge.

## Phase 4 — Implement the authoritative trade foundation

### Trade aggregate

Define a stable trade envelope and an FX spot economic payload including:

- Trade ID and optimistic version.
- Product type and schema version.
- Book, counterparty and trader.
- Trade, effective and settlement dates.
- Lifecycle state.
- Economic terms.
- Creation, submission, approval and modification provenance.
- External identifiers where applicable.

### Lifecycle

Implement:

1. Create and edit draft.
2. Submit for approval.
3. Reject or return to draft.
4. Approve with `approver != submitter`.
5. Amend with optimistic concurrency and explicit reapproval rules.
6. Cancel with effective time, actor and reason.
7. Mature according to calendar/lifecycle processing.
8. Query current and historical trade versions.
9. Durable command status and idempotency keys.
10. Complete audit and provenance.

Orders remain a separate future aggregate.

**Exit condition:** ARQ is a reliable system of record for an FX spot trade, including concurrency, four-eyes approval, correction and replay.

## Phase 5 — Positions, valuation, P&L and basic risk

1. Derive positions from active trade and lifecycle events.
2. Partition positions by relevant legal entity, desk, book, counterparty, currency and settlement dimensions.
3. Make projections rebuildable and watermark-aware.
4. Implement typed local FX spot valuation.
5. Produce contractual cashflows.
6. Add spot delta and currency exposures.
7. Add realised and unrealised P&L.
8. Record trade/position, reference snapshot, market snapshot, model/configuration and calculation-run versions.
9. Define a small extensible risk-measure API only after implementing concrete measures.
10. Reconcile trades, positions, cashflows and P&L.

**Exit condition:** An approved FX spot trade flows deterministically into positions, P&L and basic risk and can be reproduced later.

## Phase 6 — Deliver the first coherent web workflow

Using the C# gateway/bindings and React application, implement:

1. Trade capture ticket and draft management.
2. Pending-approval queue and four-eyes approval/rejection.
3. Trade blotter with lifecycle history.
4. Position, P&L and basic risk views.
5. Authorized reference and market correction screens.
6. HTTP command APIs with durable command status.
7. Redis-backed query projections.
8. NATS-triggered WebSocket or SignalR updates.
9. Authentication and desk/book entitlements.
10. Protected operational/admin endpoints.

Gateways query read projections and authoritative services; they do not independently reconstruct authoritative state from Kafka.

**Exit condition:** A dealer and approver can complete the full FX spot workflow through the web client.

## Phase 7 — EOD and historical reruns

1. Configure the London cut time and business calendar.
2. Seal official trade/position, reference and market cuts.
3. Run the same financial libraries through batch orchestration.
4. Persist EOD positions, market data, valuations, P&L and risk.
5. Support corrected reruns using the same business as-of time and a new snapshot revision.
6. Retain original and corrected runs.
7. Compare runs and explain which corrected inputs changed.
8. Add replay, reconciliation and operational recovery procedures.

**Exit condition:** Any official EOD result can be reproduced, and corrected reruns remain distinguishable from their originals.

## Phase 8 — Extend to FX forwards

1. Add forward economics and lifecycle rules.
2. Add broken-date settlement support.
3. Add curves/forward points and model versioning.
4. Implement forward PV, cashflows, carry, theta and currency risk.
5. Extend positions, P&L explain and web capture.
6. Confirm that the product is added through module composition and schema evolution without asset-specific changes to stable platform foundations.

This phase is an architectural test: if forwards require special cases throughout core services, the boundaries need correction.

## Deliberately deferred

Until the FX vertical slice is coherent:

- Runtime installation of financial plugins.
- A universal dynamically typed market.
- Additional asset classes.
- Order and execution management.
- Complex multi-source market consolidation.
- Strategy runtime and backtesting.
- Distributed scenario grids.
- Excel integration.
- Production Python packaging.
- Additional messaging technologies.
- Highly generalized risk interfaces without concrete measures.

## Critical path

```text
correctness repairs
    -> schema evolution
    -> temporal and event semantics
    -> package/module composition
    -> FX reference and market foundation
    -> trade lifecycle
    -> positions and valuation
    -> web workflow
    -> EOD
    -> FX forwards
```

From Phase 1 onward, every new financial field, event and table should use the same schema-evolution process. The FX vertical slice should continuously test the platform architecture rather than bypassing it.
