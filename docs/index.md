---
title: Home
layout: home
nav_order: 1
---

# JUCE Grid

Keyboard-first, accessible grid component for [JUCE 8](https://juce.com), replacing `TableListBox`. Cells can be navigated, selected, edited, copied, pasted and sorted.

![The demo app: a session track list in a Grid, with the cursor on a cell]({{ "/images/overview.png" | relative_url }})

Your data stays where it is. The Grid reads and writes it through the callbacks of a `GridModel`, and you notify it when the data changes.

```cpp
jgrid::GridModel model;
model.rowCount = [&] { return static_cast<int>(rows.size()); };
model.colCount = [] { return 3; };
model.getValue = [&](int row, int col) { return rows[(size_t) row][(size_t) col]; };

grid.setModel(model);
```

## Contents

1. [Getting started]({{ "/getting-started" | relative_url }}): add the module, show a Grid.
2. [How the pieces fit]({{ "/concepts" | relative_url }}): who does what.
3. [The model]({{ "/model" | relative_url }}): every callback and notification.
4. Topic pages: editing, sorting, rows, several views, selection, layout, keyboard, accessibility.
5. [API reference]({{ "/api/" | relative_url }}), generated from the headers.
