#pragma once

#include "jgrid_Axis.h"
#include "jgrid_Change.h"
#include "jgrid_Navigation.h"

#include <optional>
#include <vector>

namespace jgrid
{

/** A rectangle of cells: half-open row and column index ranges. */
struct CellRange
{
    IndexRange rows;
    IndexRange cols;

    /** Returns the smallest range containing both cells. */
    [[nodiscard]] static CellRange spanning(Cell a, Cell b);

    [[nodiscard]] bool isEmpty() const { return rows.isEmpty() || cols.isEmpty(); }
    [[nodiscard]] bool contains(Cell cell) const { return rows.contains(cell.row) && cols.contains(cell.col); }
    [[nodiscard]] int getNumCells() const { return rows.size() * cols.size(); }
    [[nodiscard]] Cell getTopLeft() const { return { .row = rows.begin, .col = cols.begin }; }

    friend bool operator==(const CellRange&, const CellRange&) = default;
};

/** The selected cells: one or more ranges, the last of which is active. The active range spans
    from an anchor cell, which stays put, to a far corner, which Shift-movement and dragging move.
    The cursor itself is held by the Grid. */
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

    void selectAll(int rowCount, int colCount);

    [[nodiscard]] const std::vector<CellRange>& getRanges() const;
    [[nodiscard]] std::optional<Cell> getAnchor() const;
    [[nodiscard]] std::optional<Cell> getCorner() const;
    [[nodiscard]] bool contains(Cell cell) const;
    [[nodiscard]] bool isEmpty() const;

    /** True when the selection is at most a single cell. */
    [[nodiscard]] bool isSingleCell() const;

    [[nodiscard]] int getNumCells() const;

    /** True if every cell of the row lies in one range. */
    [[nodiscard]] bool isWholeRowSelected(int row, int colCount) const;

    /** True if every cell of the column lies in one range. */
    [[nodiscard]] bool isWholeColumnSelected(int col, int rowCount) const;

    [[nodiscard]] bool intersectsRow(int row) const;
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
