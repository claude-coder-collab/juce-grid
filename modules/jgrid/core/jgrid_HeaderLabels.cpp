#include "jgrid_HeaderLabels.h"

namespace jgrid
{

juce::String columnLetters(int col)
{
    if (col < 0)
        return {};
    juce::String letters;
    for (auto n = col + 1; n > 0; n = (n - 1) / 26)
        letters = juce::String::charToString(static_cast<juce::juce_wchar>('A' + ((n - 1) % 26))) + letters;
    return letters;
}

juce::String rowNumber(int row) { return juce::String(row + 1); }

juce::String describeSelection(int numCells) { return juce::String(numCells) + " cells selected"; }

} // namespace jgrid
