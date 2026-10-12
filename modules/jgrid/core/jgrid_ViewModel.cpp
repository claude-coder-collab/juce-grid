#include "jgrid_ViewModel.h"

#include <algorithm>

namespace jgrid
{

ViewModel::ViewModel(DataSource& sourceIn, std::vector<int> columnsIn) : source(sourceIn),
                                                                         subscribed(sourceIn.notifier),
                                                                         columns(std::move(columnsIn)),
                                                                         order(sourceIn.rowCount ? sourceIn.rowCount() : 0)
{
    if (subscribed != nullptr)
        subscribed->addListener(this);
}

ViewModel::~ViewModel()
{
    if (subscribed != nullptr)
        subscribed->removeListener(this);
}

GridModel ViewModel::bind()
{
    GridModel model;
    model.rowCount = [this] { return order.size(); };
    model.colCount = [this] { return getColCount(); };
    model.getValue = [this](int row, int col) { return source.getValue(order.toStorage(row), toSourceColumn(col)); };
    if (source.setValue)
        model.setValue = [this](int row, int col, const juce::var& value) { return source.setValue(order.toStorage(row), toSourceColumn(col), value); };
    if (source.validate)
        model.validate = [this](int row, int col, const juce::var& value) { return source.validate(order.toStorage(row), toSourceColumn(col), value); };
    if (source.isEditable)
        model.isEditable = [this](int row, int col) { return source.isEditable(order.toStorage(row), toSourceColumn(col)); };
    if (source.colName)
        model.colName = [this](int col) { return source.colName(toSourceColumn(col)); };
    if (source.compare)
        model.compare = [this](int col, int rowA, int rowB) { return source.compare(toSourceColumn(col), order.toStorage(rowA), order.toStorage(rowB)); };
    model.reorderRows = [this](const std::vector<int>& newOrder) {
        order.reorder(newOrder);
        notifier.changed(RowsReordered{ .order = newOrder });
    };
    if (source.insertRows)
    {
        model.insertRows = [this](int at, const RowData& rows) {
            pendingInsertAt = std::clamp(at, 0, order.size());
            const auto inserted = source.insertRows(source.rowCount ? source.rowCount() : order.size(), rows);
            pendingInsertAt.reset();
            return inserted;
        };
    }
    if (source.removeRows)
    {
        model.removeRows = [this](const std::vector<int>& rows) {
            std::vector<int> storageRows;
            storageRows.reserve(rows.size());
            for (const auto row : rows)
                storageRows.push_back(order.toStorage(row));
            std::ranges::sort(storageRows);
            return source.removeRows(storageRows);
        };
    }
    model.undo = source.undo;
    model.redo = source.redo;
    model.beginBatch = source.beginBatch;
    model.endBatch = source.endBatch;
    model.notifier = &notifier;
    return model;
}

const RowOrder& ViewModel::getRowOrder() const { return order; }

void ViewModel::setRowOrder(std::vector<int> viewToStorage)
{
    order.setOrder(std::move(viewToStorage));
    notifier.changed(Reset{});
}

void ViewModel::setColumns(std::vector<int> newColumns)
{
    columns = std::move(newColumns);
    notifier.changed(Reset{});
}

int ViewModel::toSourceColumn(int viewCol) const { return columns.empty() ? viewCol : columns.at(static_cast<std::size_t>(viewCol)); }

int ViewModel::getColCount() const
{
    if (!columns.empty())
        return static_cast<int>(columns.size());
    return source.colCount ? source.colCount() : 0;
}

std::vector<int> ViewModel::viewColumnsFor(int firstSourceCol, int numSourceCols) const
{
    std::vector<int> result;
    for (auto viewCol = 0; viewCol < getColCount(); ++viewCol)
    {
        const auto sourceCol = toSourceColumn(viewCol);
        if (sourceCol >= firstSourceCol && sourceCol < firstSourceCol + numSourceCols)
            result.push_back(viewCol);
    }
    return result;
}

void ViewModel::modelChanged(const Change& change)
{
    if (const auto* cells = std::get_if<CellsChanged>(&change))
    {
        const auto viewCols = viewColumnsFor(cells->firstCol, cells->numCols);
        if (viewCols.empty())
            return;
        auto firstRow = order.size();
        auto lastRow = -1;
        for (auto storage = cells->firstRow; storage < cells->firstRow + cells->numRows && storage < order.size(); ++storage)
        {
            const auto view = order.toView(storage);
            firstRow = std::min(firstRow, view);
            lastRow = std::max(lastRow, view);
        }
        if (lastRow < firstRow)
            return;
        notifier.changed(CellsChanged{ .firstRow = firstRow,
            .numRows = lastRow - firstRow + 1,
            .firstCol = viewCols.front(),
            .numCols = viewCols.back() - viewCols.front() + 1 });
        return;
    }
    if (const auto* inserted = std::get_if<RowsInserted>(&change))
    {
        const auto at = pendingInsertAt.value_or(order.size());
        order.insert(at, inserted->at, inserted->count);
        notifier.changed(RowsInserted{ .at = at, .count = inserted->count });
        return;
    }
    if (const auto* removed = std::get_if<RowsRemoved>(&change))
    {
        notifier.changed(RowsRemoved{ .rows = order.removeStorage(removed->rows) });
        return;
    }
    if (const auto* reordered = std::get_if<RowsReordered>(&change))
    {
        order.storageReordered(reordered->order);
        return;
    }
    order.reset(source.rowCount ? source.rowCount() : 0);
    notifier.changed(Reset{});
}

void ViewModel::notifierDestroyed(GridModelNotifier& destroyed)
{
    if (&destroyed == subscribed)
        subscribed = nullptr;
}

} // namespace jgrid
