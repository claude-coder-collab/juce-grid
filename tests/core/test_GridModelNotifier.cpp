#include <jgrid/core/jgrid_GridModelNotifier.h>

#include <catch2/catch_test_macros.hpp>

#include <memory>

namespace
{

class CountingListener final : public jgrid::GridModelNotifier::Listener
{
public:
    void modelChanged(const jgrid::Change& /*change*/) override { ++changes; }
    void notifierDestroyed(jgrid::GridModelNotifier& notifier) override
    {
        ++destroyed;
        notifier.removeListener(this);
    }

    int changes = 0;
    int destroyed = 0;
};

} // namespace

TEST_CASE("The notifier broadcasts to every listener", "[notifier]")
{
    CountingListener a;
    CountingListener b;
    jgrid::GridModelNotifier notifier;
    notifier.addListener(&a);
    notifier.addListener(&b);
    CHECK(notifier.getNumListeners() == 2);

    notifier.changed(jgrid::Reset{});
    CHECK(a.changes == 1);
    CHECK(b.changes == 1);

    notifier.removeListener(&a);
    notifier.changed(jgrid::CellsChanged{});
    CHECK(a.changes == 1);
    CHECK(b.changes == 2);
}

TEST_CASE("Destroying the notifier tells its listeners", "[notifier]")
{
    CountingListener a;
    CountingListener b;
    {
        auto notifier = std::make_unique<jgrid::GridModelNotifier>();
        notifier->addListener(&a);
        notifier->addListener(&b);
    }
    CHECK(a.destroyed == 1);
    CHECK(b.destroyed == 1);
}
