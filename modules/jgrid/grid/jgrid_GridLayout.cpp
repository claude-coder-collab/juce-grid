#include "jgrid_GridInternals.h"

#include <algorithm>
#include <cmath>

namespace jgrid
{

namespace
{

constexpr int edgeTolerance = 4;
constexpr int sortButtonAllowance = 14;
constexpr int rowPadding = 8;

int textWidth(const juce::String& text) { return static_cast<int>(std::ceil(juce::GlyphArrangement::getStringWidth(cellFont(), text))); }

} // namespace

void Grid::ColumnHeader::mouseUp(const juce::MouseEvent& /*event*/) { grid.resizing.reset(); }

void Grid::ColumnHeader::mouseDoubleClick(const juce::MouseEvent& event)
{
    if (const auto col = grid.columnEdgeAt(event.getEventRelativeTo(&grid).x))
        grid.autoFitColumn(*col);
}

void Grid::RowHeader::mouseUp(const juce::MouseEvent& /*event*/) { grid.resizing.reset(); }

void Grid::RowHeader::mouseDoubleClick(const juce::MouseEvent& event)
{
    if (const auto row = grid.rowEdgeAt(event.getEventRelativeTo(&grid).y))
        grid.autoFitRow(*row);
}

void Grid::RowHeader::mouseMove(const juce::MouseEvent& event)
{
    setMouseCursor(grid.rowEdgeAt(event.getEventRelativeTo(&grid).y) ? juce::MouseCursor::UpDownResizeCursor : juce::MouseCursor::NormalCursor);
}

void Grid::setColumnWidth(int col, int width)
{
    if (col < 0 || col >= cols.getCount())
        return;
    cols.setSize(col, std::max(minimumCellSize, width));
    updateLayout();
    repaint();
}

int Grid::getColumnWidth(int col) const { return col >= 0 && col < cols.getCount() ? cols.getStoredSize(col) : 0; }

void Grid::setRowHeight(int row, int height)
{
    if (row < 0 || row >= rows.getCount())
        return;
    rows.setSize(row, std::max(minimumCellSize, height));
    updateLayout();
    repaint();
}

int Grid::getRowHeight(int row) const { return row >= 0 && row < rows.getCount() ? rows.getStoredSize(row) : 0; }

int Grid::measureColumn(int col) const
{
    auto widest = textWidth(getColumnLabel(col)) + (canSort() ? sortButtonAllowance : 0);
    for (auto row = 0; row < rows.getCount(); ++row)
        widest = std::max(widest, textWidth(getCellText({ .row = row, .col = col })));
    return widest + (2 * cellPadding) + 2;
}

void Grid::autoFitColumn(int col)
{
    if (col >= 0 && col < cols.getCount())
        setColumnWidth(col, measureColumn(col));
}

void Grid::autoFitRow(int row) { setRowHeight(row, static_cast<int>(std::ceil(cellFont().getHeight())) + rowPadding); }

void Grid::setColumnsHidden(const std::vector<int>& columns, bool hidden)
{
    for (const auto col : columns)
        if (col >= 0 && col < cols.getCount())
            cols.setHidden(col, hidden);
    hiddenStateChanged();
}

void Grid::setRowsHidden(const std::vector<int>& rowIndices, bool hidden)
{
    for (const auto row : rowIndices)
        if (row >= 0 && row < rows.getCount())
            rows.setHidden(row, hidden);
    hiddenStateChanged();
}

void Grid::showAllColumns() { setColumnsHidden(cols.getHidden(), false); }

void Grid::showAllRows() { setRowsHidden(rows.getHidden(), false); }

bool Grid::isColumnHidden(int col) const { return col >= 0 && col < cols.getCount() && cols.isHidden(col); }

bool Grid::isRowHidden(int row) const { return row >= 0 && row < rows.getCount() && rows.isHidden(row); }

std::vector<int> Grid::getHiddenColumns() const { return cols.getHidden(); }

std::vector<int> Grid::getHiddenRows() const { return rows.getHidden(); }

void Grid::hiddenStateChanged()
{
    if (editingCell && !(rows.isVisible(editingCell->row) && cols.isVisible(editingCell->col)))
        closeEditor();
    const auto clamped = clampCursor(cursor, rows, cols);
    if (clamped != cursor)
    {
        cursor = clamped;
        selection.collapseTo(cursor);
    }
    updateLayout();
    accessibility->rebuild();
    accessibility->tableStructureChanged();
    repaint();
}

std::optional<int> Grid::columnEdgeAt(int xInGrid) const
{
    if (cols.getVisibleCount() == 0)
        return std::nullopt;
    const auto contentX = xInGrid - body->getX() + scroll.x;
    if (contentX >= cols.getTotalSize())
        return contentX - cols.getTotalSize() <= edgeTolerance ? std::optional<int>(cols.lastVisible()) : std::nullopt;
    const auto col = cols.indexAt(contentX);
    if (col < 0)
        return std::nullopt;
    if (cols.getEnd(col) - contentX <= edgeTolerance)
        return col;
    if (contentX - cols.getStart(col) <= edgeTolerance)
        if (const auto previous = cols.nextVisible(col, false); previous >= 0)
            return previous;
    return std::nullopt;
}

std::optional<int> Grid::rowEdgeAt(int yInGrid) const
{
    if (rows.getVisibleCount() == 0)
        return std::nullopt;
    const auto contentY = yInGrid - body->getY() + scroll.y;
    if (contentY >= rows.getTotalSize())
        return contentY - rows.getTotalSize() <= edgeTolerance ? std::optional<int>(rows.lastVisible()) : std::nullopt;
    const auto row = rows.indexAt(contentY);
    if (row < 0)
        return std::nullopt;
    if (rows.getEnd(row) - contentY <= edgeTolerance)
        return row;
    if (contentY - rows.getStart(row) <= edgeTolerance)
        if (const auto previous = rows.nextVisible(row, false); previous >= 0)
            return previous;
    return std::nullopt;
}

} // namespace jgrid
