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

/** A dataset shared by several Grids, addressed by storage row and storage column. The user writes
    one DataSource per dataset; each Grid views it through its own ViewModel. Its notifier reports
    changes in storage indices. */
struct DataSource
{
    std::function<int()> rowCount;
    std::function<int()> colCount;
    std::function<juce::var(int row, int col)> getValue;
    std::function<bool(int row, int col, const juce::var& value)> setValue;
    std::function<juce::Result(int row, int col, const juce::var& value)> validate;
    std::function<bool(int row, int col)> isEditable;
    std::function<juce::String(int col)> colName;
    std::function<std::weak_ordering(int col, int rowA, int rowB)> compare;

    /** Inserts rows before storage row at; a ViewModel appends at the end of storage. The source
        must notify RowsInserted for the new storage rows. */
    std::function<bool(int at, const RowData& rows)> insertRows;

    /** Removes storage rows (ascending); the source must notify RowsRemoved. */
    std::function<bool(const std::vector<int>& rows)> removeRows;

    std::function<void()> undo;
    std::function<void()> redo;
    std::function<void()> beginBatch;
    std::function<void()> endBatch;
    GridModelNotifier* notifier = nullptr;
};

/** One Grid's view of a DataSource: its own row order (so sorting one Grid leaves the others
    alone) and an optional subset or arrangement of the source's columns. It translates view
    indices to storage indices for every call, and storage changes to view changes for its Grid.
    Rows inserted through this view appear where they were inserted in it; rows the source inserts
    otherwise appear at the end of the view. The source must have a notifier. Both the DataSource and this object
    must outlive the Grid bound to bind()'s model. */
class ViewModel : private GridModelNotifier::Listener
{
public:
    /** columns lists the source columns to show, in order; empty shows every column. */
    explicit ViewModel(DataSource& source, std::vector<int> columns = {});
    ~ViewModel() override;

    ViewModel(const ViewModel&) = delete;
    ViewModel& operator=(const ViewModel&) = delete;
    ViewModel(ViewModel&&) = delete;
    ViewModel& operator=(ViewModel&&) = delete;

    /** Returns the GridModel to bind to a Grid. */
    [[nodiscard]] GridModel bind();

    [[nodiscard]] const RowOrder& getRowOrder() const;

    /** Restores a saved row order (view row to storage row) and resets the Grid. */
    void setRowOrder(std::vector<int> viewToStorage);

    /** Changes the shown columns and resets the Grid. */
    void setColumns(std::vector<int> columns);

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
