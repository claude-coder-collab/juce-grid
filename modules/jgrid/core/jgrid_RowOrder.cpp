#include "jgrid_RowOrder.h"

#include <algorithm>
#include <cassert>
#include <numeric>

namespace jgrid
{

RowOrder::RowOrder(int count) { reset(count); }

void RowOrder::reset(int count)
{
    viewToStorage.resize(static_cast<std::size_t>(std::max(0, count)));
    std::ranges::iota(viewToStorage, 0);
    rebuildInverse();
}

void RowOrder::setOrder(std::vector<int> newViewToStorage)
{
    viewToStorage = std::move(newViewToStorage);
    rebuildInverse();
}

int RowOrder::size() const { return static_cast<int>(viewToStorage.size()); }

int RowOrder::toStorage(int viewRow) const { return viewToStorage.at(static_cast<std::size_t>(viewRow)); }

int RowOrder::toView(int storageRow) const { return storageToView.at(static_cast<std::size_t>(storageRow)); }

const std::vector<int>& RowOrder::getOrder() const { return viewToStorage; }

void RowOrder::reorder(const std::vector<int>& order)
{
    assert(order.size() == viewToStorage.size());
    std::vector<int> reordered;
    reordered.reserve(order.size());
    for (const auto oldView : order)
        reordered.push_back(viewToStorage.at(static_cast<std::size_t>(oldView)));
    viewToStorage = std::move(reordered);
    rebuildInverse();
}

void RowOrder::insert(int viewAt, int firstStorage, int count)
{
    if (count <= 0)
        return;
    for (auto& storage : viewToStorage)
        if (storage >= firstStorage)
            storage += count;
    std::vector<int> added(static_cast<std::size_t>(count));
    std::ranges::iota(added, firstStorage);
    const auto position = std::clamp(viewAt, 0, size());
    viewToStorage.insert(viewToStorage.begin() + position, added.begin(), added.end());
    rebuildInverse();
}

std::vector<int> RowOrder::removeStorage(const std::vector<int>& storageRows)
{
    auto removed = storageRows;
    std::ranges::sort(removed);
    const auto [first, last] = std::ranges::unique(removed);
    removed.erase(first, last);
    std::erase_if(removed, [this](int row) { return row < 0 || row >= size(); });

    std::vector<int> viewRows;
    viewRows.reserve(removed.size());
    for (const auto storage : removed)
        viewRows.push_back(toView(storage));
    std::ranges::sort(viewRows);

    std::erase_if(viewToStorage, [&](int storage) { return std::ranges::binary_search(removed, storage); });
    for (auto& storage : viewToStorage)
        storage -= static_cast<int>(std::ranges::lower_bound(removed, storage) - removed.begin());
    rebuildInverse();
    return viewRows;
}

void RowOrder::storageReordered(const std::vector<int>& order)
{
    assert(order.size() == viewToStorage.size());
    std::vector<int> newIndexOf(order.size());
    for (std::size_t newStorage = 0; newStorage < order.size(); ++newStorage)
        newIndexOf.at(static_cast<std::size_t>(order[newStorage])) = static_cast<int>(newStorage);
    for (auto& storage : viewToStorage)
        storage = newIndexOf.at(static_cast<std::size_t>(storage));
    rebuildInverse();
}

void RowOrder::rebuildInverse()
{
    storageToView.assign(viewToStorage.size(), -1);
    for (std::size_t view = 0; view < viewToStorage.size(); ++view)
        storageToView.at(static_cast<std::size_t>(viewToStorage[view])) = static_cast<int>(view);
}

} // namespace jgrid
