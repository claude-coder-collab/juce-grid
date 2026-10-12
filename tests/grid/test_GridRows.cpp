#include "GridTestSupport.h"

#include <catch2/catch_test_macros.hpp>

using jgrid::ContextMenuTarget;
using jgrid::Grid;
using jgrid::test::EditableTable;
using jgrid::test::makeEditableModel;

namespace
{

std::shared_ptr<EditableTable> rowGrid(Grid& grid, int rows = 5, int cols = 2)
{
    auto table = std::make_shared<EditableTable>(rows, cols);
    grid.setModel(makeEditableModel(table));
    grid.setSize(600, 300);
    return table;
}

std::vector<juce::String> firstColumn(const EditableTable& table)
{
    std::vector<juce::String> values;
    values.reserve(table.cells.size());
    for (const auto& row : table.cells)
        values.push_back(row.front());
    return values;
}

struct MenuEntry
{
    juce::String text;
    bool enabled = false;
    std::function<void()> action;
};

std::vector<MenuEntry> entries(const juce::PopupMenu& menu)
{
    std::vector<MenuEntry> result;
    for (juce::PopupMenu::MenuItemIterator it{ menu }; it.next();)
    {
        const auto& item = it.getItem();
        if (!item.isSeparator)
            result.push_back({ .text = item.text, .enabled = item.isEnabled, .action = item.action });
    }
    return result;
}

const MenuEntry& entry(const std::vector<MenuEntry>& menu, const juce::String& text)
{
    const auto it = std::ranges::find_if(menu, [&](const MenuEntry& e) { return e.text == text; });
    REQUIRE(it != menu.end());
    return *it;
}

} // namespace

TEST_CASE("Selected rows are whole visible rows", "[rows]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    rowGrid(grid);
    grid.selectRows(1, 3);
    grid.setRowsHidden({ 2 }, true);
    CHECK(grid.getSelectedRows() == std::vector{ 1, 3 });
    grid.selectColumns(1, 1);
    CHECK(grid.getSelectedRows().empty());
    CHECK(grid.getSelectedColumns() == std::vector{ 1 });
}

TEST_CASE("Copied rows are inserted below a row", "[rows]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    const auto table = rowGrid(grid);
    grid.selectRows(0, 1);
    CHECK(grid.copyRows());
    CHECK(juce::SystemClipboard::getTextFromClipboard() == "r0c0\tr0c1\nr1c0\tr1c1\n");

    CHECK(grid.insertCopiedRows(3));
    CHECK(firstColumn(*table) == std::vector<juce::String>{ "r0c0", "r1c0", "r2c0", "r3c0", "r0c0", "r1c0", "r4c0" });
    CHECK(grid.getSelectedRows() == std::vector{ 4, 5 });
    CHECK(table->batchesBegun == 1);
}

TEST_CASE("Inserted clipboard rows are padded or truncated to the columns", "[rows]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    const auto table = rowGrid(grid);
    juce::SystemClipboard::copyTextToClipboard("a\nb\tc\td\n");
    CHECK(grid.insertCopiedRows(-1));
    CHECK(table->cells[0] == std::vector<juce::String>{ "a", "" });
    CHECK(table->cells[1] == std::vector<juce::String>{ "b", "c" });
}

TEST_CASE("Cut rows move when inserted", "[rows]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    const auto table = rowGrid(grid);
    grid.selectRows(0, 1);
    CHECK(grid.cutRows());
    CHECK(firstColumn(*table).size() == 5);

    CHECK(grid.insertCopiedRows(3));
    CHECK(firstColumn(*table) == std::vector<juce::String>{ "r2c0", "r3c0", "r0c0", "r1c0", "r4c0" });
    CHECK(grid.getSelectedRows() == std::vector{ 2, 3 });
    CHECK(table->batchesBegun == 1);
    CHECK(table->batchesEnded == 1);
}

TEST_CASE("Cut rows move between Grids", "[rows]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid a;
    Grid b;
    const auto source = rowGrid(a);
    const auto destination = rowGrid(b, 2);
    a.selectRows(4, 4);
    CHECK(a.cutRows());
    CHECK(b.insertCopiedRows(0));
    CHECK(firstColumn(*destination) == std::vector<juce::String>{ "r0c0", "r4c0", "r1c0" });
    CHECK(source->cells.size() == 4);
}

TEST_CASE("Rows are duplicated after the selection", "[rows]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    const auto table = rowGrid(grid);
    grid.selectRows(1, 2);
    CHECK(grid.duplicateSelectedRows());
    CHECK(firstColumn(*table) == std::vector<juce::String>{ "r0c0", "r1c0", "r2c0", "r1c0", "r2c0", "r3c0", "r4c0" });
    CHECK(grid.getSelectedRows() == std::vector{ 3, 4 });
}

