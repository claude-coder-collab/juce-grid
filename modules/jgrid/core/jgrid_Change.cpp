#include "jgrid_Change.h"

#include <algorithm>
#include <cstddef>

namespace jgrid
{

namespace
{

std::optional<int> remap(int row, const CellsChanged& /*change*/) { return row; }

std::optional<int> remap(int row, const RowsInserted& change)
{
    return row >= change.at ? row + std::max(0, change.count) : row;
}

std::optional<int> remap(int row, const RowsRemoved& change)
{
    if (std::ranges::find(change.rows, row) != change.rows.end())
        return std::nullopt;
    return rowAfterRemoval(row, change);
}

std::optional<int> remap(int row, const RowsReordered& change)
{
    const auto it = std::ranges::find(change.order, row);
    if (it == change.order.end())
        return std::nullopt;
    return static_cast<int>(it - change.order.begin());
}

std::optional<int> remap(int /*row*/, const Reset& /*change*/) { return std::nullopt; }

} // namespace

std::optional<int> remapRow(int row, const Change& change)
{
    return std::visit([row](const auto& c) { return remap(row, c); }, change);
}

int rowAfterRemoval(int row, const RowsRemoved& removed)
{
    auto rows = removed.rows;
    std::ranges::sort(rows);
    const auto [first, last] = std::ranges::unique(rows);
    rows.erase(first, last);
    const auto before = std::ranges::lower_bound(rows, row) - rows.begin();
    return row - static_cast<int>(before);
}

} // namespace jgrid
