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

JUCE Grid is a JUCE module in `modules/jgrid`. Add it with `juce_add_module` and link it:

```cmake
juce_add_module(path/to/juce-grid/modules/jgrid)

target_link_libraries(MyApp
    PRIVATE
        jgrid
        juce::juce_gui_basics)
```

Only the CMake route is tested; the Projucer should also accept `modules/jgrid`.

Include the header:

```cpp
#include <jgrid/jgrid.h>
```

## Your first Grid

A Grid needs `rowCount`, `colCount` and `getValue`. `colName` is optional and sets the header titles.

```cpp
{% include_relative examples/01_ReadOnly.cpp %}
```

The Grid is a `juce::Component`. Add it and set its bounds.

{: .note }
Anything a callback captures (here, `tracks`) must outlive the Grid, or call `grid.setModel({})` first. Declare the data before the Grid in the same class.

## Making it editable

Add `setValue` to make cells editable. Add a `GridModelNotifier` so the Grid hears about changes.

```cpp
{% include_relative examples/02_Editable.cpp %}
```

`setValue` always receives a string. Convert it (here to a `double`) in the model. `validate` rejects bad input; the Grid shows the message and keeps the editor open.

![An edit rejected by validation: the editor stays open and a popup shows the model's message]({{ "/images/editing.png" | relative_url }})

## Next

- [How the pieces fit]({{ "/concepts" | relative_url }})
- [The model]({{ "/model" | relative_url }})
- `demo/` in the repository: a complete application with editing, sorting, two views, row operations and undo.
