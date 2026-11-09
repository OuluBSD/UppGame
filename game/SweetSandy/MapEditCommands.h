#ifndef _SweetSandy_MapEditCommands_h_
#define _SweetSandy_MapEditCommands_h_

// Map Editor phase, Task 14: the actual --mapedit subcommand logic,
// factored out of main.cpp so it can run identically from two places -
// the standalone one-shot `--mapedit <cmd>` CLI (main.cpp's own
// fallback path when no server is reachable) and MapEditServer's own
// persistent-process dispatch (main.cpp's own --mapedit-server loop,
// on the GUI thread, after MapEditServer::PollCommand hands it a
// command line). `win` must already have its map editor open
// (AppShell::ToggleMapEditor already called) - neither caller opens/
// closes it per call, unlike Task 12/13's own original inline version.
// `args` is the tokenized command line (args[0] = subcommand,
// args[1..] = its own parameters - the exact same shape
// CommandLine()[1..] already was). Returns a one-line human-readable
// result ("OK ..." or "ERROR ...") - logged by the standalone CLI path,
// and sent back verbatim to the client over the socket by the server
// path.
String ExecuteMapEditCommand(AppShell& win, const Vector<String>& args);

#endif
