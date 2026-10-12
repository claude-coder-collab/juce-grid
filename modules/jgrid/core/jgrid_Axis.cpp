#include "jgrid_Axis.h"

#include <algorithm>
#include <cassert>
#include <cstddef>

namespace jgrid
{

Axis::Axis(int defaultSizeIn) : defaultSize(std::max(0, defaultSizeIn))
{
}

void Axis::setCount(int count)
{
    const auto newCount = static_cast<std::size_t>(std::max(0, count));
    sizes.resize(newCount, defaultSize);
    hidden.resize(newCount, false);
    rebuild();
}

int Axis::getCount() const { return static_cast<int>(sizes.size()); }

void Axis::setDefaultSize(int size)
{
    defaultSize = std::max(0, size);
    std::ranges::fill(sizes, defaultSize);
    rebuild();
}

int Axis::getDefaultSize() const { return defaultSize; }

void Axis::setSize(int index, int size)
{
    assert(index >= 0 && index < getCount());
    sizes[static_cast<std::size_t>(index)] = std::max(0, size);
    rebuild();
}

int Axis::getSize(int index) const { return isHidden(index) ? 0 : getStoredSize(index); }

int Axis::getStoredSize(int index) const
{
    assert(index >= 0 && index < getCount());
    return sizes[static_cast<std::size_t>(index)];
}

void Axis::setHidden(int index, bool shouldBeHidden)
{
    assert(index >= 0 && index < getCount());
    hidden[static_cast<std::size_t>(index)] = shouldBeHidden;
    rebuild();
}

bool Axis::isHidden(int index) const
{
    assert(index >= 0 && index < getCount());
    return hidden[static_cast<std::size_t>(index)];
}

bool Axis::isVisible(int index) const { return index >= 0 && index < getCount() && !isHidden(index); }

int Axis::getVisibleCount() const { return visibleBefore.back(); }

bool Axis::hasHidden() const { return getVisibleCount() < getCount(); }

std::vector<int> Axis::getHidden() const
{
    std::vector<int> result;
    for (auto index = 0; index < getCount(); ++index)
        if (isHidden(index))
            result.push_back(index);
    return result;
}

int Axis::toVisibleIndex(int index) const
{
    if (!isVisible(index))
        return -1;
    return visibleBefore[static_cast<std::size_t>(index)];
}

int Axis::fromVisibleIndex(int visibleIndex) const
{
    if (visibleIndex < 0 || visibleIndex >= getVisibleCount())
        return -1;
    const auto it = std::ranges::upper_bound(visibleBefore, visibleIndex);
    return static_cast<int>(it - visibleBefore.begin()) - 1;
}

int Axis::nextVisible(int index, bool forward) const
{
    if (getCount() == 0)
        return -1;
    const auto clamped = static_cast<std::size_t>(std::clamp(index, 0, getCount() - 1));
    if (index >= getCount())
        return forward ? -1 : fromVisibleIndex(getVisibleCount() - 1);
    if (index < 0)
        return forward ? fromVisibleIndex(0) : -1;
    return forward ? fromVisibleIndex(visibleBefore[clamped + 1]) : fromVisibleIndex(visibleBefore[clamped] - 1);
}

int Axis::firstVisible() const { return fromVisibleIndex(0); }

int Axis::lastVisible() const { return fromVisibleIndex(getVisibleCount() - 1); }

int Axis::step(int from, int delta) const
{
    if (getVisibleCount() == 0)
        return -1;
    const auto clamped = std::clamp(from, 0, getCount() - 1);
    auto position = visibleBefore[static_cast<std::size_t>(clamped)];
    if (isHidden(clamped) && delta > 0)
        --position;
    else if (isHidden(clamped) && delta == 0)
        position = std::min(position, getVisibleCount() - 1);
    return fromVisibleIndex(std::clamp(position + delta, 0, getVisibleCount() - 1));
}

int Axis::getStart(int index) const
{
    assert(index >= 0 && index <= getCount());
    return starts[static_cast<std::size_t>(index)];
}

int Axis::getEnd(int index) const { return getStart(index + 1); }

int Axis::getTotalSize() const { return starts.back(); }

int Axis::indexAt(int position) const
{
    if (position < 0 || position >= getTotalSize())
        return -1;
    const auto it = std::ranges::upper_bound(starts, position);
    return static_cast<int>(it - starts.begin()) - 1;
}

IndexRange Axis::indicesIn(int position, int length) const
{
    if (length <= 0 || getCount() == 0)
        return {};
    const auto first = std::ranges::upper_bound(starts, position);
    const auto last = std::ranges::lower_bound(starts, position + length);
    const auto begin = std::max(0, static_cast<int>(first - starts.begin()) - 1);
    const auto end = std::min(getCount(), static_cast<int>(last - starts.begin()));
    return begin < end ? IndexRange{ .begin = begin, .end = end } : IndexRange{};
}

IndexRange Axis::indicesFullyIn(int position, int length) const
{
    auto range = indicesIn(position, length);
    while (!range.isEmpty() && getStart(range.begin) < position)
        ++range.begin;
    while (!range.isEmpty() && getEnd(range.end - 1) > position + length)
        --range.end;
    return range.isEmpty() ? IndexRange{} : range;
}

void Axis::insert(int at, int count)
{
    if (count <= 0)
        return;
    const auto position = static_cast<std::ptrdiff_t>(std::clamp(at, 0, getCount()));
    sizes.insert(sizes.begin() + position, static_cast<std::size_t>(count), defaultSize);
    hidden.insert(hidden.begin() + position, static_cast<std::size_t>(count), false);
    rebuild();
}

void Axis::remove(const std::vector<int>& indices)
{
    std::vector<bool> doomed(sizes.size(), false);
    for (const auto index : indices)
        if (index >= 0 && index < getCount())
            doomed[static_cast<std::size_t>(index)] = true;
    std::vector<int> keptSizes;
    std::vector<bool> keptHidden;
    for (std::size_t i = 0; i < sizes.size(); ++i)
    {
        if (doomed[i])
            continue;
        keptSizes.push_back(sizes[i]);
        keptHidden.push_back(hidden[i]);
    }
    sizes = std::move(keptSizes);
    hidden = std::move(keptHidden);
    rebuild();
}

void Axis::reorder(const std::vector<int>& order)
{
    if (order.size() != sizes.size())
        return;
    std::vector<int> newSizes;
    std::vector<bool> newHidden;
    newSizes.reserve(order.size());
    newHidden.reserve(order.size());
    for (const auto oldIndex : order)
    {
        newSizes.push_back(sizes.at(static_cast<std::size_t>(oldIndex)));
        newHidden.push_back(hidden.at(static_cast<std::size_t>(oldIndex)));
    }
    sizes = std::move(newSizes);
    hidden = std::move(newHidden);
    rebuild();
}

void Axis::rebuild()
{
    starts.resize(sizes.size() + 1);
    visibleBefore.resize(sizes.size() + 1);
    starts[0] = 0;
    visibleBefore[0] = 0;
    for (std::size_t i = 0; i < sizes.size(); ++i)
    {
        starts[i + 1] = starts[i] + (hidden[i] ? 0 : sizes[i]);
        visibleBefore[i + 1] = visibleBefore[i] + (hidden[i] ? 0 : 1);
    }
}

} // namespace jgrid
