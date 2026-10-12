#include "GridTestSupport.h"

#include <catch2/catch_test_macros.hpp>

using jgrid::Cell;
using jgrid::Grid;
using jgrid::test::key;
using jgrid::test::makeModel;
using jgrid::test::TableData;

namespace
{

constexpr int rowHeight = 22;
constexpr int headerHeight = 24;
constexpr int visibleRows = 10;

std::shared_ptr<TableData> sizedGrid(Grid& grid, int rows, int cols)
{
    auto data = std::make_shared<TableData>(TableData{ .rows = rows, .cols = cols });
    grid.setModel(makeModel(data));
    grid.setSize(1000, headerHeight + (visibleRows * rowHeight));
    return data;
}

} // namespace

TEST_CASE("An unbound or incomplete model shows an empty grid", "[grid]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    CHECK(grid.getRowCount() == 0);
    CHECK_FALSE(grid.getCursor().has_value());

    jgrid::GridModel incomplete;
    incomplete.rowCount = [] { return 5; };
    grid.setModel(incomplete);
    CHECK(grid.getRowCount() == 0);
    CHECK_FALSE(grid.getCursor().has_value());
    CHECK_FALSE(grid.keyPressed(key(juce::KeyPress::upKey, juce::ModifierKeys::altModifier)));
    CHECK(grid.keyPressed(key(juce::KeyPress::downKey)));
}

TEST_CASE("Binding a model places the cursor at the first cell", "[grid]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    sizedGrid(grid, 100, 5);
    CHECK(grid.getRowCount() == 100);
    CHECK(grid.getColCount() == 5);
    CHECK(grid.getCursor() == Cell{});
}

TEST_CASE("Arrow keys move the cursor", "[grid]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    sizedGrid(grid, 100, 5);

    CHECK(grid.keyPressed(key(juce::KeyPress::downKey)));
    CHECK(grid.keyPressed(key(juce::KeyPress::rightKey)));
    CHECK(grid.getCursor() == Cell{ .row = 1, .col = 1 });
    CHECK(grid.keyPressed(key(juce::KeyPress::upKey)));
    CHECK(grid.keyPressed(key(juce::KeyPress::leftKey)));
    CHECK(grid.keyPressed(key(juce::KeyPress::leftKey)));
    CHECK(grid.getCursor() == Cell{});
}

TEST_CASE("Page keys move by the number of fully visible rows", "[grid]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    sizedGrid(grid, 100, 5);

    CHECK(grid.keyPressed(key(juce::KeyPress::pageDownKey)));
    CHECK(grid.getCursor() == Cell{ .row = visibleRows, .col = 0 });
    CHECK(grid.keyPressed(key(juce::KeyPress::pageUpKey)));
    CHECK(grid.getCursor() == Cell{});
}

TEST_CASE("Home and end keys reach row and grid edges", "[grid]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    sizedGrid(grid, 100, 5);

    CHECK(grid.keyPressed(key(juce::KeyPress::endKey)));
    CHECK(grid.getCursor() == Cell{ .row = 0, .col = 4 });
    CHECK(grid.keyPressed(key(juce::KeyPress::endKey, juce::ModifierKeys::commandModifier)));
    CHECK(grid.getCursor() == Cell{ .row = 99, .col = 4 });
    CHECK(grid.keyPressed(key(juce::KeyPress::homeKey)));
    CHECK(grid.getCursor() == Cell{ .row = 99, .col = 0 });
    CHECK(grid.keyPressed(key(juce::KeyPress::homeKey, juce::ModifierKeys::commandModifier)));
    CHECK(grid.getCursor() == Cell{});
}

TEST_CASE("Unhandled keys are passed on", "[grid]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    sizedGrid(grid, 10, 5);

    CHECK_FALSE(grid.keyPressed(key(juce::KeyPress::downKey, juce::ModifierKeys::altModifier)));
    CHECK_FALSE(grid.keyPressed(key(juce::KeyPress::pageDownKey, juce::ModifierKeys::commandModifier)));
    CHECK_FALSE(grid.keyPressed(key('a')));
    CHECK(grid.getCursor() == Cell{});
}

TEST_CASE("Moving the cursor scrolls it into view", "[grid]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    sizedGrid(grid, 100, 30);

    grid.setCursor({ .row = 99, .col = 29 });
    const auto bounds = grid.getCellBounds(Cell{ .row = 99, .col = 29 });
    CHECK(grid.getLocalBounds().contains(bounds));
    CHECK(grid.getCellAt(bounds.getCentre()) == Cell{ .row = 99, .col = 29 });

    grid.setCursor({ .row = 0, .col = 0 });
    CHECK(grid.getCellAt(grid.getCellBounds(Cell{}).getCentre()) == Cell{});
}

TEST_CASE("Cursor positions are clamped to the grid", "[grid]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    const auto data = sizedGrid(grid, 10, 5);

    grid.setCursor({ .row = 50, .col = -3 });
    CHECK(grid.getCursor() == Cell{ .row = 9, .col = 0 });

    data->rows = 4;
    grid.refresh();
    CHECK(grid.getCursor() == Cell{ .row = 3, .col = 0 });

    data->rows = 0;
    grid.refresh();
    CHECK_FALSE(grid.getCursor().has_value());
}

