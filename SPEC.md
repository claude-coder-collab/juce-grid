# JUCE Grid — Specification

Inspired by wxPython Grid, this project reinvents the JUCE TableListBox as the Grid.

The Grid is open source under the MIT licence, at https://github.com/claude-coder-collab/juce-grid. Anything built with it also uses JUCE and so falls under JUCE's own licence (AGPLv3 or a commercial JUCE licence).

The Grid targets JUCE 8.

## Platforms

- The Grid is cross-platform: Windows, macOS and Linux. Platform-specific behaviour goes through JUCE; the Grid itself has no platform code apart from conditional workarounds that are documented where they occur.
- Development happens on macOS (arm64). GitHub Actions (`.github/workflows/ci.yml`, on pushes to main and on pull requests, never on a schedule) builds and tests the `ci` preset on Linux (Ubuntu 24.04, GCC 14, tests under `xvfb-run`), Windows (MSVC, through ilammy/msvc-dev-cmd) and macOS (Clang), all with Ninja. Sanitizers and clang-tidy run locally only, through `tools/check.py`.
- Compiler flags: GCC/Clang get the strict warning set; MSVC gets `/W4 /permissive- /utf-8 /Zc:__cplusplus`. Warnings are errors in the `ci` and `tidy` presets. Sanitizers apply to GCC/Clang only. The macOS-only settings (architecture, deployment target) are guarded by `CMAKE_HOST_APPLE`.
- Accessibility has only been tested with VoiceOver. Windows (Narrator, NVDA, JAWS through UI Automation) and Linux need their own testing; the roles and structure are chosen for cross-platform consistency, following JUCE's own table components.

## Original brief

It should have keyboard shortcuts built in:
Up, down, page up, page down, left, right should change selection. F2 should open the cell's editor with the content selected. Typing any character should open the cell's editor ready to replace the content.

The cell editors should have undo/redo within them.

Clicking on the headers should sort by that column. Perhaps sorting the Grid should not sort the data which backs the grid. Perhaps the Grid maintain indices which are used to look up rows from the model and just the indices are sorted. Let's discuss.

The Grid should contain an API that let's the user link it to a model of their choice. The model is expected to present its data in rows and columns.

Undo and redo callbacks are also passed to the model. We'll need to figure out how and when the Grid gets refreshed. Should there be a dirty flag? Should the model be responsible for having the Grid ask for up-to-date data?

How will the model work with the c++ type system? Should it be a concept or a subclass? Or maybe just register lambda callbacks with the Grid (type erasure).

The Grid also offers data validation callbacks to the model.

## Model communication

Communication is a small fixed set of typed operations, not a generic row/column/message callback.

Row order:
- The model owns the row order, so it can be serialized and, if the model chooses, undone. The Grid holds no view-to-model mapping.
- The model presents rows in view order. Every row index in callbacks and Change notifications is a view-order index. How the model maps that to its storage is private to the model.
- The model's heavy data is never reordered. The model keeps a lightweight order index (a permutation) over its storage, and reorderRows changes only that index.
- The library provides a reusable `RowOrder` helper that holds the permutation, maps view index to storage index, applies reorderRows, and adjusts itself on insert and remove. A model typically needs one RowOrder member and one lookup per callback.

Values:
- Cell values are `juce::var`, chosen so that checkbox and drop-down editors on the roadmap need no API change.
- Text editing and clipboard paste always hand the model a string var. The model's validate and setValue are responsible for converting it (e.g. "3" to an int) or rejecting it. The Grid never infers types.
- Display, copy and autofit use `var::toString()`.

Grid pulls from the model:
- rowCount(), colCount()
- getValue(row, col)
- setValue(row, col, value), returning success or failure (`bool`)
- validate(row, col, value), returning ok or an error message (`juce::Result`)
- undo(), redo()
- colName(col), optional. If absent, column headers show spreadsheet-style letters: A, B, … Z, AA, AB, …
- rowName(row), optional. If absent, row headers show the view position + 1, so rows renumber after a sort, as in a spreadsheet. A model wanting stable labels returns its storage index + 1.
- reorderRows(permutation), optional. Required for sorting.
- isEditable(row, col), optional. If absent, every cell is editable.
- beginBatch(), endBatch(), optional. The Grid wraps every multi-cell or multi-row operation in them: paste, delete, cut-then-paste, duplicate and sort. The model may use them to group undo steps or to coalesce notifications into one changed(Change). If absent, nothing changes. Batches never nest and endBatch is always called, even if an operation fails part way.

Read-only:
- If setValue is absent, the whole Grid is read-only: editors never open, and paste, delete and cut are disabled, with their menu items greyed out.
- Attempting to edit a read-only cell (F2, typing, double-click) does nothing except post a screen reader announcement that the cell is read-only.
- Paste, delete and cut skip read-only cells.

Optional row operations, which grey out the matching menu items when absent:
- insertRows(at, rowData). "Insert rows here" inserts directly after the clicked row in view order, whether or not the Grid was sorted. Nothing re-sorts and nothing else moves.
- removeRows(rows), used by cut
- duplicateRows(rows), which the Grid can otherwise build from getValue and insertRows

