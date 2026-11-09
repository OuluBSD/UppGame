#ifndef _GameCommon_MultiGameGen_h_
#define _GameCommon_MultiGameGen_h_

#include <Core/Core.h>

namespace Upp {

int RunTexasGameplayGeneration(int num_hands, const String& output_path);
int RunPloGameplayGeneration(int num_hands, const String& output_path);

}

#endif
