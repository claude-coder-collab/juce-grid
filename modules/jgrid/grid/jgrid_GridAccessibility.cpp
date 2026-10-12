#include "jgrid_GridInternals.h"

#include <utility>

namespace jgrid
{

namespace
{

class GridCellInterface final : public juce::AccessibilityCellInterface
{
public:
    explicit GridCellInterface(juce::Component& gridComponent) : grid(gridComponent)
    {
    }

    [[nodiscard]] int getDisclosureLevel() const override { return 0; }
    [[nodiscard]] const juce::AccessibilityHandler* getTableHandler() const override { return grid.getAccessibilityHandler(); }

private:
    juce::Component& grid;
};

void makePlaceholder(juce::Component& component)
{
    component.setInterceptsMouseClicks(false, false);
    component.setFocusContainerType(juce::Component::FocusContainerType::focusContainer);
}

} // namespace

class Grid::Accessibility::CellComponent final : public juce::Component
{
public:
    CellComponent(Grid& owner, Cell cellIn) : grid(owner),
                                              cell(cellIn),
                                              text(*this)
    {
        makePlaceholder(*this);
        addAndMakeVisible(text);
    }

    [[nodiscard]] int getCol() const { return cell.col; }

    [[nodiscard]] juce::Component& getTextComponent() { return text; }

    void resized() override { text.setBounds(getLocalBounds()); }

    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override
    {
        return std::make_unique<Handler>(*this);
    }

private:
    class Text final : public juce::Component
    {
    public:
        explicit Text(CellComponent& ownerIn) : owner(ownerIn) { setInterceptsMouseClicks(false, false); }

        std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override
        {
            return std::make_unique<TextHandler>(*this);
        }

    private:
        class ValueInterface final : public juce::AccessibilityTextValueInterface
        {
        public:
            explicit ValueInterface(Text& textIn) : text(textIn) {}

            [[nodiscard]] bool isReadOnly() const override { return true; }
            [[nodiscard]] juce::String getCurrentValueAsString() const override { return text.owner.grid.getCellText(text.owner.cell); }
            void setValueAsString(const juce::String& /*newValue*/) override {}

        private:
            Text& text;
        };

        class TextHandler final : public juce::AccessibilityHandler
        {
        public:
            explicit TextHandler(Text& textIn) : juce::AccessibilityHandler(textIn,
                                                     juce::AccessibilityRole::staticText,
                                                     juce::AccessibilityActions(),
                                                     Interfaces{ std::make_unique<ValueInterface>(textIn) }),
                                                 text(textIn)
            {
            }

            [[nodiscard]] juce::String getTitle() const override { return text.owner.grid.getCellText(text.owner.cell); }

            [[nodiscard]] juce::AccessibleState getCurrentState() const override
            {
                const auto state = AccessibilityHandler::getCurrentState();
                return getTitle().isEmpty() ? state.withIgnored() : state;
            }

        private:
            Text& text;
        };

        CellComponent& owner;
    };

    class Handler final : public juce::AccessibilityHandler
    {
    public:
        explicit Handler(CellComponent& componentIn) : juce::AccessibilityHandler(componentIn,
                                                           juce::AccessibilityRole::cell,
                                                           juce::AccessibilityActions()
                                                               .addAction(juce::AccessibilityActionType::focus, [&componentIn] { componentIn.grid.setCursor(componentIn.cell); })
                                                               .addAction(juce::AccessibilityActionType::showMenu,
                                                                   [&componentIn] {
                                                                       if (!componentIn.grid.selection.contains(componentIn.cell))
                                                                           componentIn.grid.setCursor(componentIn.cell);
                                                                       componentIn.grid.showContextMenuForKeyboard();
                                                                   })
                                                               .addAction(juce::AccessibilityActionType::press,
                                                                   [&componentIn] {
                                                                       componentIn.grid.setCursor(componentIn.cell);
                                                                       componentIn.grid.startEditing();
                                                                   }),
                                                           Interfaces{ std::make_unique<GridCellInterface>(componentIn.grid) }),
                                                       owner(componentIn)
        {
        }

        [[nodiscard]] juce::AccessibleState getCurrentState() const override
        {
            const auto state = AccessibilityHandler::getCurrentState().withSelectable();
            return owner.grid.cursor == owner.cell || owner.grid.selection.contains(owner.cell) ? state.withSelected() : state;
        }

