#pragma once

#include "jgrid_Navigation.h"
#include "jgrid_Selection.h"

#include <juce_core/juce_core.h>

#include <functional>
#include <vector>

namespace jgrid
{

//==============================================================================
/** Rows of cell texts, as exchanged through the clipboard.

    The functions below are the building blocks of the Grid's copy and paste. The Grid calls them
    for you; they are public so models and tests can use the same clipboard format.
*/
using TextTable = std::vector<std::vector<juce::String>>;

/** Encodes rows as tab-separated text, one line per row. Values containing a tab, newline,
    carriage return or double quote are wrapped in quotes with inner quotes doubled, as Excel does. */
[[nodiscard]] juce::String encodeTsv(const TextTable& table);

/** Decodes tab-separated text, accepting Excel-style quoting and \n, \r\n or \r line ends.
    A single trailing line end is ignored. Empty text decodes to no rows. */
[[nodiscard]] TextTable decodeTsv(const juce::String& text);

/** One value to store in one cell, as planned by planPaste(). */
struct CellWrite
{
    Cell cell; /**< The cell to write. */
    juce::String text; /**< The text to store there. */

    /** Compares two writes. */
    friend bool operator==(const CellWrite&, const CellWrite&) = default;
};

/** Works out which cells a paste writes. A single value pasted into a multi-cell selection fills
    every selected cell. Otherwise the table's top-left lands on the cursor and the table is clipped
    at the grid's last row and column. The table's rows and columns land on successive visible
    rows and columns, so hidden ones are skipped. Cells for which isWritable returns false are skipped. */
[[nodiscard]] std::vector<CellWrite> planPaste(const TextTable& table,
    const Selection& selection,
    Cell cursor,
    const Axis& rows,
    const Axis& cols,
    const std::function<bool(Cell)>& isWritable);

/** Returns the range a copy or cut takes from a selection: the active (last) range. */
[[nodiscard]] CellRange copyRange(const Selection& selection);

/** Returns every cell of the selection once, in row-major order, skipping unwritable cells. */
[[nodiscard]] std::vector<Cell> selectedCells(const Selection& selection, const std::function<bool(Cell)>& isWritable);

} // namespace jgrid
