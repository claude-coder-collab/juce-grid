#include "Examples.h"

namespace
{

class StyledGrid final : public juce::Component
{
public:
    StyledGrid()
    {
        jgrid::GridModel model;
        model.rowCount = [] { return 20; };
        model.colCount = [] { return 4; };
        model.getValue = [](int row, int col) { return juce::var((row + 1) * (col + 1)); };

        model.contextMenu = [](juce::PopupMenu& menu, const jgrid::ContextMenuTarget& target) {
            if (target.kind != jgrid::ContextMenuTarget::Kind::cells)
                return;
            menu.addSeparator();
            menu.addItem("Show Cell Position", [cell = target.cell] {
                juce::Logger::writeToLog("Row " + juce::String(cell.row + 1) + ", column " + juce::String(cell.col + 1));
            });
        };

        grid.setModel(model);
        grid.setColour(jgrid::Grid::backgroundColourId, juce::Colours::black);
        grid.setColour(jgrid::Grid::textColourId, juce::Colours::white);
        grid.setColour(jgrid::Grid::cursorColourId, juce::Colours::yellow);
        grid.setColour(jgrid::Grid::selectionColourId, juce::Colours::yellow.withAlpha(0.35f));
        addAndMakeVisible(grid);
    }

    void resized() override { grid.setBounds(getLocalBounds()); }

private:
    jgrid::Grid grid;
};

} // namespace

std::unique_ptr<juce::Component> createMenusAndColoursExample() { return std::make_unique<StyledGrid>(); }
