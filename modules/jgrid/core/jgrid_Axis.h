#pragma once

#include <vector>

namespace jgrid
{

//==============================================================================
/** A half-open range of indices: [begin, end). */
struct IndexRange
{
    int begin = 0; /**< The first index in the range. */
    int end = 0; /**< One past the last index in the range. */

    /** Returns true if the range has no indices. */
    [[nodiscard]] bool isEmpty() const { return end <= begin; }
    /** Returns the number of indices in the range. */
    [[nodiscard]] int size() const { return isEmpty() ? 0 : end - begin; }
    /** Returns true if the index is in the range. */
    [[nodiscard]] bool contains(int index) const { return index >= begin && index < end; }

    /** Compares two ranges. */
    friend bool operator==(const IndexRange&, const IndexRange&) = default;
};

//==============================================================================
/**
    The geometry of one Grid dimension, rows or columns: each index's size, whether it is hidden,
    and its pixel position.

    A hidden index keeps its size but takes no space. The Grid keeps one Axis for rows and one for
    columns; this is a building block you only need when writing your own layout code.

    @see Grid::setColumnWidth, Grid::setRowsHidden
*/
class Axis
{
public:
    /** Creates an empty axis whose indices will have the given size. */
    explicit Axis(int defaultSize = 0);

    /** Changes the count, keeping existing indices; new indices are visible at the default size. */
    void setCount(int count);
    /** Returns the number of indices. */
    [[nodiscard]] int getCount() const;

    /** Sets the size of every index, and of indices added later. */
    void setDefaultSize(int size);
    /** Returns the size new indices get. */
    [[nodiscard]] int getDefaultSize() const;

    /** Sets one index's size; negative sizes become 0. */
    void setSize(int index, int size);

    /** Returns the space an index occupies: 0 when hidden. */
    [[nodiscard]] int getSize(int index) const;

    /** Returns an index's size whether or not it is hidden. */
    [[nodiscard]] int getStoredSize(int index) const;

    /** Hides or shows an index. A hidden index keeps its size. */
    void setHidden(int index, bool hidden);
    /** Returns true if the index is hidden. */
    [[nodiscard]] bool isHidden(int index) const;
    /** Returns true if the index exists and is not hidden. */
    [[nodiscard]] bool isVisible(int index) const;
    /** Returns the number of visible indices. */
    [[nodiscard]] int getVisibleCount() const;
    /** Returns true if any index is hidden. */
    [[nodiscard]] bool hasHidden() const;
    /** Returns the hidden indices, ascending. */
    [[nodiscard]] std::vector<int> getHidden() const;

    /** Returns the position of an index among the visible ones, or -1 if it is hidden. */
    [[nodiscard]] int toVisibleIndex(int index) const;

    /** Returns the index of the nth visible index, or -1. */
    [[nodiscard]] int fromVisibleIndex(int visibleIndex) const;

    /** Returns the nearest visible index after (or before) an index, or -1 if there is none. */
    [[nodiscard]] int nextVisible(int index, bool forward) const;

    /** Returns the first visible index, or -1. */
    [[nodiscard]] int firstVisible() const;
    /** Returns the last visible index, or -1. */
    [[nodiscard]] int lastVisible() const;

    /** Moves delta visible steps from an index, clamped to the first and last visible index.
        A delta of 0 returns the index itself if visible, else the nearest visible index after
        it, else before it. Returns -1 if nothing is visible. */
    [[nodiscard]] int step(int from, int delta) const;

    /** Returns the pixel position where an index starts. getStart(getCount()) is the total size. */
    [[nodiscard]] int getStart(int index) const;
    /** Returns the pixel position where an index ends. */
    [[nodiscard]] int getEnd(int index) const;
    /** Returns the total size of the visible indices. */
    [[nodiscard]] int getTotalSize() const;

    /** Returns the visible index covering the given position, or -1 if it lies outside the axis. */
    [[nodiscard]] int indexAt(int position) const;

    /** Returns the indices that intersect [position, position + length). */
    [[nodiscard]] IndexRange indicesIn(int position, int length) const;

    /** Returns the indices lying entirely within [position, position + length). */
    [[nodiscard]] IndexRange indicesFullyIn(int position, int length) const;

    /** Inserts count visible indices of the default size before index at. */
    void insert(int at, int count);

    /** Removes the given indices (any order, duplicates allowed). */
    void remove(const std::vector<int>& indices);

    /** Reorders the indices: order[newIndex] = oldIndex. */
    void reorder(const std::vector<int>& order);

private:
    void rebuild();

    int defaultSize;
    std::vector<int> sizes;
    std::vector<bool> hidden;
    std::vector<int> starts{ 0 };
    std::vector<int> visibleBefore{ 0 };
};

} // namespace jgrid
