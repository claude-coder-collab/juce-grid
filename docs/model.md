---
title: The model
nav_order: 4
---

# The model
{: .no_toc }

A `GridModel` is a struct of `std::function` members. Fill in what your data supports; leave the rest empty. Leaving a callback out never breaks anything: it switches off the matching feature.
{: .fs-5 .fw-300 }

1. TOC
{:toc}

## Callbacks

### Required

| Callback | Called when | Your job |
|---|---|---|
| `rowCount()` | binding, refresh, every row change | Return the number of rows. |
| `colCount()` | binding, refresh, Reset | Return the number of columns. Columns only change with a `Reset`. |
| `getValue(row, col)` | painting visible cells, copy, autofit, screen readers, sorting without `compare` | Return the cell's value as a `juce::var`. Numbers are right-aligned; everything is shown with `toString()`. |

A model missing any of these shows as an empty Grid.

### Editing

| Callback | Without it | Your job |
|---|---|---|
| `setValue(row, col, value)` | The Grid is read-only. | Convert the **string** `value` to your type, store it, notify `CellsChanged`, return `true`. Return `false` if it could not be stored. |
| `validate(row, col, value)` | Every value is accepted. | Return `juce::Result::ok()`, or `juce::Result::fail("message")` to reject. Called before every `setValue` from an edit, paste or delete. |
| `isEditable(row, col)` | Every cell is editable (if `setValue` exists). | Return `false` for read-only cells. Paste and delete skip them. |

### Labels

| Callback | Without it |
|---|---|
| `colName(col)` | Headers show spreadsheet letters: A, B, … Z, AA. |
| `rowName(row)` | Row headers show the row's position + 1, so they renumber after a sort. |

### Sorting

| Callback | Without it | Your job |
|---|---|---|
| `reorderRows(order)` | The Grid cannot sort; no sort buttons are shown. | Apply the order (`order[newRow]` = old row), usually to a `RowOrder`, then notify `RowsReordered` with the same order. |
| `compare(col, rowA, rowB)` | The Grid uses `compareValues()`: numbers numerically, text naturally ("9" before "10"), case-insensitive, empty first. | Return the ascending `std::weak_ordering`. The Grid handles descending order and keeps the sort stable. |

### Rows

| Callback | Without it | Your job |
|---|---|---|
| `insertRows(at, rows)` | Insert, paste-rows, blank-row and (without `duplicateRows`) duplicate items are disabled. | Insert the rows before `at`. Values are strings from the clipboard, or the original values when duplicating. Notify `RowsInserted`. |
| `removeRows(rows)` | Delete Rows and Cut Rows are disabled. | Remove the rows (ascending). Notify `RowsRemoved`. |
| `duplicateRows(rows)` | The Grid duplicates with `getValue` + `insertRows`, after the last row. | Duplicate however you like and notify. |

### Undo and batches

| Callback | Without it | Your job |
|---|---|---|
| `undo()`, `redo()` | Cmd/Ctrl+Z and the redo keys do nothing. | Undo or redo the last change, and notify. The Grid keeps no history of its own. |
| `beginBatch()`, `endBatch()` | Nothing; each change stands alone. | Group the changes in between into one undo step, or hold back notifications. Never nested; `endBatch` always follows, even after a failure. |

The Grid batches every operation that can make several changes: paste, delete, cut-and-paste, sorting and all row operations.

### Menus and notifications

| Member | Without it | Your job |
|---|---|---|
| `contextMenu(menu, target)` | Menus show only the built-in items. | Add, remove or change items. `target` says whether the menu is for cells, rows or columns, and gives the cursor cell and selection. |
| `notifier` | The Grid only learns of changes when you call `grid.refresh()`. | Point it at a `GridModelNotifier` your model owns. |

## Notifications

After you change your data, call `notifier.changed(...)` with the most specific change that fits. Every Grid bound to a model with that notifier updates.

| Change | Send it when | What the Grid does |
|---|---|---|
| `CellsChanged{ firstRow, numRows, firstCol, numCols }` | Values changed. | Repaints; tells screen readers; clears a cut or the sort indicator if they are affected. |
| `RowsInserted{ at, count }` | Rows were inserted. | Shifts the cursor, selection, row heights and hidden rows to follow existing rows. |
| `RowsRemoved{ rows }` | Rows were removed (indices before removal). | Drops their heights, hidden state and selection; a cursor on a removed row moves to the row that took its place; an editor on a removed row is cancelled. |
| `RowsReordered{ order }` | Rows were reordered (`order[new]` = old). | Moves the cursor, heights and hidden rows with their rows. |
| `Reset{}` | Anything else, including column changes, loading a file, or an undo that replaced everything. | Re-queries everything; cancels any editor; forgets row heights, hidden rows and the selection. |

{: .note }
If your model has no notifier, the Grid still applies its **own** edits, pastes, sorts and row operations to itself. You only need `grid.refresh()` for changes that come from elsewhere.

## A complete model

The [`demo/DemoModel.cpp`](https://github.com/claude-coder-collab/juce-grid/blob/main/demo/DemoModel.cpp) in the repository implements every callback: per-column validation and conversion, read-only IDs, row insertion and removal, snapshot undo and redo, and batches.
