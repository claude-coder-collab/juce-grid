---
title: Getting started
nav_order: 2
---

# Getting started
{: .no_toc }

1. TOC
{:toc}

## Requirements

- JUCE 8, with the `juce_gui_basics` module.
- A C++23 compiler for code that uses the module: recent Clang or Apple Clang, GCC 14, or Visual Studio 2022.
- Windows, macOS or Linux.

## Add the module

JUCE Grid is a JUCE module in `modules/jgrid`. In a CMake project that already uses JUCE, add it with `juce_add_module` and link it:

```cmake
juce_add_module(path/to/juce-grid/modules/jgrid)

target_link_libraries(MyApp
    PRIVATE
        jgrid
        juce::juce_gui_basics)
```

The module follows JUCE's module format, so the Projucer should also accept `modules/jgrid` as a module; only the CMake route is tested.

Then include the module header wherever you use the Grid:

```cpp
#include <jgrid/jgrid.h>
```

## Your first Grid

A Grid needs three callbacks: how many rows, how many columns, and the value of a cell. This component shows a read-only list of tracks; `colName` gives the columns proper headers.

```cpp
{% include_relative examples/01_ReadOnly.cpp %}
```

The Grid is an ordinary `juce::Component`: add it, give it bounds, and it draws and handles input itself. Click a cell or Tab into the Grid, and the arrow keys move the cursor.

{: .note }
Everything a callback captures (here, `tracks`) must live at least as long as the Grid, or you must unbind first with `grid.setModel({})`. Declaring the data before the Grid in the same class, as above, gets the order right.

## Making it editable

Add `setValue`, and the cells become editable: F2, double-click or typing opens an editor, and Enter or Tab stores the text. Add a `GridModelNotifier` so the Grid hears about the change.

```cpp
{% include_relative examples/02_Editable.cpp %}
```

Text editing always hands your model a string. Converting it (here to a `double`) and rejecting bad input (`validate`) are the model's job; the Grid shows the message and keeps the editor open.

![An edit rejected by validation: the editor stays open and a popup shows the model's message]({{ "/images/editing.png" | relative_url }})

## Next

- [How the pieces fit]({{ "/concepts" | relative_url }}) explains who is responsible for what.
- [The model]({{ "/model" | relative_url }}) lists every callback.
- The repository's `demo/` folder is a complete application with editing, sorting, two views, row operations and undo.
