#ifndef _CardEngine_RunLocalGame_h_
#define _CardEngine_RunLocalGame_h_

#include <Core/Core.h>

NAMESPACE_UPP

class ConfigFile;
class EngineLog;
class GameTable;

void RunLocalGame(int numPlayers, int startCash, int gameSpeed, const String& provider,
                  class ConfigFile& config, EngineLog& engineLog);
void InitLocalGame(GameTable& table, int numPlayers, int startCash, int gameSpeed, const String& provider,
                   class ConfigFile& config, EngineLog& engineLog);
int RunLocalGameScripted(int numPlayers, int startCash, int gameSpeed, class ConfigFile& config, EngineLog& engineLog,
                         const String& project_name, const String& script_path, int max_ticks, int sleep_ms,
                         int seed, bool verbose, const String& dump_loop_state_json, bool headless,
                         bool auto_human_action, bool no_wait_between_actions);

END_UPP_NAMESPACE

#endif
