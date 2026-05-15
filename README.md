# Timer On Top

A minimal always-on-top stopwatch for Windows, styled after LiveSplit. No splits, no nonsense — just start, stop, and reset.

![Timer On Top screenshot](screenshot.png)

## Features

- Always on top of all other windows
- Green-on-dark display with centisecond precision
- Drag anywhere on the timer face to reposition
- Resize freely by dragging any edge or corner
- No taskbar button — lives in the system tray
- Right-click tray icon (or window) → **Quit Timer On Top**
- Double-click tray icon to show/hide

## Building

Requires **Visual Studio** (any edition, including the free Community edition) with the **Desktop development with C++** workload.

1. Open **x64 Native Tools Command Prompt for VS** (search in Start menu).
2. `cd` to this folder.
3. Run:
   ```
   build.bat
   ```
4. `TimerOnTop.exe` will appear in the same folder.

## Usage

| Action | How |
|---|---|
| Start | Click **START** |
| Pause | Click **STOP** |
| Reset | Click **RESET** |
| Move window | Drag the timer display |
| Resize | Drag any edge or corner |
| Quit | Right-click → Quit Timer On Top |
| Hide / show | Double-click tray icon |