        [[nodiscard]] juce::String getHelp() const override
        {
            return owner.grid.model.canEdit(owner.cell.row, owner.cell.col) ? "Editable. Press F2 or type to edit." : "Read-only.";
        }

    private:
        CellComponent& owner;
    };

    Grid& grid;
    Cell cell;
    Text text;
};

class Grid::Accessibility::RowComponent final : public juce::Component
{
public:
    RowComponent(Grid& owner, int rowIn) : grid(owner),
                                           row(rowIn)
    {
        makePlaceholder(*this);
        for (auto col = 0; col < grid.cols.getCount(); ++col)
        {
            if (grid.cols.isHidden(col))
                continue;
            cells.push_back(std::make_unique<CellComponent>(grid, Cell{ .row = row, .col = col }));
            addAndMakeVisible(*cells.back());
        }
    }

    [[nodiscard]] int getRow() const { return row; }

    [[nodiscard]] int getNumCells() const { return static_cast<int>(cells.size()); }

    [[nodiscard]] CellComponent* getCell(int col) const
    {
        const auto index = grid.cols.toVisibleIndex(col);
        return index >= 0 && std::cmp_less(index, cells.size()) ? cells[static_cast<std::size_t>(index)].get() : nullptr;
    }

    void updateBounds()
    {
        setBounds(-grid.scroll.x, grid.rows.getStart(row) - grid.scroll.y, grid.cols.getTotalSize(), grid.rows.getSize(row));
        for (const auto& cell : cells)
            cell->setBounds(grid.cols.getStart(cell->getCol()), 0, grid.cols.getSize(cell->getCol()), getHeight());
    }

    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override
    {
        return std::make_unique<Handler>(*this);
    }

private:
    class Handler final : public juce::AccessibilityHandler
    {
    public:
        explicit Handler(RowComponent& componentIn) : juce::AccessibilityHandler(componentIn,
                                                          juce::AccessibilityRole::row,
                                                          juce::AccessibilityActions(),
                                                          Interfaces{ std::make_unique<GridCellInterface>(componentIn.grid) }),
                                                      owner(componentIn)
        {
        }

        [[nodiscard]] juce::String getTitle() const override { return owner.grid.getRowLabel(owner.row); }

        [[nodiscard]] juce::AccessibleState getCurrentState() const override
        {
            const auto state = AccessibilityHandler::getCurrentState().withMultiSelectable();
            return owner.grid.selection.isWholeRowSelected(owner.row, owner.grid.cols.getCount()) ? state.withSelected() : state;
        }

    private:
        RowComponent& owner;
    };

    Grid& grid;
    int row;
    std::vector<std::unique_ptr<CellComponent>> cells;
};

class Grid::Accessibility::HeaderCellComponent final : public juce::Component
{
public:
    HeaderCellComponent(Grid& owner, int colIn) : grid(owner),
                                                  col(colIn)
    {
        makePlaceholder(*this);
    }

    [[nodiscard]] int getCol() const { return col; }

    void updateBounds() { setBounds(grid.cols.getStart(col) - grid.scroll.x, 0, grid.cols.getSize(col), grid.columnHeaderHeight); }

    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override
    {
        return std::make_unique<Handler>(*this);
    }

private:
    class Handler final : public juce::AccessibilityHandler
    {
    public:
        explicit Handler(HeaderCellComponent& componentIn) : juce::AccessibilityHandler(componentIn,
                                                                 juce::AccessibilityRole::tableHeader,
                                                                 juce::AccessibilityActions()
                                                                     .addAction(juce::AccessibilityActionType::press, [&componentIn] { componentIn.grid.toggleSort(componentIn.col); })
                                                                     .addAction(juce::AccessibilityActionType::showMenu,
                                                                         [&componentIn] {
                                                                             if (!componentIn.grid.selection.isWholeColumnSelected(componentIn.col, componentIn.grid.rows.getCount()))
                                                                                 componentIn.grid.selectColumns(componentIn.col, componentIn.col);
                                                                             componentIn.grid.showContextMenuForKeyboard();
                                                                         })),
                                                             owner(componentIn)
        {
        }

        [[nodiscard]] juce::String getTitle() const override { return owner.grid.getColumnAccessibleTitle(owner.col); }

