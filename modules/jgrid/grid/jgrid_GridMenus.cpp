#include "jgrid_GridInternals.h"

#include <memory>

namespace jgrid
{

juce::PopupMenu Grid::createContextMenu(ContextMenuTarget::Kind kind)
{
    juce::PopupMenu menu;
    const juce::Component::SafePointer<Grid> safe{ this };
    const auto clipboardHasText = juce::SystemClipboard::getTextFromClipboard().isNotEmpty();

    switch (kind)
    {
        case ContextMenuTarget::Kind::cells:
            menu.addItem("Cut", model.setValue != nullptr, false, [safe] {
                if (safe != nullptr)
                    safe->cut();
            });
            menu.addItem("Copy", [safe] {
                if (safe != nullptr)
                    safe->copy();
            });
            menu.addItem("Paste", model.setValue != nullptr && clipboardHasText, false, [safe] {
                if (safe != nullptr)
                    safe->paste();
            });
            if (cursor)
            {
                const auto col = cursor->col;
                menu.addSeparator();
                menu.addItem("Sort Column Ascending", canSort(), false, [safe, col] {
                    if (safe != nullptr)
                        safe->sortByColumn(col, false);
                });
                menu.addItem("Sort Column Descending", canSort(), false, [safe, col] {
                    if (safe != nullptr)
                        safe->sortByColumn(col, true);
                });
                menu.addItem("Hide Column", [safe, col] {
                    if (safe != nullptr)
                        safe->setColumnsHidden({ col }, true);
                });
            }
            break;

        case ContextMenuTarget::Kind::rows:
        {
            const auto selected = getSelectedRows();
            auto below = selected.empty() ? -1 : selected.back();
            if (selected.empty() && cursor)
                below = cursor->row;
            const auto& mark = currentCutMark();
            const auto cutRowsPending = mark && !mark->rows.empty() && mark->source != nullptr;
            menu.addItem("Cut Rows", model.removeRows != nullptr && !selected.empty(), false, [safe] {
                if (safe != nullptr)
                    safe->cutRows();
            });
            menu.addItem("Copy Rows", !selected.empty(), false, [safe] {
                if (safe != nullptr)
                    safe->copyRows();
            });
            menu.addItem("Duplicate Rows", (model.duplicateRows != nullptr || model.insertRows != nullptr) && !selected.empty(), false, [safe] {
                if (safe != nullptr)
                    safe->duplicateSelectedRows();
            });
            menu.addItem(cutRowsPending ? "Insert Cut Rows Below" : "Insert Copied Rows Below", model.insertRows != nullptr && clipboardHasText, false, [safe, below] {
                if (safe != nullptr)
                    safe->insertCopiedRows(below);
            });
            menu.addItem("Insert Blank Rows Below", model.insertRows != nullptr, false, [safe] {
                if (safe != nullptr)
                    safe->insertBlankRows();
            });
            menu.addItem("Delete Rows", model.removeRows != nullptr && !selected.empty(), false, [safe] {
                if (safe != nullptr)
                    safe->deleteSelectedRows();
            });
            menu.addSeparator();
            const auto hidden = std::make_shared<const std::vector<int>>(selected);
            menu.addItem("Hide Rows", !selected.empty(), false, [safe, hidden] {
                if (safe != nullptr)
                    safe->setRowsHidden(*hidden, true);
            });
            menu.addItem("Show Hidden Rows", rows.hasHidden(), false, [safe] {
                if (safe != nullptr)
                    safe->showAllRows();
            });
            break;
        }

        case ContextMenuTarget::Kind::columns:
        {
            const auto col = cursor ? cursor->col : -1;
            menu.addItem("Sort Ascending", canSort() && col >= 0, false, [safe, col] {
                if (safe != nullptr)
                    safe->sortByColumn(col, false);
            });
            menu.addItem("Sort Descending", canSort() && col >= 0, false, [safe, col] {
                if (safe != nullptr)
                    safe->sortByColumn(col, true);
            });
            menu.addSeparator();
            const auto selected = std::make_shared<const std::vector<int>>(getSelectedColumns());
            menu.addItem("Hide Columns", !selected->empty(), false, [safe, selected] {
                if (safe != nullptr)
                    safe->setColumnsHidden(*selected, true);
            });
            juce::PopupMenu hiddenColumns;
            for (const auto hiddenCol : cols.getHidden())
            {
                hiddenColumns.addItem(getColumnLabel(hiddenCol), [safe, hiddenCol] {
                    if (safe != nullptr)
                        safe->setColumnsHidden({ hiddenCol }, false);
                });
            }
            hiddenColumns.addSeparator();
            hiddenColumns.addItem("Show All Hidden Columns", [safe] {
                if (safe != nullptr)
                    safe->showAllColumns();
            });
            menu.addSubMenu("Show Hidden Columns", hiddenColumns, cols.hasHidden());
            break;
        }
    }

    if (model.contextMenu)
        model.contextMenu(menu, ContextMenuTarget{ .kind = kind, .cell = cursor.value_or(Cell{}), .selection = selection });
    return menu;
}

void Grid::showContextMenu(ContextMenuTarget::Kind kind, juce::Rectangle<int> screenArea)
{
    if (editor && !commitEdit())
        return;
    createContextMenu(kind).showMenuAsync(getContextMenuOptions(screenArea));
}

juce::PopupMenu::Options Grid::getContextMenuOptions(juce::Rectangle<int> screenArea)
{
    return juce::PopupMenu::Options().withTargetComponent(this).withTargetScreenArea(screenArea);
}

void Grid::showContextMenuForKeyboard()
{
    if (!cursor)
        return;
    auto kind = ContextMenuTarget::Kind::cells;
    if (!getSelectedRows().empty())
        kind = ContextMenuTarget::Kind::rows;
    else if (!getSelectedColumns().empty())
        kind = ContextMenuTarget::Kind::columns;
    showContextMenu(kind, localAreaToGlobal(getCellBounds(*cursor)));
}

} // namespace jgrid
