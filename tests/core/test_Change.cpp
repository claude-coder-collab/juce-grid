#include <jgrid/core/jgrid_Change.h>

#include <catch2/catch_test_macros.hpp>

using jgrid::remapRow;

TEST_CASE("Cell changes and inserts shift rows as expected", "[change]")
{
    CHECK(remapRow(3, jgrid::CellsChanged{}) == 3);
    CHECK(remapRow(3, jgrid::RowsInserted{ .at = 4, .count = 2 }) == 3);
    CHECK(remapRow(4, jgrid::RowsInserted{ .at = 4, .count = 2 }) == 6);
    CHECK(remapRow(9, jgrid::RowsInserted{ .at = 0, .count = 1 }) == 10);
}

TEST_CASE("Removals drop removed rows and shift later ones", "[change]")
{
    const jgrid::RowsRemoved removed{ .rows = { 5, 1, 3 } };
    CHECK(remapRow(0, removed) == 0);
    CHECK(remapRow(1, removed) == std::nullopt);
    CHECK(remapRow(2, removed) == 1);
    CHECK(remapRow(4, removed) == 2);
    CHECK(remapRow(6, removed) == 3);
}

TEST_CASE("A removed row's position goes to the next surviving row", "[change]")
{
    const jgrid::RowsRemoved removed{ .rows = { 2, 3, 3 } };
    CHECK(jgrid::rowAfterRemoval(2, removed) == 2);
    CHECK(jgrid::rowAfterRemoval(3, removed) == 2);
    CHECK(jgrid::rowAfterRemoval(4, removed) == 2);
    CHECK(jgrid::rowAfterRemoval(1, removed) == 1);
}

TEST_CASE("Reorders move rows to their new positions", "[change]")
{
    const jgrid::RowsReordered reordered{ .order = { 2, 0, 1 } };
    CHECK(remapRow(2, reordered) == 0);
    CHECK(remapRow(0, reordered) == 1);
    CHECK(remapRow(1, reordered) == 2);
    CHECK(remapRow(7, reordered) == std::nullopt);
}

TEST_CASE("Resets do not track rows", "[change]")
{
    CHECK(remapRow(0, jgrid::Reset{}) == std::nullopt);
}
