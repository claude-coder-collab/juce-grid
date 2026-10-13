---
title: Home
layout: home
nav_order: 1
---

# JUCE Grid

A spreadsheet-style, keyboard-first and accessible grid component for [JUCE 8](https://juce.com). It replaces `TableListBox` when you need cells you can navigate, select, edit, copy, paste and sort.

![The demo app: a session track list in a Grid, with the cursor on a cell]({{ "/images/overview.png" | relative_url }})

You keep your data where it is. The Grid asks for it through a handful of callbacks, a `GridModel`, and you tell it when something changed. Everything else (the cursor, selection, scrolling, editing, clipboard, sorting, context menus, keyboard and screen readers) is built in.

```cpp
jgrid::GridModel model;
model.rowCount = [&] { return static_cast<int>(rows.size()); };
model.colCount = [] { return 3; };
model.getValue = [&](int row, int col) { return rows[(size_t) row][(size_t) col]; };

grid.setModel(model);
```

## What you get

| Feature | What it means |
|---|---|
| **Your data, your way** | Bind any data with a struct of `std::function`s. No base class, and the Grid is not a template. |
| **Spreadsheet keyboard** | Arrows, Page Up/Down, Home/End, Tab and Enter, Shift to extend, Cmd/Ctrl+A, Ctrl+Tab to leave. |
| **In-cell editing** | F2, double-click or just type, including dead keys and input methods. Undo inside the editor, validation messages that never block. |
| **Clipboard** | Copy, cut, paste and delete as tab-separated text that works with Excel, Numbers and Google Sheets. |
| **Rows** | Cut, copy, insert, duplicate and delete whole rows, from menus or with Cmd/Ctrl+= and Cmd/Ctrl+−. |
| **Sorting** | Header sort buttons that reorder your model's view, never your data. |
| **Several views** | Several Grids on one dataset, each with its own sort order and columns. |
| **Layout** | Resizable, auto-fitting and hideable rows and columns. |
| **Accessible** | Screen reader support through JUCE's accessibility layer (tested with VoiceOver). |

## Where to go next

1. [Getting started]({{ "/getting-started" | relative_url }}): add the module and show your first Grid.
2. [How the pieces fit]({{ "/concepts" | relative_url }}): what the Grid does, what your model does, and what `ViewModel` does for you.
3. [The model]({{ "/model" | relative_url }}): every callback and notification, and when to use each.
4. The topic pages on editing, sorting, rows, views, selection, layout, the keyboard and accessibility.
5. The [API reference]({{ "/api/" | relative_url }}), generated from the headers.
