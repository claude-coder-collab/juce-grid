#include <jgrid/core/jgrid_RowOrder.h>

#include <catch2/catch_test_macros.hpp>

using jgrid::RowOrder;

namespace
{

void checkInverse(const RowOrder& order)
{
    for (auto view = 0; view < order.size(); ++view)
        CHECK(order.toView(order.toStorage(view)) == view);
}

} // namespace

TEST_CASE("A new order is the identity", "[roworder]")
{
    const RowOrder order{ 3 };
    CHECK(order.getOrder() == std::vector{ 0, 1, 2 });
    CHECK(order.toView(2) == 2);
}

TEST_CASE("Reordering composes with the existing order", "[roworder]")
{
    RowOrder order{ 4 };
    order.reorder({ 3, 2, 1, 0 });
    CHECK(order.getOrder() == std::vector{ 3, 2, 1, 0 });
    order.reorder({ 1, 0, 2, 3 });
    CHECK(order.getOrder() == std::vector{ 2, 3, 1, 0 });
    CHECK(order.toView(2) == 0);
    checkInverse(order);
}

TEST_CASE("Inserted storage rows land at the requested view position", "[roworder]")
{
    RowOrder order{ 3 };
    order.reorder({ 2, 0, 1 });
    order.insert(1, 1, 2);
    CHECK(order.getOrder() == std::vector{ 4, 1, 2, 0, 3 });
    checkInverse(order);
}

TEST_CASE("Removing storage rows reports their view rows", "[roworder]")
{
    RowOrder order{ 5 };
    order.reorder({ 4, 3, 2, 1, 0 });
    const auto viewRows = order.removeStorage({ 1, 3, 3 });
    CHECK(viewRows == std::vector{ 1, 3 });
    CHECK(order.getOrder() == std::vector{ 2, 1, 0 });
    checkInverse(order);
}

TEST_CASE("Storage reorders keep the view showing the same data", "[roworder]")
{
    RowOrder order{ 3 };
    order.reorder({ 1, 2, 0 });
    order.storageReordered({ 2, 0, 1 });
    CHECK(order.getOrder() == std::vector{ 2, 0, 1 });
    checkInverse(order);
}

TEST_CASE("Saved orders can be restored", "[roworder]")
{
    RowOrder order;
    order.setOrder({ 1, 0 });
    CHECK(order.toStorage(0) == 1);
    order.reset(2);
    CHECK(order.getOrder() == std::vector{ 0, 1 });
}
