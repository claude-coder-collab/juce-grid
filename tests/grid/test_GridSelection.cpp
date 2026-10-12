#include "GridTestSupport.h"

#include <catch2/catch_test_macros.hpp>

using jgrid::Cell;
using jgrid::Grid;
using jgrid::test::EditableTable;
using jgrid::test::key;
using jgrid::test::makeEditableModel;

namespace
{

std::shared_ptr<EditableTable> selectableGrid(Grid& grid)
{
    auto table = std::make_shared<EditableTable>(10, 4);
    grid.setModel(makeEditableModel(table));
    grid.setSize(600, 300);
    return table;
}

} // namespace

TEST_CASE("Shift+arrows extend the selection and keep the cursor", "[selection]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    selectableGrid(grid);
    grid.setCursor({ .row = 1, .col = 1 });

    CHECK(grid.keyPressed(key(juce::KeyPress::downKey, juce::ModifierKeys::shiftModifier)));
    CHECK(grid.keyPressed(key(juce::KeyPress::rightKey, juce::ModifierKeys::shiftModifier)));
    CHECK(grid.getCursor() == Cell{ .row = 1, .col = 1 });
    CHECK(grid.getSelection().getNumCells() == 4);
    CHECK(grid.getSelection().contains({ .row = 2, .col = 2 }));

    CHECK(grid.keyPressed(key(juce::KeyPress::downKey)));
    CHECK(grid.getCursor() == Cell{ .row = 2, .col = 1 });
    CHECK(grid.getSelection().isSingleCell());
}

TEST_CASE("Cmd/Ctrl+arrows jump to the edges", "[selection]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    selectableGrid(grid);
    grid.setCursor({ .row = 4, .col = 1 });

    CHECK(grid.keyPressed(key(juce::KeyPress::downKey, juce::ModifierKeys::commandModifier)));
    CHECK(grid.getCursor() == Cell{ .row = 9, .col = 1 });
    CHECK(grid.keyPressed(key(juce::KeyPress::rightKey, juce::ModifierKeys::commandModifier)));
    CHECK(grid.getCursor() == Cell{ .row = 9, .col = 3 });
    CHECK(grid.keyPressed(key(juce::KeyPress::upKey, juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier)));
    CHECK(grid.getSelection().isWholeColumnSelected(3, 10));
}

TEST_CASE("Enter and Tab cycle through a multi-cell selection", "[selection]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    selectableGrid(grid);
    grid.setCursor({ .row = 0, .col = 0 });
    grid.extendSelection({ .row = 1, .col = 1 });

    CHECK(grid.keyPressed(key(juce::KeyPress::returnKey)));
    CHECK(grid.getCursor() == Cell{ .row = 1, .col = 0 });
    CHECK(grid.keyPressed(key(juce::KeyPress::returnKey)));
    CHECK(grid.getCursor() == Cell{ .row = 0, .col = 1 });
    CHECK(grid.keyPressed(key(juce::KeyPress::tabKey)));
    CHECK(grid.getCursor() == Cell{ .row = 1, .col = 0 });
    CHECK(grid.getSelection().getNumCells() == 4);
}

TEST_CASE("Committing an edit in a selection moves to the next selected cell", "[selection]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    const auto table = selectableGrid(grid);
    grid.setCursor({ .row = 0, .col = 0 });
    grid.extendSelection({ .row = 0, .col = 1 });

    REQUIRE(grid.startEditingWithText("x"));
    CHECK(grid.getEditor()->keyPressed(key(juce::KeyPress::tabKey)));
    CHECK(table->at(0, 0) == "x");
    CHECK(grid.getCursor() == Cell{ .row = 0, .col = 1 });
    CHECK(grid.getSelection().getNumCells() == 2);
}

TEST_CASE("Select all selects every cell", "[selection]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    selectableGrid(grid);
    CHECK(grid.keyPressed(juce::KeyPress('a', juce::ModifierKeys::commandModifier, 0)));
    CHECK(grid.getSelection().getNumCells() == 40);
    CHECK(grid.getCursor() == Cell{});
}

TEST_CASE("Selecting rows and columns places the cursor in them", "[selection]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    selectableGrid(grid);

    grid.selectColumns(1, 2);
    CHECK(grid.getSelection().isWholeColumnSelected(1, 10));
    CHECK(grid.getSelection().isWholeColumnSelected(2, 10));
    CHECK(grid.getCursor() == Cell{ .row = 0, .col = 1 });

    grid.selectRows(5, 3, true);
    CHECK(grid.getSelection().isWholeRowSelected(4, 4));
    CHECK(grid.getSelection().isWholeColumnSelected(1, 10));
    CHECK(grid.getCursor() == Cell{ .row = 5, .col = 0 });
}

TEST_CASE("Cmd/Ctrl-click adds a range", "[selection]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    selectableGrid(grid);
    grid.setCursor({ .row = 0, .col = 0 });
    grid.addToSelection({ .row = 5, .col = 3 });
    CHECK(grid.getCursor() == Cell{ .row = 5, .col = 3 });
    CHECK(grid.getSelection().getNumCells() == 2);
}

TEST_CASE("Row notifications carry the selection", "[selection]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    const auto table = selectableGrid(grid);
    grid.setCursor({ .row = 2, .col = 0 });
    grid.extendSelection({ .row = 3, .col = 1 });

    table->cells.insert(table->cells.begin(), { "a", "b", "c", "d" });
    table->notifier.changed(jgrid::RowsInserted{ .at = 0, .count = 1 });
    CHECK(grid.getSelection().contains({ .row = 4, .col = 1 }));
    CHECK_FALSE(grid.getSelection().contains({ .row = 2, .col = 0 }));

    table->notifier.changed(jgrid::Reset{});
    CHECK(grid.getSelection().isSingleCell());
}
