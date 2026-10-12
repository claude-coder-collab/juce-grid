---
title: API reference
nav_order: 13
---

# API reference

The full API reference is generated from the headers' documentation comments with Doxygen.

[Open the API reference]({{ "/api/" | relative_url }}){: .btn .btn-primary }

The main entry points:

| Class | What it is |
|---|---|
| `jgrid::Grid` | The component. |
| `jgrid::GridModel` | The callbacks that connect a Grid to your data. |
| `jgrid::GridModelNotifier` | How your model tells Grids what changed. |
| `jgrid::Change` | The notifications: `CellsChanged`, `RowsInserted`, `RowsRemoved`, `RowsReordered`, `Reset`. |
| `jgrid::RowOrder` | A view order over stored rows, for sorting without moving data. |
| `jgrid::DataSource`, `jgrid::ViewModel` | One dataset, several independent views. |
| `jgrid::Selection`, `jgrid::Cell`, `jgrid::CellRange` | The selection model. |
