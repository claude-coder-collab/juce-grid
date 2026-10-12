#include "GridTestSupport.h"

#include <catch2/catch_test_macros.hpp>

using jgrid::Cell;
using jgrid::Grid;
using jgrid::test::EditableTable;
using jgrid::test::makeEditableModel;

namespace
{

std::shared_ptr<EditableTable> sortableGrid(Grid& grid)
{
    auto table = std::make_shared<EditableTable>(4, 2);
    table->cells = { { "b", "10" }, { "a", "9" }, { "c", "100" }, { "a", "1" } };
    grid.setModel(makeEditableModel(table));
    grid.setSize(600, 300);
    return table;
}

std::vector<juce::String> column(const EditableTable& table, int col)
{
    std::vector<juce::String> values;
    values.reserve(table.cells.size());
    for (const auto& row : table.cells)
        values.push_back(row.at(static_cast<std::size_t>(col)));
    return values;
}

} // namespace

TEST_CASE("Sorting reorders rows through the model", "[sorting]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    const auto table = sortableGrid(grid);

    CHECK(grid.canSort());
    CHECK(grid.sortByColumn(1, false));
    CHECK(column(*table, 1) == std::vector<juce::String>{ "1", "9", "10", "100" });
    CHECK(grid.getSortIndicator() == Grid::SortIndicator{ .col = 1, .descending = false });
    CHECK(table->batchesBegun == 1);
    CHECK(table->batchesEnded == 1);

    CHECK(grid.toggleSort(1));
    CHECK(column(*table, 1) == std::vector<juce::String>{ "100", "10", "9", "1" });
    CHECK(grid.getSortIndicator() == Grid::SortIndicator{ .col = 1, .descending = true });
}

TEST_CASE("Sorting is stable", "[sorting]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    const auto table = sortableGrid(grid);
    grid.sortByColumn(0, false);
    CHECK(column(*table, 1) == std::vector<juce::String>{ "9", "1", "10", "100" });
}

TEST_CASE("The cursor stays on its row's data", "[sorting]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    const auto table = sortableGrid(grid);
    grid.setCursor({ .row = 2, .col = 1 });
    grid.sortByColumn(1, true);
    CHECK(grid.getCursor() == Cell{ .row = 0, .col = 1 });
}

TEST_CASE("The indicator clears when the order is disturbed", "[sorting]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    const auto table = sortableGrid(grid);
    grid.sortByColumn(0, false);

    SECTION("An edit in another column keeps it")
    {
        grid.setCursor({ .row = 0, .col = 1 });
        REQUIRE(grid.startEditingWithText("5"));
        CHECK(grid.commitEdit());
        CHECK(grid.getSortIndicator());
    }
    SECTION("An edit in the sorted column clears it")
    {
        REQUIRE(grid.startEditingWithText("z"));
        CHECK(grid.commitEdit());
        CHECK_FALSE(grid.getSortIndicator());
    }
    SECTION("Inserted rows clear it")
    {
        table->cells.push_back({ "d", "0" });
        table->notifier.changed(jgrid::RowsInserted{ .at = 4, .count = 1 });
        CHECK_FALSE(grid.getSortIndicator());
    }
    SECTION("Another Grid sorting the shared model clears it")
    {
        Grid other;
        other.setModel(makeEditableModel(table));
        other.sortByColumn(1, false);
        CHECK_FALSE(grid.getSortIndicator());
        CHECK(other.getSortIndicator());
    }
}

TEST_CASE("A model without reorderRows cannot sort", "[sorting]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    auto model = makeEditableModel(std::make_shared<EditableTable>(3, 2));
    model.reorderRows = nullptr;
    grid.setModel(model);
    CHECK_FALSE(grid.canSort());
    CHECK_FALSE(grid.sortByColumn(0, false));
    CHECK_FALSE(grid.getSortIndicator());
}

TEST_CASE("The model's compare callback is used when present", "[sorting]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    const auto table = std::make_shared<EditableTable>(3, 1);
    table->cells = { { "bb" }, { "a" }, { "ccc" } };
    auto model = makeEditableModel(table);
    model.compare = [table](int col, int a, int b) { return std::weak_order(table->at(b, col).length(), table->at(a, col).length()); };
    grid.setModel(model);
    grid.sortByColumn(0, false);
    CHECK(column(*table, 0) == std::vector<juce::String>{ "ccc", "bb", "a" });
}

TEST_CASE("A model without a notifier still follows its own sort", "[sorting]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    const auto table = std::make_shared<EditableTable>(3, 1);
    table->cells = { { "c" }, { "a" }, { "b" } };
    auto model = makeEditableModel(table);
    model.notifier = nullptr;
    grid.setModel(model);
    grid.setCursor({ .row = 0, .col = 0 });
    grid.sortByColumn(0, false);
    CHECK(grid.getCursor() == Cell{ .row = 2, .col = 0 });
}

TEST_CASE("Views over one data source sort independently", "[sorting]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    std::vector<int> values{ 3, 1, 2 };
    jgrid::GridModelNotifier sourceNotifier;
    jgrid::DataSource source;
    source.rowCount = [&] { return static_cast<int>(values.size()); };
    source.colCount = [] { return 1; };
    source.getValue = [&](int row, int) { return juce::var(values.at(static_cast<std::size_t>(row))); };
    source.notifier = &sourceNotifier;

    jgrid::ViewModel viewA{ source };
    jgrid::ViewModel viewB{ source };
    Grid a;
    Grid b;
    a.setModel(viewA.bind());
    b.setModel(viewB.bind());

    CHECK(a.sortByColumn(0, false));
    CHECK(viewA.getRowOrder().getOrder() == std::vector{ 1, 2, 0 });
    CHECK(viewB.getRowOrder().getOrder() == std::vector{ 0, 1, 2 });
    CHECK(a.getSortIndicator());
    CHECK_FALSE(b.getSortIndicator());
    CHECK(static_cast<int>(a.getModel().getValue(0, 0)) == 1);
    CHECK(static_cast<int>(values.front()) == 3);
}

TEST_CASE("Header cells announce and perform sorting", "[sorting]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    const auto table = sortableGrid(grid);
    grid.addToDesktop(0);
    const auto* header = grid.getAccessibilityHandler()->getTableInterface()->getHeaderHandler();
    REQUIRE(header != nullptr);
    const auto* second = header->getChildren().at(1);
    CHECK(second->getRole() == juce::AccessibilityRole::tableHeader);
    CHECK(second->getTitle() == "B");
    CHECK(second->getActions().invoke(juce::AccessibilityActionType::press));
    CHECK(second->getTitle() == "B, sorted ascending");
    CHECK(column(*table, 1) == std::vector<juce::String>{ "1", "9", "10", "100" });
}
