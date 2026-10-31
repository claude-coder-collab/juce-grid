---
title: Editing and validation
nav_order: 5
---

# Editing and validation

## Opening an editor

| Action | Result |
|---|---|
| F2 | Opens the cursor cell's editor with its text selected. |
| Typing a character | Opens the editor with that character replacing the text. Dead keys (Option+E, E) and input methods work from the first keystroke. |
| Double-click | Opens the editor with the caret where you clicked. |
| VoiceOver: VO+Space on a cell | Opens the editor. |

Read-only cells (no `setValue`, or `isEditable` returns `false`) don't open; screen readers hear "Read-only cell".

## Inside the editor

| Key | Result |
|---|---|
| Enter / Shift+Enter | Commit, then move down / up. |
| Tab / Shift+Tab | Commit, then move right / left. |
| Ctrl+Tab | Commit, then leave the Grid. |
| Escape | Cancel; nothing is stored. |
| Cmd/Ctrl+Z, Shift+Cmd/Ctrl+Z | Undo and redo **inside the editor**. The editor's history ends when it closes. |

Clicking another cell commits. Clicking outside the Grid leaves the editor open.

## The commit flow

![The edit flow: validate rejects "abc", then accepts "12", setValue stores it and the notifier updates every Grid]({{ "/images/edit-flow.svg" | relative_url }})

1. If the text is unchanged, the editor just closes; the model isn't called.
2. `validate(row, col, text)` runs. A failed `Result` keeps the editor open, shakes it, and shows the message in a popup below the cell. The popup never takes focus, and goes away on the next key, a click, focus loss or after four seconds. Screen readers hear the message.
3. `setValue(row, col, text)` runs. Returning `false` shows "Could not set value" the same way.
4. The editor closes and the cursor moves on.

The value always arrives as a **string** `juce::var`, because it came from a text editor. Convert it in `setValue`:

```cpp
model.setValue = [this](int row, int col, const juce::var& value) {
    gains[row] = value.toString().getDoubleValue();
    notifier.changed(jgrid::CellsChanged{ row, 1, col, 1 });
    return true;
};
```

Paste and Delete use the same `validate` and `setValue`.

## Driving the editor from code

`Grid::startEditing()`, `startEditingWithText()`, `commitEdit()` and `cancelEdit()` do what F2, typing, Enter and Escape do; `isEditing()`, `getEditingCell()`, `getEditor()` and `getErrorMessage()` report the state.
