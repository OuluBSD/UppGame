# AGENTS - game/TexasHoldemProviderCatalog

- This package is a Core-only shared catalog for TexasHoldem provider/theme metadata.
- Keep it free of GUI, Form, Poker, GameRules, and VisualStateCommandRegistry dependencies.
- Consumers may resolve relative form filenames to absolute package paths themselves.
- Unknown providers must be represented as lookup failures in automation-facing code; do not silently fall back.
