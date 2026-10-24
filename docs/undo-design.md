# Zyri editing history design

Implementation update: see [implementation status](implementation-status.md) for delivered behavior and departures from this design. The original planning text below is retained as context.

Status: proposed replacement. The current implementation retains full before-buffers and is not connected to painting.

## User-visible contract

One completed gesture creates one undo step. Escape or a cancelled operation leaves no persistent edit. Undo and redo restore exact raster bytes, object values, layer ordering, and transparent tile absence.

A successful save establishes a clean content-state ID. Undoing to that state clears the dirty indicator; branching after undo creates a new state identity and invalidates redo. View pan, zoom, and temporary hover previews do not dirty the document.

Selection changes are session state with an explicit history policy. Start with selection commands in the same chronological history but mark them non-persistent so changing a selection alone does not mark the file modified. Persist neither transient selection nor undo history in the v1 project format.

## Transaction ownership

The editing service owns both document mutation and history. Tools produce transaction updates, not separately applied changes followed by an unrelated history push.

A transaction captures before-references on first touch, stages after-values, validates completion, then publishes one command and change set. No-op transactions are discarded. A structural edit retains enough state to restore removed layers or objects, including their original IDs.

Separate APIs may internally support applying a new command and recording an already-previewed transaction, but callers must not guess whether push also executes. The application exposes one commit operation.

Commands address persistent layer/object IDs. Positional indices may describe order but must be validated against the target state. No command holds a dangling pointer to a closed session.

## Storage strategy

Begin with immutable before/after tile references and structural value snapshots. Sharing avoids deep copies of unchanged tiles. Account for buffers retained only because history references them, as well as active edit buffers and snapshots.

Once correctness is established, compact older entries into compressed payloads. Benchmark full compressed before/after tiles against reversible XOR payloads. Do not assume XOR always wins, and do not retain an uncompressed before-tile per historical entry as a supposed memory optimization.

XOR is reversible only when applied to the exact expected counterpart. If selected, store expected state identity, payload length, and checksum; never use it against an arbitrary current tile. Store presence flags separately so missing tiles can be restored.

Compression occurs off the interactive path from immutable inputs. Replacement of an entry's storage is atomic and cannot alter its meaning. A pending undo either uses the existing resident payload or waits for a bounded load; it must never read a partially replaced payload.

## Limits and spill

Target at least 100 ordinary user edits on the reference workload, with a configurable entry target and byte budgets. A fixed number of unlimited edits cannot be guaranteed within fixed memory. Do not silently claim otherwise.

Start with a 256 MiB resident history-payload budget and a provisional 2 GiB local history-spill budget per session. Tune both with measurements. Spill completed older payloads to a session-owned temporary store after correctness and accounting are in place. Use checksums, bounded reads, and clear cleanup ownership.

If storage is exhausted, keep the current document intact, retain a valid contiguous undo range, and report reduced available history. If the active edit itself cannot be safely completed, cancel it before publishing changes. Never report a successful undo that only partly applied.

History spill is temporary session storage. It is not crash recovery and should not be replayed as a recovery journal. Recovery uses independently validated document snapshots.

## Saved state and merging

Maintain content-state IDs separately from monotonic mutation revisions. An asynchronous save records its snapshot's state as saved; if editing continued, the current document remains dirty.

Opacity sliders and repeated nudges may merge only within an explicit gesture or compatible short interaction. Do not merge across a saved-state boundary or silently mutate the meaning of an existing saved state. A brush stroke already is one transaction and needs no per-dab command macro.

Evicting old history may remove the ability to return to a saved state, but it must not cause an unrelated current state to appear clean. If redo contains a saved state, branching removes that path while preserving correct dirty comparison.

## Errors and acceptance checks

Decompression must return exactly the expected byte count and valid checksum, or an explicit error. Validate all required payloads before changing any tile. On failure, leave the document and cursor unchanged and explain that history data could not be restored.

Required tests include:

- Paint across tile boundaries; undo and redo compare every byte.
- Paint into absent tiles; undo restores absence and allocation counts.
- Cancel a stroke, fill, or transform; no history entry or persistent edit remains.
- Delete a painted layer, undo deletion, undo painting, then redo both.
- Save, edit, undo to saved, branch, and evict history.
- Save an older snapshot while a new edit commits; dirty state remains correct.
- Merge a slider gesture without crossing a save boundary.
- Spill/load entries and compare against the resident-only result.
- Inject truncated payloads, bad checksums, failed writes, and disk exhaustion.
- Close one of two documents with jobs pending; the other document remains unaffected.