TEST_CASE("A model's duplicateRows is preferred", "[rows]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    const auto table = std::make_shared<EditableTable>(3, 1);
    std::vector<int> duplicated;
    auto model = makeEditableModel(table);
    model.duplicateRows = [&](const std::vector<int>& rows) {
        duplicated = rows;
        return true;
    };
    grid.setModel(model);
    grid.selectRows(2, 0);
    CHECK(grid.duplicateSelectedRows());
    CHECK(duplicated == std::vector{ 0, 1, 2 });
}

TEST_CASE("Undo and redo keys are forwarded", "[rows]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    const auto table = rowGrid(grid);
    CHECK(grid.keyPressed(juce::KeyPress('z', juce::ModifierKeys::commandModifier, 0)));
    CHECK(grid.keyPressed(juce::KeyPress('z', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier, 0)));
    CHECK(grid.keyPressed(juce::KeyPress('y', juce::ModifierKeys::commandModifier, 0)));
    CHECK(table->undos == 1);
    CHECK(table->redos == 2);

    auto model = grid.getModel();
    model.undo = nullptr;
    grid.setModel(model);
    CHECK_FALSE(grid.keyPressed(juce::KeyPress('z', juce::ModifierKeys::commandModifier, 0)));
}

TEST_CASE("Menus offer the operations the model supports", "[rows]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    const auto table = rowGrid(grid);
    juce::SystemClipboard::copyTextToClipboard("x");

    const auto cells = entries(grid.createContextMenu(ContextMenuTarget::Kind::cells));
    CHECK(entry(cells, "Paste").enabled);

    grid.selectRows(1, 1);
    auto rows = entries(grid.createContextMenu(ContextMenuTarget::Kind::rows));
    CHECK(entry(rows, "Cut Rows").enabled);
    CHECK(entry(rows, "Insert Copied Rows Below").enabled);
    CHECK_FALSE(entry(rows, "Show Hidden Rows").enabled);
    entry(rows, "Hide Rows").action();
    CHECK(grid.isRowHidden(1));
    rows = entries(grid.createContextMenu(ContextMenuTarget::Kind::rows));
    CHECK(entry(rows, "Show Hidden Rows").enabled);

    auto model = grid.getModel();
    model.setValue = nullptr;
    model.removeRows = nullptr;
    model.insertRows = nullptr;
    model.reorderRows = nullptr;
    grid.setModel(model);
    grid.selectRows(0, 0);
    CHECK_FALSE(entry(entries(grid.createContextMenu(ContextMenuTarget::Kind::cells)), "Paste").enabled);
    CHECK_FALSE(entry(entries(grid.createContextMenu(ContextMenuTarget::Kind::rows)), "Cut Rows").enabled);
    CHECK_FALSE(entry(entries(grid.createContextMenu(ContextMenuTarget::Kind::rows)), "Duplicate Rows").enabled);
    CHECK_FALSE(entry(entries(grid.createContextMenu(ContextMenuTarget::Kind::columns)), "Sort Ascending").enabled);
}

TEST_CASE("Column menus sort and hide", "[rows]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    const auto table = rowGrid(grid);
    grid.selectColumns(1, 1);
    const auto columns = entries(grid.createContextMenu(ContextMenuTarget::Kind::columns));
    entry(columns, "Sort Descending").action();
    CHECK(grid.getSortIndicator() == Grid::SortIndicator{ .col = 1, .descending = true });
    entry(columns, "Hide Columns").action();
    CHECK(grid.isColumnHidden(1));
}

TEST_CASE("The model's hook can add menu items", "[rows]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    const auto table = std::make_shared<EditableTable>(3, 1);
    auto model = makeEditableModel(table);
    std::optional<ContextMenuTarget::Kind> seen;
    model.contextMenu = [&](juce::PopupMenu& menu, const ContextMenuTarget& target) {
        seen = target.kind;
        menu.addItem("Custom", [] {});
    };
    grid.setModel(model);
    const auto menu = entries(grid.createContextMenu(ContextMenuTarget::Kind::cells));
    CHECK(entry(menu, "Custom").enabled);
    CHECK(seen == ContextMenuTarget::Kind::cells);
}

TEST_CASE("Rows inserted through a view land where they were inserted", "[rows]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    std::vector<juce::String> values{ "a", "b", "c" };
    jgrid::GridModelNotifier notifier;
    jgrid::DataSource source;
    source.rowCount = [&] { return static_cast<int>(values.size()); };
    source.colCount = [] { return 1; };
    source.getValue = [&](int row, int) { return juce::var(values.at(static_cast<std::size_t>(row))); };
    source.insertRows = [&](int at, const jgrid::RowData& rows) {
        for (const auto& row : rows)
            values.insert(values.begin() + at, row.front().toString());
        notifier.changed(jgrid::RowsInserted{ .at = at, .count = static_cast<int>(rows.size()) });
        return true;
    };
    source.removeRows = [&](const std::vector<int>& rows) {
        for (const auto row : std::views::reverse(rows))
            values.erase(values.begin() + row);
        notifier.changed(jgrid::RowsRemoved{ .rows = rows });
        return true;
    };
    source.notifier = &notifier;

    jgrid::ViewModel viewA{ source };
    jgrid::ViewModel viewB{ source };
    Grid a;
    Grid b;
    a.setModel(viewA.bind());
    b.setModel(viewB.bind());

    juce::SystemClipboard::copyTextToClipboard("new\n");
    CHECK(a.insertCopiedRows(0));
    CHECK(a.getModel().getValue(1, 0).toString() == "new");
    CHECK(b.getModel().getValue(3, 0).toString() == "new");

    a.selectRows(1, 1);
    CHECK(a.cutRows());
    CHECK(a.insertCopiedRows(3));
    CHECK(values == std::vector<juce::String>{ "a", "b", "c", "new" });
    CHECK(a.getModel().getValue(3, 0).toString() == "new");
}