Model pushes to the Grid:
- A single notification, changed(Change), where Change is a variant: cell range updated, rows inserted, rows removed, rows reordered (with the permutation), or full reset.
- There is no dirty flag. On a notification the Grid repaints and re-queries only the visible cells.
- The Grid uses inserted, removed and reordered notifications to carry its row-keyed view state along: selection, hidden rows, row sizes and cut marks. A reset clears that state, cancels any open editor, and keeps the cursor clamped to the new shape.

Binding:
- The model is bound as a struct of std::function members (type erasure).
- No inheritance is forced on the user's model, and Grid does not become a template, which keeps it simple to use as a JUCE component.
- A concept was considered and rejected: its dispatch savings are negligible next to painting cost.
- The Grid binds with `grid.setModel(GridModel)`. The GridModel struct carries a pointer to a `GridModelNotifier` owned by the model. Each Grid subscribes in setModel and unsubscribes in setModel({}) or its destructor.
- The model calls `notifier.changed(Change)` once and every subscribed Grid receives it.
- Lifetime is safe in both directions: if the notifier is destroyed first it detaches every Grid, which then shows empty and stops calling the model. The Grid never owns the model.

## Multiple Grids

Several Grids may be attached to one dataset in v1, either as identical views or showing different columns.

Structure:
- **Data source**: one per dataset, written by the user. It owns the heavy data, speaks only in storage indices, and owns a notifier that broadcasts storage-level changes.
- **View model**: one per Grid, bound to it as its GridModel. It owns that Grid's RowOrder and an optional column mapping (a subset and/or arrangement of the data source's columns). It translates view indices to storage indices for calls, and storage indices to view indices for notifications.
- The library provides a reusable view model helper, so users write only the data source. RowOrder keeps an inverse map so notifications translate cheaply.
- The Grid itself is unaware of any of this: it sees one GridModel in view indices.

Behaviour:
- Each view has its own row order. Sorting one Grid does not reorder another.
- "Different data per Grid" means different columns only. Row filtering is not supported.
- Rows inserted through one view appear where the user clicked in that view, and at the end of every other view. Removed rows disappear from every view.
- The app may serialize each view's order alongside the data.
- Per-Grid state: selection, hidden rows and columns, row and column sizes, scroll position, open editor, sort indicator.
- Cut marks are process-wide, so cut in one Grid and paste in another works, removing or clearing the source on paste. Across two different data sources this is two separate model operations.
- If one Grid has an editor open on a cell another Grid changes, the editor stays open and the last commit wins.

## Selection

Cursor and selection (Excel model):
- The Grid has a single cursor cell, always present and drawn with a heavy outline, and a selection made of one or more ranges.
- F2, typing and the editor always act on the cursor cell. Paste places its top-left at the cursor, unless a single value is filling a range selection.
- Plain movement keys (arrows, Page Up/Down, Home/End, Ctrl+arrows) move the cursor and collapse the selection to the cursor cell.
- Shift + a movement key keeps the cursor and the active range's anchor fixed and moves the range's far corner.
- Enter, Shift+Enter, Tab and Shift+Tab inside a multi-cell selection move the cursor through the selected cells, wrapping, without changing the selection.
- Click sets the cursor and collapses the selection to that cell. Shift-click extends the active range from its anchor. Ctrl/Cmd-click adds a new range and moves the cursor to it.
- Accessibility focus follows the cursor. The selection is announced separately.

- Ranges and multi-select are supported.
- Clicking a row or column header selects the whole row or column.
- Row and column headers can each be shown or hidden as an option.

## Keyboard

On macOS, Ctrl means Cmd.

| Key | Grid | Open editor |
|---|---|---|
| Arrows, Page Up/Down | Move cursor | Move caret (Page keys do nothing) |
| Shift + those | Extend range | Select text |
| Ctrl + arrows | Move cursor to the first/last row or column | Word jump |
| Home / End | First / last column in row | Line start / end |
| Ctrl + Home / End | First / last cell | Text start / end |
| Tab / Shift+Tab | Move right / left, wrapping rows; leave the Grid past the last / first cell | Commit, then move right / left |
| Ctrl+Tab / Ctrl+Shift+Tab | Move focus out of the Grid to the next / previous control | Commit, then move focus out |
| Enter / Shift+Enter | Move down / up, stopping at the last / first row | Commit, then move down / up |
| Escape | Clear cut mark | Cancel edit |
| F2 | Open editor with content selected | — |
| Printable character | Open editor, replacing content | Typed normally |
| Delete / Backspace | Clear selected cells | Normal text deletion |
| Ctrl + A | Select all | Select all text |
| Ctrl + C / X / V | Copy / cut / paste | Text clipboard |
| Ctrl + Z / Shift+Ctrl+Z or Ctrl+Y | Forward undo / redo to model | Editor's own undo / redo |
| Ctrl + − | Delete the selected rows, or the cursor's row (removeRows) | — |
| Ctrl + = (or Ctrl + +) | Insert blank rows below: as many as are selected, or one below the cursor's row (insertRows) | — |

