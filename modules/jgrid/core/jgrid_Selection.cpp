#include "jgrid_Selection.h"

#include <algorithm>

namespace jgrid
{

namespace
{

IndexRange spanOf(int a, int b) { return { .begin = std::min(a, b), .end = std::max(a, b) + 1 }; }

IndexRange clampRange(IndexRange range, int count)
{
    return { .begin = std::clamp(range.begin, 0, count), .end = std::clamp(range.end, 0, count) };
}

std::optional<IndexRange> remapRows(IndexRange rows, const Change& change)
{
    if (const auto* inserted = std::get_if<RowsInserted>(&change))
    {
        const auto count = std::max(0, inserted->count);
        return IndexRange{ .begin = rows.begin >= inserted->at ? rows.begin + count : rows.begin,
            .end = rows.end > inserted->at ? rows.end + count : rows.end };
    }
    if (const auto* removed = std::get_if<RowsRemoved>(&change))
    {
        const IndexRange result{ .begin = rowAfterRemoval(rows.begin, *removed), .end = rowAfterRemoval(rows.end, *removed) };
        return result.isEmpty() ? std::nullopt : std::optional<IndexRange>(result);
    }
    return rows;
}

} // namespace

CellRange CellRange::spanning(Cell a, Cell b) { return { .rows = spanOf(a.row, b.row), .cols = spanOf(a.col, b.col) }; }

void Selection::collapseTo(std::optional<Cell> cell)
{
    ranges.clear();
    anchor = cell;
    corner = cell;
    if (cell)
        ranges.push_back(CellRange::spanning(*cell, *cell));
}

void Selection::extendTo(Cell newCorner)
{
    if (!anchor)
    {
        collapseTo(newCorner);
        return;
    }
    corner = newCorner;
    if (ranges.empty())
        ranges.emplace_back();
    ranges.back() = CellRange::spanning(*anchor, newCorner);
}

void Selection::addCell(Cell cell)
{
    anchor = cell;
    corner = cell;
    ranges.push_back(CellRange::spanning(cell, cell));
}

void Selection::selectRows(int first, int last, int colCount, bool add)
{
    if (!add)
        ranges.clear();
    anchor = Cell{ .row = first, .col = 0 };
    corner = Cell{ .row = last, .col = std::max(0, colCount - 1) };
    ranges.push_back({ .rows = spanOf(first, last), .cols = { .begin = 0, .end = colCount } });
}

void Selection::selectCols(int first, int last, int rowCount, bool add)
{
    if (!add)
        ranges.clear();
    anchor = Cell{ .row = 0, .col = first };
    corner = Cell{ .row = std::max(0, rowCount - 1), .col = last };
    ranges.push_back({ .rows = { .begin = 0, .end = rowCount }, .cols = spanOf(first, last) });
}

void Selection::selectAll(int rowCount, int colCount)
{
    ranges.clear();
    if (rowCount <= 0 || colCount <= 0)
    {
        anchor.reset();
        corner.reset();
        return;
    }
    anchor = Cell{};
    corner = Cell{ .row = rowCount - 1, .col = colCount - 1 };
    ranges.push_back({ .rows = { .begin = 0, .end = rowCount }, .cols = { .begin = 0, .end = colCount } });
}

const std::vector<CellRange>& Selection::getRanges() const { return ranges; }

std::optional<Cell> Selection::getAnchor() const { return anchor; }

std::optional<Cell> Selection::getCorner() const { return corner; }

bool Selection::contains(Cell cell) const
{
    return std::ranges::any_of(ranges, [cell](const CellRange& range) { return range.contains(cell); });
}

bool Selection::isEmpty() const { return ranges.empty(); }

bool Selection::isSingleCell() const { return getNumCells() <= 1; }

int Selection::getNumCells() const
{
    if (ranges.size() == 1)
        return ranges.front().getNumCells();
    auto total = 0;
    for (const auto& range : ranges)
        total += range.getNumCells();
    return total;
}

bool Selection::isWholeRowSelected(int row, int colCount) const
{
    return colCount > 0
        && std::ranges::any_of(ranges, [&](const CellRange& range) { return range.rows.contains(row) && range.cols.begin <= 0 && range.cols.end >= colCount; });
}

bool Selection::isWholeColumnSelected(int col, int rowCount) const
{
    return rowCount > 0
        && std::ranges::any_of(ranges, [&](const CellRange& range) { return range.cols.contains(col) && range.rows.begin <= 0 && range.rows.end >= rowCount; });
}

bool Selection::intersectsRow(int row) const
{
    return std::ranges::any_of(ranges, [row](const CellRange& range) { return !range.cols.isEmpty() && range.rows.contains(row); });
}

bool Selection::intersectsCol(int col) const
{
    return std::ranges::any_of(ranges, [col](const CellRange& range) { return !range.rows.isEmpty() && range.cols.contains(col); });
}

std::optional<Cell> Selection::step(Cell from, bool forward, bool columnMajor) const
{
    std::vector<const CellRange*> nonEmpty;
    for (const auto& range : ranges)
        if (!range.isEmpty())
            nonEmpty.push_back(&range);
    if (nonEmpty.empty())
        return std::nullopt;

    const auto count = static_cast<int>(nonEmpty.size());
    auto rangeIndex = count - 1;
    for (auto i = count - 1; i >= 0; --i)
    {
        if (nonEmpty[static_cast<std::size_t>(i)]->contains(from))
        {
            rangeIndex = i;
            break;
        }
    }
    auto range = *nonEmpty[static_cast<std::size_t>(rangeIndex)];
    if (!range.contains(from))
        return range.getTopLeft();

    const auto height = range.rows.size();
    const auto width = range.cols.size();
    const auto rowOffset = from.row - range.rows.begin;
    const auto colOffset = from.col - range.cols.begin;
    auto index = columnMajor ? (colOffset * height) + rowOffset : (rowOffset * width) + colOffset;
    index += forward ? 1 : -1;

    if (index < 0 || index >= range.getNumCells())
    {
        rangeIndex = (rangeIndex + (forward ? 1 : count - 1)) % count;
        range = *nonEmpty[static_cast<std::size_t>(rangeIndex)];
        index = forward ? 0 : range.getNumCells() - 1;
    }

    const auto rangeHeight = range.rows.size();
    const auto rangeWidth = range.cols.size();
    if (columnMajor)
        return Cell{ .row = range.rows.begin + (index % rangeHeight), .col = range.cols.begin + (index / rangeHeight) };
    return Cell{ .row = range.rows.begin + (index / rangeWidth), .col = range.cols.begin + (index % rangeWidth) };
}

void Selection::clampTo(int rowCount, int colCount)
{
    for (auto& range : ranges)
        range = { .rows = clampRange(range.rows, rowCount), .cols = clampRange(range.cols, colCount) };
    std::erase_if(ranges, [](const CellRange& range) { return range.isEmpty(); });
    const auto clampCell = [&](std::optional<Cell>& cell) {
        if (cell)
            cell = clampCursor(cell, rowCount, colCount);
    };
    clampCell(anchor);
    clampCell(corner);
}

void Selection::applyChange(const Change& change, std::optional<Cell> collapseCell)
{
    if (std::holds_alternative<CellsChanged>(change))
        return;
    if (std::holds_alternative<RowsReordered>(change) || std::holds_alternative<Reset>(change))
    {
        collapseTo(collapseCell);
        return;
    }

    std::vector<CellRange> remapped;
    for (const auto& range : ranges)
        if (const auto rows = remapRows(range.rows, change))
            remapped.push_back({ .rows = *rows, .cols = range.cols });
    ranges = std::move(remapped);

    const auto remapCell = [&](std::optional<Cell>& cell) {
        if (!cell)
            return;
        if (const auto row = remapRow(cell->row, change))
            cell->row = *row;
        else if (const auto* removed = std::get_if<RowsRemoved>(&change))
            cell->row = rowAfterRemoval(cell->row, *removed);
    };
    remapCell(anchor);
    remapCell(corner);
    if (ranges.empty())
        collapseTo(collapseCell);
}

} // namespace jgrid
