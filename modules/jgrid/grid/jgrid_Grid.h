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

/**
    A spreadsheet-style grid component that displays and edits the data of a GridModel.

    The Grid holds no data of its own. You describe your data with a GridModel, a struct of
    callbacks (row and column counts, getValue, and optionally setValue, validate, sorting and
    row operations), and bind it with setModel(). The Grid calls the model when it needs values
    and when the user edits, and the model tells the Grid about changes through a
    GridModelNotifier.

    The Grid owns everything about how the data is viewed: the cursor and selection, scrolling,
    column widths and row heights, hidden rows and columns, the open cell editor and the sort
    indicator. The model owns the data, its row order (so sorts can be saved and undone), undo,
    and the rules for what is valid.

    @code
    jgrid::GridModel model;
    model.rowCount = [&] { return (int) rows.size(); };
    model.colCount = [] { return 3; };
    model.getValue = [&](int row, int col) { return rows[(size_t) row][(size_t) col]; };

    grid.setModel(model);
    @endcode

    Keyboard, mouse, clipboard, context menus and screen reader support are built in; see the
    guide for the full keymap.

    Colours are customised with setColour() using the ColourIds below, on the Grid, any
    ancestor component or the LookAndFeel. Unset colours fall back to the ListBox,
    TableHeaderComponent and TextEditor colours of the current LookAndFeel.

    The Grid is a TextInputTarget so that typing, dead keys and input-method composition can open
    the cursor cell's editor without losing the first keystroke; you do not need to call those
    methods yourself.

    All methods must be called on the message thread.

    @see GridModel, GridModelNotifier, ViewModel, Selection

    @tags{GUI}
*/
class Grid : public juce::Component, public juce::TextInputTarget, private juce::ScrollBar::Listener, private GridModelNotifier::Listener
{
public:
    //==============================================================================
    /** Colour IDs for customising the Grid with setColour().

        Each colour is looked up on the Grid, then its ancestors, then the LookAndFeel; if none
        sets it, a matching standard JUCE colour is used.
    */
    enum ColourIds // NOLINT(cppcoreguidelines-use-enum-class)
    {
        backgroundColourId = 0x7a6c001, /**< Cell background. Falls back to ListBox::backgroundColourId. */
        textColourId = 0x7a6c002, /**< Cell text. Falls back to ListBox::textColourId. */
        headerBackgroundColourId = 0x7a6c003, /**< Row and column header background. Falls back to TableHeaderComponent::backgroundColourId. */
        headerTextColourId = 0x7a6c004, /**< Header labels and sort arrows. Falls back to TableHeaderComponent::textColourId. */
        gridLineColourId = 0x7a6c005, /**< Lines between cells and headers. Falls back to TableHeaderComponent::outlineColourId. */
        cursorColourId = 0x7a6c006, /**< Cursor outline and cut marks. Falls back to an opaque TextEditor::highlightColourId. */
        selectionColourId = 0x7a6c007 /**< Selected cells and the tint on selected headers. Falls back to TextEditor::highlightColourId. */
    };

    //==============================================================================
    /** Creates an empty Grid. Call setModel() to show data. */
    Grid();

    /** Destructor. Unsubscribes from the model's notifier and forgets any cut made in this Grid. */
    ~Grid() override;

    Grid(const Grid&) = delete;
    Grid& operator=(const Grid&) = delete;
    Grid(Grid&&) = delete;
    Grid& operator=(Grid&&) = delete;

    //==============================================================================
    /** Binds a model, replacing any previous one.

        Any open editor is cancelled, the cursor returns to the first cell, the sort indicator is
        cleared, and the Grid subscribes to the model's notifier, if it has one. Pass an empty
        GridModel to unbind. Everything the model's callbacks capture must outlive the binding.

        @see GridModel, refresh
    */
    void setModel(GridModel newModel);

    /** Returns the bound model. */
    [[nodiscard]] const GridModel& getModel() const;

    /** Re-queries the model's row and column counts and header labels, and repaints.

        Models with a GridModelNotifier never need to call this. A model without one must call it
        after changing its data, because the Grid has no other way to find out.
    */
    void refresh();

    /** Returns the number of rows, as last reported by the model. */
    [[nodiscard]] int getRowCount() const;

    /** Returns the number of columns, as last reported by the model. */
    [[nodiscard]] int getColCount() const;

    //==============================================================================
    /** Shows or hides the row header, the column of row labels on the left. */
    void setRowHeaderVisible(bool shouldBeVisible);

