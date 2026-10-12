#include "GridTestSupport.h"

#include <catch2/catch_test_macros.hpp>

using jgrid::Cell;
using jgrid::Grid;
using jgrid::test::makeModel;
using jgrid::test::TableData;

namespace
{

constexpr int visibleRows = 10;

const juce::AccessibilityTableInterface& tableOf(Grid& grid)
{
    auto* handler = grid.getAccessibilityHandler();
    REQUIRE(handler != nullptr);
    REQUIRE(handler->getRole() == juce::AccessibilityRole::table);
    auto* table = handler->getTableInterface();
    REQUIRE(table != nullptr);
    return *table;
}

void setUp(Grid& grid, bool named = false)
{
    grid.setModel(makeModel(std::make_shared<TableData>(TableData{ .rows = 100, .cols = 4, .named = named })));
    grid.setSize(800, 24 + (visibleRows * 22));
    grid.addToDesktop(0);
}

} // namespace

TEST_CASE("The table reports the model's shape", "[accessibility]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    setUp(grid);
    const auto& table = tableOf(grid);
    CHECK(table.getNumRows() == 100);
    CHECK(table.getNumColumns() == 4);
    CHECK(grid.getAccessibilityHandler()->getChildren().size() >= static_cast<std::size_t>(visibleRows));
}

TEST_CASE("Only visible rows have handlers", "[accessibility]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    setUp(grid);
    const auto& table = tableOf(grid);
    table.getNumRows();

    for (auto row = 0; row < visibleRows; ++row)
        CHECK(table.getRowHandler(row) != nullptr);
    CHECK(table.getRowHandler(50) == nullptr);
    CHECK(table.getCellHandler(50, 0) == nullptr);
    CHECK(table.getCellHandler(0, 4) == nullptr);

    grid.setCursor({ .row = 50, .col = 0 });
    CHECK(table.getRowHandler(50) != nullptr);
    CHECK(table.getRowHandler(0) == nullptr);
}

TEST_CASE("Cell handlers expose values, positions and the cursor", "[accessibility]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    setUp(grid);
    const auto& table = tableOf(grid);

    const auto* cell = table.getCellHandler(2, 3);
    REQUIRE(cell != nullptr);
    CHECK(cell->getRole() == juce::AccessibilityRole::cell);
    const auto children = cell->getChildren();
    REQUIRE(children.size() == 1);
    const auto* text = children.front();
    CHECK(text->getRole() == juce::AccessibilityRole::staticText);
    CHECK(text->getTitle() == "r2c3");
    REQUIRE(text->getValueInterface() != nullptr);
    CHECK(text->getValueInterface()->getCurrentValueAsString() == "r2c3");
    CHECK(text->getValueInterface()->isReadOnly());
    CHECK(table.getRowSpan(*cell)->begin == 2);
    CHECK(table.getColumnSpan(*cell)->begin == 3);
    CHECK(cell->getCurrentState().isSelectable());
    CHECK_FALSE(cell->getCurrentState().isSelected());

    const auto* row = table.getRowHandler(2);
    REQUIRE(row != nullptr);
    CHECK(row->getRole() == juce::AccessibilityRole::row);
    CHECK(row->getTitle() == "3");
    CHECK(table.getRowSpan(*row)->begin == 2);
    CHECK(row->getChildren().size() == 4);

    CHECK(table.getCellHandler(0, 0)->getCurrentState().isSelected());
}

TEST_CASE("The focus action moves the cursor", "[accessibility]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    setUp(grid);
    const auto& table = tableOf(grid);

    const auto* cell = table.getCellHandler(3, 2);
    REQUIRE(cell != nullptr);
    CHECK(cell->getActions().invoke(juce::AccessibilityActionType::focus));
    CHECK(grid.getCursor() == Cell{ .row = 3, .col = 2 });
}

TEST_CASE("The header has one child per column", "[accessibility]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    setUp(grid, true);
    const auto& table = tableOf(grid);

    const auto* header = table.getHeaderHandler();
    REQUIRE(header != nullptr);
    const auto children = header->getChildren();
    REQUIRE(children.size() == 4);
    CHECK(children[0]->getTitle() == "Column 0");
    CHECK(table.getColumnSpan(*children[3])->begin == 3);

    grid.setColumnHeaderVisible(false);
    CHECK(table.getHeaderHandler() == nullptr);
}

