#pragma once

#include "../core/jgrid_Axis.h"
#include "../core/jgrid_Clipboard.h"
#include "../core/jgrid_GridModel.h"
#include "../core/jgrid_GridModelNotifier.h"
#include "../core/jgrid_Navigation.h"
#include "../core/jgrid_Selection.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>
#include <optional>

namespace jgrid
{

/** A spreadsheet-style grid component that displays a GridModel.

    The Grid inherits its look and feel from its parent. Its colours can be customised with
    setColour() using the ColourIds below, on the Grid, any ancestor, or the LookAndFeel.
    Unset colours fall back to the matching ListBox, TableHeaderComponent and TextEditor colours;
    the cursor falls back to an opaque TextEditor highlight colour.

    The Grid is a TextInputTarget so that typing, dead keys and input-method composition open the
    cursor cell's editor without losing the first keystroke.
*/
class Grid : public juce::Component, public juce::TextInputTarget, private juce::ScrollBar::Listener, private GridModelNotifier::Listener
{
public:
    enum ColourIds // NOLINT(cppcoreguidelines-use-enum-class)
    {
        backgroundColourId = 0x7a6c001,
        textColourId = 0x7a6c002,
        headerBackgroundColourId = 0x7a6c003,
        headerTextColourId = 0x7a6c004,
        gridLineColourId = 0x7a6c005,
        cursorColourId = 0x7a6c006,
        selectionColourId = 0x7a6c007
    };

    Grid();
    ~Grid() override;

    Grid(const Grid&) = delete;
    Grid& operator=(const Grid&) = delete;
    Grid(Grid&&) = delete;
    Grid& operator=(Grid&&) = delete;

    /** Binds the model, subscribes to its notifier and refreshes. Pass an empty GridModel to unbind. */
    void setModel(GridModel newModel);
    [[nodiscard]] const GridModel& getModel() const;

    /** Re-queries the model's row and column counts and header labels, and repaints. */
    void refresh();

    [[nodiscard]] int getRowCount() const;
    [[nodiscard]] int getColCount() const;

    void setRowHeaderVisible(bool shouldBeVisible);
    [[nodiscard]] bool isRowHeaderVisible() const;
    void setColumnHeaderVisible(bool shouldBeVisible);
    [[nodiscard]] bool isColumnHeaderVisible() const;

    /** The smallest width or height a resize can give a column or row. */
    static constexpr int minimumCellSize = 8;

    void setColumnWidth(int col, int width);
    [[nodiscard]] int getColumnWidth(int col) const;
    void setRowHeight(int row, int height);
    [[nodiscard]] int getRowHeight(int row) const;

    /** Sizes a column to its widest content, measuring the header and every row. */
    void autoFitColumn(int col);

    /** Sizes a row to its content's height. */
    void autoFitRow(int row);

    /** Hides or shows columns. Hidden columns keep their width and are skipped by navigation,
        copy, paste, delete and screen readers. The cursor moves off a hidden column. */
    void setColumnsHidden(const std::vector<int>& columns, bool hidden);
    void setRowsHidden(const std::vector<int>& rowIndices, bool hidden);
    void showAllColumns();
    void showAllRows();
    [[nodiscard]] bool isColumnHidden(int col) const;
    [[nodiscard]] bool isRowHidden(int row) const;
    [[nodiscard]] std::vector<int> getHiddenColumns() const;
    [[nodiscard]] std::vector<int> getHiddenRows() const;

    /** Returns the cursor cell, or nothing when the grid is empty. */
    [[nodiscard]] std::optional<Cell> getCursor() const;

    /** Moves the cursor, clamped to the grid, collapses the selection to it, and scrolls it into view. */
    void setCursor(Cell cell);

    [[nodiscard]] const Selection& getSelection() const;

    /** Extends the active selection range from its anchor to the given corner, keeping the cursor. */
    void extendSelection(Cell corner);

