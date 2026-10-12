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

Copied rows travel as tab-separated text, every column included, so you can paste rows from a spreadsheet too: each row is padded or truncated to the Grid's column count. **Cut rows are removed only when they are inserted somewhere**, in the same Grid or another; a cut that is never pasted loses nothing. Inserted rows end up selected.

Delete Rows and Cmd/Ctrl+− act on the selected rows, or on the cursor's row if no whole rows are selected. Cmd/Ctrl+= inserts as many blank rows as are selected, below them, or one below the cursor.

## Undo is the model's

The Grid has no undo history. Cmd/Ctrl+Z, Shift+Cmd/Ctrl+Z and Cmd/Ctrl+Y call the model's `undo` and `redo`, and the model decides what they mean. (Cell editors have their own undo while they are open.)

`beginBatch` and `endBatch` tell the model where one user action starts and ends, so a paste of 50 cells or a cut-and-paste of rows can be one undo step:

```cpp
{% include_relative examples/04_RowsAndUndo.cpp %}
```

This example remembers a snapshot before each change, or once per batch, and restores it on undo, notifying `Reset`. A larger model would record commands instead, for example with a `juce::UndoManager`.

## From code

`grid.copyRows()`, `cutRows()`, `insertCopiedRows(belowRow)`, `duplicateSelectedRows()`, `deleteSelectedRows()`, `insertBlankRows()`, `getSelectedRows()`, `undo()` and `redo()`.
