#include "GridTestSupport.h"

#include <catch2/catch_test_macros.hpp>

using jgrid::Cell;
using jgrid::Grid;
using jgrid::test::EditableTable;
using jgrid::test::key;
using jgrid::test::makeEditableModel;

namespace
{

std::shared_ptr<EditableTable> editableGrid(Grid& grid, int rows = 5, int cols = 3)
{
    auto table = std::make_shared<EditableTable>(rows, cols);
    grid.setModel(makeEditableModel(table));
    grid.setSize(600, 300);
    return table;
}

bool pressInEditor(Grid& grid, const juce::KeyPress& press)
{
    auto* editor = grid.getEditor();
    REQUIRE(editor != nullptr);
    return editor->keyPressed(press);
}

} // namespace

TEST_CASE("F2 opens the editor with the content selected", "[editing]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    editableGrid(grid);

    CHECK(grid.keyPressed(key(juce::KeyPress::F2Key)));
    REQUIRE(grid.isEditing());
    CHECK(grid.getEditingCell() == Cell{});
    CHECK(grid.getEditor()->getText() == "r0c0");
    CHECK(grid.getEditor()->getHighlightedRegion() == juce::Range<int>(0, 4));
}

TEST_CASE("Typing a character opens the editor replacing the content", "[editing]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    editableGrid(grid);

    CHECK(grid.isTextInputActive());
    CHECK(grid.keyPressed(juce::KeyPress('x', {}, 'x')));
    REQUIRE(grid.isEditing());
    CHECK(grid.getEditor()->getText() == "x");
}

TEST_CASE("Text input opens the editor and forwards composition", "[editing]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    editableGrid(grid);

    grid.setHighlightedRegion({ 0, 0 });
    grid.insertTextAtCaret(juce::String::fromUTF8("\xe3\x81\xab"));
    REQUIRE(grid.isEditing());
    grid.setHighlightedRegion({ 0, 1 });
    grid.insertTextAtCaret(juce::String::fromUTF8("\xe6\x97\xa5\xe6\x9c\xac"));
    CHECK(grid.getEditor()->getText() == juce::String::fromUTF8("\xe6\x97\xa5\xe6\x9c\xac"));
    CHECK(grid.getTotalNumChars() == 2);
}

TEST_CASE("The macOS F2 character opens the editor with the content selected", "[editing]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    editableGrid(grid);

    grid.insertTextAtCaret(juce::String::charToString(jgrid::macF2Character));
    REQUIRE(grid.isEditing());
    CHECK(grid.getEditor()->getText() == "r0c0");
}

TEST_CASE("Enter commits and moves down, Shift+Enter commits and moves up", "[editing]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    const auto table = editableGrid(grid);

    grid.setCursor({ .row = 2, .col = 1 });
    REQUIRE(grid.startEditingWithText("new"));
    CHECK(pressInEditor(grid, key(juce::KeyPress::returnKey)));
    CHECK_FALSE(grid.isEditing());
    CHECK(table->at(2, 1) == "new");
    CHECK(grid.getCursor() == Cell{ .row = 3, .col = 1 });

    REQUIRE(grid.startEditingWithText("up"));
    CHECK(pressInEditor(grid, key(juce::KeyPress::returnKey, juce::ModifierKeys::shiftModifier)));
    CHECK(table->at(3, 1) == "up");
    CHECK(grid.getCursor() == Cell{ .row = 2, .col = 1 });
}

TEST_CASE("Tab commits and moves to the next cell", "[editing]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    const auto table = editableGrid(grid);

    REQUIRE(grid.startEditingWithText("a"));
    CHECK(pressInEditor(grid, key(juce::KeyPress::tabKey)));
    CHECK(table->at(0, 0) == "a");
    CHECK(grid.getCursor() == Cell{ .row = 0, .col = 1 });
}

TEST_CASE("Escape cancels without storing", "[editing]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    const auto table = editableGrid(grid);

    REQUIRE(grid.startEditingWithText("discard"));
    CHECK(pressInEditor(grid, key(juce::KeyPress::escapeKey)));
    CHECK_FALSE(grid.isEditing());
    CHECK(table->at(0, 0) == "r0c0");
    CHECK(table->sets == 0);
    CHECK(grid.getCursor() == Cell{});
}

TEST_CASE("Committing unchanged text does not call the model", "[editing]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    const auto table = editableGrid(grid);

    REQUIRE(grid.startEditing());
    CHECK(grid.commitEdit());
    CHECK(table->sets == 0);
}