    /** Adds a single-cell range at the cell and moves the cursor there, as Cmd/Ctrl-click does. */
    void addToSelection(Cell cell);

    void selectAll();

    /** Selects whole rows first..last, as clicking (or dragging over) row headers does. The cursor
        moves to row first, in the leftmost visible column. */
    void selectRows(int first, int last, bool addToExisting = false);

    /** Selects whole columns first..last, as clicking (or dragging over) column headers does. The
        cursor moves to column first, in the topmost visible row. */
    void selectColumns(int first, int last, bool addToExisting = false);

    /** Scrolls the minimum distance needed to show the cell. */
    void scrollToCell(Cell cell);

    /** Returns the cell under a position relative to the Grid, if any. */
    [[nodiscard]] std::optional<Cell> getCellAt(juce::Point<int> position) const;

    /** Returns a cell's bounds relative to the Grid, which may lie outside the visible area. */
    [[nodiscard]] juce::Rectangle<int> getCellBounds(Cell cell) const;

    /** Returns a colour, resolved through the Grid, its ancestors, the LookAndFeel and the fallbacks. */
    [[nodiscard]] juce::Colour getGridColour(int colourId) const;

    /** Opens the cursor cell's editor with its content selected, as F2 does. Returns false if
        the cell is read-only or the grid is empty. */
    bool startEditing();

    /** Opens the cursor cell's editor with its content replaced by the given text, as typing does. */
    bool startEditingWithText(const juce::String& text);

    [[nodiscard]] bool isEditing() const;

    /** Returns the cell being edited, if any. */
    [[nodiscard]] std::optional<Cell> getEditingCell() const;

    /** Returns the open cell editor, or nullptr. */
    [[nodiscard]] juce::TextEditor* getEditor() const;

    /** Validates and stores the editor's text and closes the editor. If validation or setValue
        fails, the editor stays open, shakes, and shows the error. Returns true if the editor closed. */
    bool commitEdit();

    /** Closes the editor without storing its text. */
    void cancelEdit();

    /** Copies the active selection range to the system clipboard as tab-separated text. */
    void copy();

    /** Copies the active selection range and marks it as cut. Nothing is cleared until the text
        is pasted into a Grid; a cut that is never pasted loses nothing. */
    void cut();

    /** Pastes the clipboard text at the cursor, or fills a multi-cell selection with a single
        value. Every target is validated before anything is written. Returns true if cells were written. */
    bool paste();

    /** Clears every editable selected cell by storing empty text, validating first. */
    bool deleteSelection();

    /** Forgets the current cut, wherever it was made. */
    static void clearCutMark();

    /** Returns the range this Grid has marked as cut, if any. */
    [[nodiscard]] std::optional<CellRange> getCutRange() const;

    /** The column the Grid last sorted by, shown in its header until the order is disturbed. */
    struct SortIndicator
    {
        int col = 0;
        bool descending = false;

        friend bool operator==(const SortIndicator&, const SortIndicator&) = default;
    };

    /** True if the model supports sorting (it has reorderRows). */
    [[nodiscard]] bool canSort() const;

    /** Stably sorts the rows by a column, using the model's compare or compareValues, and applies
        the result with reorderRows. Returns false if the model cannot sort. */
    bool sortByColumn(int col, bool descending);

    /** Sorts ascending, or descending if the column is already sorted ascending, as the header's
        sort button does. */
    bool toggleSort(int col);

    [[nodiscard]] std::optional<SortIndicator> getSortIndicator() const;

    /** Returns the visible rows that are wholly selected, ascending. */
    [[nodiscard]] std::vector<int> getSelectedRows() const;

    /** Returns the visible columns that are wholly selected, ascending. */
    [[nodiscard]] std::vector<int> getSelectedColumns() const;

    /** Copies the wholly selected rows, every column, to the clipboard as tab-separated text. */
    bool copyRows();

