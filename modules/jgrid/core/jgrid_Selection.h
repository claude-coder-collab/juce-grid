#pragma once

#include "jgrid_Axis.h"
#include "jgrid_Change.h"
#include "jgrid_Navigation.h"

#include <optional>
#include <vector>

namespace jgrid
{

//==============================================================================
/** A rectangle of cells, as half-open row and column ranges.

    @see Selection
*/
struct CellRange
{
    IndexRange rows; /**< The rows [begin, end). */
    IndexRange cols; /**< The columns [begin, end). */

    /** Returns the smallest range containing both cells. */
    [[nodiscard]] static CellRange spanning(Cell a, Cell b);

    /** Returns true if the range has no cells. */
    [[nodiscard]] bool isEmpty() const { return rows.isEmpty() || cols.isEmpty(); }
    /** Returns true if the cell is in the range. */
    [[nodiscard]] bool contains(Cell cell) const { return rows.contains(cell.row) && cols.contains(cell.col); }
    /** Returns the number of cells in the range. */
    [[nodiscard]] int getNumCells() const { return rows.size() * cols.size(); }
    /** Returns the range's top-left cell. */
    [[nodiscard]] Cell getTopLeft() const { return { .row = rows.begin, .col = cols.begin }; }

    /** Compares two ranges. */
    friend bool operator==(const CellRange&, const CellRange&) = default;
};

//==============================================================================
/**
    The selected cells of a Grid: one or more rectangular ranges, the last of which is active.

    The active range spans from an anchor cell, which stays put, to a far corner, which Shift with
    a movement key and dragging move. Cmd/Ctrl+click adds a new range. The cursor itself is held by
    the Grid and is always in or next to the selection.

    Read a Grid's selection with Grid::getSelection(); change it through the Grid (setCursor,
    extendSelection, selectRows and so on) so it repaints and stays consistent.

    @see Grid::getSelection, CellRange
*/
class Selection
{
public:
    /** Selects just the given cell, or nothing. */
    void collapseTo(std::optional<Cell> cell);

    /** Moves the active range's far corner, keeping its anchor. */
    void extendTo(Cell corner);

    /** Starts a new active range at the cell, keeping the existing ranges. */
    void addCell(Cell cell);

    /** Selects whole rows first..last (in either order), replacing or adding to the selection. */
    void selectRows(int first, int last, int colCount, bool add);

    /** Selects whole columns first..last (in either order), replacing or adding to the selection. */
    void selectCols(int first, int last, int rowCount, bool add);

    /** Selects every cell of a grid of the given size, or nothing if it is empty. */
    void selectAll(int rowCount, int colCount);

    /** Returns the ranges, the active one last. */
    [[nodiscard]] const std::vector<CellRange>& getRanges() const;
    /** Returns the active range's anchor, the corner that stays put. */
    [[nodiscard]] std::optional<Cell> getAnchor() const;
    /** Returns the active range's far corner, the one extending moves. */
    [[nodiscard]] std::optional<Cell> getCorner() const;
    /** Returns true if any range contains the cell. */
    [[nodiscard]] bool contains(Cell cell) const;
    /** Returns true if nothing is selected. */
    [[nodiscard]] bool isEmpty() const;

    /** True when the selection is at most a single cell. */
    [[nodiscard]] bool isSingleCell() const;

    /** Returns the number of selected cells, counting overlaps between ranges more than once. */
    [[nodiscard]] int getNumCells() const;

    /** True if every cell of the row lies in one range. */
    [[nodiscard]] bool isWholeRowSelected(int row, int colCount) const;

    /** True if every cell of the column lies in one range. */
    [[nodiscard]] bool isWholeColumnSelected(int col, int rowCount) const;

    /** Returns true if any selected cell is in the row. */
    [[nodiscard]] bool intersectsRow(int row) const;
    /** Returns true if any selected cell is in the column. */
    [[nodiscard]] bool intersectsCol(int col) const;

    /** Returns the cell after (or before) the given one when cycling through every selected cell:
        down each column first when columnMajor (Enter), across each row otherwise (Tab), then
        on to the next range, wrapping around. */
    [[nodiscard]] std::optional<Cell> step(Cell from, bool forward, bool columnMajor) const;

    /** Clips the selection to a grid of the given size. */
    void clampTo(int rowCount, int colCount);

    /** Carries the selection through a row change. Ranges keep their rows through inserts and
        removals; a reorder or reset collapses the selection to the given cell. */
    void applyChange(const Change& change, std::optional<Cell> collapseCell);

private:
    std::vector<CellRange> ranges;
    std::optional<Cell> anchor;
    std::optional<Cell> corner;
};

} // namespace jgrid