TEST_CASE("Validation failure keeps the editor open and shows the error", "[editing]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    const auto table = editableGrid(grid);
    table->validate = [](int, int, const juce::var& value) {
        return value.toString().containsOnly("0123456789") ? juce::Result::ok() : juce::Result::fail("Enter a whole number");
    };

    REQUIRE(grid.startEditingWithText("abc"));
    CHECK(pressInEditor(grid, key(juce::KeyPress::returnKey)));
    CHECK(grid.isEditing());
    CHECK(grid.getErrorMessage() == "Enter a whole number");
    CHECK(table->sets == 0);
    CHECK(grid.getCursor() == Cell{});

    pressInEditor(grid, juce::KeyPress('1', {}, '1'));
    CHECK(grid.getErrorMessage().isEmpty());

    CHECK_FALSE(grid.commitEdit());
    CHECK(grid.getErrorMessage().isNotEmpty());
    grid.cancelEdit();
    CHECK(grid.getErrorMessage().isEmpty());
}

TEST_CASE("Text changes from input methods dismiss the error", "[editing]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    const auto table = editableGrid(grid);
    table->validate = [](int, int, const juce::var&) { return juce::Result::fail("No"); };

    REQUIRE(grid.startEditingWithText("a"));
    CHECK_FALSE(grid.commitEdit());
    REQUIRE(grid.getErrorMessage() == "No");
    grid.getEditor()->insertTextAtCaret("b");
    CHECK(grid.getErrorMessage().isEmpty());
}

TEST_CASE("A rejected setValue keeps the editor open", "[editing]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    const auto table = editableGrid(grid);
    table->acceptSets = false;

    REQUIRE(grid.startEditingWithText("a"));
    CHECK_FALSE(grid.commitEdit());
    CHECK(grid.isEditing());
    CHECK(grid.getErrorMessage() == jgrid::couldNotSetValueMessage());
}

TEST_CASE("Read-only cells do not open an editor", "[editing]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    const auto table = editableGrid(grid);
    table->isEditable = [](int, int col) { return col != 0; };

    CHECK_FALSE(grid.isTextInputActive());
    CHECK_FALSE(grid.startEditing());
    CHECK(grid.keyPressed(juce::KeyPress('x', {}, 'x')));
    CHECK_FALSE(grid.isEditing());

    grid.setCursor({ .row = 0, .col = 1 });
    CHECK(grid.startEditing());
}

TEST_CASE("A model without setValue is read-only", "[editing]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    auto model = makeEditableModel(std::make_shared<EditableTable>(2, 2));
    model.setValue = nullptr;
    grid.setModel(model);
    CHECK_FALSE(grid.startEditing());
    CHECK_FALSE(grid.startEditingWithText("a"));
}

TEST_CASE("The editor has its own undo and redo", "[editing]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    editableGrid(grid);

    REQUIRE(grid.startEditing());
    auto* editor = grid.getEditor();
    editor->insertTextAtCaret("changed");
    CHECK(editor->getText() == "changed");
    CHECK(editor->keyPressed(juce::KeyPress('z', juce::ModifierKeys::commandModifier, 0)));
    CHECK(editor->getText() == "r0c0");
    CHECK(editor->keyPressed(juce::KeyPress('z', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier, 0)));
    CHECK(editor->getText() == "changed");
}

TEST_CASE("A dead key composes with the next key as the first input", "[editing]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    editableGrid(grid);
    const auto acute = juce::String::fromUTF8("\xc2\xb4");
    const auto eAcute = juce::String::fromUTF8("\xc3\xa9");

    grid.setHighlightedRegion(grid.getHighlightedRegion());
    grid.insertTextAtCaret(acute);
    grid.setTemporaryUnderlining({ juce::Range<int>(0, 1) });
    grid.setHighlightedRegion({ 1, 1 });
    REQUIRE(grid.isEditing());
    CHECK(grid.isTextInputActive());

    grid.setHighlightedRegion({ 0, 1 });
    grid.insertTextAtCaret(eAcute);
    grid.setTemporaryUnderlining({});
    CHECK(grid.getEditor()->getText() == eAcute);
}

TEST_CASE("Keys reaching the Grid while its editor is open go to the editor", "[editing]")
{
    const juce::ScopedJuceInitialiser_GUI juce;
    Grid grid;
    const auto table = editableGrid(grid);

    grid.insertTextAtCaret("z");
    REQUIRE(grid.isEditing());
    CHECK(grid.keyPressed(key(juce::KeyPress::returnKey)));
    CHECK_FALSE(grid.isEditing());
    CHECK(table->at(0, 0) == "z");
    CHECK(grid.getCursor() == Cell{ .row = 1, .col = 0 });
}