    /** Copies the wholly selected rows and marks them as cut: they are removed (with removeRows)
        when inserted elsewhere with insertCopiedRows. Requires removeRows. */
    bool cutRows();

    /** Inserts the clipboard's rows below the given row with insertRows, padding or truncating each
        to the column count, then removes the source rows if they were cut. Selects the inserted rows. */
    bool insertCopiedRows(int belowRow);

    /** Removes the wholly selected rows, or the cursor's row, with removeRows (Cmd/Ctrl+-). */
    bool deleteSelectedRows();

    /** Inserts blank rows with insertRows (Cmd/Ctrl+=): as many as are wholly selected, below the
        last of them, or one below the cursor's row. */
    bool insertBlankRows();

    /** Duplicates the wholly selected rows with duplicateRows, or with getValue and insertRows. */
    bool duplicateSelectedRows();

    /** Forwards Grid-level undo or redo to the model. Returns false if the model has none. */
    bool undo() const;
    bool redo() const;

    /** Builds the context menu for cells, rows or columns, including the model's contextMenu hook. */
    [[nodiscard]] juce::PopupMenu createContextMenu(ContextMenuTarget::Kind kind);

    /** Returns the options context menus are shown with: attached to this Grid and placed at the
        given screen area (the click, or the cursor cell for the keyboard). */
    [[nodiscard]] juce::PopupMenu::Options getContextMenuOptions(juce::Rectangle<int> screenArea);

    /** Shows the context menu for cells, rows or columns, positioned at a screen area. */
    void showContextMenu(ContextMenuTarget::Kind kind, juce::Rectangle<int> screenArea);

    /** Returns the error message currently shown, or an empty string. */
    [[nodiscard]] juce::String getErrorMessage() const;

    /** How long an error message stays visible if nothing else dismisses it. */
    static constexpr int errorTimeoutMilliseconds = 4000;

    void paint(juce::Graphics& g) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress& key) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;
    void focusGained(FocusChangeType cause) override;
    void focusLost(FocusChangeType cause) override;
    void colourChanged() override;
    void lookAndFeelChanged() override;
    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;

    [[nodiscard]] bool isTextInputActive() const override;
    [[nodiscard]] juce::Range<int> getHighlightedRegion() const override;
    void setHighlightedRegion(const juce::Range<int>& newRange) override;
    void setTemporaryUnderlining(const juce::Array<juce::Range<int>>& underlinedRegions) override;
    [[nodiscard]] juce::String getTextInRange(const juce::Range<int>& range) const override;
    void insertTextAtCaret(const juce::String& textToInsert) override;
    [[nodiscard]] int getCaretPosition() const override;
    [[nodiscard]] juce::Rectangle<int> getCaretRectangleForCharIndex(int characterIndex) const override;
    [[nodiscard]] int getTotalNumChars() const override;
    [[nodiscard]] int getCharIndexForPoint(juce::Point<int> point) const override;
    [[nodiscard]] juce::RectangleList<int> getTextBounds(juce::Range<int> textRange) const override;

private:
    class Body;
    class ColumnHeader;
    class RowHeader;
    class Accessibility;
    class CellEditor;
    class ErrorPopup;
    class Shaker;

