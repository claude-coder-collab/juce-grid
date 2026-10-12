#include <jgrid/jgrid.h>

int main()
{
    const juce::ScopedJuceInitialiser_GUI juce;
    jgrid::GridModel model;
    model.rowCount = [] { return 2; };
    model.colCount = [] { return 2; };
    model.getValue = [](int row, int col) { return juce::var(row * 10 + col); };
    jgrid::Grid grid;
    grid.setModel(model);
    return grid.getRowCount() == 2 && grid.getColCount() == 2 ? 0 : 1;
}
