# GameLauncher

Scope
- Applies to `uppsrc/GameLauncher` and its sub-tree.

Purpose
- Minimal standalone launcher for `.gamestate` projects.

Rules
- Keep the package focused on launch/runtime wiring only.
- Reuse ScriptIDE standalone-window runtime paths instead of duplicating game-host logic.
