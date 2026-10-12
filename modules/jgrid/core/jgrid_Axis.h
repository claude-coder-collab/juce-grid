#pragma once

#include <vector>

namespace jgrid
{

/** Half-open range of indices [begin, end). */
struct IndexRange
{
    int begin = 0;
    int end = 0;

    [[nodiscard]] bool isEmpty() const { return end <= begin; }
    [[nodiscard]] int size() const { return isEmpty() ? 0 : end - begin; }
    [[nodiscard]] bool contains(int index) const { return index >= begin && index < end; }

    friend bool operator==(const IndexRange&, const IndexRange&) = default;
};

/** Geometry of one grid dimension (rows or columns): each index's size, whether it is hidden,
    and its pixel position. A hidden index keeps its size but occupies no space. */
class Axis
{
public:
    explicit Axis(int defaultSize = 0);

    /** Changes the count, keeping existing indices; new indices are visible at the default size. */
    void setCount(int count);
    [[nodiscard]] int getCount() const;

    /** Sets the size of every index, and of indices added later. */
    void setDefaultSize(int size);
    [[nodiscard]] int getDefaultSize() const;

    void setSize(int index, int size);

    /** Returns the space an index occupies: 0 when hidden. */
    [[nodiscard]] int getSize(int index) const;

    /** Returns an index's size whether or not it is hidden. */
    [[nodiscard]] int getStoredSize(int index) const;

    void setHidden(int index, bool hidden);
    [[nodiscard]] bool isHidden(int index) const;
    [[nodiscard]] bool isVisible(int index) const;
    [[nodiscard]] int getVisibleCount() const;
    [[nodiscard]] bool hasHidden() const;
    [[nodiscard]] std::vector<int> getHidden() const;

    /** Returns the position of an index among the visible ones, or -1 if it is hidden. */
    [[nodiscard]] int toVisibleIndex(int index) const;

    /** Returns the index of the nth visible index, or -1. */
    [[nodiscard]] int fromVisibleIndex(int visibleIndex) const;

    /** Returns the nearest visible index after (or before) an index, or -1 if there is none. */
    [[nodiscard]] int nextVisible(int index, bool forward) const;

    [[nodiscard]] int firstVisible() const;
    [[nodiscard]] int lastVisible() const;

    /** Moves delta visible steps from an index, clamped to the first and last visible index.
        A delta of 0 returns the index itself if visible, else the nearest visible index after
        it, else before it. Returns -1 if nothing is visible. */
    [[nodiscard]] int step(int from, int delta) const;

    [[nodiscard]] int getStart(int index) const;
    [[nodiscard]] int getEnd(int index) const;
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
