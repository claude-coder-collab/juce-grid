---
title: Several views
nav_order: 8
---

# Several views of one dataset
{: .no_toc }

1. TOC
{:toc}

Several Grids can show the same data. Each keeps its own cursor, selection, sizes and hidden rows; an edit in one shows up in all of them.
{: .fs-5 .fw-300 }

## Sharing one model

The simplest case: bind the same `GridModel` to two Grids. Both subscribe to its notifier, so they stay in step. They also share the model's row order, so sorting one sorts both.

## Independent views: DataSource and ViewModel

To give each Grid its **own** sort order or its **own columns**, describe your data once as a `DataSource`, in storage terms, and give each Grid a `ViewModel`:

![One DataSource feeding two ViewModels, each feeding its own Grid]({{ "/images/multiple-views.svg" | relative_url }})

```cpp
{% include_relative examples/05_MultipleViews.cpp %}
```

What each part does:

| | DataSource (you write) | ViewModel (provided) | Grid |
|---|---|---|---|
| Rows | storage rows, never reordered | its own `RowOrder`: view row → storage row | view rows |
| Columns | all source columns | an optional list of source columns, in any order | view columns |
| Sorting | optional `compare` | implements `reorderRows` | sort buttons |
| Changes | notifies in storage indices | translates them to view indices | updates |
| Inserted rows | appends to storage | places them where the user inserted them; other views get them at the end | |

![Two Grids on one dataset: the top one sorted by Track, the bottom one showing three columns sorted by Name]({{ "/images/views.png" | relative_url }})

The `DataSource` must have a notifier. To save a view's sort, keep `view.getRowOrder().getOrder()` and restore it with `view.setRowOrder(...)`; `view.setColumns(...)` changes the columns.
