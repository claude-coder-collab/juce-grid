#include <jgrid/core/jgrid_ViewModel.h>

#include <catch2/catch_test_macros.hpp>

#include <vector>

namespace
{

struct Store
{
    std::vector<std::vector<int>> rows{ { 30, 1, 100 }, { 10, 2, 200 }, { 20, 3, 300 } };
    jgrid::GridModelNotifier notifier;
    jgrid::DataSource source;

    Store()
    {
        source.rowCount = [this] { return static_cast<int>(rows.size()); };
        source.colCount = [] { return 3; };
        source.getValue = [this](int row, int col) { return juce::var(rows.at(static_cast<std::size_t>(row)).at(static_cast<std::size_t>(col))); };
        source.setValue = [this](int row, int col, const juce::var& value) {
            rows.at(static_cast<std::size_t>(row)).at(static_cast<std::size_t>(col)) = static_cast<int>(value);
            notifier.changed(jgrid::CellsChanged{ .firstRow = row, .numRows = 1, .firstCol = col, .numCols = 1 });
            return true;
        };
        source.colName = [](int col) { return "S" + juce::String(col); };
        source.notifier = &notifier;
    }
};

class Recorder final : public jgrid::GridModelNotifier::Listener
{
public:
    void modelChanged(const jgrid::Change& change) override { changes.push_back(change); }
    void notifierDestroyed(jgrid::GridModelNotifier& /*notifier*/) override {}

    std::vector<jgrid::Change> changes;
};

} // namespace

TEST_CASE("A view maps columns and rows to the source", "[viewmodel]")
{
    Store store;
    jgrid::ViewModel view{ store.source, { 2, 0 } };
    const auto model = view.bind();
    CHECK(model.colCount() == 2);
    CHECK(model.colName(0) == "S2");
    CHECK(static_cast<int>(model.getValue(1, 1)) == 10);

    CHECK(model.setValue(1, 0, 222));
    CHECK(store.rows[1][2] == 222);
}

TEST_CASE("Each view sorts independently", "[viewmodel]")
{
    Store store;
    jgrid::ViewModel a{ store.source };
    jgrid::ViewModel b{ store.source };
    const auto modelA = a.bind();
    const auto modelB = b.bind();
    Recorder recorder;
    modelA.notifier->addListener(&recorder);

    modelA.reorderRows({ 1, 2, 0 });
    CHECK(static_cast<int>(modelA.getValue(0, 0)) == 10);
    CHECK(static_cast<int>(modelB.getValue(0, 0)) == 30);
    REQUIRE(recorder.changes.size() == 1);
    CHECK(std::holds_alternative<jgrid::RowsReordered>(recorder.changes.front()));
    modelA.notifier->removeListener(&recorder);
}

TEST_CASE("Source changes are translated to view indices", "[viewmodel]")
{
    Store store;
    jgrid::ViewModel view{ store.source, { 2 } };
    const auto model = view.bind();
    model.reorderRows({ 2, 1, 0 });
    Recorder recorder;
    model.notifier->addListener(&recorder);

    SECTION("Cell changes")
    {
        store.source.setValue(0, 2, 5);
        REQUIRE(recorder.changes.size() == 1);
        const auto& cells = std::get<jgrid::CellsChanged>(recorder.changes.front());
        CHECK(cells.firstRow == 2);
        CHECK(cells.firstCol == 0);
        store.source.setValue(0, 1, 5);
        CHECK(recorder.changes.size() == 1);
    }
    SECTION("Inserted rows go to the end of the view")
    {
        store.rows.insert(store.rows.begin(), { 0, 0, 0 });
        store.notifier.changed(jgrid::RowsInserted{ .at = 0, .count = 1 });
        CHECK(std::get<jgrid::RowsInserted>(recorder.changes.front()).at == 3);
        CHECK(view.getRowOrder().getOrder() == std::vector{ 3, 2, 1, 0 });
    }
    SECTION("Removed rows report their view positions")
    {
        store.rows.erase(store.rows.begin());
        store.notifier.changed(jgrid::RowsRemoved{ .rows = { 0 } });
        CHECK(std::get<jgrid::RowsRemoved>(recorder.changes.front()).rows == std::vector{ 2 });
        CHECK(model.rowCount() == 2);
    }
    SECTION("Resets restore the identity order")
    {
        store.notifier.changed(jgrid::Reset{});
        CHECK(std::holds_alternative<jgrid::Reset>(recorder.changes.front()));
        CHECK(view.getRowOrder().getOrder() == std::vector{ 0, 1, 2 });
    }
    model.notifier->removeListener(&recorder);
}

TEST_CASE("Saved orders and column changes reset the view", "[viewmodel]")
{
    Store store;
    jgrid::ViewModel view{ store.source };
    const auto model = view.bind();
    Recorder recorder;
    model.notifier->addListener(&recorder);
    view.setRowOrder({ 2, 0, 1 });
    view.setColumns({ 1 });
    CHECK(recorder.changes.size() == 2);
    CHECK(static_cast<int>(model.getValue(0, 0)) == 3);
    model.notifier->removeListener(&recorder);
}
