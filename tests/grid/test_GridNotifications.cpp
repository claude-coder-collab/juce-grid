#include "GridTestSupport.h"

#include <catch2/catch_test_macros.hpp>

using jgrid::Cell;
using jgrid::Grid;
using jgrid::test::EditableTable;
using jgrid::test::makeEditableModel;

TEST_CASE("Every bound Grid subscribes to the notifier", "[notifications]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    const auto table = std::make_shared<EditableTable>(4, 2);
    {
        Grid a;
        Grid b;
        a.setModel(makeEditableModel(table));
        b.setModel(makeEditableModel(table));
        CHECK(table->notifier.getNumListeners() == 2);
        b.setModel({});
        CHECK(table->notifier.getNumListeners() == 1);
    }
    CHECK(table->notifier.getNumListeners() == 0);
}

TEST_CASE("A reset refreshes every Grid", "[notifications]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    const auto table = std::make_shared<EditableTable>(4, 2);
    Grid a;
    Grid b;
    a.setModel(makeEditableModel(table));
    b.setModel(makeEditableModel(table));

    table->cells.resize(10, { "x", "y" });
    table->notifier.changed(jgrid::Reset{});
    CHECK(a.getRowCount() == 10);
    CHECK(b.getRowCount() == 10);
}

TEST_CASE("The cursor follows its row through inserts, removals and reorders", "[notifications]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    const auto table = std::make_shared<EditableTable>(6, 2);
    Grid grid;
    grid.setModel(makeEditableModel(table));
    grid.setCursor({ .row = 3, .col = 1 });

    table->cells.insert(table->cells.begin(), { "a", "b" });
    table->notifier.changed(jgrid::RowsInserted{ .at = 0, .count = 1 });
    CHECK(grid.getCursor() == Cell{ .row = 4, .col = 1 });

    table->cells.erase(table->cells.begin());
    table->notifier.changed(jgrid::RowsRemoved{ .rows = { 0 } });
    CHECK(grid.getCursor() == Cell{ .row = 3, .col = 1 });

    table->notifier.changed(jgrid::RowsReordered{ .order = { 5, 4, 3, 2, 1, 0 } });
    CHECK(grid.getCursor() == Cell{ .row = 2, .col = 1 });

    table->cells.erase(table->cells.begin() + 2);
    table->notifier.changed(jgrid::RowsRemoved{ .rows = { 2 } });
    CHECK(grid.getCursor() == Cell{ .row = 2, .col = 1 });
}

TEST_CASE("Removing the edited row cancels the edit", "[notifications]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    const auto table = std::make_shared<EditableTable>(4, 2);
    Grid grid;
    grid.setModel(makeEditableModel(table));
    grid.setSize(400, 200);
    grid.setCursor({ .row = 1, .col = 0 });
    REQUIRE(grid.startEditingWithText("pending"));

    table->cells.insert(table->cells.begin(), { "a", "b" });
    table->notifier.changed(jgrid::RowsInserted{ .at = 0, .count = 1 });
    CHECK(grid.getEditingCell() == Cell{ .row = 2, .col = 0 });

    table->cells.erase(table->cells.begin() + 2);
    table->notifier.changed(jgrid::RowsRemoved{ .rows = { 2 } });
    CHECK_FALSE(grid.isEditing());
}

TEST_CASE("An edit in one Grid keeps another Grid's open editor", "[notifications]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    const auto table = std::make_shared<EditableTable>(4, 2);
    Grid a;
    Grid b;
    a.setModel(makeEditableModel(table));
    b.setModel(makeEditableModel(table));
    a.setSize(400, 200);
    b.setSize(400, 200);

    REQUIRE(b.startEditingWithText("from b"));
    REQUIRE(a.startEditingWithText("from a"));
    CHECK(a.commitEdit());
    CHECK(table->at(0, 0) == "from a");
    CHECK(b.isEditing());
    CHECK(b.commitEdit());
    CHECK(table->at(0, 0) == "from b");
}

TEST_CASE("Destroying the notifier detaches the Grid", "[notifications]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    const auto table = std::make_shared<EditableTable>(4, 2);
    auto notifier = std::make_unique<jgrid::GridModelNotifier>();
    auto model = makeEditableModel(table);
    model.notifier = notifier.get();
    Grid grid;
    grid.setModel(model);
    grid.setSize(400, 200);
    REQUIRE(grid.startEditing());

    notifier.reset();
    CHECK(grid.getRowCount() == 0);
    CHECK_FALSE(grid.isEditing());
    CHECK_FALSE(grid.getModel().isComplete());
}
