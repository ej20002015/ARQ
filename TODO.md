- Get working for market data too


- Change RefDataGatekeeper service
  - Use snapshotting technique and just keep all version data in-memory
- Deploy service to 
- Update refdata dll interfaces to use new approach used for the serialisation - namely templating
- Create RefDataViewManager
- Create subscription bit
- Do all the same for mkt data

- Equality operators for entities + other helpers (e.g. toString())
- Integration testing for ARQClickHouse?
- Add stack trace library
- Add event logging of some description
- Create FXSpot trade class and valuation class?
- Get it to a place where there is PV code
- Create required mkt data structs
- Provide a coherent live-update or invalidation path for reference-data read caches
  - Hook RefData into the messaging service so caches can subscribe for reloads and publishers can notify other processes to reload.
  - Revisit live-data replay and watermark semantics, including whether Redis should conditionally reject stale writes and whether that should use Lua or `WATCH`/`MULTI`/`EXEC`.
  - Define notification behaviour for newly applied, duplicate and stale updates.
