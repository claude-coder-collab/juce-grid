#include "jgrid_Navigation.h"

#include <algorithm>

namespace jgrid
{

Cell moveCursor(Cell from, Move move, int rowCount, int colCount, int pageRows)
{
    if (rowCount <= 0 || colCount <= 0)
        return {};

    const auto page = std::max(1, pageRows);
    auto to = from;
    switch (move)
    {
        case Move::up:
            --to.row;
            break;
        case Move::down:
            ++to.row;
            break;
        case Move::left:
            --to.col;
            break;
        case Move::right:
            ++to.col;
            break;
        case Move::pageUp:
            to.row -= page;
            break;
        case Move::pageDown:
            to.row += page;
            break;
        case Move::rowStart:
            to.col = 0;
            break;
        case Move::rowEnd:
            to.col = colCount - 1;
            break;
        case Move::columnTop:
            to.row = 0;
            break;
        case Move::columnBottom:
            to.row = rowCount - 1;
            break;
        case Move::gridStart:
            to = {};
            break;
        case Move::gridEnd:
            to = { .row = rowCount - 1, .col = colCount - 1 };
            break;
    }
    return { .row = std::clamp(to.row, 0, rowCount - 1), .col = std::clamp(to.col, 0, colCount - 1) };
}

std::optional<Cell> stepCell(Cell from, bool forward, int rowCount, int colCount)
{
    if (rowCount <= 0 || colCount <= 0)
        return std::nullopt;
    const auto index = (from.row * colCount) + from.col + (forward ? 1 : -1);
    if (index < 0 || index >= rowCount * colCount)
        return std::nullopt;
    return Cell{ .row = index / colCount, .col = index % colCount };
}

Cell moveCursor(Cell from, Move move, const Axis& rows, const Axis& cols, int pageRows)
{
    if (rows.getVisibleCount() == 0 || cols.getVisibleCount() == 0)
        return {};

    const auto page = std::max(1, pageRows);
    auto to = Cell{ .row = rows.step(from.row, 0), .col = cols.step(from.col, 0) };
    switch (move)
    {
        case Move::up:
            to.row = rows.step(from.row, -1);
            break;
        case Move::down:
            to.row = rows.step(from.row, 1);
            break;
        case Move::left:
            to.col = cols.step(from.col, -1);
            break;
        case Move::right:
            to.col = cols.step(from.col, 1);
            break;
        case Move::pageUp:
            to.row = rows.step(from.row, -page);
            break;
        case Move::pageDown:
            to.row = rows.step(from.row, page);
            break;
        case Move::rowStart:
            to.col = cols.firstVisible();
            break;
        case Move::rowEnd:
            to.col = cols.lastVisible();
            break;
        case Move::columnTop:
            to.row = rows.firstVisible();
            break;
        case Move::columnBottom:
            to.row = rows.lastVisible();
            break;
        case Move::gridStart:
            to = { .row = rows.firstVisible(), .col = cols.firstVisible() };
            break;
        case Move::gridEnd:
            to = { .row = rows.lastVisible(), .col = cols.lastVisible() };
            break;
    }
    return to;
}

std::optional<Cell> stepCell(Cell from, bool forward, const Axis& rows, const Axis& cols)
{
    if (rows.getVisibleCount() == 0 || cols.getVisibleCount() == 0)
        return std::nullopt;
    if (const auto col = cols.nextVisible(from.col, forward); col >= 0)
        return Cell{ .row = rows.step(from.row, 0), .col = col };
    const auto row = rows.nextVisible(from.row, forward);
    if (row < 0)
        return std::nullopt;
    return Cell{ .row = row, .col = forward ? cols.firstVisible() : cols.lastVisible() };
}

std::optional<Cell> clampCursor(std::optional<Cell> cursor, const Axis& rows, const Axis& cols)
{
    if (rows.getVisibleCount() == 0 || cols.getVisibleCount() == 0)
        return std::nullopt;
    const auto from = cursor.value_or(Cell{});
    return Cell{ .row = rows.step(from.row, 0), .col = cols.step(from.col, 0) };
}

std::optional<Cell> clampCursor(std::optional<Cell> cursor, int rowCount, int colCount)
{
    if (rowCount <= 0 || colCount <= 0)
        return std::nullopt;
    const auto from = cursor.value_or(Cell{});
    return Cell{ .row = std::clamp(from.row, 0, rowCount - 1), .col = std::clamp(from.col, 0, colCount - 1) };
}

} // namespace jgrid
