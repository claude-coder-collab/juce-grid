# JUCE Grid

**[Guide and API reference](https://claude-coder-collab.github.io/juce-grid/)**

Spreadsheet-style grid component for [JUCE 8](https://juce.com), replacing `TableListBox`. You bind it to your data with a `GridModel`, a struct of callbacks.

## Features

- Cursor and range selection, full keyboard navigation.
- In-cell editing with model-side validation.
- Copy, cut, paste and delete as tab-separated text (Excel, Numbers, Google Sheets compatible).
- Whole-row cut, copy, insert, duplicate and delete.
- Sorting that leaves your data in place.
- Several Grids on one dataset, each with its own sort order and columns (`DataSource` + `ViewModel`).
- Resizable, auto-fitting and hideable rows and columns.
- Context menus the model can extend.
- Undo and redo forwarded to the model.
- Screen reader support through JUCE's accessibility layer (tested with VoiceOver).
- Colours set per Grid, per parent or per LookAndFeel.

## Using it

The Grid is a JUCE module in `modules/jgrid`. Add it to a JUCE CMake project with:

```cmake
juce_add_module(path/to/juce-grid/modules/jgrid)
target_link_libraries(MyApp PRIVATE jgrid juce::juce_gui_basics)
```

Bind a model:

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

All other callbacks are optional. See the [guide](https://claude-coder-collab.github.io/juce-grid/), the examples in `docs/examples/`, and the demo application in `demo/`. `SPEC.md` is the design specification.

## Building the demo and tests

Requires CMake 3.28+, Ninja and a C++26 compiler (GCC 14, recent Clang or Apple Clang, Visual Studio 2022). JUCE 8 and Catch2 are fetched automatically. On Linux, install the X11, FreeType and Fontconfig development packages (list in `.github/workflows/ci.yml`). Code using the module needs C++23.

```sh
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
```

`tools/check.py` runs formatting, the dev build and tests, ASan/UBSan, TSan and clang-tidy.

## Platforms

Windows, macOS and Linux. Developed on macOS; Windows and Linux are built and tested in CI.

## Licence

[MIT](LICENSE). JUCE is not included, and applications using JUCE Grid must comply with one of JUCE's licences: [AGPLv3](https://www.gnu.org/licenses/agpl-3.0.en.html) or the commercial [JUCE licence](https://juce.com/legal/juce-8-licence/).