    /** Returns true if the row header is shown. */
    [[nodiscard]] bool isRowHeaderVisible() const;

    /** Shows or hides the column header, the row of column labels and sort buttons at the top. */
    void setColumnHeaderVisible(bool shouldBeVisible);

    /** Returns true if the column header is shown. */
    [[nodiscard]] bool isColumnHeaderVisible() const;

    //==============================================================================
    /** The smallest width or height a resize can give a column or row, in pixels. */
    static constexpr int minimumCellSize = 8;

    /** Sets a column's width in pixels, as dragging its header edge does.

        @param col    the column index
        @param width  the new width, clamped to at least minimumCellSize
    */
    void setColumnWidth(int col, int width);

    /** Returns a column's width in pixels, even while it is hidden. */
    [[nodiscard]] int getColumnWidth(int col) const;

    /** Sets a row's height in pixels, as dragging its header edge does.

        Row heights follow their rows when the model inserts, removes or reorders rows.

        @param row     the row index
        @param height  the new height, clamped to at least minimumCellSize
    */
    void setRowHeight(int row, int height);

    /** Returns a row's height in pixels, even while it is hidden. */
    [[nodiscard]] int getRowHeight(int row) const;

    /** Sizes a column to fit its widest content, measuring the header label and every row's
        text, as double-clicking the column's header edge does. */
    void autoFitColumn(int col);

    /** Sizes a row to fit one line of text, as double-clicking the row's header edge does. */
    void autoFitRow(int row);

    /** Hides or shows columns.

        Hidden columns keep their width and are skipped by keyboard navigation, copy, paste,
        delete and screen readers. If the cursor is on a column being hidden, it moves to the
        nearest visible column. Hidden state belongs to the Grid, not the model.

        @param columns  the column indices to change
        @param hidden   true to hide them, false to show them
        @see showAllColumns, getHiddenColumns
    */
    void setColumnsHidden(const std::vector<int>& columns, bool hidden);

    /** Hides or shows rows. Hidden rows follow their rows when the model inserts, removes or
        reorders rows, and are forgotten when the model resets.

        @see setColumnsHidden
    */
    void setRowsHidden(const std::vector<int>& rowIndices, bool hidden);

    /** Shows every hidden column. */
    void showAllColumns();

    /** Shows every hidden row. */
    void showAllRows();

    /** Returns true if the column is hidden. */
    [[nodiscard]] bool isColumnHidden(int col) const;

    /** Returns true if the row is hidden. */
    [[nodiscard]] bool isRowHidden(int row) const;

    /** Returns the hidden columns, ascending. */
    [[nodiscard]] std::vector<int> getHiddenColumns() const;

    /** Returns the hidden rows, ascending. */
    [[nodiscard]] std::vector<int> getHiddenRows() const;

    //==============================================================================
    /** Returns the cursor cell, or nothing when the Grid has no visible cells.

        The cursor is the single cell the keyboard acts on: it is where editing, paste and the
        context menu apply. It is always inside or next to the selection.
    */
    [[nodiscard]] std::optional<Cell> getCursor() const;

    /** Moves the cursor, collapses the selection to it and scrolls it into view, as a plain
        click or arrow key does. The cell is clamped to the visible cells. */
    void setCursor(Cell cell);

    /** Returns the selected ranges. A single cell (the cursor) is the smallest selection. */
    [[nodiscard]] const Selection& getSelection() const;

    /** Moves the far corner of the active selection range, keeping its anchor and the cursor, as
        Shift+arrow or Shift+click does. */
    void extendSelection(Cell corner);

    /** Adds a single-cell range and moves the cursor to it, as Cmd/Ctrl+click does. */
    void addToSelection(Cell cell);

    /** Selects every cell, keeping the cursor, as Cmd/Ctrl+A does. */
    void selectAll();

    /** Selects whole rows, as clicking or dragging over row headers does.

        @param first          the row the selection starts from; the cursor moves to it
        @param last           the other end of the range, in either direction
        @param addToExisting  true to keep the existing ranges, as Cmd/Ctrl+click does
    */
    void selectRows(int first, int last, bool addToExisting = false);

    /** Selects whole columns, as clicking or dragging over column headers does.

        @param first          the column the selection starts from; the cursor moves to it
        @param last           the other end of the range, in either direction
        @param addToExisting  true to keep the existing ranges, as Cmd/Ctrl+click does
    */
    void selectColumns(int first, int last, bool addToExisting = false);

    /** Returns the visible rows that are wholly selected, ascending. Row operations such as
        cutRows() and duplicateSelectedRows() act on these. */
    [[nodiscard]] std::vector<int> getSelectedRows() const;