- Type-to-edit must not lose the first keystroke, including dead keys and IME composition: the editor opens and receives that key itself. Verify on macOS early.
- Hidden rows and columns are skipped by all cursor movement.
- Tab moves between cells as in a spreadsheet, so the Grid must never trap keyboard focus: Ctrl+Tab always leaves it (the macOS and Windows convention for controls that use Tab themselves; Ctrl is the actual Ctrl key on macOS, since Cmd+Tab is the app switcher), and Tab past the last cell or Shift+Tab before the first cell also leaves it. The Grid's accessibility help text mentions Ctrl+Tab.
- In a plugin host, the host may intercept keys before the Grid sees them. This is a known limitation, not handled by the Grid.

## Clipboard

- Copy, paste and delete are built in.
- Clipboard format is tab-separated text: rows separated by newlines, values from `toString()`. Values containing a tab, newline or double quote are quoted Excel-style (wrapped in quotes, inner quotes doubled). This interoperates with Excel, Numbers and Google Sheets.
- Paste parses the same format, including from other apps, producing string vars.
- The cut mark is internal: it records the source Grid and cells. Pasting the text into another app never clears the source. The mark is dropped if the system clipboard content changes.
- Copy rows and insert copied rows use the same text format. Insert parses the clipboard into rowData for insertRows; rows with too few columns are padded with empty values and extra columns are dropped.
- Paste and delete go through the same setValue and validate path as editing.
- Paste validates every target cell before any setValue. If any cell fails, nothing is written: the selection moves to the first failing cell and the error popup shows there with the model's message, plus "and N more" if several failed. The screen reader announcement says the same.
- If setValue fails after validation passed, the Grid stops at that cell. Cells already written stay written and endBatch is still called, so a model wanting all-or-nothing can roll back there. The popup shows "Could not set value" at that cell.
- A paste larger than the space available is clipped at the last row and column. Paste never adds rows.
- A single copied value pasted into a range selection fills the whole range.
- Cut does not remove anything immediately. The cut cells or rows are removed or cleared only when pasted, so a cut that is never pasted loses nothing.
- The cut mark is cleared by any changed(Change) notification that touches the marked cells or rows, and a later paste then behaves as a plain copy.

## Editing

- Text editors only to start. Checkboxes and other editor types are on the roadmap.
- Each cell editor has its own undo/redo stack, which is discarded once the edit is committed.
- If validation fails, the editor stays open. A brief shake animation signals the failure, and a popup/tooltip shows the error message returned by the model.
- The popup never requires a click to dismiss and does not take focus. It disappears on the next keystroke, on focus change, on commit or cancel, or after a timeout.
- Committing text identical to the cell's original display text closes the editor without calling validate or setValue.
- When the editor loses focus it stays open with its text, so nothing typed is lost. Clicking another cell in the same Grid commits first; if the commit fails, the click is ignored and the editor keeps focus. Returning focus to the Grid returns it to the open editor.
- Double-click opens the editor with the caret at the clicked position.
- Rows inserted, removed or reordered while editing carry the editor along; removing the edited row, or a reset, cancels the edit.

## Undo and redo

- Grid-level undo/redo is forwarded to the model. Undo/redo implementation is out of scope for the Grid: the model decides what the requests do, which may be nothing. The Grid prescribes no undo granularity or scope, including across multiple views.
- The undo and redo callbacks are optional. A model that does not implement them gets no Grid-level undo.

## Sorting

- Sorting is a one-shot action applied to the model. When the user sorts, the Grid computes the sorted permutation and calls reorderRows(permutation). The model's heavy data is never reordered (see Row order).
- Clicking a column header selects the column. Sorting is triggered by a sort button at the right end of each column header: the sorted column always shows its direction arrow, and other columns show faint up/down arrows on hover. Each column header cell also has a screen reader press action that sorts, and its accessible title includes ", sorted ascending" or ", sorted descending". Sort Ascending / Descending will also be in the column header context menu (milestone 4).
- If reorderRows is absent, sorting is disabled: no sort buttons are shown and a header click only selects the column.
- The header shows a "last sorted by" indicator. It is cleared by any reordered, inserted, removed or reset notification not caused by the Grid's own sort, and by edits to a sorted column.
- There is no automatic re-sort. Edits, inserts and pastes never move rows. The user clicks the header again to re-sort.
- Undo and redo of a sort are the model's, like any other change.
- The Grid asks the model to compare two cells in the same column: compare(col, rowA, rowB) returns `std::weak_ordering` (a strong ordering converts to it). Where unordered values such as NaN go is the model's choice.
- The compare callback is optional. If absent, the Grid falls back to comparing getValue results: numerically when both values are numeric, otherwise by natural string order of `toString()` (so "9" sorts before "10"). Void values sort first.
- Descending order is handled by the Grid, not the model.
- Sorting is stable, so equal rows keep their current relative order.
- The sort button sorts ascending; pressing it again on the same column sorts descending. There is no unsorted state, since a sort permanently reorders the view; undo, if the model supports it, is the way back.
- There is no multi-column sort. Because sorting is stable and the order persists, sorting by a secondary column then a primary column gives the combined order, as in Excel. The indicator shows only the last sorted column.

## Layout

