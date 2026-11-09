# CardGameClient

Reference Strategy Advisor Client application using `Core/Game` package.

## Features

- Connects to `CardGameServer` on TCP port 8765.
- Sends `SUGGEST_REQUEST` JSON messages over socket connection using `CardGameProtocol`.
- Receives `SUGGEST_RESPONSE` JSON messages and outputs advice to console / client interface.