    /** Returns the visible columns that are wholly selected, ascending. */
    [[nodiscard]] std::vector<int> getSelectedColumns() const;

    //==============================================================================
    /** Scrolls the minimum distance needed to show the cell. */
    void scrollToCell(Cell cell);

    /** Returns the cell under a position given relative to the Grid, if any. */
    [[nodiscard]] std::optional<Cell> getCellAt(juce::Point<int> position) const;

    /** Returns a cell's bounds relative to the Grid. The bounds may lie outside the visible area. */
    [[nodiscard]] juce::Rectangle<int> getCellBounds(Cell cell) const;

    /** Returns one of the ColourIds as the Grid will paint it, resolved through the Grid, its
        ancestors, the LookAndFeel and the fallbacks. */
    [[nodiscard]] juce::Colour getGridColour(int colourId) const;

    //==============================================================================
    /** Opens the cursor cell's editor with its text selected, as F2 does.

        @returns false if the Grid is empty or the cell is read-only (the model has no setValue,
                 or its isEditable returns false), in which case screen readers are told so.
    */
    bool startEditing();

    /** Opens the cursor cell's editor with its content replaced by the given text, as typing does.

        @returns false if the Grid is empty or the cell is read-only.
    */
    bool startEditingWithText(const juce::String& text);

    /** Returns true while a cell editor is open. */
    [[nodiscard]] bool isEditing() const;

    /** Returns the cell being edited, if any. */
    [[nodiscard]] std::optional<Cell> getEditingCell() const;

    /** Returns the open cell editor, or nullptr. The editor is owned by the Grid. */
    [[nodiscard]] juce::TextEditor* getEditor() const;

    /** Stores the editor's text in the model and closes the editor, as Enter or Tab does.

        The text is passed to the model's validate and then setValue as a string var; converting
        it to another type is the model's job. Unchanged text closes the editor without calling the
        model. If validation or setValue fails, the editor stays open, shakes, and shows the error
        in a popup that never takes focus; screen readers hear the message too.

        @returns true if the editor closed.
    */
    bool commitEdit();

    /** Closes the editor without storing its text, as Escape does. */
    void cancelEdit();

    /** Returns the error message currently shown, or an empty string. */
    [[nodiscard]] juce::String getErrorMessage() const;

    /** How long an error message stays visible if nothing else dismisses it, in milliseconds. */
    static constexpr int errorTimeoutMilliseconds = 4000;

    //==============================================================================
    /** Copies the active selection range to the system clipboard as tab-separated text, as
        Cmd/Ctrl+C does. Hidden rows and columns are skipped. */
    void copy();

    /** Copies the active selection range and marks it as cut, as Cmd/Ctrl+X does.

        Nothing is cleared until the text is pasted into a Grid, which then clears the source
        cells, so a cut that is never pasted loses nothing. The cut becomes a plain copy if its
        cells change, the clipboard changes, or Escape is pressed.
    */
    void cut();

    /** Pastes the clipboard's tab-separated text at the cursor, as Cmd/Ctrl+V does.

        A single value pasted into a multi-cell selection fills every selected cell. Read-only and
        hidden cells are skipped, and the paste is clipped at the last row and column. Every target
        is validated before anything is written; if any fails, nothing is written. All the writes
        happen inside one beginBatch/endBatch pair.

        @returns true if cells were written.
    */
    bool paste();

    /** Clears every editable selected cell by storing empty text, as Delete does. Validation
        runs first, all-or-nothing.

        @returns true if cells were cleared.
    */
    bool deleteSelection();

    /** Forgets the current cut, whichever Grid it was made in. */
    static void clearCutMark();

    /** Returns the range this Grid has marked as cut, if any. */
    [[nodiscard]] std::optional<CellRange> getCutRange() const;

    //==============================================================================
    /** The column the Grid last sorted by, shown in its header until the order is disturbed. */
    struct SortIndicator
    {
        int col = 0; /**< The sorted column. */
        bool descending = false; /**< True for a descending sort. */

        /** Compares two indicators. */
        friend bool operator==(const SortIndicator&, const SortIndicator&) = default;
    };

    /** Returns true if the model supports sorting, which needs GridModel::reorderRows. */
    [[nodiscard]] bool canSort() const;

