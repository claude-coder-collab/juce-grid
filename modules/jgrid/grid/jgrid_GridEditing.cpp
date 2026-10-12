#include "../core/jgrid_Editing.h"
#include "jgrid_GridInternals.h"

#include <algorithm>

namespace jgrid
{

namespace
{

constexpr int popupMargin = 4;
constexpr int popupGap = 2;
constexpr int popupPadding = 8;
constexpr int popupHeight = 24;
constexpr float popupCornerSize = 4.0f;

juce::Font popupFont() { return juce::FontOptions{ 13.0f }; }

} // namespace

Grid::CellEditor::CellEditor(Grid& owner) : grid(owner)
{
}

bool Grid::CellEditor::keyPressed(const juce::KeyPress& key)
{
    return grid.handleEditorKey(key) || juce::TextEditor::keyPressed(key);
}

void Grid::CellEditor::insertTextAtCaret(const juce::String& textToInsert)
{
    grid.hideError();
    juce::TextEditor::insertTextAtCaret(textToInsert);
}

void Grid::CellEditor::focusLost(FocusChangeType cause)
{
    juce::TextEditor::focusLost(cause);
    grid.hideError();
}

Grid::ErrorPopup::ErrorPopup(Grid& owner) : grid(owner)
{
    setInterceptsMouseClicks(false, false);
    setWantsKeyboardFocus(false);
    setAlwaysOnTop(true);
}

void Grid::ErrorPopup::show(const juce::String& text, juce::Rectangle<int> cellBounds)
{
    message = text;
    reposition(cellBounds);
    setVisible(true);
    toFront(false);
    repaint();
    startTimer(errorTimeoutMilliseconds);
}

void Grid::ErrorPopup::reposition(juce::Rectangle<int> cellBounds)
{
    const auto textWidth = juce::roundToInt(std::ceil(juce::GlyphArrangement::getStringWidth(popupFont(), message)));
    const auto width = std::max(0, std::min(textWidth + (2 * popupPadding), grid.getWidth() - (2 * popupMargin)));
    const auto x = std::clamp(cellBounds.getX(), popupMargin, std::max(popupMargin, grid.getWidth() - width - popupMargin));
    auto y = cellBounds.getBottom() + popupGap;
    if (y + popupHeight > grid.getHeight())
        y = std::max(0, cellBounds.getY() - popupHeight - popupGap);
    setBounds(x, y, width, popupHeight);
}

void Grid::ErrorPopup::dismiss()
{
    stopTimer();
    message.clear();
    setVisible(false);
}

juce::String Grid::ErrorPopup::getMessage() const { return isVisible() ? message : juce::String(); }

void Grid::ErrorPopup::paint(juce::Graphics& g)
{
    const auto area = getLocalBounds().toFloat().reduced(0.5f);
    g.setColour(findColour(juce::TooltipWindow::backgroundColourId, true));
    g.fillRoundedRectangle(area, popupCornerSize);
    g.setColour(findColour(juce::TooltipWindow::outlineColourId, true));
    g.drawRoundedRectangle(area, popupCornerSize, 1.0f);
    g.setColour(findColour(juce::TooltipWindow::textColourId, true));
    g.setFont(popupFont());
    g.drawText(message, getLocalBounds().reduced(popupPadding, 0), juce::Justification::centredLeft, true);
}

std::unique_ptr<juce::AccessibilityHandler> Grid::ErrorPopup::createAccessibilityHandler()
{
    return createIgnoredAccessibilityHandler(*this);
}

void Grid::ErrorPopup::timerCallback() { dismiss(); }

Grid::Shaker::Shaker(Grid& owner) : grid(owner)
{
}

void Grid::Shaker::start()
{
    startMilliseconds = juce::Time::getMillisecondCounterHiRes();
    startTimerHz(60);
}

void Grid::Shaker::stop()
{
    stopTimer();
    startMilliseconds = 0.0;
}

int Grid::Shaker::getOffset() const { return isTimerRunning() ? shakeOffset(getElapsedSeconds()) : 0; }

void Grid::Shaker::timerCallback()
{
    if (getElapsedSeconds() >= shakeDurationSeconds)
        stopTimer();
    grid.layoutEditor();
}

double Grid::Shaker::getElapsedSeconds() const { return (juce::Time::getMillisecondCounterHiRes() - startMilliseconds) / 1000.0; }

bool Grid::startEditing() { return openEditor(cursor ? getCellText(*cursor) : juce::String(), true, true); }

bool Grid::startEditingWithText(const juce::String& text)
{
    if (!openEditor({}, false, true))
        return false;
    editor->insertTextAtCaret(text);
    return true;
}

bool Grid::isEditing() const { return editor != nullptr; }

std::optional<Cell> Grid::getEditingCell() const { return editingCell; }

juce::TextEditor* Grid::getEditor() const { return editor.get(); }

bool Grid::commitEdit()
{
    if (!editor || !editingCell)
        return false;

    const auto text = editor->getText();
    if (text != editingOriginalText)
    {
        const auto outcome = commitValue(model, *editingCell, juce::var(text));
        if (outcome.kind != CommitOutcome::Kind::committed)
        {
            if (!editor->hasKeyboardFocus(true))
                editor->grabKeyboardFocus();
            showError(outcome.message);
            shaker->start();
            postAnnouncement(outcome.message);
            return false;
        }
        const CellsChanged changed{ .firstRow = editingCell->row, .numRows = 1, .firstCol = editingCell->col, .numCols = 1 };
        accessibility->cellsChanged(changed);
        closeEditor();
        notifySelf(changed);
        return true;
    }
    closeEditor();
    return true;
}

void Grid::cancelEdit() { closeEditor(); }

juce::String Grid::getErrorMessage() const { return errorPopup->getMessage(); }

bool Grid::isTextInputActive() const { return isEnabled() && (editor != nullptr || canEditCursor()); }

juce::Range<int> Grid::getHighlightedRegion() const { return editor ? editor->getHighlightedRegion() : juce::Range<int>(); }

void Grid::setHighlightedRegion(const juce::Range<int>& newRange)
{
    if (editor)
        editor->setHighlightedRegion(newRange);
}

void Grid::setTemporaryUnderlining(const juce::Array<juce::Range<int>>& underlinedRegions)
{
    if (!editor)
        return;
    editor->setTemporaryUnderlining(underlinedRegions);
    if (editorFocusPending && underlinedRegions.isEmpty())
    {
        editorFocusPending = false;
        juce::MessageManager::callAsync([safe = juce::Component::SafePointer<Grid>(this)] {
            if (safe != nullptr && safe->editor && safe->hasKeyboardFocus(false))
                safe->editor->grabKeyboardFocus();
        });
    }
}

juce::String Grid::getTextInRange(const juce::Range<int>& range) const { return editor ? editor->getTextInRange(range) : juce::String(); }

void Grid::insertTextAtCaret(const juce::String& textToInsert)
{
    if (editor)
    {
        editor->insertTextAtCaret(textToInsert);
        return;
    }
    if (textToInsert.length() == 1 && textToInsert[0] == macF2Character)
    {
        startEditing();
        return;
    }
    if (isTypedText(textToInsert) && openEditor({}, false, false))
    {
        editorFocusPending = true;
        editor->insertTextAtCaret(textToInsert);
    }
}

int Grid::getCaretPosition() const { return editor ? editor->getCaretPosition() : 0; }

juce::Rectangle<int> Grid::getCaretRectangleForCharIndex(int characterIndex) const
{
    if (editor)
        return editor->getCaretRectangleForCharIndex(characterIndex) + editor->getBoundsInParent().getPosition() + body->getPosition();
    return cursor ? getCellBounds(*cursor).withWidth(1) : juce::Rectangle<int>();
}

int Grid::getTotalNumChars() const { return editor ? editor->getTotalNumChars() : 0; }

int Grid::getCharIndexForPoint(juce::Point<int> point) const
{
    return editor ? editor->getCharIndexForPoint(point - body->getPosition() - editor->getPosition()) : 0;
}

juce::RectangleList<int> Grid::getTextBounds(juce::Range<int> textRange) const
{
    if (!editor)
        return {};
    auto bounds = editor->getTextBounds(textRange);
    bounds.offsetAll(editor->getPosition() + body->getPosition());
    return bounds;
}

void Grid::modelChanged(const Change& change)
{
    cutMarkTouched(change);
    sortTouched(change);
    if (const auto* cells = std::get_if<CellsChanged>(&change))
    {
        body->repaint();
        accessibility->cellsChanged(*cells);
        return;
    }
    if (std::holds_alternative<Reset>(change))
    {
        closeEditor();
        selection.collapseTo(cursor);
        rows.setCount(0);
        refresh();
        return;
    }
    if (const auto* inserted = std::get_if<RowsInserted>(&change))
        rows.insert(inserted->at, inserted->count);
    else if (const auto* removed = std::get_if<RowsRemoved>(&change))
        rows.remove(removed->rows);
    else if (const auto* reordered = std::get_if<RowsReordered>(&change))
        rows.reorder(reordered->order);

    if (cursor)
    {
        if (const auto row = remapRow(cursor->row, change))
            cursor->row = *row;
        else if (const auto* removed = std::get_if<RowsRemoved>(&change))
            cursor->row = rowAfterRemoval(cursor->row, *removed);
    }
    selection.applyChange(change, cursor);
    if (editingCell)
    {
        if (const auto row = remapRow(editingCell->row, change))
            editingCell->row = *row;
        else
            closeEditor();
    }
    refresh();
}

void Grid::notifierDestroyed(GridModelNotifier& notifier)
{
    if (&notifier != subscribedNotifier)
        return;
    subscribedNotifier = nullptr;
    closeEditor();
    model = {};
    refresh();
}

void Grid::subscribe()
{
    if (model.notifier == nullptr)
        return;
    subscribedNotifier = model.notifier;
    subscribedNotifier->addListener(this);
}

void Grid::unsubscribe()
{
    if (subscribedNotifier == nullptr)
        return;
    subscribedNotifier->removeListener(this);
    subscribedNotifier = nullptr;
}

bool Grid::canEditCursor() const { return cursor && model.canEdit(cursor->row, cursor->col); }

bool Grid::openEditor(const juce::String& text, bool selectAll, bool takeFocus)
{
    if (editor)
        return true;
    if (!cursor)
        return false;
    if (!canEditCursor())
    {
        postAnnouncement("Read-only cell");
        return false;
    }

    const auto cell = *cursor;
    scrollToCell(cell);
    editingCell = cell;
    editingOriginalText = getCellText(cell);
    editor = std::make_unique<CellEditor>(*this);
    editor->setFont(cellFont());
    editor->setIndents(cellPadding - 1, 0);
    editor->setJustification(juce::Justification::centredLeft);
    editor->setColour(juce::TextEditor::backgroundColourId, getGridColour(backgroundColourId));
    editor->setColour(juce::TextEditor::textColourId, getGridColour(textColourId));
    editor->setColour(juce::TextEditor::highlightColourId, getGridColour(selectionColourId));
    editor->setColour(juce::TextEditor::outlineColourId, getGridColour(cursorColourId));
    editor->setColour(juce::TextEditor::focusedOutlineColourId, getGridColour(cursorColourId));
    editor->setTitle(getColumnLabel(cell.col) + ", " + getRowLabel(cell.row));
    editor->setText(text, false);
    if (selectAll)
        editor->selectAll();
    else
        editor->moveCaretToEnd();
    editor->onTextChange = [this] { hideError(); };
    body->addAndMakeVisible(*editor);
    layoutEditor();
    if (takeFocus)
        editor->grabKeyboardFocus();
    return true;
}

void Grid::closeEditor()
{
    if (!editor)
        return;
    hideError();
    shaker->stop();
    const auto hadFocus = editor->hasKeyboardFocus(true);
    if (hadFocus)
        grabKeyboardFocus();
    editor.reset();
    editorFocusPending = false;
    editingCell.reset();
    editingOriginalText.clear();
    body->repaint();
}

void Grid::layoutEditor()
{
    if (errorPopup->isVisible() && errorCell)
        errorPopup->reposition(getCellBounds(*errorCell));
    if (!editor || !editingCell)
        return;
    editor->setBounds((getCellBounds(*editingCell) - body->getPosition()).translated(shaker->getOffset(), 0));
}

bool Grid::handleEditorKey(const juce::KeyPress& key)
{
    hideError();
    const auto mods = key.getModifiers();
    const auto code = key.getKeyCode();

    if (code == juce::KeyPress::escapeKey)
    {
        cancelEdit();
        return true;
    }
    if (code == juce::KeyPress::returnKey && !mods.isCommandDown() && !mods.isAltDown())
    {
        if (commitEdit())
            advanceCursor(true, mods);
        return true;
    }
    if (code == juce::KeyPress::tabKey && !mods.isAltDown() && (!mods.isCommandDown() || mods.isCtrlDown()))
    {
        if (commitEdit())
            advanceCursor(false, mods);
        return true;
    }
    return false;
}

void Grid::handleTypedKey(const juce::KeyPress& key)
{
    const auto text = juce::String::charToString(key.getTextCharacter());
    if (canEditCursor())
        startEditingWithText(text);
    else
        postAnnouncement("Read-only cell");
}

void Grid::editAt(Cell cell, juce::Point<int> positionInGrid)
{
    if (editor && editingCell != cell && !commitEdit())
        return;
    setCursor(cell);
    if (!startEditing() || !editor)
        return;
    const auto local = positionInGrid - body->getPosition() - editor->getPosition();
    editor->setCaretPosition(editor->getTextIndexAt(local));
}

void Grid::showError(const juce::String& message)
{
    if (editingCell)
        showErrorAt(*editingCell, message);
}

void Grid::showErrorAt(Cell cell, const juce::String& message)
{
    errorCell = cell;
    errorPopup->show(message, getCellBounds(cell));
}

void Grid::hideError()
{
    if (errorPopup->isVisible())
        errorPopup->dismiss();
}

} // namespace jgrid
