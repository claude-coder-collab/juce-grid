#pragma once

#include "jgrid_GridModel.h"
#include "jgrid_GridModelNotifier.h"
#include "jgrid_RowOrder.h"

#include <compare>
#include <functional>
#include <optional>
#include <vector>

namespace jgrid
{

//==============================================================================
/**
    A dataset that several Grids can view at once, each through its own ViewModel.

    A DataSource is like a GridModel, but addresses rows by their position in your storage
    (storage rows) and columns by their position in your data (source columns). It has no row
    order, and no row or column labels beyond colName: those belong to each view. Its notifier
    reports changes in storage indices, and is required.

    Write one DataSource per dataset, then create a ViewModel for each Grid that shows it.

    @see ViewModel, GridModel

    @tags{GUI}
*/
struct DataSource
{
    /** Required. Returns the number of stored rows. */
    std::function<int()> rowCount;

    /** Required. Returns the number of source columns. */
    std::function<int()> colCount;

    /** Required. Returns a stored cell's value. */
    std::function<juce::var(int row, int col)> getValue;

    /** Stores a value, as GridModel::setValue. Without it, every view is read-only. */
    std::function<bool(int row, int col, const juce::var& value)> setValue;

    /** Checks a value before setValue, as GridModel::validate. */
    std::function<juce::Result(int row, int col, const juce::var& value)> validate;

    /** Returns whether a cell may be edited, as GridModel::isEditable. */
    std::function<bool(int row, int col)> isEditable;

    /** Returns a source column's header label, as GridModel::colName. */
    std::function<juce::String(int col)> colName;

    /** Compares two stored rows in a source column, as GridModel::compare. */
    std::function<std::weak_ordering(int col, int rowA, int rowB)> compare;

    /** Inserts rows before storage row at. A ViewModel always appends at the end of storage and
        places the rows in its own view itself. The source must notify RowsInserted. */
    std::function<bool(int at, const RowData& rows)> insertRows;

    /** Removes storage rows, given in ascending order. The source must notify RowsRemoved. */
    std::function<bool(const std::vector<int>& rows)> removeRows;

    /** Undo, as GridModel::undo. */
    std::function<void()> undo;

    /** Redo, as GridModel::redo. */
    std::function<void()> redo;

    /** Called around multi-change operations, as GridModel::beginBatch. */
    std::function<void()> beginBatch;

    /** Called after multi-change operations, as GridModel::endBatch. */
    std::function<void()> endBatch;

    /** Required. Where the source reports changes, in storage indices. */
    GridModelNotifier* notifier = nullptr;
};

//==============================================================================
/**
    One Grid's view of a DataSource: its own row order and, optionally, its own columns.

    A ViewModel turns a DataSource into a GridModel for one Grid. It keeps a RowOrder, so sorting
    one Grid leaves the others alone, and can show a subset or rearrangement of the source's
    columns. It translates every call from view indices to storage indices, and every change the
    source reports from storage indices to view indices.

    Rows inserted through this view appear where they were inserted in it; rows inserted any other
    way appear at the end of the view.

    @code
    jgrid::ViewModel overview { source };              // every column
    jgrid::ViewModel names { source, { 1, 2 } };       // only source columns 1 and 2

    overviewGrid.setModel(overview.bind());
    namesGrid.setModel(names.bind());
    @endcode

    The DataSource and the ViewModel must both outlive the Grid bound to bind()'s model.

    @see DataSource, RowOrder, GridModel

    @tags{GUI}
*/
class ViewModel : private GridModelNotifier::Listener
{
public:
    /** Creates a view of a source.

        @param source   the dataset to view; it must have a notifier and outlive this object
        @param columns  the source columns to show, in order; empty shows every column
    */
    explicit ViewModel(DataSource& source, std::vector<int> columns = {});

    /** Destructor. */
    ~ViewModel() override;

    ViewModel(const ViewModel&) = delete;
    ViewModel& operator=(const ViewModel&) = delete;
    ViewModel(ViewModel&&) = delete;
    ViewModel& operator=(ViewModel&&) = delete;

    /** Returns the GridModel to bind to a Grid with Grid::setModel(). */
    [[nodiscard]] GridModel bind();

    /** Returns this view's row order. Save getRowOrder().getOrder() to restore a sort later. */
    [[nodiscard]] const RowOrder& getRowOrder() const;

    /** Restores a saved row order (one storage row per view row) and resets the Grid. */
    void setRowOrder(std::vector<int> viewToStorage);

    /** Changes the shown columns (source column indices, in order; empty for all) and resets the Grid. */
    void setColumns(std::vector<int> columns);

    /** Returns the source column shown at a view column. */
    [[nodiscard]] int toSourceColumn(int viewCol) const;

private:
    void modelChanged(const Change& change) override;
    void notifierDestroyed(GridModelNotifier& destroyed) override;
    [[nodiscard]] int getColCount() const;
    [[nodiscard]] std::vector<int> viewColumnsFor(int firstSourceCol, int numSourceCols) const;

    DataSource& source;
    GridModelNotifier* subscribed = nullptr;
    std::vector<int> columns;
    RowOrder order;
    std::optional<int> pendingInsertAt;
    GridModelNotifier notifier;
};

} // namespace jgrid
