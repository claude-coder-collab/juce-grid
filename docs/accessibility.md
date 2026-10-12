---
title: Accessibility
nav_order: 12
---

# Accessibility
{: .no_toc }

1. TOC
{:toc}

The Grid exposes itself to screen readers as a table through JUCE's accessibility layer: no platform code of its own. It has been tested with VoiceOver on macOS; Windows and Linux screen readers have not been tested yet.

## What screen readers get

- A **table** with rows, cells and column headers. Only visible rows and columns are exposed, in the order the Grid shows them.
- Each **cell** reads its text ("Vocal, cell, 3 of 8"), and is reported as selected when it is in the selection. Empty cells read as empty.
- **Hints**: "Editable. Press F2 or type to edit." or "Read-only.".
- **Actions** on a cell: press opens its editor; show menu opens the context menu.
- **Column headers** have the sort state in their title ("Track, sorted ascending"); press sorts and show menu opens the column menu.
- **Announcements** for validation errors, selections ("12 cells selected"), copy, cut, paste, delete, sorts and row operations.
- Focus follows the cursor; the editor takes focus while open and hands it back when it closes.

## VoiceOver tips

| Keys | Does |
|---|---|
| Arrows, Tab, Enter | Move the Grid's cursor; VoiceOver reads each cell. |
| VO+arrows | Move VoiceOver's own cursor through the table. |
| VO+Space | Edit a cell, or sort from a header. |
| VO+Shift+M or Shift+F10 | Open the context menu. |
| VO+Shift+\| | Move to the column headers. |
| Ctrl+Tab | Leave the Grid. |

## Your part

- Give the Grid a title: `grid.setTitle("Session tracks")`.
- Give columns names with `colName`; they become the header titles.
- Write validation messages that make sense when heard on their own.
- If you change colours, keep enough contrast: the colour IDs make high-contrast schemes easy.
