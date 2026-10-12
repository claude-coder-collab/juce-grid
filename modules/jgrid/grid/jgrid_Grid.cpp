#include "jgrid_Grid.h"

#include "../core/jgrid_Editing.h"
#include "../core/jgrid_HeaderLabels.h"
#include "jgrid_GridInternals.h"

#include <algorithm>
#include <cmath>

namespace jgrid
{

namespace
{

constexpr int headerPadding = 12;
constexpr int minRowHeaderWidth = 30;
constexpr int cursorThickness = 2;

int fallbackColourId(int colourId)
{
    switch (colourId)
    {
        case Grid::backgroundColourId:
            return juce::ListBox::backgroundColourId;
        case Grid::textColourId:
            return juce::ListBox::textColourId;
        case Grid::headerBackgroundColourId:
            return juce::TableHeaderComponent::backgroundColourId;
        case Grid::headerTextColourId:
            return juce::TableHeaderComponent::textColourId;
        case Grid::gridLineColourId:
            return juce::TableHeaderComponent::outlineColourId;
        case Grid::selectionColourId:
            return juce::TextEditor::highlightColourId;
        default:
            return colourId;
    }
}

int rescaleWheelDistance(float distance, int stepSize)
{
    if (distance == 0.0f)
        return 0;
    const auto scaled = distance * 14.0f * static_cast<float>(stepSize);
    return juce::roundToInt(scaled < 0.0f ? std::min(scaled, -1.0f) : std::max(scaled, 1.0f));
}

struct KeyMove
{
    Move move;
    bool extend;
};

std::optional<KeyMove> moveForKey(const juce::KeyPress& key)
{
    const auto mods = key.getModifiers();
    if (mods.isAltDown())
        return std::nullopt;

    const auto command = mods.isCommandDown();
    const auto extend = mods.isShiftDown();
    const auto code = key.getKeyCode();
    const auto make = [extend](Move move) { return std::optional<KeyMove>(KeyMove{ .move = move, .extend = extend }); };
    if (code == juce::KeyPress::homeKey)
        return make(command ? Move::gridStart : Move::rowStart);
    if (code == juce::KeyPress::endKey)
        return make(command ? Move::gridEnd : Move::rowEnd);
    if (code == juce::KeyPress::upKey)
        return make(command ? Move::columnTop : Move::up);
    if (code == juce::KeyPress::downKey)
        return make(command ? Move::columnBottom : Move::down);
    if (code == juce::KeyPress::leftKey)
        return make(command ? Move::rowStart : Move::left);
    if (code == juce::KeyPress::rightKey)
        return make(command ? Move::rowEnd : Move::right);
    if (command)
        return std::nullopt;
    if (code == juce::KeyPress::pageUpKey)
        return make(Move::pageUp);
    if (code == juce::KeyPress::pageDownKey)
        return make(Move::pageDown);
    return std::nullopt;
}

} // namespace

Grid::Body::Body(Grid& owner) : grid(owner)
{
}

void Grid::Body::paint(juce::Graphics& g) { grid.paintBody(g); }

void Grid::Body::mouseDoubleClick(const juce::MouseEvent& event)
{
    const auto position = event.getEventRelativeTo(&grid).getPosition();
    if (const auto cell = grid.getCellAt(position))
        grid.editAt(*cell, position);
}

std::unique_ptr<juce::AccessibilityHandler> Grid::Body::createAccessibilityHandler()
{
    return createIgnoredAccessibilityHandler(*this);
}

Grid::ColumnHeader::ColumnHeader(Grid& owner) : grid(owner)
{
    setFocusContainerType(FocusContainerType::focusContainer);
}

void Grid::ColumnHeader::paint(juce::Graphics& g) { grid.paintColumnHeader(g, getWidth(), getHeight()); }

std::unique_ptr<juce::AccessibilityHandler> Grid::ColumnHeader::createAccessibilityHandler()
{
    return std::make_unique<juce::AccessibilityHandler>(*this, juce::AccessibilityRole::tableHeader);
}

Grid::RowHeader::RowHeader(Grid& owner) : grid(owner)
{
}

void Grid::RowHeader::paint(juce::Graphics& g) { grid.paintRowHeader(g, getWidth(), getHeight()); }

std::unique_ptr<juce::AccessibilityHandler> Grid::RowHeader::createAccessibilityHandler()
{
    return createIgnoredAccessibilityHandler(*this);
}

Grid::Grid() : body(std::make_unique<Body>(*this)),
               columnHeader(std::make_unique<ColumnHeader>(*this)),
               rowHeader(std::make_unique<RowHeader>(*this)),
               accessibility(std::make_unique<Accessibility>(*this)),
               errorPopup(std::make_unique<ErrorPopup>(*this)),
               shaker(std::make_unique<Shaker>(*this))
{
    setWantsKeyboardFocus(true);
    setFocusContainerType(FocusContainerType::focusContainer);
    setTitle("Grid");
    setHelpText("Use Ctrl+Tab to leave the grid.");
    addAndMakeVisible(*body);
    addAndMakeVisible(*columnHeader);
    addAndMakeVisible(*rowHeader);
    for (auto* scrollBar : { &verticalScrollBar, &horizontalScrollBar })
    {
        scrollBar->setAutoHide(false);
        scrollBar->addListener(this);
        addChildComponent(*scrollBar);
    }
    addChildComponent(*errorPopup);
}

Grid::~Grid()
{
    if (getCutRange())
        clearCutMark();
    unsubscribe();
    shaker->stop();
    editor.reset();
    invalidateAccessibilityHandler();
    accessibility.reset();
    verticalScrollBar.removeListener(this);
    horizontalScrollBar.removeListener(this);
}

void Grid::setModel(GridModel newModel)
{
    closeEditor();
    unsubscribe();
    model = std::move(newModel);
    subscribe();
    cursor.reset();
    sortIndicator.reset();
    hoverSortCol.reset();
    selection.collapseTo(std::nullopt);
    scroll = {};
    refresh();
}

const GridModel& Grid::getModel() const { return model; }

void Grid::refresh()
{
    const auto previousRows = rows.getVisibleCount();
    const auto previousCols = cols.getVisibleCount();
    const auto complete = model.isComplete();
    rows.setCount(complete ? std::max(0, model.rowCount()) : 0);
    cols.setCount(complete ? std::max(0, model.colCount()) : 0);

    const auto font = cellFont();
    auto widest = 0.0f;
    for (auto row = 0; row < rows.getCount(); ++row)
        widest = std::max(widest, juce::GlyphArrangement::getStringWidth(font, getRowLabel(row)));
    rowHeaderWidth = std::max(minRowHeaderWidth, static_cast<int>(std::ceil(widest)) + headerPadding);

    cursor = clampCursor(cursor, rows, cols);
    selection.clampTo(rows.getCount(), cols.getCount());
    if (selection.isEmpty() || !cursor)
        selection.collapseTo(cursor);
    if (editingCell && (editingCell->row >= rows.getCount() || editingCell->col >= cols.getCount()))
        closeEditor();
    updateLayout();
    accessibility->refresh();
    if (rows.getVisibleCount() != previousRows || cols.getVisibleCount() != previousCols)
        accessibility->tableStructureChanged();
    repaint();
}

int Grid::getRowCount() const { return rows.getCount(); }

int Grid::getColCount() const { return cols.getCount(); }

void Grid::setRowHeaderVisible(bool shouldBeVisible)
{
    if (rowHeaderVisible == shouldBeVisible)
        return;
    rowHeaderVisible = shouldBeVisible;
    updateLayout();
    repaint();
}

bool Grid::isRowHeaderVisible() const { return rowHeaderVisible; }

void Grid::setColumnHeaderVisible(bool shouldBeVisible)
{
    if (columnHeaderVisible == shouldBeVisible)
        return;
    columnHeaderVisible = shouldBeVisible;
    updateLayout();
    accessibility->rebuild();
    repaint();
}

bool Grid::isColumnHeaderVisible() const { return columnHeaderVisible; }

std::optional<Cell> Grid::getCursor() const { return cursor; }

void Grid::scrollToCell(Cell cell)
{
    if (cell.row < 0 || cell.row >= rows.getCount() || cell.col < 0 || cell.col >= cols.getCount())
        return;

    auto target = scroll;
    const auto viewWidth = body->getWidth();
    const auto viewHeight = body->getHeight();

    if (rows.getStart(cell.row) < target.y)
        target.y = rows.getStart(cell.row);
    else if (rows.getEnd(cell.row) > target.y + viewHeight)
        target.y = rows.getEnd(cell.row) - viewHeight;

    if (cols.getStart(cell.col) < target.x)
        target.x = cols.getStart(cell.col);
    else if (cols.getEnd(cell.col) > target.x + viewWidth)
        target.x = cols.getEnd(cell.col) - viewWidth;

    setScroll(target);
}

std::optional<Cell> Grid::getCellAt(juce::Point<int> position) const
{
    if (!body->getBounds().contains(position))
        return std::nullopt;
    const auto content = position - body->getPosition() + scroll;
    const auto row = rows.indexAt(content.y);
    const auto col = cols.indexAt(content.x);
    if (row < 0 || col < 0)
        return std::nullopt;
    return Cell{ .row = row, .col = col };
}

juce::Rectangle<int> Grid::getCellBounds(Cell cell) const
{
    if (cell.row < 0 || cell.row >= rows.getCount() || cell.col < 0 || cell.col >= cols.getCount())
        return {};
    return juce::Rectangle<int>(cols.getStart(cell.col), rows.getStart(cell.row), cols.getSize(cell.col), rows.getSize(cell.row))
        + body->getPosition() - scroll;
}

juce::Colour Grid::getGridColour(int colourId) const
{
    for (const auto* component = static_cast<const Component*>(this); component != nullptr; component = component->getParentComponent())
        if (component->isColourSpecified(colourId))
            return component->findColour(colourId);
    if (getLookAndFeel().isColourSpecified(colourId))
        return getLookAndFeel().findColour(colourId);
    if (colourId == cursorColourId)
        return findColour(juce::TextEditor::highlightColourId, true).withAlpha(1.0f);
    return findColour(fallbackColourId(colourId), true);
}

void Grid::paint(juce::Graphics& g)
{
    g.fillAll(getGridColour(backgroundColourId));
    if (rowHeaderVisible && columnHeaderVisible)
    {
        const auto corner = juce::Rectangle<int>(0, 0, body->getX(), body->getY());
        g.setColour(getGridColour(headerBackgroundColourId));
        g.fillRect(corner);
        g.setColour(getGridColour(gridLineColourId));
        g.drawRect(corner.withTrimmedLeft(-1).withTrimmedTop(-1));
    }
}

void Grid::resized() { updateLayout(); }

bool Grid::keyPressed(const juce::KeyPress& key)
{
    if (editor)
        return editor->keyPressed(key);
    hideError();

    const auto code = key.getKeyCode();
    const auto mods = key.getModifiers();
    if (code == juce::KeyPress::escapeKey && getCutRange())
    {
        clearCutMark();
        return true;
    }
    if ((code == juce::KeyPress::backspaceKey || code == juce::KeyPress::deleteKey) && !mods.isAnyModifierKeyDown())
    {
        deleteSelection();
        return true;
    }
    if (mods.isCommandDown() && !mods.isAltDown() && !mods.isShiftDown() && code == '-')
        return deleteSelectedRows();
    if (mods.isCommandDown() && !mods.isAltDown() && (code == '=' || code == '+'))
        return insertBlankRows();
    if (key == juce::KeyPress('z', juce::ModifierKeys::commandModifier, 0))
        return undo();
    if (key == juce::KeyPress('z', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier, 0)
        || key == juce::KeyPress('y', juce::ModifierKeys::commandModifier, 0))
        return redo();
    if (key == juce::KeyPress(juce::KeyPress::F10Key, juce::ModifierKeys::shiftModifier, 0))
    {
        showContextMenuForKeyboard();
        return true;
    }
    if (key == juce::KeyPress('c', juce::ModifierKeys::commandModifier, 0))
    {
        copy();
        return true;
    }
    if (key == juce::KeyPress('x', juce::ModifierKeys::commandModifier, 0))
    {
        cut();
        return true;
    }
    if (key == juce::KeyPress('v', juce::ModifierKeys::commandModifier, 0))
    {
        paste();
        return true;
    }
    if (code == juce::KeyPress::tabKey)
    {
        if (mods.isAltDown() || (mods.isCommandDown() && !mods.isCtrlDown()))
            return false;
        advanceCursor(false, mods);
        return true;
    }
    if (code == juce::KeyPress::returnKey)
    {
        if (mods.isAltDown() || mods.isCommandDown())
            return false;
        advanceCursor(true, mods);
        return true;
    }
    if (code == juce::KeyPress::F2Key)
    {
        startEditing();
        return true;
    }
    if (key == juce::KeyPress('a', juce::ModifierKeys::commandModifier, 0))
    {
        selectAll();
        return true;
    }

    const auto keyMove = moveForKey(key);
    if (!keyMove)
    {
        if (mods.isCommandDown() || mods.isCtrlDown() || !isTypedText(juce::String::charToString(key.getTextCharacter())))
            return false;
        handleTypedKey(key);
        return true;
    }
    if (!cursor)
        return true;
    if (keyMove->extend)
    {
        const auto from = selection.getCorner().value_or(*cursor);
        extendSelection(moveCursor(from, keyMove->move, rows, cols, getPageRows()));
        return true;
    }
    setCursor(moveCursor(*cursor, keyMove->move, rows, cols, getPageRows()));
    return true;
}

void Grid::mouseWheelMove(const juce::MouseEvent& /*event*/, const juce::MouseWheelDetails& wheel)
{
    const auto step = rows.getDefaultSize();
    auto deltaX = wheel.deltaX;
    auto deltaY = wheel.deltaY;
    if (deltaX == 0.0f && juce::ModifierKeys::currentModifiers.isShiftDown())
        std::swap(deltaX, deltaY);
    setScroll(scroll - juce::Point<int>(rescaleWheelDistance(deltaX, step), rescaleWheelDistance(deltaY, step)));
}

void Grid::focusGained(FocusChangeType /*cause*/)
{
    repaint();
    juce::MessageManager::callAsync([safe = juce::Component::SafePointer<Grid>(this)] {
        if (safe == nullptr || !safe->hasKeyboardFocus(false))
            return;
        if (safe->editor)
            safe->editor->grabKeyboardFocus();
        else
            safe->accessibility->focusCursorCell();
    });
}

void Grid::focusLost(FocusChangeType /*cause*/)
{
    if (!editor)
        hideError();
    repaint();
}

void Grid::colourChanged() { repaint(); }

void Grid::lookAndFeelChanged() { refresh(); }

std::unique_ptr<juce::AccessibilityHandler> Grid::createAccessibilityHandler()
{
    return accessibility->createTableHandler();
}

void Grid::scrollBarMoved(juce::ScrollBar* scrollBar, double newRangeStart)
{
    const auto position = juce::roundToInt(newRangeStart);
    if (scrollBar == &verticalScrollBar)
        setScroll({ scroll.x, position });
    else
        setScroll({ position, scroll.y });
}

void Grid::updateLayout()
{
    const auto scrollBarSize = getLookAndFeel().getDefaultScrollbarWidth();
    const auto top = columnHeaderVisible ? columnHeaderHeight : 0;
    const auto left = rowHeaderVisible ? rowHeaderWidth : 0;
    const auto contentWidth = cols.getTotalSize();
    const auto contentHeight = rows.getTotalSize();

    auto needVertical = false;
    auto needHorizontal = false;
    auto viewWidth = 0;
    auto viewHeight = 0;
    for (auto pass = 0; pass < 2; ++pass)
    {
        viewWidth = std::max(0, getWidth() - left - (needVertical ? scrollBarSize : 0));
        viewHeight = std::max(0, getHeight() - top - (needHorizontal ? scrollBarSize : 0));
        needHorizontal = contentWidth > viewWidth;
        needVertical = contentHeight > viewHeight;
    }
    viewWidth = std::max(0, getWidth() - left - (needVertical ? scrollBarSize : 0));
    viewHeight = std::max(0, getHeight() - top - (needHorizontal ? scrollBarSize : 0));

    body->setBounds(left, top, viewWidth, viewHeight);
    columnHeader->setBounds(left, 0, viewWidth, top);
    columnHeader->setVisible(columnHeaderVisible);
    rowHeader->setBounds(0, top, left, viewHeight);
    rowHeader->setVisible(rowHeaderVisible);

    verticalScrollBar.setVisible(needVertical);
    verticalScrollBar.setBounds(left + viewWidth, top, scrollBarSize, viewHeight);
    verticalScrollBar.setRangeLimits(0.0, static_cast<double>(contentHeight), juce::dontSendNotification);
    verticalScrollBar.setSingleStepSize(static_cast<double>(rows.getDefaultSize()));

    horizontalScrollBar.setVisible(needHorizontal);
    horizontalScrollBar.setBounds(left, top + viewHeight, viewWidth, scrollBarSize);
    horizontalScrollBar.setRangeLimits(0.0, static_cast<double>(contentWidth), juce::dontSendNotification);
    horizontalScrollBar.setSingleStepSize(static_cast<double>(cols.getDefaultSize()));

    setScroll(scroll);
    verticalScrollBar.setCurrentRange(scroll.y, viewHeight, juce::dontSendNotification);
    horizontalScrollBar.setCurrentRange(scroll.x, viewWidth, juce::dontSendNotification);
    scrollChanged();
}

void Grid::setScroll(juce::Point<int> newScroll)
{
    const auto maxX = std::max(0, cols.getTotalSize() - body->getWidth());
    const auto maxY = std::max(0, rows.getTotalSize() - body->getHeight());
    const auto clamped = juce::Point<int>(std::clamp(newScroll.x, 0, maxX), std::clamp(newScroll.y, 0, maxY));
    if (clamped == scroll)
        return;
    scroll = clamped;
    verticalScrollBar.setCurrentRange(scroll.y, body->getHeight(), juce::dontSendNotification);
    horizontalScrollBar.setCurrentRange(scroll.x, body->getWidth(), juce::dontSendNotification);
    scrollChanged();
}

void Grid::scrollChanged()
{
    layoutEditor();
    body->repaint();
    columnHeader->repaint();
    rowHeader->repaint();
    accessibility->sync();
}

void Grid::cursorChanged()
{
    body->repaint();
    columnHeader->repaint();
    rowHeader->repaint();
    accessibility->sync();
    accessibility->focusCursorCell();
}

int Grid::getPageRows() const { return std::max(1, rows.indicesFullyIn(scroll.y, body->getHeight()).size()); }

juce::String Grid::getColumnLabel(int col) const { return model.colName ? model.colName(col) : columnLetters(col); }

juce::String Grid::getRowLabel(int row) const { return model.rowName ? model.rowName(row) : rowNumber(row); }

juce::String Grid::getCellText(Cell cell) const
{
    if (!model.getValue)
        return {};
    return model.getValue(cell.row, cell.col).toString();
}

IndexRange Grid::getVisibleRows() const { return rows.indicesIn(scroll.y, body->getHeight()); }

IndexRange Grid::getVisibleCols() const { return cols.indicesIn(scroll.x, body->getWidth()); }

void Grid::paintBody(juce::Graphics& g) const
{
    g.fillAll(getGridColour(backgroundColourId));

    const auto visibleRows = getVisibleRows();
    const auto visibleCols = getVisibleCols();
    const auto contentRight = cols.getTotalSize() - scroll.x;
    const auto contentBottom = rows.getTotalSize() - scroll.y;

    if (!selection.isSingleCell())
    {
        g.setColour(getGridColour(selectionColourId));
        for (auto row = visibleRows.begin; row < visibleRows.end; ++row)
            for (auto col = visibleCols.begin; col < visibleCols.end; ++col)
                if (selection.contains({ .row = row, .col = col }) && cursor != Cell{ .row = row, .col = col })
                    g.fillRect(cols.getStart(col) - scroll.x, rows.getStart(row) - scroll.y, cols.getSize(col), rows.getSize(row));
    }

    g.setFont(cellFont());
    g.setColour(getGridColour(textColourId));
    for (auto row = visibleRows.begin; row < visibleRows.end; ++row)
    {
        if (rows.isHidden(row))
            continue;
        for (auto col = visibleCols.begin; col < visibleCols.end; ++col)
        {
            if (cols.isHidden(col))
                continue;
            const auto value = model.getValue(row, col);
            const auto area = juce::Rectangle<int>(cols.getStart(col) - scroll.x, rows.getStart(row) - scroll.y, cols.getSize(col), rows.getSize(row));
            const auto numeric = value.isInt() || value.isInt64() || value.isDouble();
            g.drawText(value.toString(), area.reduced(cellPadding, 0), numeric ? juce::Justification::centredRight : juce::Justification::centredLeft, true);
        }
    }

    g.setColour(getGridColour(gridLineColourId));
    for (auto col = visibleCols.begin; col < visibleCols.end; ++col)
        g.fillRect(cols.getEnd(col) - scroll.x - 1, 0, 1, std::min(contentBottom, getHeight()));
    for (auto row = visibleRows.begin; row < visibleRows.end; ++row)
        g.fillRect(0, rows.getEnd(row) - scroll.y - 1, std::min(contentRight, getWidth()), 1);

    if (cursor)
    {
        const auto area = getCellBounds(*cursor) - body->getPosition();
        g.setColour(getGridColour(cursorColourId));
        g.drawRect(area, cursorThickness);
    }
    paintCutMark(g);
}

void Grid::paintColumnHeader(juce::Graphics& g, int width, int height) const
{
    g.fillAll(getGridColour(headerBackgroundColourId));
    const auto visibleCols = cols.indicesIn(scroll.x, width);
    g.setFont(cellFont());
    for (auto col = visibleCols.begin; col < visibleCols.end; ++col)
    {
        if (cols.isHidden(col))
            continue;
        const auto area = juce::Rectangle<int>(cols.getStart(col) - scroll.x, 0, cols.getSize(col), height);
        if ((cursor && cursor->col == col) || selection.intersectsCol(col))
        {
            g.setColour(getGridColour(selectionColourId));
            g.fillRect(area);
            if (selection.isWholeColumnSelected(col, rows.getCount()))
                g.fillRect(area);
        }
        g.setColour(getGridColour(headerTextColourId));
        const auto labelArea = canSort() ? area.reduced(cellPadding, 0).withTrimmedRight(14) : area.reduced(cellPadding, 0);
        g.drawText(getColumnLabel(col), labelArea, juce::Justification::centred, true);
        paintSortButton(g, area, col);
        g.setColour(getGridColour(gridLineColourId));
        g.fillRect(area.getRight() - 1, 0, 1, height);
    }
    g.setColour(getGridColour(gridLineColourId));
    g.fillRect(0, height - 1, width, 1);
}

void Grid::paintRowHeader(juce::Graphics& g, int width, int height) const
{
    g.fillAll(getGridColour(headerBackgroundColourId));
    const auto visibleRows = rows.indicesIn(scroll.y, height);
    g.setFont(cellFont());
    for (auto row = visibleRows.begin; row < visibleRows.end; ++row)
    {
        if (rows.isHidden(row))
            continue;
        const auto area = juce::Rectangle<int>(0, rows.getStart(row) - scroll.y, width, rows.getSize(row));
        if ((cursor && cursor->row == row) || selection.intersectsRow(row))
        {
            g.setColour(getGridColour(selectionColourId));
            g.fillRect(area);
            if (selection.isWholeRowSelected(row, cols.getCount()))
                g.fillRect(area);
        }
        g.setColour(getGridColour(headerTextColourId));
        g.drawText(getRowLabel(row), area.reduced(cellPadding, 0), juce::Justification::centredRight, true);
        g.setColour(getGridColour(gridLineColourId));
        g.fillRect(0, area.getBottom() - 1, width, 1);
    }
    g.setColour(getGridColour(gridLineColourId));
    g.fillRect(width - 1, 0, 1, height);
}

} // namespace jgrid