TEST_CASE("Cells are found by position under the headers", "[grid]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    sizedGrid(grid, 10, 5);

    const auto first = grid.getCellBounds(Cell{});
    CHECK(first.getY() == headerHeight);
    CHECK(first.getX() > 0);
    CHECK_FALSE(grid.getCellAt({ 1, 1 }).has_value());
    CHECK(grid.getCellAt(first.getCentre()) == Cell{});

    grid.setRowHeaderVisible(false);
    grid.setColumnHeaderVisible(false);
    CHECK_FALSE(grid.isRowHeaderVisible());
    CHECK_FALSE(grid.isColumnHeaderVisible());
    CHECK(grid.getCellBounds(Cell{}).getPosition() == juce::Point<int>{});
    CHECK(grid.getCellAt({ 1, 1 }) == Cell{});
}

TEST_CASE("Colours resolve through the grid, its ancestors and fallbacks", "[grid]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    juce::Component parent;
    Grid grid;
    parent.addAndMakeVisible(grid);

    CHECK(grid.getGridColour(Grid::backgroundColourId) == grid.findColour(juce::ListBox::backgroundColourId));
    CHECK(grid.getGridColour(Grid::selectionColourId) == grid.findColour(juce::TextEditor::highlightColourId));
    CHECK(grid.getGridColour(Grid::cursorColourId) == grid.findColour(juce::TextEditor::highlightColourId).withAlpha(1.0f));

    parent.setColour(Grid::backgroundColourId, juce::Colours::orange);
    CHECK(grid.getGridColour(Grid::backgroundColourId) == juce::Colours::orange);

    grid.setColour(Grid::backgroundColourId, juce::Colours::teal);
    CHECK(grid.getGridColour(Grid::backgroundColourId) == juce::Colours::teal);
}

TEST_CASE("Enter moves down and Shift+Enter moves up", "[grid]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    sizedGrid(grid, 3, 2);

    CHECK(grid.keyPressed(key(juce::KeyPress::returnKey)));
    CHECK(grid.getCursor() == Cell{ .row = 1, .col = 0 });
    CHECK(grid.keyPressed(key(juce::KeyPress::returnKey)));
    CHECK(grid.keyPressed(key(juce::KeyPress::returnKey)));
    CHECK(grid.getCursor() == Cell{ .row = 2, .col = 0 });
    CHECK(grid.keyPressed(key(juce::KeyPress::returnKey, juce::ModifierKeys::shiftModifier)));
    CHECK(grid.getCursor() == Cell{ .row = 1, .col = 0 });
}

TEST_CASE("Tab and Shift+Tab move between cells, wrapping rows", "[grid]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    sizedGrid(grid, 2, 2);

    CHECK(grid.keyPressed(key(juce::KeyPress::tabKey)));
    CHECK(grid.getCursor() == Cell{ .row = 0, .col = 1 });
    CHECK(grid.keyPressed(key(juce::KeyPress::tabKey)));
    CHECK(grid.getCursor() == Cell{ .row = 1, .col = 0 });
    CHECK(grid.keyPressed(key(juce::KeyPress::tabKey, juce::ModifierKeys::shiftModifier)));
    CHECK(grid.getCursor() == Cell{ .row = 0, .col = 1 });
}

TEST_CASE("Tab past the last cell and Ctrl+Tab move focus out of the grid", "[grid]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    juce::Component parent;
    Grid grid;
    juce::TextButton before{ "before" };
    juce::TextButton after{ "after" };
    for (auto* component : std::initializer_list<juce::Component*>{ &before, &grid, &after })
    {
        component->setWantsKeyboardFocus(true);
        parent.addAndMakeVisible(component);
    }
    sizedGrid(grid, 2, 2);
    parent.setBounds(0, 0, 1200, 400);
    before.setBounds(0, 0, 10, 10);
    grid.setTopLeftPosition(0, 20);
    after.setBounds(0, 380, 10, 10);
    parent.addToDesktop(0);
    parent.setVisible(true);
    parent.toFront(true);

    REQUIRE(jgrid::test::focus(grid));
    CHECK(grid.keyPressed(key(juce::KeyPress::tabKey, juce::ModifierKeys::ctrlModifier)));
    CHECK(jgrid::test::waitFor([&] { return after.hasKeyboardFocus(false); }));
    CHECK(grid.getCursor() == Cell{});

    REQUIRE(jgrid::test::focus(grid));
    CHECK(grid.keyPressed(key(juce::KeyPress::tabKey, juce::ModifierKeys::shiftModifier)));
    CHECK(jgrid::test::waitFor([&] { return before.hasKeyboardFocus(false); }));

    grid.setCursor({ .row = 1, .col = 1 });
    REQUIRE(jgrid::test::focus(grid));
    CHECK(grid.keyPressed(key(juce::KeyPress::tabKey)));
    CHECK(jgrid::test::waitFor([&] { return after.hasKeyboardFocus(false); }));
    CHECK(grid.getCursor() == Cell{ .row = 1, .col = 1 });
}
