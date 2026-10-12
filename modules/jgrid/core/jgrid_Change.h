#pragma once

#include <optional>
#include <variant>
#include <vector>

namespace jgrid
{

/** The values in a rectangle of cells changed. Row indices are view-order indices. */
struct CellsChanged
{
    int firstRow = 0;
    int numRows = 0;
    int firstCol = 0;
    int numCols = 0;
};

/** count rows were inserted, the first at index at. */
struct RowsInserted
{
    int at = 0;
    int count = 0;
};

/** The rows at these indices, as they were before removal, were removed. */
struct RowsRemoved
{
    std::vector<int> rows;
};

/** The rows were reordered: order[newIndex] is the row's index before the change. */
struct RowsReordered
{
    std::vector<int> order;
};

/** Anything may have changed, including the column count. */
struct Reset
{
};

/** A notification from a model to the Grids viewing it. */
using Change = std::variant<CellsChanged, RowsInserted, RowsRemoved, RowsReordered, Reset>;

/** Returns where a row is after a change, or nothing if it was removed or the change is a reset. */
[[nodiscard]] std::optional<int> remapRow(int row, const Change& change);

/** Returns the index a row position takes after a removal: the position of the first surviving
    row at or after it, which is where a cursor on a removed row should go. */
[[nodiscard]] int rowAfterRemoval(int row, const RowsRemoved& removed);

} // namespace jgrid
