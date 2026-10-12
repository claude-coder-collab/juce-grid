#include "GridTestSupport.h"

#include <catch2/catch_test_macros.hpp>

using jgrid::Cell;
using jgrid::Grid;
using jgrid::test::EditableTable;
using jgrid::test::key;
using jgrid::test::makeEditableModel;

namespace
{

std::shared_ptr<EditableTable> clipboardGrid(Grid& grid, int rows = 5, int cols = 3)
{
    auto table = std::make_shared<EditableTable>(rows, cols);
    grid.setModel(makeEditableModel(table));
    grid.setSize(600, 300);
    return table;
}

void selectRange(Grid& grid, Cell from, Cell to)
{
    grid.setCursor(from);
    grid.extendSelection(to);
}

juce::KeyPress commandKey(char c) { return { c, juce::ModifierKeys::commandModifier, 0 }; }

} // namespace

TEST_CASE("Copy puts the active range on the clipboard as TSV", "[clipboard]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    clipboardGrid(grid);
    selectRange(grid, { .row = 0, .col = 1 }, { .row = 1, .col = 2 });

    CHECK(grid.keyPressed(commandKey('c')));
    CHECK(juce::SystemClipboard::getTextFromClipboard() == "r0c1\tr0c2\nr1c1\tr1c2\n");
}

TEST_CASE("Paste writes at the cursor, clips, and selects what it wrote", "[clipboard]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    const auto table = clipboardGrid(grid);
    juce::SystemClipboard::copyTextToClipboard("a\tb\tc\nd\te\tf\n");
    grid.setCursor({ .row = 3, .col = 1 });

    CHECK(grid.keyPressed(commandKey('v')));
    CHECK(table->at(3, 1) == "a");
    CHECK(table->at(3, 2) == "b");
    CHECK(table->at(4, 1) == "d");
    CHECK(table->at(4, 2) == "e");
    CHECK(table->batchesBegun == 1);
    CHECK(table->batchesEnded == 1);
    CHECK(grid.getSelection().getNumCells() == 4);
    CHECK(grid.getCursor() == Cell{ .row = 3, .col = 1 });
}

TEST_CASE("A single value fills the selection", "[clipboard]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    const auto table = clipboardGrid(grid);
    juce::SystemClipboard::copyTextToClipboard("z");
    selectRange(grid, { .row = 0, .col = 0 }, { .row = 2, .col = 1 });

    CHECK(grid.paste());
    for (auto row = 0; row < 3; ++row)
        for (auto col = 0; col < 2; ++col)
            CHECK(table->at(row, col) == "z");
    CHECK(table->at(0, 2) == "r0c2");
}

TEST_CASE("A paste that fails validation writes nothing", "[clipboard]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    const auto table = clipboardGrid(grid);
    table->validate = [](int, int, const juce::var& value) { return value.toString() == "bad" ? juce::Result::fail("Not allowed") : juce::Result::ok(); };
    juce::SystemClipboard::copyTextToClipboard("ok\tbad\nbad\tok\n");

    CHECK_FALSE(grid.paste());
    CHECK(table->sets == 0);
    CHECK(grid.getCursor() == Cell{ .row = 0, .col = 1 });
    CHECK(grid.getErrorMessage() == "Not allowed, and 1 more");
    CHECK(table->batchesBegun == 0);
}

TEST_CASE("A failing setValue stops the paste but ends the batch", "[clipboard]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    const auto table = clipboardGrid(grid);
    table->rejectSet = [](int row, int col) { return row == 0 && col == 1; };
    juce::SystemClipboard::copyTextToClipboard("a\tb\tc\n");

    CHECK_FALSE(grid.paste());
    CHECK(table->at(0, 0) == "a");
    CHECK(table->at(0, 2) == "r0c2");
    CHECK(table->batchesBegun == 1);
    CHECK(table->batchesEnded == 1);
    CHECK(grid.getErrorMessage() == jgrid::couldNotSetValueMessage());
    CHECK(grid.getCursor() == Cell{ .row = 0, .col = 1 });
}

TEST_CASE("Paste skips read-only cells", "[clipboard]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    const auto table = clipboardGrid(grid);
    table->isEditable = [](int, int col) { return col != 1; };
    juce::SystemClipboard::copyTextToClipboard("a\tb\tc\n");

    CHECK(grid.paste());
    CHECK(table->at(0, 0) == "a");
    CHECK(table->at(0, 1) == "r0c1");
    CHECK(table->at(0, 2) == "c");
}

