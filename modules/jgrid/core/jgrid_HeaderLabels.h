#pragma once

#include <juce_core/juce_core.h>

namespace jgrid
{

/** Returns the spreadsheet-style letters for a zero-based column index: A, B, ... Z, AA, AB, ... */
[[nodiscard]] juce::String columnLetters(int col);

/** Returns the default label for a zero-based row index, its one-based position. */
[[nodiscard]] juce::String rowNumber(int row);

/** Returns the screen reader description of a multi-cell selection. */
[[nodiscard]] juce::String describeSelection(int numCells);

} // namespace jgrid
