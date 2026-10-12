#include <jgrid/core/jgrid_Navigation.h>

#include <catch2/catch_test_macros.hpp>

using jgrid::Cell;
using jgrid::Move;

TEST_CASE("Arrow moves step one cell and clamp at edges", "[navigation]")
{
    constexpr Cell origin{};
    CHECK(moveCursor(origin, Move::down, 3, 3, 1) == Cell{ .row = 1, .col = 0 });
    CHECK(moveCursor(origin, Move::right, 3, 3, 1) == Cell{ .row = 0, .col = 1 });
    CHECK(moveCursor(origin, Move::up, 3, 3, 1) == origin);
    CHECK(moveCursor(origin, Move::left, 3, 3, 1) == origin);

    constexpr Cell corner{ .row = 2, .col = 2 };
    CHECK(moveCursor(corner, Move::down, 3, 3, 1) == corner);
    CHECK(moveCursor(corner, Move::right, 3, 3, 1) == corner);
}

TEST_CASE("Page moves travel the page size and clamp", "[navigation]")
{
    constexpr Cell start{ .row = 5, .col = 1 };
    CHECK(moveCursor(start, Move::pageDown, 100, 3, 10) == Cell{ .row = 15, .col = 1 });
    CHECK(moveCursor(start, Move::pageUp, 100, 3, 10) == Cell{ .row = 0, .col = 1 });
    CHECK(moveCursor(start, Move::pageDown, 8, 3, 10) == Cell{ .row = 7, .col = 1 });
    CHECK(moveCursor(start, Move::pageDown, 100, 3, 0) == Cell{ .row = 6, .col = 1 });
}

TEST_CASE("Home and end moves reach row and grid edges", "[navigation]")
{
    constexpr Cell start{ .row = 4, .col = 2 };
    CHECK(moveCursor(start, Move::rowStart, 10, 5, 1) == Cell{ .row = 4, .col = 0 });
    CHECK(moveCursor(start, Move::rowEnd, 10, 5, 1) == Cell{ .row = 4, .col = 4 });
    CHECK(moveCursor(start, Move::gridStart, 10, 5, 1) == Cell{ .row = 0, .col = 0 });
    CHECK(moveCursor(start, Move::gridEnd, 10, 5, 1) == Cell{ .row = 9, .col = 4 });
}

TEST_CASE("Moves in an empty grid return the origin", "[navigation]")
{
    CHECK(moveCursor(Cell{ .row = 3, .col = 3 }, Move::down, 0, 5, 1) == Cell{});
}

TEST_CASE("Clamping places, keeps or removes the cursor", "[navigation]")
{
    CHECK(jgrid::clampCursor(std::nullopt, 0, 0) == std::nullopt);
    CHECK(jgrid::clampCursor(Cell{ .row = 1, .col = 1 }, 0, 4) == std::nullopt);
    CHECK(jgrid::clampCursor(std::nullopt, 3, 3) == Cell{});
    CHECK(jgrid::clampCursor(Cell{ .row = 1, .col = 2 }, 3, 3) == Cell{ .row = 1, .col = 2 });
    CHECK(jgrid::clampCursor(Cell{ .row = 9, .col = 9 }, 3, 2) == Cell{ .row = 2, .col = 1 });
}

TEST_CASE("Stepping moves through cells in reading order", "[navigation]")
{
    CHECK(jgrid::stepCell(Cell{ .row = 0, .col = 0 }, true, 2, 3) == Cell{ .row = 0, .col = 1 });
    CHECK(jgrid::stepCell(Cell{ .row = 0, .col = 2 }, true, 2, 3) == Cell{ .row = 1, .col = 0 });
    CHECK(jgrid::stepCell(Cell{ .row = 1, .col = 0 }, false, 2, 3) == Cell{ .row = 0, .col = 2 });
    CHECK(jgrid::stepCell(Cell{ .row = 1, .col = 1 }, false, 2, 3) == Cell{ .row = 1, .col = 0 });
}

TEST_CASE("Stepping past either end of the grid returns nothing", "[navigation]")
{
    CHECK(jgrid::stepCell(Cell{ .row = 1, .col = 2 }, true, 2, 3) == std::nullopt);
    CHECK(jgrid::stepCell(Cell{}, false, 2, 3) == std::nullopt);
    CHECK(jgrid::stepCell(Cell{}, true, 0, 3) == std::nullopt);
}

TEST_CASE("Column moves reach the top and bottom row", "[navigation]")
{
    CHECK(moveCursor(Cell{ .row = 4, .col = 2 }, Move::columnTop, 10, 5, 1) == Cell{ .row = 0, .col = 2 });
    CHECK(moveCursor(Cell{ .row = 4, .col = 2 }, Move::columnBottom, 10, 5, 1) == Cell{ .row = 9, .col = 2 });
}

namespace
{

jgrid::Axis navigationAxis(int count, const std::vector<int>& hidden)
{
    jgrid::Axis result{ 10 };
    result.setCount(count);
    for (const auto index : hidden)
        result.setHidden(index, true);
    return result;
}

} // namespace

TEST_CASE("Moves skip hidden rows and columns", "[navigation]")
{
    const auto rows = navigationAxis(6, { 1, 2 });
    const auto cols = navigationAxis(4, { 0, 3 });
    CHECK(moveCursor(Cell{ .row = 0, .col = 1 }, Move::down, rows, cols, 1) == Cell{ .row = 3, .col = 1 });
    CHECK(moveCursor(Cell{ .row = 3, .col = 1 }, Move::up, rows, cols, 1) == Cell{ .row = 0, .col = 1 });
    CHECK(moveCursor(Cell{ .row = 0, .col = 1 }, Move::left, rows, cols, 1) == Cell{ .row = 0, .col = 1 });
    CHECK(moveCursor(Cell{ .row = 0, .col = 1 }, Move::rowEnd, rows, cols, 1) == Cell{ .row = 0, .col = 2 });
    CHECK(moveCursor(Cell{ .row = 0, .col = 1 }, Move::pageDown, rows, cols, 2) == Cell{ .row = 4, .col = 1 });
    CHECK(moveCursor(Cell{ .row = 4, .col = 2 }, Move::gridStart, rows, cols, 1) == Cell{ .row = 0, .col = 1 });
}

TEST_CASE("Stepping skips hidden cells", "[navigation]")
{
    const auto rows = navigationAxis(3, { 1 });
    const auto cols = navigationAxis(3, { 1 });
    CHECK(jgrid::stepCell(Cell{ .row = 0, .col = 0 }, true, rows, cols) == Cell{ .row = 0, .col = 2 });
    CHECK(jgrid::stepCell(Cell{ .row = 0, .col = 2 }, true, rows, cols) == Cell{ .row = 2, .col = 0 });
    CHECK(jgrid::stepCell(Cell{ .row = 2, .col = 0 }, false, rows, cols) == Cell{ .row = 0, .col = 2 });
    CHECK(jgrid::stepCell(Cell{ .row = 2, .col = 2 }, true, rows, cols) == std::nullopt);
}

TEST_CASE("A cursor on a hidden row moves to a visible one", "[navigation]")
{
    const auto rows = navigationAxis(3, { 1, 2 });
    const auto cols = navigationAxis(2, {});
    CHECK(jgrid::clampCursor(Cell{ .row = 2, .col = 1 }, rows, cols) == Cell{ .row = 0, .col = 1 });
    CHECK(jgrid::clampCursor(Cell{}, navigationAxis(1, { 0 }), cols) == std::nullopt);
}
