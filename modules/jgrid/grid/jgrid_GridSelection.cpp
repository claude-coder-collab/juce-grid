#include "../core/jgrid_HeaderLabels.h"
#include "jgrid_GridInternals.h"

#include <algorithm>

namespace jgrid
{

void Grid::Body::mouseDown(const juce::MouseEvent& event)
{
    if (grid.isEditing() && !grid.commitEdit())
        return;
    grid.hideError();
    grid.grabKeyboardFocus();
    const auto position = event.getEventRelativeTo(&grid).getPosition();
    const auto cell = grid.getCellAt(position);
    if (!cell)
        return;
    if (event.mods.isPopupMenu())
    {
        if (!grid.selection.contains(*cell))
            grid.setCursor(*cell);
        grid.showContextMenu(ContextMenuTarget::Kind::cells, { event.getScreenX(), event.getScreenY(), 1, 1 });
        return;
    }
    if (event.mods.isShiftDown())
        grid.extendSelection(*cell);
    else if (event.mods.isCommandDown())
        grid.addToSelection(*cell);
    else
        grid.setCursor(*cell);
    beginDragAutoRepeat(50);
}

void Grid::Body::mouseDrag(const juce::MouseEvent& event)
{
    if (grid.isEditing() || !grid.cursor || event.mouseWasClicked())
        return;
    const auto cell = grid.getNearestCell(event.getEventRelativeTo(&grid).getPosition());
    grid.scrollToCell(cell);
    if (grid.selection.getCorner() != cell)
        grid.extendSelection(cell);
}

void Grid::ColumnHeader::mouseDown(const juce::MouseEvent& event)
{
    if (grid.isEditing() && !grid.commitEdit())
        return;
    grid.grabKeyboardFocus();
    const auto x = event.getEventRelativeTo(&grid).x;
    if (const auto edge = grid.columnEdgeAt(x); edge && event.mods.isLeftButtonDown())
    {
        grid.headerDragStart.reset();
        grid.resizing = Resize{ .column = true, .index = *edge, .startSize = grid.cols.getStoredSize(*edge), .startPosition = x };
        return;
    }
    const auto col = grid.getNearestCol(x);
    if (col < 0)
        return;
    if (event.mods.isPopupMenu())
    {
        if (grid.selection.isWholeColumnSelected(col, grid.rows.getCount()))
            grid.placeCursor({ .row = grid.cursor ? grid.cursor->row : 0, .col = col }, false);
        else
            grid.selectColumns(col, col);
        grid.showContextMenu(ContextMenuTarget::Kind::columns, { event.getScreenX(), event.getScreenY(), 1, 1 });
        return;
    }
    if (!event.mods.isAnyModifierKeyDown() && grid.isOnSortButton(col, x))
    {
        grid.headerDragStart.reset();
        grid.toggleSort(col);
        return;
    }
    const auto anchorCol = grid.selection.getAnchor() ? grid.selection.getAnchor()->col : col;
    if (event.mods.isShiftDown())
    {
        grid.headerDragStart = anchorCol;
        grid.selectColumns(anchorCol, col);
    }
    else
    {
        grid.headerDragStart = col;
        grid.selectColumns(col, col, event.mods.isCommandDown());
    }
    beginDragAutoRepeat(50);
}

void Grid::ColumnHeader::mouseDrag(const juce::MouseEvent& event)
{
    if (grid.resizing && grid.resizing->column)
    {
        grid.setColumnWidth(grid.resizing->index, grid.resizing->startSize + event.getEventRelativeTo(&grid).x - grid.resizing->startPosition);
        return;
    }
    if (!grid.headerDragStart || event.mouseWasClicked() || grid.rows.getCount() == 0)
        return;
    const auto col = grid.getNearestCol(event.getEventRelativeTo(&grid).x);
    if (col < 0)
        return;
    if (grid.cursor)
        grid.scrollToCell({ .row = grid.cursor->row, .col = col });
    grid.selection.extendTo({ .row = grid.rows.getCount() - 1, .col = col });
    grid.selectionChanged(true);
}

void Grid::RowHeader::mouseDown(const juce::MouseEvent& event)
{
    if (grid.isEditing() && !grid.commitEdit())
        return;
    grid.grabKeyboardFocus();
    const auto y = event.getEventRelativeTo(&grid).y;
    if (const auto edge = grid.rowEdgeAt(y); edge && event.mods.isLeftButtonDown())
    {
        grid.headerDragStart.reset();
        grid.resizing = Resize{ .column = false, .index = *edge, .startSize = grid.rows.getStoredSize(*edge), .startPosition = y };
        return;
    }
    const auto row = grid.getNearestRow(y);
    if (row < 0)
        return;
    if (event.mods.isPopupMenu())
    {
        if (grid.selection.isWholeRowSelected(row, grid.cols.getCount()))
            grid.placeCursor({ .row = row, .col = grid.cursor ? grid.cursor->col : 0 }, false);
        else
            grid.selectRows(row, row);
        grid.showContextMenu(ContextMenuTarget::Kind::rows, { event.getScreenX(), event.getScreenY(), 1, 1 });
        return;
    }
    const auto anchorRow = grid.selection.getAnchor() ? grid.selection.getAnchor()->row : row;
    if (event.mods.isShiftDown())
    {
        grid.headerDragStart = anchorRow;
        grid.selectRows(anchorRow, row);
    }
    else
    {
        grid.headerDragStart = row;
        grid.selectRows(row, row, event.mods.isCommandDown());
    }
    beginDragAutoRepeat(50);
}

void Grid::RowHeader::mouseDrag(const juce::MouseEvent& event)
{
    if (grid.resizing && !grid.resizing->column)
    {
        grid.setRowHeight(grid.resizing->index, grid.resizing->startSize + event.getEventRelativeTo(&grid).y - grid.resizing->startPosition);
        return;
    }
    if (!grid.headerDragStart || event.mouseWasClicked() || grid.cols.getCount() == 0)
        return;
    const auto row = grid.getNearestRow(event.getEventRelativeTo(&grid).y);
    if (row < 0)
        return;
    if (grid.cursor)
        grid.scrollToCell({ .row = row, .col = grid.cursor->col });
    grid.selection.extendTo({ .row = row, .col = grid.cols.getCount() - 1 });
    grid.selectionChanged(true);
}

void Grid::setCursor(Cell cell)
{
    const auto clamped = clampCursor(cell, rows, cols);
    if (!clamped)
        return;
    const auto hadRange = !selection.isSingleCell() || selection.getAnchor() != clamped;
    selection.collapseTo(clamped);
    placeCursor(*clamped, true);
    if (hadRange)
        selectionChanged(false);
}

const Selection& Grid::getSelection() const { return selection; }

void Grid::extendSelection(Cell corner)
{
    const auto clamped = clampCursor(corner, rows, cols);
    if (!clamped || !cursor)
        return;
    if (!selection.getAnchor())
        selection.collapseTo(cursor);
    selection.extendTo(*clamped);
    scrollToCell(*clamped);
    selectionChanged(true);
}

void Grid::addToSelection(Cell cell)
{
    const auto clamped = clampCursor(cell, rows, cols);
    if (!clamped)
        return;
    selection.addCell(*clamped);
    placeCursor(*clamped, true);
    selectionChanged(false);
}

void Grid::selectAll()
{
    if (!cursor)
        return;
    selection.selectAll(rows.getCount(), cols.getCount());
    selectionChanged(true);
}

void Grid::selectRows(int first, int last, bool addToExisting)
{
    if (rows.getCount() == 0 || cols.getCount() == 0)
        return;
    first = std::clamp(first, 0, rows.getCount() - 1);
    last = std::clamp(last, 0, rows.getCount() - 1);
    selection.selectRows(first, last, cols.getCount(), addToExisting);
    const auto visibleCols = getVisibleCols();
    placeCursor({ .row = first, .col = visibleCols.isEmpty() ? 0 : visibleCols.begin }, false);
    selectionChanged(true);
}

void Grid::selectColumns(int first, int last, bool addToExisting)
{
    if (rows.getCount() == 0 || cols.getCount() == 0)
        return;
    first = std::clamp(first, 0, cols.getCount() - 1);
    last = std::clamp(last, 0, cols.getCount() - 1);
    selection.selectCols(first, last, rows.getCount(), addToExisting);
    const auto visibleRows = getVisibleRows();
    placeCursor({ .row = visibleRows.isEmpty() ? 0 : visibleRows.begin, .col = first }, false);
    selectionChanged(true);
}

void Grid::placeCursor(Cell cell, bool scrollIntoView)
{
    const auto moved = cursor != cell;
    cursor = cell;
    if (scrollIntoView)
        scrollToCell(cell);
    if (moved)
        cursorChanged();
}

void Grid::selectionChanged(bool announce)
{
    accessibility->rowSelectionChanged(getSelectedRows());
    body->repaint();
    columnHeader->repaint();
    rowHeader->repaint();
    if (announce && !selection.isSingleCell() && hasKeyboardFocus(true))
        postAnnouncement(describeSelection(selection.getNumCells()));
}

void Grid::advanceCursor(bool enter, juce::ModifierKeys mods)
{
    if (!cursor)
        return;
    const auto forward = !mods.isShiftDown();
    if (!enter && mods.isCtrlDown())
    {
        moveKeyboardFocusToSibling(forward);
        return;
    }
    if (!selection.isSingleCell())
    {
        auto next = selection.step(*cursor, forward, enter);
        for (auto tries = selection.getNumCells(); next && tries > 0 && !(rows.isVisible(next->row) && cols.isVisible(next->col)); --tries)
            next = selection.step(*next, forward, enter);
        if (next && rows.isVisible(next->row) && cols.isVisible(next->col))
            placeCursor(*next, true);
        return;
    }
    if (enter)
    {
        setCursor(moveCursor(*cursor, forward ? Move::down : Move::up, rows, cols, getPageRows()));
        return;
    }
    if (const auto next = stepCell(*cursor, forward, rows, cols))
        setCursor(*next);
    else
        moveKeyboardFocusToSibling(forward);
}

int Grid::getNearestRow(int yInGrid) const
{
    if (rows.getCount() == 0)
        return -1;
    const auto content = std::clamp(yInGrid - body->getY() + scroll.y, 0, std::max(0, rows.getTotalSize() - 1));
    return std::max(0, rows.indexAt(content));
}

int Grid::getNearestCol(int xInGrid) const
{
    if (cols.getCount() == 0)
        return -1;
    const auto content = std::clamp(xInGrid - body->getX() + scroll.x, 0, std::max(0, cols.getTotalSize() - 1));
    return std::max(0, cols.indexAt(content));
}

Cell Grid::getNearestCell(juce::Point<int> positionInGrid) const
{
    return { .row = std::max(0, getNearestRow(positionInGrid.y)), .col = std::max(0, getNearestCol(positionInGrid.x)) };
}

} // namespace jgrid
