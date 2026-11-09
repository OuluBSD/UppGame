#include "SocketUtils.h"

NAMESPACE_UPP

bool socket_startup() { return true; }
void socket_cleanup() {}

bool socket_has_sctp() { return false; }
bool socket_has_ipv6() { return true; }
bool socket_has_dual_stack() { return true; }

END_UPP_NAMESPACE
