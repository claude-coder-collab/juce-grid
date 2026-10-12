# JUCE Grid

**[Guide and API reference](https://claude-coder-collab.github.io/juce-grid/)**

A spreadsheet-style grid component for [JUCE 8](https://juce.com), inspired by wxPython's Grid: a keyboard-first, accessible replacement for `TableListBox` that you bind to your own data through a small set of callbacks.

## Features

- Bind any data with a `GridModel` struct of `std::function`s: no base class, and the Grid is not a template.
- Excel-style cursor and range selection, with the full keyboard: arrows, Page, Home/End, Tab/Enter, Shift to extend, Ctrl/Cmd+A, Ctrl+Tab to leave.
- In-cell text editing (F2, double-click or just type, including dead keys and input methods), with undo/redo in the editor and model-side validation shown in a non-modal popup.
- Copy, cut, paste and delete as tab-separated text that works with Excel, Numbers and Google Sheets; whole-row cut, copy, insert, duplicate and delete.
- Sorting through header sort buttons, kept in the model's own row order; several Grids can view one dataset with independent orders and column subsets (`DataSource` + `ViewModel`).
- Resizable, auto-fitting and hideable rows and columns; context menus the model can extend; Grid-level undo/redo forwarded to the model.
- Screen reader support through JUCE's accessibility layer (tested with VoiceOver).
- Colours customisable per Grid, per parent or per LookAndFeel.

## Using it

The Grid is a JUCE module in `modules/jgrid`. Add it to a JUCE CMake project with:

```cmake
juce_add_module(path/to/juce-grid/modules/jgrid)
target_link_libraries(MyApp PRIVATE jgrid juce::juce_gui_basics)
```

Then bind a model:

```cpp
#include <jgrid/jgrid.h>

std::vector<std::vector<juce::String>> data = { { "Alex", "Piano" }, { "Sam", "Bass" } };
jgrid::GridModelNotifier notifier;

jgrid::GridModel model;
model.rowCount = [&] { return (int) data.size(); };
model.colCount = [] { return 2; };
model.getValue = [&](int row, int col) { return juce::var(data[(size_t) row][(size_t) col]); };
model.setValue = [&](int row, int col, const juce::var& value) {
    data[(size_t) row][(size_t) col] = value.toString();
    notifier.changed(jgrid::CellsChanged{ row, 1, col, 1 });
    return true;
};
model.colName = [](int col) { return col == 0 ? "Name" : "Instrument"; };
model.notifier = &notifier;

jgrid::Grid grid;
grid.setModel(model);
```

Everything else (validation, sorting, row operations, undo, menus) is optional. The [guide](https://claude-coder-collab.github.io/juce-grid/) explains each part, `docs/examples/` has small complete examples, and `demo/` is a full application. `SPEC.md` is the design specification.

## Building the demo and tests

Requirements: CMake 3.28+, Ninja, and a C++26 compiler (GCC 14, a recent Clang or Apple Clang, or Visual Studio 2022). JUCE 8 and Catch2 are fetched automatically. On Linux, JUCE needs the usual X11, FreeType and Fontconfig development packages; see `.github/workflows/ci.yml` for the list. Projects using the module need C++23.

```sh
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
```

`tools/check.py` runs the full local check: formatting, the dev build and tests, AddressSanitizer/UndefinedBehaviorSanitizer, ThreadSanitizer and clang-tidy.

## Platforms

The Grid targets Windows, macOS and Linux. It is developed on macOS; Windows and Linux are built and tested in CI.

## Licence

JUCE Grid is released under the [MIT licence](LICENSE).

It depends on JUCE, which is dual-licensed under the [AGPLv3](https://www.gnu.org/licenses/agpl-3.0.en.html) and the commercial [JUCE licence](https://juce.com/legal/juce-8-licence/). JUCE is not included in this repository, but any application you build with JUCE Grid also uses JUCE and must comply with one of JUCE's licences.
