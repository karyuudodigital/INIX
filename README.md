# INI Editor (Qt 6 / C++20)

Cross-platform desktop INI editor for Bethesda-style `.ini` files with:
- open/view/edit
- global search/filter
- base vs compare diff (raw summary + semantic diff)
- selective import/merge with conflict policies
- undo support for merge apply

## Build

## Prerequisites
- CMake 3.21+
- C++20 compiler
- Qt 6 (`Core`, `Widgets`, `Concurrent`, `Test`)

## Windows (PowerShell)
```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.x.x\msvcxxxx_64"
cmake --build build --config Release
ctest --test-dir build --output-on-failure -C Release
```

## macOS/Linux
```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x.x
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

Run app binary:
- Windows: `build/src/Release/ini_editor.exe` (or `build/src/ini_editor.exe`)
- macOS/Linux: `build/src/ini_editor`

## Architecture

`src/domain`
- `IniTypes.h`: shared value types and enums
- `IniDocument`: editable in-memory document with formatting-preserving lines and dirty tracking

`src/services`
- `IniParser`: robust parsing, line classification, encoding detection
- `IniSerializer`: save with line ending + encoding preservation
- `IniDiffService`: raw line summary + semantic `(section,key)` diff
- `IniMergeService`: preview + apply merge policies
- `IniSearchService`: case/regex-aware matcher

`src/ui/models`
- `IniSettingsTableModel`: settings table model for editing
- `IniFilterProxyModel`: search filtering and highlight
- `DiffTableModel`: semantic diff table
- `MergePreviewTableModel`: selectable merge rows

`src/app`
- `MainWindow`: menus, docks, async parse/diff, merge controls, undo stack

## Notes and limitations
- Semantic diff currently compares the last occurrence of duplicate `(section,key)` entries.
- Raw text diff summary is a fast positional summary, not a full Myers/LCS patch output.
- ANSI fallback uses local 8-bit encoding (`QString::fromLocal8Bit` / `toLocal8Bit`).
- Merge undo currently snapshots the full document state.

## Tests

Tests are in `tests/` using QtTest:
- parser edge cases
- serializer round trip
- semantic diff behavior
- merge policy behavior
- integration-style merge on sample INIs in `tests/data/`

Run:
```bash
ctest --test-dir build --output-on-failure
```

## Screenshots / GIFs

- Main editor view: `docs/screenshots/editor-main.png` (placeholder)
- Diff tab: `docs/screenshots/diff-tab.png` (placeholder)
- Merge tab: `docs/screenshots/merge-tab.gif` (placeholder)

