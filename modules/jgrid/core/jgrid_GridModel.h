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

/** Rows of cell values, as passed to insertRows. */
using RowData = std::vector<std::vector<juce::var>>;

/** What a context menu is being shown for, passed to GridModel::contextMenu. */
struct ContextMenuTarget
{
    enum class Kind {
        cells,
        rows,
        columns
    };

    Kind kind = Kind::cells;

    /** The cell, row or column clicked (or the cursor, for the keyboard), in model indices. */
    Cell cell;

    Selection selection;
};

/** Binds a Grid to the user's data as a set of callbacks.

    rowCount, colCount and getValue are required; a model missing any of them is treated as
    empty. The other members are optional. Row indices are view-order indices. All callbacks
    are invoked on the message thread, and anything they capture must outlive the binding.
*/
struct GridModel
{
    std::function<int()> rowCount;
    std::function<int()> colCount;
    std::function<juce::var(int row, int col)> getValue;

    /** Stores an edited value, returning false if it could not be stored. Text editing always
        passes a string var; converting it is the model's job. If absent, the Grid is read-only. */
    std::function<bool(int row, int col, const juce::var& value)> setValue;

    /** Checks a value before setValue. A failed Result keeps the editor open and shows its
        error message. If absent, every value is accepted. */
    std::function<juce::Result(int row, int col, const juce::var& value)> validate;

    /** Whether a cell may be edited. If absent, every cell is editable when setValue exists. */
    std::function<bool(int row, int col)> isEditable;

    /** Called around every multi-cell or multi-row operation (paste, delete, cut-then-paste,
        sort), so the model can group undo steps or coalesce notifications. Never nested; endBatch
        is always called, even if the operation fails part way. */
    std::function<void()> beginBatch;
    std::function<void()> endBatch;

    /** Compares two rows' values in a column, for sorting. If absent, the Grid compares getValue
        results with compareValues. Descending order is handled by the Grid. */
    std::function<std::weak_ordering(int col, int rowA, int rowB)> compare;

    /** Reorders the model's view order: order[newRow] = oldRow. The model should then notify
        RowsReordered with the same order. Required for sorting; if absent, the Grid cannot sort. */
    std::function<void(const std::vector<int>& order)> reorderRows;

    /** Inserts rows before row at. Each row has colCount values: strings when they come from
        the clipboard, the source values when duplicating. The model should notify RowsInserted.
        Returns false if nothing was inserted. If absent, row insertion is unavailable. */
    std::function<bool(int at, const RowData& rows)> insertRows;

    /** Removes the given rows (ascending). Used when cut rows are inserted elsewhere. The model
        should notify RowsRemoved. If absent, rows cannot be cut. */
    std::function<bool(const std::vector<int>& rows)> removeRows;

    /** Duplicates the given rows (ascending) wherever the model chooses. If absent, the Grid
        duplicates through getValue and insertRows, placing the copies after the last row. */
    std::function<bool(const std::vector<int>& rows)> duplicateRows;

    /** Grid-level undo and redo (Cmd/Ctrl+Z, Shift+Cmd/Ctrl+Z or Cmd/Ctrl+Y), forwarded as they
        are; what they do is the model's choice. If absent, the keys do nothing. */
    std::function<void()> undo;
    std::function<void()> redo;

    /** Called before a context menu is shown, to add, remove or change items. Items can carry
        their own actions (PopupMenu::Item::action). */
    std::function<void(juce::PopupMenu& menu, const ContextMenuTarget& target)> contextMenu;

    /** Column header label. If absent, headers show spreadsheet letters: A, B, ... */
    std::function<juce::String(int col)> colName;

    /** Row header label. If absent, headers show the view position + 1. */
    std::function<juce::String(int row)> rowName;

    /** Where the model announces changes. Optional, but without it the Grid only learns of
        changes through Grid::refresh(). */
    GridModelNotifier* notifier = nullptr;

    [[nodiscard]] bool isComplete() const { return rowCount && colCount && getValue; }

    [[nodiscard]] bool canEdit(int row, int col) const { return setValue && (!isEditable || isEditable(row, col)); }
};

} // namespace jgrid
