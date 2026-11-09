#ifndef _CardEngine_Exception_h_
#define _CardEngine_Exception_h_

#include <Core/Core.h>

NAMESPACE_UPP

class PokerTHException : public Exc
{
public:
	PokerTHException(const char *sourcefile, int sourceline, int errorId, int osErrorCode);
	
	int GetErrorId() const { return m_errorId; }
	int GetOsErrorCode() const { return m_osErrorCode; }

private:
	int m_errorId;
	int m_osErrorCode;
};

class LocalException : public PokerTHException
{
public:
	LocalException(const char *sourcefile, int sourceline, int errorId)
		: PokerTHException(sourcefile, sourceline, errorId, 0) {}
};

END_UPP_NAMESPACE

#endif
