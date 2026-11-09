# CardGameServer

Reference GTO & Heuristic Action Advisor Server using the `Core/Game` package.

## Features

- Listens on TCP port 8765 (configurable).
- Parses `SUGGEST_REQUEST` JSON messages over socket connection using `CardGameProtocol`.
- Evaluates `CardGamePathInfo` compact state path.
- Computes heuristic strategy distribution (`fold`, `call`, `raise`), equity, and primary action suggestion.
- Returns `SUGGEST_RESPONSE` JSON lines back to the client.
