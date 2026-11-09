#ifndef _CardEngine_SocketUtils_h_
#define _CardEngine_SocketUtils_h_

#include <Core/Core.h>

NAMESPACE_UPP

bool socket_startup();
void socket_cleanup();

bool socket_has_sctp();
bool socket_has_ipv6();
bool socket_has_dual_stack();

END_UPP_NAMESPACE

#endif