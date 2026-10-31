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

- **Your data** stays where it is. The Grid never copies or owns it.
- **`GridModel`**: a struct of `std::function` callbacks that read and write your data. Set the ones your data supports.
- **`Grid`**: the component. It calls the model to read values when painting, and to write when the user edits, pastes, sorts or changes rows.

After changing data, call `notifier.changed(...)` on the `GridModelNotifier` your model owns. Every Grid bound to the model updates.

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

**Data, including row order, belongs to the model. How it is viewed belongs to the Grid.** So for sorting, the Grid computes the order and the model stores it.

## Rows are in view order

Row indices in callbacks and notifications are **view rows**: positions as the Grid shows them. Without sorting, view rows equal storage rows.

With sorting, keep a [`RowOrder`]({{ "/sorting" | relative_url }}) and translate: `data[order.toStorage(row)]`. A [`ViewModel`]({{ "/multiple-views" | relative_url }}) does this for you.

## When you need a ViewModel

Not for a single Grid without sorting. Use `DataSource` and `ViewModel` when:

- you want sorting without managing a `RowOrder`, or
- several Grids share data with their own sort orders or column subsets.

Describe the data once as a `DataSource` in storage terms, and give each Grid a `ViewModel`. See [Several views]({{ "/multiple-views" | relative_url }}).

## Threading and lifetime

- Everything runs on the message thread, including `notifier.changed()`. Post notifications from other threads to it.
- Captured state must outlive the Grid, or call `grid.setModel({})` first.
- The notifier may be destroyed before or after the Grid. If it goes first, the Grid shows as empty.