    void scrollBarMoved(juce::ScrollBar* scrollBar, double newRangeStart) override;
    void updateLayout();
    void setScroll(juce::Point<int> newScroll);
    void scrollChanged();
    void cursorChanged();
    void placeCursor(Cell cell, bool scrollIntoView);
    void selectionChanged(bool announce);
    void advanceCursor(bool enter, juce::ModifierKeys mods);
    [[nodiscard]] Cell getNearestCell(juce::Point<int> positionInGrid) const;
    [[nodiscard]] int getNearestRow(int yInGrid) const;
    [[nodiscard]] int getNearestCol(int xInGrid) const;
    void modelChanged(const Change& change) override;
    void notifierDestroyed(GridModelNotifier& notifier) override;
    void subscribe();
    void unsubscribe();
    [[nodiscard]] bool canEditCursor() const;
    bool openEditor(const juce::String& text, bool selectAll, bool takeFocus);
    void closeEditor();
    void layoutEditor();
    bool handleEditorKey(const juce::KeyPress& key);
    void handleTypedKey(const juce::KeyPress& key);
    void editAt(Cell cell, juce::Point<int> positionInGrid);
    void showError(const juce::String& message);
    void showErrorAt(Cell cell, const juce::String& message);
    struct WriteFailure;
    [[nodiscard]] std::optional<WriteFailure> validateWrites(const std::vector<CellWrite>& writes) const;
    bool applyWrites(const std::vector<CellWrite>& writes);
    void reportWriteFailure(const WriteFailure& failure);
    [[nodiscard]] bool isCellWritable(Cell cell) const;
    void paintCutMark(juce::Graphics& g) const;
    void cutMarkTouched(const Change& change) const;
    [[nodiscard]] TextTable rowsTable(const std::vector<int>& rowIndices) const;
    [[nodiscard]] RowData clipboardRows() const;
    void showContextMenuForKeyboard();
    void reportFailure(const juce::String& message);
    void hiddenStateChanged();
    [[nodiscard]] std::optional<int> columnEdgeAt(int xInGrid) const;
    [[nodiscard]] std::optional<int> rowEdgeAt(int yInGrid) const;
    [[nodiscard]] int measureColumn(int col) const;
    void sortTouched(const Change& change);
    void notifySelf(const Change& change);
    [[nodiscard]] bool isOnSortButton(int col, int xInGrid) const;
    void paintSortButton(juce::Graphics& g, juce::Rectangle<int> columnArea, int col) const;
    [[nodiscard]] juce::String getColumnAccessibleTitle(int col) const;
    void hideError();
    [[nodiscard]] int getPageRows() const;
    [[nodiscard]] juce::String getColumnLabel(int col) const;
    [[nodiscard]] juce::String getRowLabel(int row) const;
    [[nodiscard]] juce::String getCellText(Cell cell) const;
    [[nodiscard]] IndexRange getVisibleRows() const;
    [[nodiscard]] IndexRange getVisibleCols() const;
    void paintBody(juce::Graphics& g) const;
    void paintColumnHeader(juce::Graphics& g, int width, int height) const;
    void paintRowHeader(juce::Graphics& g, int width, int height) const;

    GridModel model;
    Axis rows{ 22 };
    Axis cols{ 100 };
    int columnHeaderHeight = 24;
    int rowHeaderWidth = 40;
    bool rowHeaderVisible = true;
    bool columnHeaderVisible = true;
    std::optional<Cell> cursor;
    Selection selection;
    std::optional<int> headerDragStart;
    std::optional<Cell> errorCell;
    std::optional<SortIndicator> sortIndicator;
    bool sortInProgress = false;
    std::optional<int> hoverSortCol;

    struct Resize
    {
        bool column = true;
        int index = 0;
        int startSize = 0;
        int startPosition = 0;
    };
    std::optional<Resize> resizing;
    juce::Point<int> scroll;

    std::unique_ptr<Body> body;
    std::unique_ptr<ColumnHeader> columnHeader;
    std::unique_ptr<RowHeader> rowHeader;
    juce::ScrollBar verticalScrollBar{ true };
    juce::ScrollBar horizontalScrollBar{ false };
    std::unique_ptr<Accessibility> accessibility;
    std::unique_ptr<CellEditor> editor;
    std::optional<Cell> editingCell;
    juce::String editingOriginalText;
    bool editorFocusPending = false;
    std::unique_ptr<ErrorPopup> errorPopup;
    std::unique_ptr<Shaker> shaker;
    GridModelNotifier* subscribedNotifier = nullptr;

    JUCE_LEAK_DETECTOR(Grid)
};

} // namespace jgrid
