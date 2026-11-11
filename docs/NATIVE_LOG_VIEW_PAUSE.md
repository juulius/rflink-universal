# Native v0.7 — stable log reading while new events arrive

The Logs page refreshes automatically every five seconds while viewing the
first page. Previously `renderLogs()` rebuilt the HTML table on each refresh:
new rows at the top would physically move an expanded RF record out from under
the reader, including its GDO2 timing or CC1101 FIFO byte detail pane.

## User-facing behavior

- On the first page, expanding **any** log event pauses only the browser's
  live log view. The device keeps receiving and retaining records normally.
- The snapshot displayed when the row was expanded is held fixed; new events
  will not move the inspected row. An already-running live HTTP response is
  ignored if it returns after the pause.
- The existing **Latest events** button becomes **Resume live** while paused,
  even on page 1. It refreshes the newest events and scrolls the log back to
  the top. Closing an expanded row does not silently resume polling.
- On older pages, the existing historical-snapshot paging remains unchanged.
  Returning to page 1 from a paused history view still preserves the snapshot
  until the reader chooses **Resume live**.
- Changing tab, radio or noise filter explicitly begins a fresh live view.
- Refreshing the actual ESP32 event history and CSV exports is unchanged.
  A raw RF capture stored in the short eight-entry per-radio PSRAM history
  can independently expire even while its metadata log entry is frozen.

## Coverage

`firmware/tests/test_log_live_pause.js` executes the actual
`pauseLogViewing`, `resetLogPages` and `refreshLogs` functions in a minimal
Node.js VM harness and tests normal live rendering, fixed-snapshot query,
late-response guard, explicit resume and top-scrolling. Both scheduled and
tab-visibility refresh paths are checked for the pause condition.

Changes are limited to embedded web UI assets and a host test. No radio,
OTA, reset, watchdog, PSRAM capture, or log engine changes.
