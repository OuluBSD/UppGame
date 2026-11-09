#include <GameRules/Exception.h>

NAMESPACE_UPP

PokerTHException::PokerTHException(const char *sourcefile, int sourceline, int errorId, int osErrorCode)
	: m_errorId(errorId), m_osErrorCode(osErrorCode)
{
	const char *filename = sourcefile;
	if (sourcefile) {
		const char *lastSlash = strrchr(sourcefile, '/');
		if (!lastSlash)
			lastSlash = strrchr(sourcefile, '\\');
		if (lastSlash)
			filename = lastSlash + 1;
	}

	String msg;
	msg << filename << " (" << sourceline << "): Error " << errorId;
	if (osErrorCode)
		msg << " (system error " << osErrorCode << ")";
	
	*static_cast<String*>(this) = msg;
}

END_UPP_NAMESPACE
