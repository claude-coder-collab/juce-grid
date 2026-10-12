---
title: Layout and appearance
nav_order: 10
---

# Layout and appearance
{: .no_toc }

1. TOC
{:toc}

## Sizes and hiding

![Notes widened, Pan hidden, one row taller, and two whole rows selected]({{ "/images/layout.png" | relative_url }})

- **Resize** a column or row by dragging its header edge; **double-click** the edge to fit its content. From code: `setColumnWidth()`, `setRowHeight()`, `autoFitColumn()`, `autoFitRow()`.
- **Hide** rows or columns from the header context menu; Show Hidden Columns lists each hidden column so you can bring back just one. From code: `setColumnsHidden()`, `setRowsHidden()`, `showAllColumns()`, `showAllRows()`.
- Sizes and hidden state belong to the Grid, not the model. Row heights and hidden rows follow their rows when rows are inserted, removed or sorted.
- Headers can be switched off: `setRowHeaderVisible(false)`, `setColumnHeaderVisible(false)`.

## Colours

The Grid takes its look from its parent's LookAndFeel. Set any of these with `setColour()` on the Grid, any parent component, or the LookAndFeel:

| Colour ID | Used for | Falls back to |
|---|---|---|
| `backgroundColourId` | cell background | `ListBox::backgroundColourId` |
| `textColourId` | cell text | `ListBox::textColourId` |
| `headerBackgroundColourId` | header background | `TableHeaderComponent::backgroundColourId` |
| `headerTextColourId` | header text, sort arrows | `TableHeaderComponent::textColourId` |
| `gridLineColourId` | lines between cells | `TableHeaderComponent::outlineColourId` |
| `cursorColourId` | cursor outline, cut marks | `TextEditor::highlightColourId`, opaque |
| `selectionColourId` | selected cells, header tint | `TextEditor::highlightColourId` |

![The demo with a high-contrast colour scheme]({{ "/images/contrast.png" | relative_url }})

## Context menu items

Your model can add items to any context menu, or change the built-in ones, with `contextMenu`:

```cpp
{% include_relative examples/06_MenusAndColours.cpp %}
```
