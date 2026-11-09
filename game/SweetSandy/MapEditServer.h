#ifndef _SweetSandy_MapEditServer_h_
#define _SweetSandy_MapEditServer_h_

// Arbitrary, fixed - distinct from uppsrc/MCP's own default (7326) so
// the two can run at the same time without colliding. Both
// --mapedit's own client-connect-attempt and --mapedit-server's own
// listener use this same constant, so there is exactly one number to
// change if it ever needs to.
const int kMapEditServerPort = 47821;

// Map Editor phase, Task 14: "I'd appreciate if there was editor
// instance on, and then some cli command would just communicate with
// that" - the user's own explicit ask, on top of Task 12/13's own
// "spawn a fresh process, do one thing, exit" design (kept as a
// fallback - see main.cpp's own --mapedit dispatch). A real persistent
// server: one long-running `--mapedit-server` process keeps a
// MapEditorWindow - and, once baked once, its own already-generated
// WorldBuilder content - alive in memory; short-lived `--mapedit <cmd>`
// client processes connect over a local TCP socket, send one line, get
// one line back, and exit - no GL window, no bake, on the client side
// at all.
//
// TCP on 127.0.0.1, not a Unix domain socket - this codebase has no
// LocalSocket-style class anywhere (confirmed by search). uppsrc/MCP's
// own server (uppsrc/MCP/Server.cpp) is the closest existing
// precedent and uses the identical accept-with-a-short-timeout-then-
// recheck-a-stop-flag idiom this class's own ThreadLoop copies
// directly (`s.Timeout(5); s.Accept(listener);` inside `while(!stop)`).
//
// Threading otherwise mirrors WorldChunkGenWorker's own established
// convention (Map Editor phase, Task 9) rather than MCP's own (a
// second thread polling many concurrent client connections) - this
// server only ever has one command in flight at a time (a client's own
// single request/response round trip; --mapedit is not expected to be
// called concurrently from multiple places at once), so a single
// background Thread suffices: it owns the listening socket, accepts
// one client, reads its one command line, hands it to the GUI thread
// via one Mutex-protected slot, blocks (short-sleep polling, same
// convention) waiting for that slot's own "done" flag, then writes the
// response, closes the connection, and loops back to accept the next
// one. The actual GeomObject/scene mutation always happens on the GUI
// thread (MapEditorWindow's own AppShell, via its regular timer -
// see main.cpp's own --mapedit-server loop), never from this
// background thread directly - the same "background thread never
// touches live UI/scene state directly" discipline WorldChunkGenWorker
// already established for Terrain/WorldBuilder.
class MapEditServer {
public:
	MapEditServer() {}
	~MapEditServer() { Stop(); }
	MapEditServer(const MapEditServer&) = delete;
	void operator=(const MapEditServer&) = delete;

	// Starts listening on 127.0.0.1:port. Logs and leaves the server
	// not running if the port is already in use (e.g. another
	// --mapedit-server already running) - callers should check
	// IsRunning() after Start() if they need to know.
	void Start(int port);
	void Stop();
	bool IsRunning() const { return thread_running; }

	// Called from the GUI thread's own periodic timer. Returns true
	// and fills `out_command` if a client is waiting for a response -
	// caller MUST call PostResponse exactly once after this returns
	// true, before the next PollCommand call (single command in flight
	// at a time - see this class's own header comment).
	bool PollCommand(String& out_command);
	// Called from the GUI thread once PollCommand's own command has
	// been executed - unblocks the background thread to send
	// `response` back to the waiting client and accept the next
	// connection.
	void PostResponse(const String& response);

private:
	void ThreadLoop(int port);

	Thread thread;
	bool thread_running = false;

	Mutex lock;
	bool has_command = false;
	String pending_command;
	bool has_response = false;
	String pending_response;
};

#endif
