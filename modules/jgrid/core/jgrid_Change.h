#pragma once

#include <optional>
#include <variant>
#include <vector>

namespace jgrid
{

//==============================================================================
/** Notification that the values in a rectangle of cells changed.

    Send it after setValue, or whenever cell values change for any other reason. Row indices are
    the model's view-order indices. Grids repaint and tell screen readers; the cut mark and sort
    indicator are cleared if they are affected.

    @see Change, GridModelNotifier
*/
struct CellsChanged
{
    int firstRow = 0; /**< The first changed row. */
    int numRows = 0; /**< How many consecutive rows changed. */
    int firstCol = 0; /**< The first changed column. */
    int numCols = 0; /**< How many consecutive columns changed. */
};

/** Notification that rows were inserted: `count` new rows, the first now at index `at`.

    Grids shift their cursor, selection, row heights and hidden rows to follow the existing rows.
*/
struct RowsInserted
{
    int at = 0; /**< Where the first new row now is. */
    int count = 0; /**< How many rows were inserted. */
};

/** Notification that rows were removed, given by their indices before the removal, in any order.

    Grids drop the removed rows' heights, hidden state and selection, and a cursor on a removed row
    moves to the row that took its place.
*/
struct RowsRemoved
{
    std::vector<int> rows; /**< The removed rows' indices before the removal. */
};

/** Notification that the rows were reordered: `order[newIndex]` is each row's previous index.

    Send it after reorderRows, with the same order. Grids move their cursor, row heights and hidden
    rows with the rows; a multi-cell selection collapses to the cursor.
*/
struct RowsReordered
{
    std::vector<int> order; /**< For each new position, the row's previous position. */
};

/** Notification that anything may have changed, including the number of columns.

    Grids re-query everything, cancel any open editor, and forget row heights, hidden rows and the
    selection; the cursor stays where it was, clamped to the new size.
*/
struct Reset
{
};

/** A notification from a model to the Grids viewing it, sent with GridModelNotifier::changed().

    Send the most specific change you can: CellsChanged for value edits, RowsInserted, RowsRemoved
    and RowsReordered for structural changes, and Reset only when nothing more specific fits (for
    example after loading a file or an undo that replaced everything).
*/
using Change = std::variant<CellsChanged, RowsInserted, RowsRemoved, RowsReordered, Reset>;

/** Returns where a row is after a change.

    @returns the row's new index, or nothing if the row was removed or the change is a Reset.
*/
[[nodiscard]] std::optional<int> remapRow(int row, const Change& change);

/** Returns the index a row position takes after a removal: the position of the first surviving
    row at or after it, which is where a cursor on a removed row should go. */
[[nodiscard]] int rowAfterRemoval(int row, const RowsRemoved& removed);

} // namespace jgrid
