#pragma once

#include "jgrid_Navigation.h"
#include "jgrid_Selection.h"

#include <juce_core/juce_core.h>

#include <compare>
#include <functional>
#include <vector>

namespace juce
{
class PopupMenu;
}

namespace jgrid
{

class GridModelNotifier;

/** Rows of cell values, one inner vector per row with one value per column, as passed to
    GridModel::insertRows. */
using RowData = std::vector<std::vector<juce::var>>;

//==============================================================================
/**
    Describes what a context menu is being shown for, so GridModel::contextMenu can tailor it.

    @see GridModel::contextMenu, Grid::createContextMenu

    @tags{GUI}
*/
struct ContextMenuTarget
{
    /** The kind of menu. */
    enum class Kind {
        cells, /**< Right-click on cells, or Shift+F10 with an ordinary selection. */
        rows, /**< Right-click on a row header, or Shift+F10 with whole rows selected. */
        columns /**< Right-click on a column header, or Shift+F10 with whole columns selected. */
    };

    /** The kind of menu being shown. */
    Kind kind = Kind::cells;

    /** The cursor cell when the menu opened: the clicked cell, or a cell in the clicked row or column. */
    Cell cell;

    /** A copy of the selection when the menu opened. */
    Selection selection;
};

//==============================================================================
/**
    Connects a Grid to your data, as a set of callbacks.

    A GridModel is a plain struct of std::function members, so your data needs no base class and
    the Grid is not a template. Fill in what your data supports and bind it with Grid::setModel().

    Only rowCount, colCount and getValue are required; a model missing any of them shows as
    empty. Every other member is optional, and leaving one out switches off the matching feature:
    without setValue the Grid is read-only, without reorderRows it cannot sort, without insertRows
    the row-insertion items are disabled, and so on.

    Rows are addressed by their position in the order the Grid shows them (the model's view
    order). If your model supports sorting, it keeps that order itself, usually as a RowOrder over
    data that never moves; ViewModel does this for you.

    When your data changes, tell the Grid through the GridModelNotifier that @ref notifier points
    to. The Grid then repaints, and carries its cursor, selection, row heights and hidden rows
    along with any rows that moved.

    All callbacks are called on the message thread. Anything they capture must outlive the Grid
    they are bound to, or be unbound first with Grid::setModel({}).

    @code
    jgrid::GridModel model;
    model.rowCount = [&] { return static_cast<int>(rows.size()); };
    model.colCount = [] { return 2; };
    model.getValue = [&](int row, int col) { return rows[(size_t) row][(size_t) col]; };
    model.setValue = [&](int row, int col, const juce::var& value) {
        rows[(size_t) row][(size_t) col] = value;
        notifier.changed(jgrid::CellsChanged{ row, 1, col, 1 });
        return true;
    };
    model.notifier = &notifier;
    @endcode

    @see Grid, GridModelNotifier, ViewModel, DataSource

    @tags{GUI}
*/
struct GridModel
{
    //==============================================================================
    /** Required. Returns the number of rows. */
    std::function<int()> rowCount;

    /** Required. Returns the number of columns. The column count only changes with a Reset. */
    std::function<int()> colCount;

    /** Required. Returns a cell's value. The Grid paints `value.toString()`, right-aligned for
        numbers; it is also what copy, autofit, sorting (without compare) and screen readers use. */
    std::function<juce::var(int row, int col)> getValue;

    //==============================================================================
    /** Stores an edited value. Without it the Grid is read-only.

        Text editing and paste always pass a string var; converting it (for example "3" to an
        int) is the model's job. The model should notify CellsChanged afterwards.

        @returns false if the value could not be stored; the Grid then shows "Could not set value".
    */
    std::function<bool(int row, int col, const juce::var& value)> setValue;

    /** Checks a value before setValue is called. Without it, every value is accepted.

        Return juce::Result::fail() with a message to reject the value: an edit keeps its editor
        open and shows the message, and a paste or delete writes nothing.
    */
    std::function<juce::Result(int row, int col, const juce::var& value)> validate;

    /** Returns whether a cell may be edited. Without it, every cell is editable when setValue
        exists. Read-only cells are skipped by paste and delete, and announced to screen readers. */
    std::function<bool(int row, int col)> isEditable;

    //==============================================================================
    /** Called before every operation that may make several changes: paste, delete, cut-then-paste,
        sort and row operations. Use it to group undo steps or hold back notifications. Calls are
        never nested. */
    std::function<void()> beginBatch;

    /** Called after every operation that called beginBatch, even if it failed part way. */
    std::function<void()> endBatch;

    //==============================================================================
    /** Compares two rows' values in one column, for sorting. Without it, the Grid compares the
        getValue results with compareValues(). Return the ascending order; the Grid reverses it for
        descending sorts and keeps the sort stable. */
    std::function<std::weak_ordering(int col, int rowA, int rowB)> compare;

    /** Reorders the rows: `order[newRow]` is each row's old position. Required for sorting.

        The Grid calls this with the sorted order; the model applies it (usually by reordering a
        RowOrder rather than the data itself) and then notifies RowsReordered with the same order.
    */
    std::function<void(const std::vector<int>& order)> reorderRows;

    //==============================================================================
    /** Inserts rows before row `at`. Required for inserting copied, cut, blank and (unless
        duplicateRows exists) duplicated rows.

        Each row has one value per column: strings when they come from the clipboard, the original
        values when duplicating. The model should notify RowsInserted.

        @returns false if nothing was inserted.
    */
    std::function<bool(int at, const RowData& rows)> insertRows;

    /** Removes rows, given in ascending order. Required for Delete Rows and for cutting rows. The
        model should notify RowsRemoved.

        @returns false if nothing was removed.
    */
    std::function<bool(const std::vector<int>& rows)> removeRows;

    /** Duplicates rows, given in ascending order, wherever the model chooses. Without it, the Grid
        duplicates with getValue and insertRows, placing the copies after the last row. */
    std::function<bool(const std::vector<int>& rows)> duplicateRows;

    //==============================================================================
    /** Undoes the last change, for Cmd/Ctrl+Z. What that means is entirely up to the model; the
        Grid keeps no undo history of its own (its cell editors have their own undo while open). */
    std::function<void()> undo;

    /** Redoes the last undone change, for Shift+Cmd/Ctrl+Z and Cmd/Ctrl+Y. */
    std::function<void()> redo;

    //==============================================================================
    /** Called just before a context menu is shown, to add, remove or change its items. Items can
        carry their own actions (juce::PopupMenu::Item::action). */
    std::function<void(juce::PopupMenu& menu, const ContextMenuTarget& target)> contextMenu;

    /** Returns a column's header label. Without it, headers show spreadsheet letters: A, B, ... AA. */
    std::function<juce::String(int col)> colName;

    /** Returns a row's header label. Without it, headers show the row's position + 1, so rows are
        renumbered after a sort, as in a spreadsheet. */
    std::function<juce::String(int row)> rowName;

    //==============================================================================
    /** The notifier the model announces its changes through. Several Grids can listen to one
        notifier. Without it, the Grid only learns of changes when Grid::refresh() is called. */
    GridModelNotifier* notifier = nullptr;

    //==============================================================================
    /** Returns true if the required callbacks are all set. */
    [[nodiscard]] bool isComplete() const { return rowCount && colCount && getValue; }

    /** Returns true if a cell can be edited: setValue exists and isEditable allows it. */
    [[nodiscard]] bool canEdit(int row, int col) const { return setValue && (!isEditable || isEditable(row, col)); }
};

} // namespace jgrid