TEST_CASE("Header titles default to column letters", "[accessibility]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    setUp(grid);
    const auto& table = tableOf(grid);
    const auto* header = table.getHeaderHandler();
    REQUIRE(header != nullptr);
    const auto children = header->getChildren();
    REQUIRE(children.size() == 4);
    CHECK(children[1]->getTitle() == "B");
}

TEST_CASE("showCell scrolls the cell into view", "[accessibility]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    setUp(grid);
    const auto& table = tableOf(grid);

    grid.setCursor({ .row = 60, .col = 0 });
    const auto* cell = table.getCellHandler(60, 1);
    REQUIRE(cell != nullptr);
    grid.scrollToCell({ .row = 0, .col = 0 });
    CHECK(table.getRowHandler(5) != nullptr);
    table.showCell(*cell);
    CHECK(table.getRowHandler(5) == nullptr);
}

TEST_CASE("Accessibility focus follows the cursor to the cell", "[accessibility]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    juce::Component window;
    window.setFocusContainerType(juce::Component::FocusContainerType::keyboardFocusContainer);
    Grid grid;
    grid.setModel(makeModel(std::make_shared<TableData>(TableData{ .rows = 100, .cols = 4 })));
    window.addAndMakeVisible(grid);
    window.setBounds(0, 0, 800, 400);
    grid.setBounds(window.getLocalBounds());
    window.addToDesktop(0);
    window.setVisible(true);
    const auto& table = tableOf(grid);
    table.getNumRows();

    grid.grabKeyboardFocus();
    REQUIRE(grid.hasKeyboardFocus(false));
    CHECK(grid.keyPressed(juce::KeyPress(juce::KeyPress::downKey)));
    CHECK(grid.keyPressed(juce::KeyPress(juce::KeyPress::rightKey)));

    auto* focused = window.getAccessibilityHandler()->getChildFocus();
    REQUIRE(focused != nullptr);
    CHECK(focused->getRole() == juce::AccessibilityRole::cell);
    CHECK(focused == table.getCellHandler(1, 1));
}

TEST_CASE("Pressing a cell opens its editor", "[accessibility]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    grid.setModel(jgrid::test::makeEditableModel(std::make_shared<jgrid::test::EditableTable>(5, 3)));
    grid.setSize(600, 300);
    grid.addToDesktop(0);
    const auto& table = tableOf(grid);

    const auto* cell = table.getCellHandler(2, 1);
    REQUIRE(cell != nullptr);
    CHECK(cell->getHelp() == "Editable. Press F2 or type to edit.");
    CHECK(cell->getActions().invoke(juce::AccessibilityActionType::press));
    CHECK(grid.getEditingCell() == Cell{ .row = 2, .col = 1 });
}

TEST_CASE("Read-only cells say so in their help", "[accessibility]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    setUp(grid);
    const auto& table = tableOf(grid);
    const auto* cell = table.getCellHandler(0, 0);
    REQUIRE(cell != nullptr);
    CHECK(cell->getHelp() == "Read-only.");
    CHECK(cell->getActions().invoke(juce::AccessibilityActionType::press));
    CHECK_FALSE(grid.isEditing());
}

TEST_CASE("Wholly selected rows report themselves selected", "[accessibility]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    setUp(grid);
    const auto& table = tableOf(grid);
    table.getNumRows();
    grid.selectRows(2, 3);
    CHECK(table.getRowHandler(2)->getCurrentState().isSelected());
    CHECK(table.getRowHandler(3)->getCurrentState().isSelected());
    CHECK_FALSE(table.getRowHandler(4)->getCurrentState().isSelected());
    CHECK(table.getRowHandler(4)->getCurrentState().isMultiSelectable());
}

TEST_CASE("Empty cells expose no text, so VoiceOver reads them as empty", "[accessibility]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    const auto table = std::make_shared<jgrid::test::EditableTable>(3, 2);
    grid.setModel(jgrid::test::makeEditableModel(table));
    grid.setSize(600, 300);
    grid.addToDesktop(0);
    const auto& tableInterface = tableOf(grid);
    tableInterface.getNumRows();

    const auto* text = tableInterface.getCellHandler(1, 1)->getChildren().front();
    CHECK_FALSE(text->isIgnored());
    grid.setCursor({ .row = 1, .col = 1 });
    CHECK(grid.deleteSelection());
    CHECK(text->isIgnored());
    CHECK(tableInterface.getCellHandler(1, 1)->getChildren().empty());
}
