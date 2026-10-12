#pragma once

#include <vector>

namespace jgrid
{

//==============================================================================
/**
    A view order over stored rows, so a model can sort without moving its data.

    A RowOrder maps each row the Grid shows (a view row) to a row in your storage (a storage row),
    and back. Sorting reorders the map, not the data, which keeps large datasets cheap to sort and
    makes the order something the model can save and undo.

    A model that supports sorting typically holds a RowOrder, looks up `toStorage(row)` in every
    callback, and implements GridModel::reorderRows with reorder(). ViewModel does all of this.

    @code
    model.getValue = [this](int row, int col) { return data[order.toStorage(row)][col]; };
    model.reorderRows = [this](const std::vector<int>& newOrder) {
        order.reorder(newOrder);
        notifier.changed(jgrid::RowsReordered{ newOrder });
    };
    @endcode

    @see ViewModel, GridModel::reorderRows
*/
class RowOrder
{
public:
    /** Creates the identity order for count rows. */
    explicit RowOrder(int count = 0);

    /** Restores the identity order for count rows. */
    void reset(int count);

    /** Replaces the order with a saved one: `newViewToStorage[viewRow]` is that row's storage row.
        It must be a permutation of 0 to size - 1. */
    void setOrder(std::vector<int> newViewToStorage);

    /** Returns the number of rows. */
    [[nodiscard]] int size() const;

    /** Returns the storage row shown at a view row. */
    [[nodiscard]] int toStorage(int viewRow) const;

    /** Returns the view row showing a storage row. */
    [[nodiscard]] int toView(int storageRow) const;

    /** Returns the whole order: one storage row per view row. Save this to restore a sort later. */
    [[nodiscard]] const std::vector<int>& getOrder() const;

    /** Applies a reorder of the view: `order[newViewRow]` is each row's old view position, as in
        RowsReordered and GridModel::reorderRows. */
    void reorder(const std::vector<int>& order);

    /** Records count new storage rows starting at storage index firstStorage (later storage rows
        shift up), and places them in the view starting at view row viewAt. */
    void insert(int viewAt, int firstStorage, int count);

    /** Records the removal of storage rows, given by their indices before removal.

        @returns the view rows they occupied, ascending, ready to send as RowsRemoved.
    */
    std::vector<int> removeStorage(const std::vector<int>& storageRows);

    /** Follows a reorder of the storage itself (`order[newStorage]` is each row's old storage
        index), keeping the view showing the same data in the same order. */
    void storageReordered(const std::vector<int>& order);

private:
    void rebuildInverse();

    std::vector<int> viewToStorage;
    std::vector<int> storageToView;
};

} // namespace jgrid
