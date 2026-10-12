---
title: How the pieces fit
nav_order: 3
---

# How the pieces fit
{: .no_toc }

1. TOC
{:toc}

## The three parts

![Your data, the GridModel callbacks and the Grid, with change notifications flowing back to the Grid]({{ "/images/architecture.svg" | relative_url }})

- **Your data** stays wherever it lives: vectors, a database, a document model. The Grid never copies or owns it.
- **The `GridModel`** is the contract between them: a struct of `std::function` callbacks that read and write your data. You fill in the callbacks your data supports.
- **The `Grid`** is the component. It asks the model for values when it paints, and calls the model when the user edits, pastes, sorts or inserts rows.

Changes flow back through a **`GridModelNotifier`** that your model owns. After you change the data, you call `notifier.changed(...)` with what changed, and every Grid bound to that model updates.

## Who is responsible for what

| Concern | Grid | Your model | ViewModel (optional helper) |
|---|---|---|---|
| Storing the data | | ✔ | |
| Converting typed text to your types | | ✔ in `setValue` | |
| Deciding what is valid | | ✔ in `validate` | |
| Deciding what is read-only | | ✔ in `isEditable` | |
| Telling Grids what changed | | ✔ through the notifier | ✔ translates the source's changes |
| Row order (sorting) | computes the sorted order | ✔ applies and keeps it (`reorderRows`) | ✔ keeps a `RowOrder` per view |
| Undo and redo history | forwards the keys | ✔ entirely | forwards to the source |
| Grouping changes into one undo step | calls `beginBatch`/`endBatch` | ✔ decides what to do | forwards |
| Row operations | menus, keys, clipboard rows | ✔ `insertRows`, `removeRows` | ✔ maps view rows to storage |
| Cursor and selection | ✔ | | |
| Scrolling | ✔ | | |
| Column widths, row heights | ✔ | | |
| Hidden rows and columns | ✔ | | |
| Cell editor, validation popup | ✔ | | |
| Clipboard format, cut and paste | ✔ | | |
| Sort indicator in the header | ✔ | | |
| Context menus | ✔ built-in items | ✔ extra items (`contextMenu`) | |
| Keyboard, mouse, screen readers | ✔ | | |
| Colours | ✔ (`setColour`) | | |

The rule of thumb: **anything that is part of the data, including its order, belongs to the model; anything about how it is viewed belongs to the Grid.** That is why sorting is split: the Grid works out the order, but the model keeps it, so the model can save it and undo it.

## Rows are in view order

Every row index the Grid passes to your callbacks, and every row index in your notifications, is a **view row**: the row's position as the Grid shows it. If your model never sorts, view rows and storage rows are the same thing and you can ignore the distinction.

If it does sort, keep a [`RowOrder`]({{ "/sorting" | relative_url }}) and translate: `data[order.toStorage(row)]`. Or let a [`ViewModel`]({{ "/multiple-views" | relative_url }}) do it for you.

## When you need a ViewModel

You don't, for a single Grid without sorting. Reach for `DataSource` and `ViewModel` when:

- you want sorting without writing the `RowOrder` bookkeeping yourself, or
- several Grids show the same data, each with its own sort order or its own subset of columns.

You then describe your data once, in storage terms, as a `DataSource`, and give each Grid a `ViewModel` over it. See [Several views]({{ "/multiple-views" | relative_url }}).

## Threading and lifetime

- Call everything on the message thread, including `notifier.changed()`. If your data changes on another thread, post the notification to the message thread.
- Everything your callbacks capture must outlive the Grid, or unbind first with `grid.setModel({})`.
- The notifier may be destroyed before or after the Grid: either way is safe. If it goes first, the Grid shows as empty.
