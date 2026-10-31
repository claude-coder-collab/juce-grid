---
title: Sorting
nav_order: 6
---

# Sorting
{: .no_toc }

1. TOC
{:toc}

## For the user

Click a header's sort button for ascending, again for descending. The cell and column menus have Sort Ascending and Sort Descending. With VoiceOver, press a header (VO+Space; reach headers with VO+Shift+|).

Sorting is **stable**: to sort by two columns, sort by the secondary column first, then the primary. A sort is one-off: later edits and inserts never move rows, and the header arrow disappears once the order is disturbed.

## For the model: the data never moves

The Grid computes the sorted order and passes it to `reorderRows`. The model stores it, usually as a `RowOrder` (view row to stored row), so data never moves.

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
