#include <jgrid/core/jgrid_Selection.h>

#include <catch2/catch_test_macros.hpp>

using jgrid::Cell;
using jgrid::CellRange;
using jgrid::Selection;

namespace
{

CellRange range(int top, int left, int bottom, int right)
{
    return CellRange::spanning({ .row = top, .col = left }, { .row = bottom, .col = right });
}

} // namespace

TEST_CASE("Ranges span two corners in any order", "[selection]")
{
    CHECK(range(3, 4, 1, 2) == CellRange{ .rows = { .begin = 1, .end = 4 }, .cols = { .begin = 2, .end = 5 } });
    CHECK(range(1, 1, 1, 1).getNumCells() == 1);
}

TEST_CASE("Collapsing selects a single cell or nothing", "[selection]")
{
    Selection selection;
    CHECK(selection.isEmpty());
    selection.collapseTo(Cell{ .row = 2, .col = 3 });
    CHECK(selection.isSingleCell());
    CHECK(selection.contains({ .row = 2, .col = 3 }));
    selection.collapseTo(std::nullopt);
    CHECK(selection.isEmpty());
}

TEST_CASE("Extending moves the far corner and keeps the anchor", "[selection]")
{
    Selection selection;
    selection.collapseTo(Cell{ .row = 2, .col = 2 });
    selection.extendTo({ .row = 4, .col = 0 });
    CHECK(selection.getRanges() == std::vector{ range(2, 0, 4, 2) });
    CHECK(selection.getAnchor() == Cell{ .row = 2, .col = 2 });
    CHECK(selection.getCorner() == Cell{ .row = 4, .col = 0 });
    CHECK(selection.getNumCells() == 9);
    selection.extendTo({ .row = 2, .col = 3 });
    CHECK(selection.getRanges() == std::vector{ range(2, 2, 2, 3) });
}

TEST_CASE("Adding a cell keeps earlier ranges", "[selection]")
{
    Selection selection;
    selection.collapseTo(Cell{});
    selection.extendTo({ .row = 1, .col = 1 });
    selection.addCell({ .row = 5, .col = 5 });
    selection.extendTo({ .row = 6, .col = 5 });
    CHECK(selection.getRanges().size() == 2);
    CHECK(selection.contains({ .row = 1, .col = 0 }));
    CHECK(selection.contains({ .row = 6, .col = 5 }));
    CHECK_FALSE(selection.contains({ .row = 3, .col = 3 }));
    CHECK(selection.getNumCells() == 6);
}

TEST_CASE("Whole rows and columns are recognised", "[selection]")
{
    Selection selection;
    selection.selectCols(3, 1, 10, false);
    CHECK(selection.isWholeColumnSelected(2, 10));
    CHECK_FALSE(selection.isWholeColumnSelected(0, 10));
    CHECK(selection.intersectsRow(9));
    CHECK_FALSE(selection.isWholeRowSelected(0, 5));

    selection.selectRows(4, 4, 5, true);
    CHECK(selection.isWholeRowSelected(4, 5));
    CHECK(selection.isWholeColumnSelected(1, 10));

    selection.selectAll(3, 2);
    CHECK(selection.getNumCells() == 6);
    selection.selectAll(0, 2);
    CHECK(selection.isEmpty());
}

TEST_CASE("Stepping cycles through the selection", "[selection]")
{
    Selection selection;
    selection.collapseTo(Cell{});
    selection.extendTo({ .row = 1, .col = 1 });

    SECTION("Enter goes down each column")
    {
        CHECK(selection.step({ .row = 0, .col = 0 }, true, true) == Cell{ .row = 1, .col = 0 });
        CHECK(selection.step({ .row = 1, .col = 0 }, true, true) == Cell{ .row = 0, .col = 1 });
        CHECK(selection.step({ .row = 1, .col = 1 }, true, true) == Cell{ .row = 0, .col = 0 });
        CHECK(selection.step({ .row = 0, .col = 0 }, false, true) == Cell{ .row = 1, .col = 1 });
    }
    SECTION("Tab goes across each row")
    {
        CHECK(selection.step({ .row = 0, .col = 0 }, true, false) == Cell{ .row = 0, .col = 1 });
        CHECK(selection.step({ .row = 0, .col = 1 }, true, false) == Cell{ .row = 1, .col = 0 });
    }
    SECTION("Stepping moves on to the next range")
    {
        selection.addCell({ .row = 5, .col = 5 });
        CHECK(selection.step({ .row = 1, .col = 1 }, true, true) == Cell{ .row = 5, .col = 5 });
        CHECK(selection.step({ .row = 5, .col = 5 }, true, true) == Cell{ .row = 0, .col = 0 });
    }
}

TEST_CASE("Clamping trims ranges to the grid", "[selection]")
{
    Selection selection;
    selection.collapseTo(Cell{ .row = 2, .col = 2 });
    selection.extendTo({ .row = 8, .col = 8 });
    selection.clampTo(5, 4);
    CHECK(selection.getRanges() == std::vector{ CellRange{ .rows = { .begin = 2, .end = 5 }, .cols = { .begin = 2, .end = 4 } } });
    selection.clampTo(1, 1);
    CHECK(selection.isEmpty());
}

TEST_CASE("Row changes carry the selection along", "[selection]")
{
    Selection selection;
    selection.collapseTo(Cell{ .row = 2, .col = 0 });
    selection.extendTo({ .row = 4, .col = 1 });

    selection.applyChange(jgrid::RowsInserted{ .at = 3, .count = 2 }, std::nullopt);
    CHECK(selection.getRanges() == std::vector{ range(2, 0, 6, 1) });

    selection.applyChange(jgrid::RowsInserted{ .at = 0, .count = 1 }, std::nullopt);
    CHECK(selection.getRanges() == std::vector{ range(3, 0, 7, 1) });
    CHECK(selection.getAnchor() == Cell{ .row = 3, .col = 0 });

    selection.applyChange(jgrid::RowsRemoved{ .rows = { 4, 5, 0 } }, std::nullopt);
    CHECK(selection.getRanges() == std::vector{ range(2, 0, 4, 1) });

    selection.applyChange(jgrid::RowsRemoved{ .rows = { 2, 3, 4 } }, Cell{ .row = 2, .col = 0 });
    CHECK(selection.getRanges() == std::vector{ range(2, 0, 2, 0) });

    selection.extendTo({ .row = 3, .col = 1 });
    selection.applyChange(jgrid::RowsReordered{ .order = { 1, 0, 2, 3 } }, Cell{ .row = 1, .col = 1 });
    CHECK(selection.isSingleCell());
    CHECK(selection.contains({ .row = 1, .col = 1 }));
}
