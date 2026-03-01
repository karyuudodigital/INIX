# INIX

INIX is a desktop app for editing `.ini` files easier.

It is built for large, messy config files where you need to search fast, compare changes clearly, and merge settings without losing context. I intended it to be used for game configs (SkyrimPrefs.ini, Morrowind.ini, etc)

## Features

- Open and edit `.ini` files with a table-style view
- Search and filter across sections and keys
- Compare a base file and another file in a Diff tab
- Preview and apply merges in a Merge tab
- Save sections and values to presets that you can apply to different files later 
- Choose conflict behavior when importing changes
- Undo merge actions if something looks off
- Keep original file style details like line endings and encoding when saving



## Tabs

- Main editor view: edit sections/keys/values directly
- Diff tab: quick summary plus semantic key-level differences
- Merge tab: pick exactly which changes to bring in

## Notes

- If a section/key appears multiple times, comparison currently uses the last occurrence.
- Merge undo restores document state so you can back out safely.

## Build From Source

Detailed developer/build documentation lives in `documentation.md`.

Quick start:

### Windows (PowerShell)
```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.x.x\msvcxxxx_64"
cmake --build build --config Release
ctest --test-dir build --output-on-failure -C Release
```

### macOS/Linux
```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x.x
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

App binary:
- Windows: `build/src/Release/ini_editor.exe` (or `build/src/ini_editor.exe`)
- macOS/Linux: `build/src/ini_editor`