TEST_CASE("Delete clears selected editable cells", "[clipboard]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    const auto table = clipboardGrid(grid);
    table->isEditable = [](int, int col) { return col != 0; };
    selectRange(grid, { .row = 0, .col = 0 }, { .row = 1, .col = 1 });

    CHECK(grid.keyPressed(key(juce::KeyPress::backspaceKey)));
    CHECK(table->at(0, 0) == "r0c0");
    CHECK(table->at(0, 1).isEmpty());
    CHECK(table->at(1, 1).isEmpty());
    CHECK(table->batchesBegun == 1);
}

TEST_CASE("Delete that fails validation clears nothing", "[clipboard]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    const auto table = clipboardGrid(grid);
    table->validate = [](int, int col, const juce::var& value) { return col == 1 && value.toString().isEmpty() ? juce::Result::fail("Required") : juce::Result::ok(); };
    selectRange(grid, { .row = 0, .col = 0 }, { .row = 0, .col = 2 });

    CHECK_FALSE(grid.deleteSelection());
    CHECK(table->sets == 0);
    CHECK(grid.getErrorMessage() == "Required");
}

TEST_CASE("Cut clears its cells only when pasted", "[clipboard]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    const auto table = clipboardGrid(grid);
    selectRange(grid, { .row = 0, .col = 0 }, { .row = 0, .col = 1 });

    CHECK(grid.keyPressed(commandKey('x')));
    REQUIRE(grid.getCutRange());
    CHECK(table->at(0, 0) == "r0c0");

    grid.setCursor({ .row = 0, .col = 1 });
    CHECK(grid.paste());
    CHECK(table->at(0, 1) == "r0c0");
    CHECK(table->at(0, 2) == "r0c1");
    CHECK(table->at(0, 0).isEmpty());
    CHECK_FALSE(grid.getCutRange());
    CHECK(table->batchesBegun == 1);
}

TEST_CASE("Cut and paste works across Grids", "[clipboard]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid a;
    Grid b;
    const auto source = clipboardGrid(a);
    const auto destination = clipboardGrid(b);
    a.setCursor({ .row = 2, .col = 2 });
    a.cut();
    b.setCursor({ .row = 4, .col = 0 });

    CHECK(b.paste());
    CHECK(destination->at(4, 0) == "r2c2");
    CHECK(source->at(2, 2).isEmpty());
    CHECK_FALSE(a.getCutRange());
}

TEST_CASE("A cut becomes a copy when its cells change or the clipboard changes", "[clipboard]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    const auto table = clipboardGrid(grid);
    grid.setCursor({ .row = 1, .col = 1 });

    SECTION("Changed cells")
    {
        grid.cut();
        table->notifier.changed(jgrid::CellsChanged{ .firstRow = 1, .numRows = 1, .firstCol = 1, .numCols = 1 });
        CHECK_FALSE(grid.getCutRange());
    }
    SECTION("Changes elsewhere keep the cut")
    {
        grid.cut();
        table->notifier.changed(jgrid::CellsChanged{ .firstRow = 3, .numRows = 1, .firstCol = 0, .numCols = 3 });
        CHECK(grid.getCutRange());
    }
    SECTION("Changed clipboard")
    {
        grid.cut();
        juce::SystemClipboard::copyTextToClipboard("other");
        grid.setCursor({ .row = 4, .col = 0 });
        CHECK(grid.paste());
        CHECK(table->at(1, 1) == "r1c1");
        CHECK_FALSE(grid.getCutRange());
    }
    SECTION("Escape")
    {
        grid.cut();
        CHECK(grid.keyPressed(key(juce::KeyPress::escapeKey)));
        CHECK_FALSE(grid.getCutRange());
        CHECK_FALSE(grid.keyPressed(key(juce::KeyPress::escapeKey)));
    }
}

TEST_CASE("A cut whose source cannot be cleared pastes nothing", "[clipboard]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    const auto table = clipboardGrid(grid);
    table->validate = [](int, int col, const juce::var& value) { return col == 0 && value.toString().isEmpty() ? juce::Result::fail("Required") : juce::Result::ok(); };
    grid.setCursor({ .row = 0, .col = 0 });
    grid.cut();
    grid.setCursor({ .row = 0, .col = 2 });

    CHECK_FALSE(grid.paste());
    CHECK(table->sets == 0);
    CHECK(grid.getErrorMessage() == "Required");
}