        [[nodiscard]] juce::String getHelp() const override { return owner.grid.canSort() ? "Press to sort by this column" : juce::String(); }

    private:
        HeaderCellComponent& owner;
    };

    Grid& grid;
    int col;
};

class Grid::Accessibility::TableInterface final : public juce::AccessibilityTableInterface
{
public:
    TableInterface(Grid& gridIn, Accessibility& accessibilityIn) : grid(gridIn),
                                                                   accessibility(accessibilityIn)
    {
    }

    [[nodiscard]] int getNumRows() const override
    {
        accessibility.activate();
        return grid.rows.getVisibleCount();
    }

    [[nodiscard]] int getNumColumns() const override
    {
        accessibility.activate();
        return grid.cols.getVisibleCount();
    }

    [[nodiscard]] const juce::AccessibilityHandler* getCellHandler(int row, int column) const override
    {
        accessibility.activate();
        return accessibility.getCellHandler(grid.rows.fromVisibleIndex(row), grid.cols.fromVisibleIndex(column));
    }

    [[nodiscard]] const juce::AccessibilityHandler* getRowHandler(int row) const override
    {
        accessibility.activate();
        return accessibility.getRowHandler(grid.rows.fromVisibleIndex(row));
    }

    [[nodiscard]] const juce::AccessibilityHandler* getHeaderHandler() const override
    {
        accessibility.activate();
        return accessibility.getHeaderHandler();
    }

    [[nodiscard]] juce::Optional<Span> getRowSpan(const juce::AccessibilityHandler& handler) const override
    {
        if (const auto row = accessibility.findRow(handler); row && grid.rows.isVisible(*row))
            return Span{ .begin = grid.rows.toVisibleIndex(*row), .num = 1 };
        return juce::nullopt;
    }

    [[nodiscard]] juce::Optional<Span> getColumnSpan(const juce::AccessibilityHandler& handler) const override
    {
        if (const auto col = accessibility.findCol(handler); col && grid.cols.isVisible(*col))
            return Span{ .begin = grid.cols.toVisibleIndex(*col), .num = 1 };
        return juce::nullopt;
    }

