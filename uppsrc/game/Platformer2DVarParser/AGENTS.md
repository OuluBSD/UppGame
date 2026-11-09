# Platformer2DVarParser Package Guide

## Scope
- Applies to `uppsrc/game/Platformer2DVarParser/`.
- U++ CLI port of OmniSpeak `varparser` tool.

## Responsibilities

1. **EPISODE.CKx Parsing**
   - Parse variable definitions (bool, int, string, arrays, actions)
   - Handle includes
   - Episode metadata (number, name)

2. **Output Formats**
   - Generated C declarations (`-decls`)
   - Generated action definitions (`-actions`)
   - Static data (`-static`)
   - Ordered output matching OmniSpeak varparser

3. **CLI Interface**
   - `Platformer2DVarParser [options] <episode_file>`
   - Options: `-decls`, `-actions`, `-static`, `-headers`, `-help`

## Binary Compatibility

- Output must be textually identical to OmniSpeak varparser
- Deterministic ordering of declarations

## Dependencies

- Uses: `Platformer2D`, `Resource` (for EpisodeVars parser)
- No GUI, no platform dependencies