---
title: Selection and clipboard
nav_order: 9
---

# Selection and clipboard

## Cursor and selection

There is one **cursor** cell (heavy outline) that the keyboard acts on, and a **selection** of one or more rectangular ranges.

![A range selection marked as cut, with the dashed cut outline]({{ "/images/selection.png" | relative_url }})

| Action | Result |
|---|---|
| Click, arrows | Move the cursor and collapse the selection to it. |
| Shift+click, Shift+arrows, drag | Extend the selection; the cursor stays put. |
| Cmd/Ctrl+click | Add another range. |
| Click or drag row/column headers | Select whole rows or columns. |
| Cmd/Ctrl+A | Select everything. |
| Enter, Tab inside a selection | Move the cursor through the selected cells without losing the selection. |

From code: `getCursor()`, `setCursor()`, `getSelection()`, `extendSelection()`, `addToSelection()`, `selectRows()`, `selectColumns()`, `selectAll()`.

## Copy, cut, paste and delete

The clipboard format is tab-separated text with Excel-style quoting.

- **Copy** (Cmd/Ctrl+C) takes the active range. Hidden rows and columns are skipped.
- **Paste** (Cmd/Ctrl+V) writes from the cursor, clipped at the last row and column. A single value pasted into a multi-cell selection fills all of it. Read-only and hidden cells are skipped.
- **Every target is validated before anything is written.** If any cell fails, nothing changes; the cursor moves to the first failing cell and the popup shows the model's message ("…, and 3 more" if several fail).
- **Cut** (Cmd/Ctrl+X) clears its cells only when pasted. It becomes a plain copy if its cells or the clipboard change; Escape forgets it.
- **Delete** or Backspace clears the selected editable cells, validating first.

Each paste or delete is one `beginBatch`/`endBatch` pair.
