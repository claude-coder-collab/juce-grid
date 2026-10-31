---
title: Rows, undo and batches
nav_order: 7
---

# Rows, undo and batches
{: .no_toc }

1. TOC
{:toc}

## Row operations

Select whole rows by clicking (or dragging over) the row headers, then use the row header's context menu or the keyboard:

| Action | Menu item | Key | Needs |
|---|---|---|---|
| Copy rows | Copy Rows | | |
| Cut rows | Cut Rows | | `removeRows` |
| Paste rows | Insert Copied Rows Below / Insert Cut Rows Below | | `insertRows` |
| Duplicate | Duplicate Rows | | `duplicateRows`, or `insertRows` |
| Insert blank rows | Insert Blank Rows Below | Cmd/Ctrl+= | `insertRows` |
| Delete | Delete Rows | Cmd/Ctrl+− | `removeRows` |
| Hide / show | Hide Rows, Show Hidden Rows | | |

Copied rows are tab-separated text with every column, so rows can come from a spreadsheet; each is padded or truncated to the Grid's column count. **Cut rows are removed only when inserted elsewhere**, in this Grid or another. Inserted rows end up selected.

Delete Rows and Cmd/Ctrl+− act on the selected rows, or on the cursor's row if no whole rows are selected. Cmd/Ctrl+= inserts as many blank rows as are selected, below them, or one below the cursor.

## Undo is the model's

The Grid keeps no undo history. Cmd/Ctrl+Z, Shift+Cmd/Ctrl+Z and Cmd/Ctrl+Y call the model's `undo` and `redo`. Open cell editors have their own undo.

`beginBatch` and `endBatch` bracket one user action, so a multi-cell paste or a row cut-and-paste can be one undo step:

```cpp
{% include_relative examples/04_RowsAndUndo.cpp %}
```

This example snapshots before each change (once per batch) and restores on undo, notifying `Reset`. Larger models can record commands, e.g. with a `juce::UndoManager`.

## From code

`grid.copyRows()`, `cutRows()`, `insertCopiedRows(belowRow)`, `duplicateSelectedRows()`, `deleteSelectedRows()`, `insertBlankRows()`, `getSelectedRows()`, `undo()` and `redo()`.
