---
title: Sorting
nav_order: 6
---

# Sorting
{: .no_toc }

1. TOC
{:toc}

## For the user

Each column header has a sort button at its right end: the sorted column always shows its direction, and other columns show faint arrows on hover. Click once for ascending, again for descending. The cell and column menus have Sort Ascending and Sort Descending too, and VoiceOver users can press a header (VO+Space), reached with VO+Shift+|.

Sorting is **stable**, so to sort by two columns you sort by the secondary column first, then the primary, as in Excel. A sort is a one-off action: later edits and inserts never move rows, and the header's arrow disappears once the order has been disturbed.

## For the model: the data never moves

The Grid works out the sorted order and hands it to `reorderRows`. Your model keeps that order, usually as a `RowOrder`, a map from view rows to stored rows, so the data itself never has to move. The order is part of the model, so it can be saved and undone like anything else.

![Storage rows stay put; a RowOrder maps each view row to a stored row]({{ "/images/row-order.svg" | relative_url }})

```cpp
{% include_relative examples/03_Sorting.cpp %}
```

- Every callback translates the view row first: `order.toStorage(row)`.
- `reorderRows` applies the new order and notifies `RowsReordered` with it.
- `compare` is optional. Without it the Grid uses `compareValues()`: numbers numerically, text naturally and case-insensitively, empty values first.
- To keep a sort, save `order.getOrder()` and restore it with `order.setOrder(...)`.

A [`ViewModel`]({{ "/multiple-views" | relative_url }}) does all of this for you.

## From code

`grid.sortByColumn(col, descending)`, `grid.toggleSort(col)`, `grid.canSort()` and `grid.getSortIndicator()`.