    void showCell(const juce::AccessibilityHandler& handler) const override
    {
        const auto row = accessibility.findRow(handler);
        const auto col = accessibility.findCol(handler);
        if (row && col)
            grid.scrollToCell({ .row = *row, .col = *col });
    }

private:
    Grid& grid;
    Accessibility& accessibility;
};

Grid::Accessibility::Accessibility(Grid& owner) : grid(owner)
{
}

Grid::Accessibility::~Accessibility() = default;

std::unique_ptr<juce::AccessibilityHandler> Grid::Accessibility::createTableHandler()
{
    return std::make_unique<juce::AccessibilityHandler>(grid,
        juce::AccessibilityRole::table,
        juce::AccessibilityActions(),
        juce::AccessibilityHandler::Interfaces{ std::make_unique<TableInterface>(grid, *this) });
}

void Grid::Accessibility::activate()
{
    if (active)
        return;
    active = true;
    rebuild();
}

bool Grid::Accessibility::isActive() const { return active; }

void Grid::Accessibility::refresh()
{
    if (!active)
        return;
    const auto expectedHeaderCells = grid.columnHeaderVisible ? static_cast<std::size_t>(grid.cols.getVisibleCount()) : 0;
    const auto rowsMatch = std::ranges::all_of(rowComponents, [this](const auto& entry) { return entry.second->getNumCells() == grid.cols.getVisibleCount(); });
    if (headerCells.size() != expectedHeaderCells || !rowsMatch)
        rebuild();
    else
        sync();
}

void Grid::Accessibility::rebuild()
{
    if (!active)
        return;
    rowComponents.clear();
    headerCells.clear();
    if (grid.columnHeaderVisible)
    {
        for (auto col = 0; col < grid.cols.getCount(); ++col)
        {
            if (grid.cols.isHidden(col))
                continue;
            headerCells.push_back(std::make_unique<HeaderCellComponent>(grid, col));
            grid.columnHeader->addAndMakeVisible(*headerCells.back());
        }
    }
    sync();
}

IndexRange Grid::Accessibility::getRowsToKeep() const { return grid.getVisibleRows(); }

void Grid::Accessibility::sync()
{
    if (!active)
        return;

    const auto visible = getRowsToKeep();
    const auto cursorRow = grid.cursor ? std::optional<int>(grid.cursor->row) : std::nullopt;
    const auto keep = [&](int row) { return grid.rows.isVisible(row) && (visible.contains(row) || row == cursorRow); };

    std::erase_if(rowComponents, [&](const auto& entry) { return !keep(entry.first); });

    const auto add = [&](int row) {
        if (!keep(row) || rowComponents.contains(row))
            return;
        auto component = std::make_unique<RowComponent>(grid, row);
        grid.body->addAndMakeVisible(*component);
        rowComponents.emplace(row, std::move(component));
    };
    for (auto row = visible.begin; row < visible.end; ++row)
        add(row);
    if (cursorRow)
        add(*cursorRow);

    for (auto& [row, component] : rowComponents)
        component->updateBounds();
    for (auto& headerCell : headerCells)
        headerCell->updateBounds();
}

void Grid::Accessibility::focusCursorCell()
{
    if (!active || !grid.cursor || !grid.hasKeyboardFocus(true))
        return;
    if (auto* handler = getCellHandler(grid.cursor->row, grid.cursor->col))
        handler->grabFocus();
}

void Grid::Accessibility::cellsChanged(const CellsChanged& change)
{
    if (!active)
        return;
    for (auto& [row, component] : rowComponents)
    {
        if (row < change.firstRow || row >= change.firstRow + change.numRows)
            continue;
        for (auto col = change.firstCol; col < change.firstCol + change.numCols; ++col)
            if (auto* cell = component->getCell(col))
                if (const auto* handler = cell->getTextComponent().getAccessibilityHandler())
                {
                    handler->notifyAccessibilityEvent(juce::AccessibilityEvent::titleChanged);
                    handler->notifyAccessibilityEvent(juce::AccessibilityEvent::valueChanged);
                }
    }
}

void Grid::Accessibility::tableStructureChanged()
{
    if (!active)
        return;
    if (const auto* handler = grid.getAccessibilityHandler())
        handler->notifyAccessibilityEvent(juce::AccessibilityEvent::structureChanged);
}

void Grid::Accessibility::rowSelectionChanged(const std::vector<int>& selectedRows)
{
    if (!active || selectedRows == lastSelectedRows)
        return;
    lastSelectedRows = selectedRows;
    if (const auto* handler = grid.getAccessibilityHandler())
        handler->notifyAccessibilityEvent(juce::AccessibilityEvent::rowSelectionChanged);
}

void Grid::Accessibility::headersChanged()
{
    for (auto& headerCell : headerCells)
        if (const auto* handler = headerCell->getAccessibilityHandler())
            handler->notifyAccessibilityEvent(juce::AccessibilityEvent::titleChanged);
}

juce::AccessibilityHandler* Grid::Accessibility::getRowHandler(int row)
{
    const auto it = rowComponents.find(row);
    return it != rowComponents.end() ? it->second->getAccessibilityHandler() : nullptr;
}

juce::AccessibilityHandler* Grid::Accessibility::getCellHandler(int row, int col)
{
    const auto it = rowComponents.find(row);
    if (it == rowComponents.end())
        return nullptr;
    auto* cell = it->second->getCell(col);
    return cell != nullptr ? cell->getAccessibilityHandler() : nullptr;
}

juce::AccessibilityHandler* Grid::Accessibility::getHeaderHandler()
{
    return grid.columnHeaderVisible ? grid.columnHeader->getAccessibilityHandler() : nullptr;
}

std::optional<int> Grid::Accessibility::findRow(const juce::AccessibilityHandler& handler) const
{
    for (const auto* component = &handler.getComponent(); component != nullptr && component != &grid; component = component->getParentComponent())
        if (const auto* row = dynamic_cast<const RowComponent*>(component))
            return row->getRow();
    return std::nullopt;
}

std::optional<int> Grid::Accessibility::findCol(const juce::AccessibilityHandler& handler) const
{
    for (const auto* component = &handler.getComponent(); component != nullptr && component != &grid; component = component->getParentComponent())
    {
        if (const auto* cell = dynamic_cast<const CellComponent*>(component))
            return cell->getCol();
        if (const auto* headerCell = dynamic_cast<const HeaderCellComponent*>(component))
            return headerCell->getCol();
    }
    return std::nullopt;
}

} // namespace jgrid
