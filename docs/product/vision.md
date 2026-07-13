# ARQ product vision

## Purpose

ARQ is intended to be a modern front-office trading and risk platform for medium-sized banks, private banks, specialist dealers and similar institutions.

These institutions trade multiple asset classes and primarily serve clients rather than operate as ultra-low-latency market makers. They need more than a simple order-management system, but do not want the cost, rigidity and implementation burden of a very large vendor platform.

ARQ should give a small internal quant-development team a reusable foundation while allowing that team to respond quickly to traders and dealers and build proprietary products, models and workflows.

## Intended capabilities

Over time, ARQ should support:

- FX, equities, rates, bonds, commodities and structured/OTC products.
- Pricing and client quoting.
- Trade capture and lifecycle management.
- Positions, cashflows and P&L.
- Risk, hedging and execution workflows.
- Discretionary dealer workflows and automated strategies.
- Consistent live, end-of-day and historical views.
- Modern web applications as the primary UI.
- C#, Python, C++ and potentially other supported clients where they add practical value.

This is a long-term product direction, not a claim that all capabilities are currently implemented.

## Initial product scope

The first coherent desk is FX spot, followed by FX forwards.

For the initial release:

- ARQ is the authoritative system of record for trades.
- ARQ owns positions derived from active trades and lifecycle events.
- The initial trade states are Draft, Pending, Active, Cancelled and Matured.
- Activation requires four-eyes approval by someone other than the submitter.
- The web application is the required user client, using the C# gateway and bindings.
- EOD is governed by a London business calendar, with a configurable cut time.
- Trades, official EOD market data and official risk results are retained.
- Customer and institution teams extend ARQ primarily through compile-time modules and configuration.

External trade feeds and order/execution workflows are expected eventually, but are not part of the first vertical slice.

## Guiding principles

1. Use a strongly typed financial-domain core with controlled extensibility.
2. Let central services own authoritative business state and workflows.
3. Use reusable in-process libraries for deterministic, performance-sensitive calculations.
4. Use durable events for durable business state changes.
5. Make auditability and temporal/as-of semantics first-class concerns.
6. Produce consistent live, EOD and historical views from the same underlying financial model.
7. Support discretionary workflows and automated strategies without designing the first release around low-latency market making.
8. Use modern web applications as the primary UI while retaining useful programmatic clients.
9. Separate stable platform foundations, financial libraries, asset-class modules, institution extensions and desk applications.
10. Enable rapid product delivery without filling shared foundations with customer-specific special cases.
11. Prefer a compiled customer distribution when it preserves strong typing and substantially reduces runtime complexity.
12. Introduce abstractions and infrastructure only when they solve a concrete product, deployment or operational problem.

## Extensibility model

ARQ customers or embedded delivery teams are expected to receive and build on an appropriate portion of the codebase. Their developers may add institution or desk modules which are compiled with selected ARQ platform and asset modules.

Recompilation is not inherently undesirable. A compiled distribution can provide:

- Strongly typed products and markets.
- Compile-time validation of module relationships.
- A smaller supported runtime surface.
- Customer-specific composition without runtime type erasure throughout financial code.

Configuration remains important for deployment, workflows, source selection, cut times, permissions and model parameters. It should not replace useful domain typing.

Independently installable runtime financial plugins are not an initial requirement. Runtime extensibility is most appropriate for external adapters and infrastructure implementations where deployment independence provides clear value.

## State, messaging and calculation philosophy

ARQ does not need to be a pure microservice architecture.

- Small calculation operations belong in linked libraries working over immutable local snapshots.
- Authoritative commands and lifecycle transitions belong in central services.
- Durable facts belong in replayable durable streams and authoritative stores.
- Redis and other current-state stores are projections and caches.
- NATS and similar transports are appropriate for ephemeral distribution, not the only record of business success.
- Batch and live workflows should share financial types and calculation implementations without being forced through identical orchestration.

ARQ also does not need to event-source every cache or calculation. Durable business facts must be auditable and reproducible; operational caches may use simpler rebuildable models.

## Temporal and audit intent

ARQ must distinguish when a fact is financially effective from when it was recorded by the system.

Users may create a correction today whose business-effective time is in the past. This is expected to be rare but is required for correcting reference or market objects and rerunning historical/EOD risk without pretending the original market cut occurred at a new time.

The system must retain both the original historical knowledge and the corrected historical view. An official calculation must identify the exact versions of trades/positions, reference data, market data and models used.

## Current non-goals

The following should deliberately not drive near-term architecture:

- Ultra-low-latency market-making infrastructure.
- Recompilation-free addition of every new financial type.
- A universal dynamically typed market.
- Interfaces or remote services for every small cross-module calculation.
- Strict binary independence between every asset class.
- Runtime installation of arbitrary financial plugins.
- Additional messaging technologies without a demonstrated requirement.
- An Excel client for the first production release.
- Parallel implementation of many asset classes before the FX vertical slice is coherent.

Python remains a desirable future client and may eventually remove the need for a dedicated Excel plugin.
