#include "../core/jgrid_Sorting.h"
#include "jgrid_GridInternals.h"

namespace jgrid
{

namespace
{

constexpr int sortButtonWidth = 18;

} // namespace

void Grid::ColumnHeader::mouseMove(const juce::MouseEvent& event)
{
    const auto x = event.getEventRelativeTo(&grid).x;
    setMouseCursor(grid.columnEdgeAt(x) ? juce::MouseCursor::LeftRightResizeCursor : juce::MouseCursor::NormalCursor);
    const auto col = grid.getNearestCol(x);
    const auto hover = grid.canSort() && col >= 0 ? std::optional<int>(col) : std::nullopt;
    if (hover != grid.hoverSortCol)
    {
        grid.hoverSortCol = hover;
        repaint();
    }
}

void Grid::ColumnHeader::mouseExit(const juce::MouseEvent& /*event*/)
{
    if (grid.hoverSortCol)
    {
        grid.hoverSortCol.reset();
        repaint();
    }
}

bool Grid::canSort() const { return model.reorderRows != nullptr && model.isComplete(); }

bool Grid::sortByColumn(int col, bool descending)
{
    if (!canSort() || col < 0 || col >= cols.getCount())
        return false;
    if (editor && !commitEdit())
        return false;

    const auto compare = [this, col](int a, int b) {
        return model.compare ? model.compare(col, a, b) : compareValues(model.getValue(a, col), model.getValue(b, col));
    };
    const auto order = sortedOrder(rows.getCount(), compare, descending);
    if (!isIdentityOrder(order))
    {
        sortInProgress = true;
        const juce::ScopeGuard done{ [this] { sortInProgress = false; } };
        if (model.beginBatch)
            model.beginBatch();
        const juce::ScopeGuard endBatch{ [this] {
            if (model.endBatch)
                model.endBatch();
        } };
        model.reorderRows(order);
        notifySelf(RowsReordered{ .order = order });
    }
    sortIndicator = SortIndicator{ .col = col, .descending = descending };
    columnHeader->repaint();
    accessibility->headersChanged();
    postAnnouncement("Sorted by " + getColumnLabel(col) + (descending ? ", descending" : ", ascending"));
    return true;
}

bool Grid::toggleSort(int col)
{
    const auto descending = sortIndicator && sortIndicator->col == col && !sortIndicator->descending;
    return sortByColumn(col, descending);
}

std::optional<Grid::SortIndicator> Grid::getSortIndicator() const { return sortIndicator; }

void Grid::sortTouched(const Change& change)
{
    if (!sortIndicator || sortInProgress)
        return;
    if (const auto* cells = std::get_if<CellsChanged>(&change); cells != nullptr && (sortIndicator->col < cells->firstCol || sortIndicator->col >= cells->firstCol + cells->numCols))
        return;
    sortIndicator.reset();
    columnHeader->repaint();
    accessibility->headersChanged();
}

void Grid::notifySelf(const Change& change)
{
    if (subscribedNotifier == nullptr)
        modelChanged(change);
}

bool Grid::isOnSortButton(int col, int xInGrid) const
{
    if (!canSort() || col < 0 || col >= cols.getCount())
        return false;
    const auto contentX = xInGrid - body->getX() + scroll.x;
    return contentX < cols.getEnd(col) && cols.getEnd(col) - contentX <= sortButtonWidth;
}

void Grid::paintSortButton(juce::Graphics& g, juce::Rectangle<int> columnArea, int col) const
{
    const auto sorted = sortIndicator && sortIndicator->col == col;
    if (!canSort() || (!sorted && hoverSortCol != col))
        return;
    const auto button = columnArea.removeFromRight(sortButtonWidth).toFloat().withSizeKeepingCentre(8.0f, 10.0f);
    const auto colour = getGridColour(headerTextColourId);
    juce::Path arrows;
    if (sorted)
    {
        const auto down = sortIndicator->descending;
        const auto top = down ? button.getY() + 2.0f : button.getBottom() - 2.0f;
        const auto tip = down ? button.getBottom() - 2.0f : button.getY() + 2.0f;
        arrows.addTriangle(button.getX(), top, button.getRight(), top, button.getCentreX(), tip);
        g.setColour(colour);
    }
    else
    {
        arrows.addTriangle(button.getX(), button.getCentreY() - 1.0f, button.getRight(), button.getCentreY() - 1.0f, button.getCentreX(), button.getY());
        arrows.addTriangle(button.getX(), button.getCentreY() + 1.0f, button.getRight(), button.getCentreY() + 1.0f, button.getCentreX(), button.getBottom());
        g.setColour(colour.withMultipliedAlpha(0.4f));
    }
    g.fillPath(arrows);
}

juce::String Grid::getColumnAccessibleTitle(int col) const
{
    auto title = getColumnLabel(col);
    if (sortIndicator && sortIndicator->col == col)
        title << (sortIndicator->descending ? ", sorted descending" : ", sorted ascending");
    return title;
}

} // namespace jgrid
