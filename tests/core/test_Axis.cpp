#include <jgrid/core/jgrid_Axis.h>

#include <catch2/catch_test_macros.hpp>

using jgrid::Axis;
using jgrid::IndexRange;

TEST_CASE("Axis positions uniform sizes", "[axis]")
{
    Axis axis{ 10 };
    axis.setCount(5);
    CHECK(axis.getCount() == 5);
    CHECK(axis.getTotalSize() == 50);
    CHECK(axis.getStart(0) == 0);
    CHECK(axis.getStart(3) == 30);
    CHECK(axis.getEnd(4) == 50);
}

TEST_CASE("Axis handles explicit sizes", "[axis]")
{
    Axis axis{ 10 };
    axis.setCount(3);
    axis.setSize(1, 25);
    CHECK(axis.getSize(1) == 25);
    CHECK(axis.getStart(2) == 35);
    CHECK(axis.getTotalSize() == 45);

    axis.setSize(0, -5);
    CHECK(axis.getSize(0) == 0);
}

TEST_CASE("Axis keeps sizes when the count changes", "[axis]")
{
    Axis axis{ 10 };
    axis.setCount(2);
    axis.setSize(0, 30);
    axis.setCount(4);
    CHECK(axis.getSize(0) == 30);
    CHECK(axis.getSize(3) == 10);
    axis.setCount(1);
    CHECK(axis.getTotalSize() == 30);
    axis.setCount(-1);
    CHECK(axis.getCount() == 0);
    CHECK(axis.getTotalSize() == 0);
}

TEST_CASE("Axis default size resets all sizes", "[axis]")
{
    Axis axis{ 10 };
    axis.setCount(2);
    axis.setSize(0, 30);
    axis.setDefaultSize(15);
    CHECK(axis.getDefaultSize() == 15);
    CHECK(axis.getTotalSize() == 30);
}

TEST_CASE("Axis finds the index at a position", "[axis]")
{
    Axis axis{ 10 };
    axis.setCount(3);
    CHECK(axis.indexAt(-1) == -1);
    CHECK(axis.indexAt(0) == 0);
    CHECK(axis.indexAt(9) == 0);
    CHECK(axis.indexAt(10) == 1);
    CHECK(axis.indexAt(29) == 2);
    CHECK(axis.indexAt(30) == -1);
}

TEST_CASE("Axis finds indices intersecting a span", "[axis]")
{
    Axis axis{ 10 };
    axis.setCount(10);
    CHECK(axis.indicesIn(0, 10) == IndexRange{ 0, 1 });
    CHECK(axis.indicesIn(5, 10) == IndexRange{ 0, 2 });
    CHECK(axis.indicesIn(10, 20) == IndexRange{ 1, 3 });
    CHECK(axis.indicesIn(95, 100) == IndexRange{ 9, 10 });
    CHECK(axis.indicesIn(100, 10).isEmpty());
    CHECK(axis.indicesIn(0, 0).isEmpty());
    CHECK(Axis{ 10 }.indicesIn(0, 100).isEmpty());
}

TEST_CASE("Axis finds indices fully inside a span", "[axis]")
{
    Axis axis{ 10 };
    axis.setCount(10);
    CHECK(axis.indicesFullyIn(0, 25) == IndexRange{ 0, 2 });
    CHECK(axis.indicesFullyIn(5, 30) == IndexRange{ 1, 3 });
    CHECK(axis.indicesFullyIn(5, 8).isEmpty());
}

TEST_CASE("IndexRange reports size and membership", "[axis]")
{
    constexpr IndexRange range{ .begin = 2, .end = 5 };
    CHECK(range.size() == 3);
    CHECK(range.contains(2));
    CHECK_FALSE(range.contains(5));
    CHECK(IndexRange{ 4, 1 }.size() == 0);
}

TEST_CASE("Hidden indices keep their size but take no space", "[axis]")
{
    Axis axis{ 10 };
    axis.setCount(4);
    axis.setSize(1, 25);
    axis.setHidden(1, true);
    CHECK(axis.getSize(1) == 0);
    CHECK(axis.getStoredSize(1) == 25);
    CHECK(axis.getTotalSize() == 30);
    CHECK(axis.getStart(2) == 10);
    CHECK(axis.indexAt(10) == 2);
    CHECK(axis.indexAt(9) == 0);
    CHECK(axis.getVisibleCount() == 3);
    CHECK(axis.hasHidden());
    CHECK(axis.getHidden() == std::vector{ 1 });

    axis.setHidden(1, false);
    CHECK(axis.getTotalSize() == 55);
}

TEST_CASE("Visible indices map to and from axis indices", "[axis]")
{
    Axis axis{ 10 };
    axis.setCount(5);
    axis.setHidden(0, true);
    axis.setHidden(3, true);
    CHECK(axis.toVisibleIndex(1) == 0);
    CHECK(axis.toVisibleIndex(4) == 2);
    CHECK(axis.toVisibleIndex(3) == -1);
    CHECK(axis.fromVisibleIndex(1) == 2);
    CHECK(axis.fromVisibleIndex(3) == -1);
    CHECK(axis.firstVisible() == 1);
    CHECK(axis.lastVisible() == 4);
}

TEST_CASE("Stepping skips hidden indices", "[axis]")
{
    Axis axis{ 10 };
    axis.setCount(6);
    axis.setHidden(2, true);
    axis.setHidden(3, true);
    CHECK(axis.step(1, 1) == 4);
    CHECK(axis.step(4, -1) == 1);
    CHECK(axis.step(0, 10) == 5);
    CHECK(axis.step(5, -10) == 0);
    CHECK(axis.step(2, 0) == 4);
    CHECK(axis.step(2, 1) == 4);
    CHECK(axis.step(3, -1) == 1);

    axis.setHidden(4, true);
    axis.setHidden(5, true);
    CHECK(axis.step(4, 0) == 1);

    Axis empty{ 10 };
    CHECK(empty.step(0, 1) == -1);
}

TEST_CASE("Inserting, removing and reordering carry sizes and hidden flags", "[axis]")
{
    Axis axis{ 10 };
    axis.setCount(3);
    axis.setSize(0, 30);
    axis.setHidden(2, true);

    axis.insert(1, 2);
    CHECK(axis.getCount() == 5);
    CHECK(axis.getStoredSize(0) == 30);
    CHECK(axis.isHidden(4));
    CHECK(axis.getStoredSize(1) == 10);

    axis.remove({ 1, 2, 2 });
    CHECK(axis.getCount() == 3);
    CHECK(axis.isHidden(2));

    axis.reorder({ 2, 0, 1 });
    CHECK(axis.isHidden(0));
    CHECK(axis.getStoredSize(1) == 30);
}