- Columns and rows are resizable.
- Double-clicking a row or column edge autofits it to its content.
- Rows and columns can be hidden and shown.
- Right-clicking a row or column header offers to hide it, and to show hidden rows or columns.
- Reordering columns is on the roadmap.
- Each row and column can be resized individually by dragging its header edge. Variable row heights supplied by the model (for example, wrapped text) are on the roadmap.
- Frozen rows and columns are not supported.
- Hidden state is view state held by the Grid, not the model.
- Copy, paste and delete over a range selection affect only visible cells, so hidden rows and columns are skipped.

## Mouse interaction

- Dragging across cells selects a range.
- Double-clicking a cell opens its editor.
- Right-clicking inside the current selection keeps it. Right-clicking outside it moves the selection to the clicked cell.

### Context menus

Cell or cell range:
- Cut contents
- Copy contents
- Paste contents
- Sort Column Ascending / Descending and Hide Column, for the cursor's column, so these are available without moving to the column header
- Hook for a model callback to add or change items

Row or full row selection:
- Cut rows
- Copy rows
- Duplicate rows
- Insert previously cut or copied rows
- Hook for a model callback to add or change items

Column or column selection:
- Sort ascending / descending
- Hide columns
- Show Hidden Columns: a submenu listing each hidden column by its header label, to show it alone, then Show All Hidden Columns
- Hook for a model callback to add or change items

Row menus also offer Insert Blank Rows Below and Delete Rows, the same operations as Ctrl+= and Ctrl+−.

## Look and feel

- The Grid inherits its look and feel from its parent component.
- It offers colour scheme customization points, such as background, text, header, selection and grid line colours.
- It does not implement LookAndFeel methods for heavy drawing customization.

## Accessibility

- Screen readers are supported through JUCE's AccessibilityHandler with a table interface, not custom platform code.
- Accessible components are created lazily: nothing exists until a screen reader first queries the table. From then on the Grid keeps invisible row components (each with one cell child per column) for the visible rows and the cursor row only, and the column header holds one invisible child per column. Offscreen rows report no handler, as JUCE's own TableListBox does. See Implementation for the details JUCE requires.
- Hidden rows and columns are skipped, and rows are exposed in view order.
- The editor takes accessibility focus when it opens and returns it to the cell on commit or cancel.
- Range selection may need custom announcement text, since the table interface maps best to single-cell selection.
- Validation errors never require a click to dismiss. The error popup does not take focus, and it disappears on the next keystroke, on focus change, on commit or cancel, or after a timeout.
- Validation errors are also announced to screen readers via AccessibilityHandler::postAnnouncement. The editor keeps focus throughout.
- Keyboard operation is covered by the built-in shortcuts, and high-contrast support comes from the colour customization points.

## Milestones

1. Display and navigation
   - Model binding as a struct of std::function members, with a read-only getValue path.
   - Row and column headers, optional.
   - Colour customization points, inheriting the parent's look and feel.
   - Single-cell selection and keyboard navigation: up, down, left, right, page up, page down.
   - Accessibility prototype: lazy cell handlers, verified with VoiceOver (navigate cells, hear values and headers, cursor tracking).
   - Demo app.
2. Editing
   - F2, double-click and type-to-edit, with in-editor undo/redo.
   - setValue and validate, with the shake, error popup, announcement and dismissal rules.
   - changed(Change) notifications through GridModelNotifier, and repainting, with several Grids subscribed.
3. Selection, clipboard and sorting
   - Range and multi-select, drag-select, and row and column selection by header click.
   - Copy, cut, paste and delete.
   - Header-click sorting through compare and reorderRows, and the RowOrder helper.
   - The view model helper for multiple Grids over one data source, with column mapping.
4. Layout and menus
   - Resizing, autofit, and hiding and showing rows and columns.
   - Context menus with model callback hooks.
   - Optional row operations: insertRows, removeRows, duplicateRows.
   - Grid-level undo and redo forwarded to the model.
5. Accessibility
   - AccessibilityHandler with lazy cell handlers.
   - The accessibility mapping of cells to rows and columns, including lazy cell components, is prototyped and verified with VoiceOver during milestone 1 so it is not a retrofit.

Roadmap after the milestones: checkbox and other editor types, column reordering, variable row heights, separate display and edit values (e.g. formatted numbers).

## Architecture and testing

- Logic lives in plain classes with no GUI dependency, each unit-tested with Catch2: RowOrder, the view model helper, selection and cursor, clipboard TSV encode/parse, sort permutation, and paste planning (target cells, clipping, fill).
- The Grid component is a thin layer over these classes.
- A few headless JUCE tests (ScopedJuceInitialiser_GUI) cover key handling.
- A demo app is used for manual and VoiceOver testing.

## Performance

- The Grid is not virtualized. Performance on very large models is not a goal, and slow huge grids are acceptable.
- Autofit measures all rows, not just the visible ones.

## Threading

- The model is responsible for making all updates, including changed(Change) notifications, on the message thread.

## Implementation

This section records how the spec is built, so another agent can continue the work.