TEST_CASE("Cmd/Ctrl+minus deletes rows", "[rows]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    const auto table = rowGrid(grid);
    grid.setCursor({ .row = 1, .col = 1 });
    CHECK(grid.keyPressed(juce::KeyPress('-', juce::ModifierKeys::commandModifier, 0)));
    CHECK(firstColumn(*table) == std::vector<juce::String>{ "r0c0", "r2c0", "r3c0", "r4c0" });

    grid.selectRows(1, 2);
    CHECK(grid.keyPressed(juce::KeyPress('-', juce::ModifierKeys::commandModifier, 0)));
    CHECK(firstColumn(*table) == std::vector<juce::String>{ "r0c0", "r4c0" });
    CHECK(table->batchesBegun == 2);
}

TEST_CASE("Cmd/Ctrl+equals inserts blank rows below", "[rows]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    const auto table = rowGrid(grid);
    grid.setCursor({ .row = 1, .col = 1 });
    CHECK(grid.keyPressed(juce::KeyPress('=', juce::ModifierKeys::commandModifier, 0)));
    CHECK(firstColumn(*table) == std::vector<juce::String>{ "r0c0", "r1c0", "", "r2c0", "r3c0", "r4c0" });
    CHECK(grid.getCursor() == jgrid::Cell{ .row = 2, .col = 1 });

    grid.selectRows(3, 4);
    CHECK(grid.keyPressed(juce::KeyPress('+', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier, 0)));
    CHECK(firstColumn(*table) == std::vector<juce::String>{ "r0c0", "r1c0", "", "r2c0", "r3c0", "", "", "r4c0" });
    CHECK(grid.getSelectedRows() == std::vector{ 5, 6 });
}

TEST_CASE("Row keys do nothing without row operations", "[rows]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    auto model = makeEditableModel(std::make_shared<EditableTable>(2, 1));
    model.insertRows = nullptr;
    model.removeRows = nullptr;
    grid.setModel(model);
    CHECK_FALSE(grid.keyPressed(juce::KeyPress('-', juce::ModifierKeys::commandModifier, 0)));
    CHECK_FALSE(grid.keyPressed(juce::KeyPress('=', juce::ModifierKeys::commandModifier, 0)));
}

TEST_CASE("Context menus are placed at the click, not the whole Grid", "[rows]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    rowGrid(grid);
    const juce::Rectangle<int> click{ 140, 12, 1, 1 };
    const auto options = grid.getContextMenuOptions(click);
    CHECK(options.getTargetScreenArea() == click);
    CHECK(options.getTargetComponent() == &grid);
}

TEST_CASE("Hidden columns can be shown one at a time or all together", "[rows]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    rowGrid(grid, 3, 4);
    CHECK_FALSE(entry(entries(grid.createContextMenu(ContextMenuTarget::Kind::columns)), "Show Hidden Columns").enabled);

    grid.setColumnsHidden({ 1, 3 }, true);
    const auto menu = grid.createContextMenu(ContextMenuTarget::Kind::columns);
    const juce::PopupMenu* submenu = nullptr;
    for (juce::PopupMenu::MenuItemIterator it{ menu }; it.next();)
        if (it.getItem().text == "Show Hidden Columns")
            submenu = it.getItem().subMenu.get();
    REQUIRE(submenu != nullptr);

    const auto hidden = entries(*submenu);
    REQUIRE(hidden.size() == 3);
    CHECK(hidden[0].text == "B");
    CHECK(hidden[1].text == "D");
    CHECK(hidden[2].text == "Show All Hidden Columns");

    hidden[1].action();
    CHECK(grid.getHiddenColumns() == std::vector{ 1 });
    hidden[2].action();
    CHECK(grid.getHiddenColumns().empty());
}

TEST_CASE("The cell menu can sort and hide the cursor's column", "[rows]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    rowGrid(grid);
    grid.setCursor({ .row = 2, .col = 1 });
    const auto cells = entries(grid.createContextMenu(ContextMenuTarget::Kind::cells));
    entry(cells, "Sort Column Descending").action();
    CHECK(grid.getSortIndicator() == Grid::SortIndicator{ .col = 1, .descending = true });
    entry(cells, "Hide Column").action();
    CHECK(grid.isColumnHidden(1));
}