    /** Sorts the rows by a column.

        The Grid works out a stable sorted order with the model's compare callback, or with
        compareValues() if there is none, and hands it to the model's reorderRows inside one
        batch. The model reorders its rows (usually just its RowOrder, not the data) and notifies
        RowsReordered. The header then shows the sort direction until the order is disturbed.

        @param col         the column to sort by
        @param descending  true for descending order
        @returns false if the model cannot sort.
        @see toggleSort, GridModel::compare, GridModel::reorderRows, RowOrder
    */
    bool sortByColumn(int col, bool descending);

    /** Sorts ascending, or descending if the column is already sorted ascending, as the header's
        sort button does. */
    bool toggleSort(int col);

    /** Returns the column last sorted by, if the order has not changed since. */
    [[nodiscard]] std::optional<SortIndicator> getSortIndicator() const;

    //==============================================================================
    /** Copies the wholly selected rows, every column including hidden ones, to the clipboard as
        tab-separated text. */
    bool copyRows();

    /** Copies the wholly selected rows and marks them as cut. They are removed (with the model's
        removeRows) only when insertCopiedRows() inserts them somewhere, in this Grid or another.
        Requires GridModel::removeRows. */
    bool cutRows();

    /** Inserts the clipboard's rows below a row with the model's insertRows.

        Each row is padded or truncated to the column count. If the rows were cut, the source rows
        are then removed. The inserted rows end up selected.

        @param belowRow  the row to insert below; -1 inserts at the top
    */
    bool insertCopiedRows(int belowRow);

    /** Removes the wholly selected rows, or the cursor's row, with the model's removeRows, as
        Cmd/Ctrl+minus does. */
    bool deleteSelectedRows();

    /** Inserts blank rows with the model's insertRows, as Cmd/Ctrl+equals does: as many as are
        wholly selected, below the last of them, or one below the cursor's row. */
    bool insertBlankRows();

    /** Duplicates the wholly selected rows with the model's duplicateRows, or, if it has none,
        with getValue and insertRows, placing the copies after the last selected row. */
    bool duplicateSelectedRows();

    //==============================================================================
    /** Calls the model's undo, as Cmd/Ctrl+Z does. What undo means is entirely the model's choice.

        @returns false if the model has no undo.
    */
    bool undo() const;

    /** Calls the model's redo, as Shift+Cmd/Ctrl+Z or Cmd/Ctrl+Y does.

        @returns false if the model has no redo.
    */
    bool redo() const;

    //==============================================================================
    /** Builds the context menu the Grid would show for cells, rows or columns, including the items
        added by the model's contextMenu callback. Items the model cannot support are disabled. */
    [[nodiscard]] juce::PopupMenu createContextMenu(ContextMenuTarget::Kind kind);

    /** Returns the options context menus are shown with: attached to this Grid and placed at the
        given screen area. */
    [[nodiscard]] juce::PopupMenu::Options getContextMenuOptions(juce::Rectangle<int> screenArea);

    /** Shows the context menu for cells, rows or columns at a screen area, as right-click and
        Shift+F10 do. */
    void showContextMenu(ContextMenuTarget::Kind kind, juce::Rectangle<int> screenArea);

    //==============================================================================
    /** @internal */
    void paint(juce::Graphics& g) override;
    /** @internal */
    void resized() override;
    /** @internal */
    bool keyPressed(const juce::KeyPress& key) override;
    /** @internal */
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;
    /** @internal */
    void focusGained(FocusChangeType cause) override;
    /** @internal */
    void focusLost(FocusChangeType cause) override;
    /** @internal */
    void colourChanged() override;
    /** @internal */
    void lookAndFeelChanged() override;
    /** @internal */
    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;

    /** @internal */
    [[nodiscard]] bool isTextInputActive() const override;
    /** @internal */
    [[nodiscard]] juce::Range<int> getHighlightedRegion() const override;
    /** @internal */
    void setHighlightedRegion(const juce::Range<int>& newRange) override;
    /** @internal */
    void setTemporaryUnderlining(const juce::Array<juce::Range<int>>& underlinedRegions) override;
    /** @internal */
    [[nodiscard]] juce::String getTextInRange(const juce::Range<int>& range) const override;
    /** @internal */
    void insertTextAtCaret(const juce::String& textToInsert) override;
    /** @internal */
    [[nodiscard]] int getCaretPosition() const override;
    /** @internal */
    [[nodiscard]] juce::Rectangle<int> getCaretRectangleForCharIndex(int characterIndex) const override;
    /** @internal */
    [[nodiscard]] int getTotalNumChars() const override;
    /** @internal */
    [[nodiscard]] int getCharIndexForPoint(juce::Point<int> point) const override;
    /** @internal */
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
