#pragma once

#include <vector>

namespace jgrid
{

/** A model's view order over its storage: a permutation mapping each view row to a storage row,
    so the view can be sorted and saved without moving the stored data. It keeps the inverse map
    too, so both directions are constant-time lookups. */
class RowOrder
{
public:
    explicit RowOrder(int count = 0);

    /** Restores the identity order for count rows. */
    void reset(int count);

    /** Replaces the order with a saved one: viewToStorage[viewRow] = storageRow. It must be a
        permutation of 0..size-1. */
    void setOrder(std::vector<int> newViewToStorage);

    [[nodiscard]] int size() const;
    [[nodiscard]] int toStorage(int viewRow) const;
    [[nodiscard]] int toView(int storageRow) const;
    [[nodiscard]] const std::vector<int>& getOrder() const;

    /** Applies a reorder of the view: order[newViewRow] = oldViewRow, as in RowsReordered. */
    void reorder(const std::vector<int>& order);

    /** Records count new storage rows starting at storage index firstStorage (later storage rows
        shift up) and places them in the view starting at viewAt. */
    void insert(int viewAt, int firstStorage, int count);

    /** Records the removal of storage rows, given as their indices before removal. Returns the
        view rows they occupied, ascending. */
    std::vector<int> removeStorage(const std::vector<int>& storageRows);

    /** Follows a reorder of the storage itself (order[newStorage] = oldStorage), keeping the view
        showing the same data in the same order. */
    void storageReordered(const std::vector<int>& order);

private:
    void rebuildInverse();

    std::vector<int> viewToStorage;
    std::vector<int> storageToView;
};

} // namespace jgrid