### Layout and build
- `modules/jgrid/` is a JUCE module (`jgrid.h` with the module declaration, `jgrid.cpp` including every source), so users can add it with `juce_add_module`. Each `.cpp` also compiles on its own, which is how this project's CMake builds it.
  - `core/`: logic with no GUI dependency: `Axis` (sizes and positions along one dimension, hit-testing, visible ranges), `Navigation` (`Cell`, `Move`, `moveCursor`, `stepCell`, `clampCursor`), `HeaderLabels` (`columnLetters`, `rowNumber`), `GridModel` (the callback struct, with `canEdit`), `Change` (the notification variant, `remapRow`, `rowAfterRemoval`), `GridModelNotifier` (a `juce::ListenerList` fan-out), `Editing` (`commitValue`, `isTypedText`, `shakeOffset`).
  - `grid/`: `Grid` (the component), `GridInternals.h` (private: Body, ColumnHeader, RowHeader, CellEditor, ErrorPopup, Shaker, Accessibility), `GridAccessibility.cpp`, `GridEditing.cpp` (editor, popup, shake, text input, notifications).
- Namespace `jgrid`. Project code is C++26.
- CMake with Ninja Multi-Config. FetchContent pins JUCE 8.0.15 and Catch2 v3.16.1, downloaded once into `build/_deps`.
- JUCE is compiled into its own static library `jgrid_juce` (C++20, JUCE's recommended flags), with its include directories exported as SYSTEM. Strict warnings and clang-tidy therefore apply only to project code. `jgrid_static` (alias `jgrid::jgrid`) is a static library of the module sources linking it, and `jgrid_demo` is a `juce_add_gui_app`. The name `jgrid` is left free for `juce_add_module`, which creates a target named after the module.
- The module declares `minimumCppStandard: 23` (it uses C++23 library features such as `std::ranges::iota`); the project itself builds as C++26.
- `tests/module` (option `JGRID_BUILD_MODULE_CHECK`, on in the `ci` preset) builds a small console program that uses the Grid only through `juce_add_module`, as users would, and runs it as the `module_check` test.
- Presets: `dev`, `asan` (address + undefined), `tsan`, `tidy` (clang-tidy, warnings as errors), `ci` (warnings as errors, module check). The sanitizer presets skip the demo app.
- `tools/check.py` runs pytest, a clang-format check (the repository's `.clang-format`) and build + test for every preset. Run it with `.venv/bin/python` (a local venv with pytest). Use `--fix-format` to reformat and `--preset` to limit presets. The asan and tsan build directories (about 0.5 GB each) are deleted after their tests unless `--keep-builds` is given, to save disk space.
- `.clang-tidy` enables the bugprone, cert, clang-analyzer, concurrency, cppcoreguidelines, misc, modernize, performance, portability and readability groups, with these exclusions: braces-around-statements (the format style allows brace-less single-line ifs), uppercase literal suffixes, reference data members (JUCE's owner-reference idiom), and multiple inheritance (JUCE's listener idiom). `ColourIds` is a plain enum with a NOLINT, as JUCE colour ids must be.
- The demo app (`demo/`) shows 500 generated rows with 8 named columns, header toggles, a high-contrast colour scheme and a "Second view" toggle that shows a second Grid on the same model. Every column except ID is editable, with validation: Name non-empty, Track 1–64, Gain a number up to 24 dB, Pan −100–100, Take 1–999. The model converts edited strings to ints or doubles and notifies `CellsChanged`. `"JUCE Grid Demo" --snapshot <file.png>` renders the window with the second view open and a rejected edit (its error popup visible) in D3, and exits, for checking rendering without UI automation.

### Grid behaviour as built (milestone 1)
- Defaults: row height 22, column width 100, column header height 24. The row header width fits the widest row label plus 12 px of padding, and is at least 30 px. Labels are measured for every row when the model is bound or refreshed, which is acceptable because the Grid is not virtualized.
- Cell text comes from `getValue(...).toString()`. Numeric vars (int, int64, double) are right-aligned; everything else is left-aligned.
- `GridModel` requires rowCount, colCount and getValue. If any is missing the Grid shows as empty.
- `Grid::refresh()` re-queries counts and labels, for models without a notifier.
- The cursor is drawn as a 2 px outline. The cursor's row and column headers are filled with the selection colour.
- Keys handled: arrows, Page Up/Down (moves by the number of fully visible rows), Home/End (row start and end), Ctrl/Cmd+Home/End (first and last cell), Enter/Shift+Enter (down/up), Tab/Shift+Tab (next/previous cell, wrapping rows, leaving the Grid past either end), Ctrl+Tab/Ctrl+Shift+Tab (leave the Grid). Leaving uses `moveKeyboardFocusToSibling`. Shift-, Alt- and Ctrl/Cmd-modified arrows return false (unhandled) until milestone 3. Clicking a cell moves the cursor and takes keyboard focus.
- Scrolling is the Grid's own: two `juce::ScrollBar`s plus mouse wheel (Shift+wheel scrolls horizontally). Moving the cursor scrolls the minimum distance to show it.
- Colour resolution: the colour set on the Grid, else on its nearest ancestor that sets it, else the LookAndFeel's, else a fallback:

| Grid colour | Fallback |
|---|---|
| background | ListBox::backgroundColourId |
| text | ListBox::textColourId |
| headerBackground | TableHeaderComponent::backgroundColourId |
| headerText | TableHeaderComponent::textColourId |
| gridLine | TableHeaderComponent::outlineColourId |
| cursor | TextEditor::highlightColourId at full opacity |
| selection | TextEditor::highlightColourId |

### Editing as built (milestone 2)
- `Grid` publicly derives from `juce::TextInputTarget`. On macOS, JUCE sends typed text, dead keys and IME composition to the focused component only if it is a TextInputTarget, and routes special keys (arrows, Tab, Enter, Escape) back through `keyPressed`. `isTextInputActive()` is true while the cursor cell is editable or an editor is open. With no editor, the Grid presents an empty text; the first `insertTextAtCaret` opens the editor with that text without moving keyboard focus, and every later TextInputTarget call is forwarded to the editor. Keyboard focus moves to the editor (asynchronously) only when `setTemporaryUnderlining` is called with no regions, i.e. when no composition is in progress. This matters because JUCE discards the input method's marked text whenever keyboard focus changes: moving focus at the first dead key (Option+E) would leave a stray accent and a separate "e". While the Grid keeps focus with an editor open, `Grid::keyPressed` forwards every key to the editor. On other platforms typed characters arrive through `keyPressed` and open the editor the same way.
- macOS can deliver F2 as the private-use character U+F705 through text input; the Grid treats it as F2. `isTypedText` rejects control characters and the macOS function-key range U+F700–U+F8FF.
- The editor is a `juce::TextEditor` subclass placed over the cell, using the Grid's colours and font. Its built-in undo/redo (Cmd/Ctrl+Z, Shift+Cmd/Ctrl+Z, Cmd/Ctrl+Y) is the in-editor undo; the stack goes with the editor. Its title is "<column label>, <row label>" for screen readers.
- Editor keys: Enter/Shift+Enter commit and move down/up; Tab/Shift+Tab commit and step cells (leaving the Grid past either end); Ctrl+Tab commits and leaves; Escape cancels. Any key or text insertion dismisses the error popup.
- Public editing API: `startEditing()` (F2), `startEditingWithText(text)` (typing), `commitEdit()`, `cancelEdit()`, `isEditing()`, `getEditingCell()`, `getEditor()`, `getErrorMessage()`.
- Errors: the popup is a non-focusable child of the Grid, below the cell (above it if there is no room), coloured with the TooltipWindow colours, dismissed after `Grid::errorTimeoutMilliseconds` (4000). The shake moves the editor horizontally by `shakeOffset(t)`: 6 px decaying over 3 cycles in 0.35 s, driven by a 60 Hz timer. The message is posted with `AccessibilityHandler::postAnnouncement` at high priority. setValue returning false shows "Could not set value". Attempting to edit a read-only cell announces "Read-only cell".
- Notifications: `setModel` subscribes to `model.notifier` and unsubscribes on rebinding or destruction. If the notifier is destroyed first, the Grid cancels any edit, drops the model and shows empty. `CellsChanged` repaints and posts title-changed accessibility events for the affected visible cells. Inserts, removals and reorders remap the cursor and editing cell with `remapRow`; a cursor on a removed row moves to `rowAfterRemoval`.

### Selection, clipboard and sorting as built (milestone 3)
- `Selection` (core) holds ranges plus the active range's anchor and far corner; the Grid holds the cursor. `setCursor` collapses the selection; `extendSelection` moves the far corner; `addToSelection` adds a range (Cmd/Ctrl-click); `selectRows`/`selectColumns` select whole rows/columns, putting the cursor on the first row/column given, in the first visible column/row, without scrolling; `selectAll` (Cmd/Ctrl+A) keeps the cursor.
- Dragging in the body extends the active range with auto-scroll; dragging in a header extends the row/column range. Shift-click on a header extends from the anchor; Cmd/Ctrl-click adds.
- Selected cells other than the cursor cell are filled with the selection colour; headers of rows/columns touching the selection are tinted, twice for wholly selected ones. A cell's accessible state is selected when it is in the selection. Extending a selection announces "N cells selected".
- Row changes carry ranges along (inserts shift or grow them, removals shrink them); a reorder or reset collapses the selection to the cursor, since ranges cannot follow scattered rows.
- Copy (Cmd/Ctrl+C) takes the active range only (multi-range copy is not supported). Cut (Cmd/Ctrl+X) does the same and records a process-wide cut mark (source Grid, range, clipboard text), drawn as a dashed outline in the cursor colour.
- Paste (Cmd/Ctrl+V) plans its writes with `planPaste`, skipping read-only cells. A cut is honoured only if the clipboard still holds the cut's text; its source cells (other than those the paste overwrites in the same Grid) are cleared by writing empty text. Destination writes and source clears are all validated before anything is written, so a source cell that may not be empty makes the whole paste fail. In one Grid, writes and clears form one batch; across Grids, each Grid's writes are its own batch. A rectangular paste of several cells selects what it wrote.
- The cut mark is dropped by Escape, a copy, any paste, a different clipboard text, any row insert/remove/reorder/reset in the source Grid, and any `CellsChanged` overlapping the cut range.
- Delete or Backspace (no modifiers) clears the editable selected cells through validate and setValue, all-or-nothing on validation.
- Write failures: validation failures move the cursor to the first failing cell and show the popup there ("<message>, and N more" if several). A failing setValue stops the operation at that cell with "Could not set value"; `endBatch` is always called. The popup is dismissed by the next key, a click, focus loss or the timeout. Copy, cut, paste and delete each post an announcement ("Copied 4 cells", ...).
- Sorting: `sortByColumn(col, descending)` compares with `model.compare` or `compareValues` (void first; numbers numerically, including bools; otherwise `String::compareNatural`, case-insensitive), builds the order with `sortedOrder` (stable), and calls `reorderRows` inside one batch unless the order is the identity. `toggleSort` flips direction on the sorted column. The indicator is cleared by any change notification not caused by the Grid's own sort, except `CellsChanged` outside the sorted column. "Sorted by <column>, ascending/descending" is announced.
- Models without a notifier: the Grid applies its own committed edits, writes and sorts to itself (`notifySelf`), so its cursor, selection and indicator stay right.
- Accessibility components are rebuilt only when the column count or header visibility changes; otherwise a refresh just syncs rows, so the element VoiceOver is on (for example a header cell it just pressed to sort) survives row changes.
- `RowOrder` (core) keeps view-to-storage and storage-to-view maps; `reorder`, `insert` (new storage rows at a view position, shifting later storage indices), `removeStorage` (returns the view rows removed) and `storageReordered` keep both in step.
- `DataSource` + `ViewModel` (core): one ViewModel per Grid, with its own RowOrder and optional column list. Its `bind()` model translates every call to storage indices and implements `reorderRows`. It listens to the source notifier: `CellsChanged` becomes one `CellsChanged` spanning the affected view rows and shown columns (none if no shown column is affected); `RowsInserted` appends the new rows to the view; `RowsRemoved` maps to view rows; a storage `RowsReordered` updates the map without notifying (the view shows the same data); `Reset` resets the order. `setRowOrder` restores a saved order and `setColumns` changes the columns, each sending `Reset`.
- The demo's two Grids are ViewModels over one DataSource; the second shows Name, Instrument and Track, and each sorts independently. The snapshot shows a descending sort, a cut range and a column selection.

### Layout, menus and row operations as built (milestone 4)
- `Axis` stores each index's size and a hidden flag; a hidden index keeps its size but occupies no space. It maps between indices and visible positions (`toVisibleIndex`, `fromVisibleIndex`), steps over hidden indices (`step`, `nextVisible`), and supports `insert`, `remove` and `reorder`, which the Grid applies on RowsInserted/Removed/Reordered so row heights and hidden rows follow their rows. A reset clears row sizes and hidden rows; column widths and hidden columns persist.
- Navigation (`moveCursor`, `stepCell`, `clampCursor` overloads taking Axes), Enter/Tab cycling, copy, paste (`planPaste` lands table rows and columns on successive visible ones), delete and cut skip hidden rows and columns. Hiding the cursor's row or column moves the cursor to the nearest visible one (and closes an editor there).
- Screen readers see only visible rows and columns: table counts, row/column spans, header cells and row cells all use visible positions. Hiding or showing rebuilds the accessible components.
- Resizing: within 4 px of a header edge the mouse cursor changes and dragging resizes the column or row on the edge's left/top (minimum `Grid::minimumCellSize`, 8 px). Double-clicking the edge autofits: columns to the widest of the header label (plus the sort button) and every row's text, plus padding; rows to the font height plus 8 px. API: `setColumnWidth`, `getColumnWidth`, `setRowHeight`, `getRowHeight`, `autoFitColumn`, `autoFitRow`, `setColumnsHidden`, `setRowsHidden`, `showAllColumns`, `showAllRows`, `isColumnHidden`, `isRowHidden`, `getHiddenColumns`, `getHiddenRows`.
- Context menus open on right-click (a click outside the selection first moves it: to the cell, or to the clicked row or column), Shift+F10 (cells, rows or columns depending on whether whole rows or columns are selected), and a cell's accessibility show-menu action (VoiceOver: VO+Shift+M). `createContextMenu(kind)` builds a menu (used by tests and apps); `showContextMenu` shows it. Unsupported items are disabled. `GridModel::contextMenu(menu, target)` is called last, so the model can add, remove or change items; `ContextMenuTarget` gives the kind, the cursor cell and a copy of the selection.
- Rows: `getSelectedRows`/`getSelectedColumns` return wholly selected visible rows/columns. `copyRows` copies every column (hidden ones included, since rows are records) of the selected rows as TSV. `cutRows` (requires removeRows) also records a row cut mark (dashed outline round each run of rows). `insertCopiedRows(belowRow)` decodes the clipboard into `RowData` padded or truncated to the column count, inserts below the row, and, for a cut that still matches the clipboard, removes the source rows (adjusted for the insertion when in the same Grid), all in one batch per Grid; the inserted rows end up selected. `duplicateSelectedRows` uses `duplicateRows` if present, else copies values with getValue and inserts them after the last selected row. `deleteSelectedRows` (Ctrl+−) removes the selected rows or the cursor's row; `insertBlankRows` (Ctrl+=, also Ctrl++) inserts as many empty rows as are selected below them, or one below the cursor's row, which the cursor moves to. Blank rows are inserted below, matching "Insert ... Below", rather than above as in Excel. Failures show the error popup at the cursor and are announced.
- Undo/redo: Ctrl+Z, Shift+Ctrl+Z and Ctrl+Y call `model.undo`/`model.redo` when not editing; without them the keys are not handled.
- `DataSource` gains `insertRows`, `removeRows`, `undo` and `redo`. A ViewModel inserts by appending to storage while remembering the requested view position, so the new rows appear there in that view and at the end of other views; it maps removals to storage rows.
- The demo model supports insert, remove and snapshot-based undo/redo (a snapshot per edit, or per batch), new rows get fresh IDs, and an undo or redo notifies Reset.

### Accessibility as built
- Each cell component has one invisible child with the `staticText` role whose title and read-only `AccessibilityTextValueInterface` value are the cell's display text, mirroring a native macOS table cell (AXCell containing AXStaticText). VoiceOver reads a cell through that child: a title or value on the cell itself is ignored, and VoiceOver then only says "you are currently on a cell". The cell handler has no title or value of its own. `CellsChanged` posts title- and value-changed events on the text child. When the cell's text is empty the text child reports itself ignored, so the cell has no readable child and VoiceOver reads it as an empty cell instead of reusing stale speech. Known limitation, accepted: this works when VoiceOver's own cursor moves onto the cleared cell; when the Grid's cursor moves back onto it with the arrow keys, VoiceOver may still repeat earlier speech ("Cleared 1 cell").
- The Grid's handler has the `table` role and an `AccessibilityTableInterface`. The Grid and its column header are focus containers, because JUCE only reports accessibility children under focus containers.
- Rows (`row` role) are children of the body, which has an ignored handler. Each row has one `cell` child per column, and the row's title is its row label. A cell is selectable, and selected when it is the cursor. Its focus action moves the cursor there.
- The column header (`tableHeader` role, a group on macOS) has exactly one child per visible column, because macOS navigation requires the header's child count to match the column count. Each child has the `tableHeader` role, as in JUCE's own TableHeaderComponent, for cross-platform consistency: JUCE maps it to a header on Windows (UI Automation) and iOS, and to a group on macOS, where native tables use buttons instead (VoiceOver reaches the headers with VO+Shift+| either way), titled with the column label (plus the sort state); its press action sorts and its show-menu action (VO+Shift+M) selects the column and opens the column menu. With the column header hidden, the table reports no header.
- Any table interface call activates accessibility. Activation creates the header cells and the row components, which are then kept in sync on scroll, resize, refresh and cursor moves.
- Accessibility focus follows the cursor: on every cursor move while the Grid has keyboard focus, and asynchronously after the Grid gains keyboard focus, since JUCE gives focus to the table's own handler straight after focusGained.
- With the static text child in place, VoiceOver reads the focused cell itself when the cursor moves ("Vocal, cell, 3 of 8"). An earlier workaround that also announced every cursor move was removed in milestone 5, because it only duplicated VoiceOver. Cursor moves by VoiceOver's own navigation and by the Grid's keys now take the same path.
- Cell actions: focus (moves the cursor there), press (VO+Space: moves the cursor there and opens the editor, or announces "Read-only cell"), and show menu (VO+Shift+M: the context menu). A cell's help, read by VoiceOver as its hint, is "Editable. Press F2 or type to edit." or "Read-only.".
- Rows are multi-selectable and report themselves selected when the whole row is in the selection; a change in the set of wholly selected rows posts a row-selection-changed event on the table.
- A change in the number of visible rows or columns (refresh, hiding or showing) posts a structure-changed event on the table.
- A component needs a native window before it has an accessibility handler, so tests put the Grid on the desktop with `addToDesktop(0)`.

### Manual testing
- Build the demo with `cmake --build --preset dev-release --target jgrid_demo`. On macOS the linker's ad-hoc signature does not cover the bundle resources, so re-sign the app with `codesign --force --deep -s - "<app>"` before copying it to another Mac.
- VoiceOver checklist for the demo:
  1. Tab into the Grid: VoiceOver names the table and the cursor cell.
  2. Arrows, Tab, Enter: VoiceOver reads each cell ("Vocal, cell, 3 of 8"); an empty cell reads as empty (a cell just cleared with Delete reads as empty when reached with VoiceOver's own cursor; with the arrow keys VoiceOver may repeat earlier speech, an accepted limitation).
  3. VO+arrows: VoiceOver's own cursor reads each cell's text; VO+Space on a cell opens its editor; on the ID column it says "Read-only cell".
  4. Type an invalid value (Track "abc") and press Enter: the error is announced and the editor keeps focus; Escape cancels.
  5. Shift+arrows: "N cells selected". Select whole rows from the row header: VoiceOver reports them selected.
  6. VO+Shift+M or Shift+F10: the context menu opens at the cursor and is navigable.
  7. Column header: VO+Shift+| (Control+Option+Shift+|) moves to the column headers, as in native macOS tables; VO+Space on a header sorts and it then reads "<name>, sorted ascending"; VO+Shift+M opens the column menu. The cell menu's Sort Column and Hide Column items do the same from a cell.
  8. Hide a column from its menu: it disappears from VoiceOver's table navigation; Show Hidden Columns brings it back.
  9. Ctrl+Tab leaves the Grid.
