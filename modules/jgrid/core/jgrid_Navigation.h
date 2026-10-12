#pragma once

#include "jgrid_Axis.h"

#include <optional>

namespace jgrid
{

/** A cell position, as row and column indices. */
struct Cell
{
    int row = 0;
    int col = 0;

    friend bool operator==(const Cell&, const Cell&) = default;
};

/** A cursor movement requested from the keyboard. */
enum class Move {
    up,
    down,
    left,
    right,
    pageUp,
    pageDown,
    rowStart,
    rowEnd,
    columnTop,
    columnBottom,
    gridStart,
    gridEnd
};

/** Returns the cursor position after a move, clamped to a grid of the given size.
    pageRows is the number of rows a page move travels and is treated as at least 1. */
[[nodiscard]] Cell moveCursor(Cell from, Move move, int rowCount, int colCount, int pageRows);

/** Returns the next (or previous) cell in reading order, wrapping across rows, or nothing when
    stepping past the last (or before the first) cell. */
[[nodiscard]] std::optional<Cell> stepCell(Cell from, bool forward, int rowCount, int colCount);

/** Returns the cursor position after a move, skipping hidden rows and columns. */
[[nodiscard]] Cell moveCursor(Cell from, Move move, const Axis& rows, const Axis& cols, int pageRows);

/** Returns the next (or previous) visible cell in reading order, or nothing past either end. */
[[nodiscard]] std::optional<Cell> stepCell(Cell from, bool forward, const Axis& rows, const Axis& cols);

/** Clamps a cursor to the visible cells: a cursor on a hidden row or column moves to the nearest
    visible one, and with nothing visible there is no cursor. */
[[nodiscard]] std::optional<Cell> clampCursor(std::optional<Cell> cursor, const Axis& rows, const Axis& cols);

/** Clamps a cursor to a grid of the given size. An empty grid has no cursor, and a grid
    that gains cells while it has no cursor places it at the first cell. */
[[nodiscard]] std::optional<Cell> clampCursor(std::optional<Cell> cursor, int rowCount, int colCount);

} // namespace jgrid
