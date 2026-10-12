#include "GridTestSupport.h"

#include <catch2/catch_test_macros.hpp>

using jgrid::Cell;
using jgrid::Grid;
using jgrid::test::EditableTable;
using jgrid::test::key;
using jgrid::test::makeEditableModel;

namespace
{

std::shared_ptr<EditableTable> layoutGrid(Grid& grid, int rows = 6, int cols = 4)
{
    auto table = std::make_shared<EditableTable>(rows, cols);
    grid.setModel(makeEditableModel(table));
    grid.setSize(600, 300);
    return table;
}

} // namespace

TEST_CASE("Columns and rows can be resized", "[layout]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    layoutGrid(grid);

    grid.setColumnWidth(1, 150);
    CHECK(grid.getColumnWidth(1) == 150);
    CHECK(grid.getCellBounds({ .row = 0, .col = 2 }).getX() - grid.getCellBounds({ .row = 0, .col = 1 }).getX() == 150);
    grid.setColumnWidth(1, 1);
    CHECK(grid.getColumnWidth(1) == Grid::minimumCellSize);

    grid.setRowHeight(2, 40);
    CHECK(grid.getRowHeight(2) == 40);
    CHECK(grid.getCellBounds({ .row = 2, .col = 0 }).getHeight() == 40);
}

TEST_CASE("Autofit sizes a column to its widest content", "[layout]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    const auto table = layoutGrid(grid);
    table->cells[4][2] = "a much longer value than the others in this column";
    grid.autoFitColumn(2);
    CHECK(grid.getColumnWidth(2) > 250);

    table->cells[4][2] = "x";
    grid.autoFitColumn(2);
    CHECK(grid.getColumnWidth(2) < 60);

    grid.setRowHeight(1, 80);
    grid.autoFitRow(1);
    CHECK(grid.getRowHeight(1) < 30);
}

TEST_CASE("Hidden columns and rows are skipped by navigation", "[layout]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    layoutGrid(grid);
    grid.setColumnsHidden({ 1 }, true);
    grid.setRowsHidden({ 1, 2 }, true);
    CHECK(grid.isColumnHidden(1));
    CHECK(grid.getHiddenRows() == std::vector{ 1, 2 });

    CHECK(grid.keyPressed(key(juce::KeyPress::rightKey)));
    CHECK(grid.getCursor() == Cell{ .row = 0, .col = 2 });
    CHECK(grid.keyPressed(key(juce::KeyPress::downKey)));
    CHECK(grid.getCursor() == Cell{ .row = 3, .col = 2 });
    CHECK(grid.keyPressed(key(juce::KeyPress::homeKey)));
    CHECK(grid.getCursor() == Cell{ .row = 3, .col = 0 });
    CHECK(grid.keyPressed(key(juce::KeyPress::tabKey)));
    CHECK(grid.getCursor() == Cell{ .row = 3, .col = 2 });

    grid.showAllColumns();
    grid.showAllRows();
    CHECK(grid.getHiddenColumns().empty());
    CHECK_FALSE(grid.isRowHidden(1));
}

TEST_CASE("Hiding the cursor's column moves the cursor", "[layout]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    layoutGrid(grid);
    grid.setCursor({ .row = 2, .col = 1 });
    REQUIRE(grid.startEditing());
    grid.setColumnsHidden({ 1 }, true);
    CHECK(grid.getCursor() == Cell{ .row = 2, .col = 2 });
    CHECK_FALSE(grid.isEditing());
}

TEST_CASE("Copy, paste and delete skip hidden cells", "[layout]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    const auto table = layoutGrid(grid);
    grid.setColumnsHidden({ 1 }, true);
    grid.setCursor({ .row = 0, .col = 0 });
    grid.extendSelection({ .row = 0, .col = 2 });

    grid.copy();
    CHECK(juce::SystemClipboard::getTextFromClipboard() == "r0c0\tr0c2\n");

    CHECK(grid.deleteSelection());
    CHECK(table->at(0, 1) == "r0c1");
    CHECK(table->at(0, 2).isEmpty());

    juce::SystemClipboard::copyTextToClipboard("a\tb\n");
    grid.setCursor({ .row = 1, .col = 0 });
    CHECK(grid.paste());
    CHECK(table->at(1, 0) == "a");
    CHECK(table->at(1, 1) == "r1c1");
    CHECK(table->at(1, 2) == "b");
}

TEST_CASE("Row sizes and hidden rows follow their rows through changes", "[layout]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    const auto table = layoutGrid(grid);
    grid.setRowHeight(3, 50);
    grid.setRowsHidden({ 4 }, true);

    table->cells.insert(table->cells.begin(), { "a", "b", "c", "d" });
    table->notifier.changed(jgrid::RowsInserted{ .at = 0, .count = 1 });
    CHECK(grid.getRowHeight(4) == 50);
    CHECK(grid.isRowHidden(5));

    table->cells.erase(table->cells.begin() + 1);
    table->notifier.changed(jgrid::RowsRemoved{ .rows = { 1 } });
    CHECK(grid.getRowHeight(3) == 50);
    CHECK(grid.isRowHidden(4));

    table->notifier.changed(jgrid::Reset{});
    CHECK_FALSE(grid.isRowHidden(4));
    CHECK(grid.getRowHeight(3) == 22);
}

TEST_CASE("Screen readers see only visible rows and columns", "[layout]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    layoutGrid(grid);
    grid.addToDesktop(0);
    const auto& table = *grid.getAccessibilityHandler()->getTableInterface();
    table.getNumRows();
    grid.setColumnsHidden({ 0 }, true);
    grid.setRowsHidden({ 0 }, true);

    CHECK(table.getNumRows() == 5);
    CHECK(table.getNumColumns() == 3);
    const auto* cell = table.getCellHandler(0, 0);
    REQUIRE(cell != nullptr);
    CHECK(cell->getChildren().front()->getTitle() == "r1c1");
    CHECK(table.getRowSpan(*cell)->begin == 0);
    CHECK(table.getColumnSpan(*cell)->begin == 0);
    CHECK(table.getHeaderHandler()->getChildren().size() == 3);
}
