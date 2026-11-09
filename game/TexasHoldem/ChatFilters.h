#ifndef _CardEngine_ChatFilters_h_
#define _CardEngine_ChatFilters_h_

#include <Core/Core.h>

NAMESPACE_UPP

class BadWordCheck
{
public:
	void SetBadWords(const Vector<String>& bw) { badWords <<= bw; }
	void SetBadWordsException(const Vector<String>& bwe) { badWordsException <<= bwe; }
	bool Run(const String& msg);

private:
	Vector<String> badWords;
	Vector<String> badWordsException;
};

class CapsFloodCheck
{
public:
	void SetCapsNumberToTrigger(int n) { capsNumberToTrigger = n; }
	bool Run(const String& msg);

private:
	int capsNumberToTrigger = 0;
};

class LetterRepeatingCheck
{
public:
	void SetLetterNumberToTrigger(int n) { letterNumberToTrigger = n; }
	bool Run(const String& msg);

private:
	int letterNumberToTrigger = 0;
};

struct TextFloodInfos : Moveable<TextFloodInfos> {
	int floodLevel;
	int64 timeStamp;
};

class TextFloodCheck
{
public:
	TextFloodCheck();
	void SetTextFloodLevelToTrigger(int n) { textFloodLevelToTrigger = n; }
	bool Run(unsigned playerId);
	void RemovePlayer(unsigned playerId);
	void CleanMsgTimesList();

private:
	VectorMap<unsigned, TextFloodInfos> msgTimesList;
	int textFloodLevelToTrigger = 5;
	TimeStop timer;
};

class UrlCheck
{
public:
	void SetUrlStrings(const Vector<String>& urls) { urlStrings <<= urls; }
	void SetUrlExceptionStrings(const Vector<String>& exceptions) { urlExceptionStrings <<= exceptions; }
	bool Run(const String& msg);

private:
	Vector<String> urlStrings;
	Vector<String> urlExceptionStrings;
};

struct ClientWarnInfos : Moveable<ClientWarnInfos> {
	String nick;
	int lastWarnType;
	int warnLevel;
};

struct ClientKickInfos : Moveable<ClientKickInfos> {
	int64 lastKickTimestamp;
	int kickNumber;
};

class MessageFilter
{
public:
	MessageFilter(class ConfigFile* c);
	~MessageFilter();

	struct Result {
		String action;
		String message;
	};

	Result Check(unsigned gameId, unsigned playerId, const String& nick, const String& msg);
	void RefreshConfig();
	void ProcessCleanup();

private:
	BadWordCheck myBadWordCheck;
	TextFloodCheck myTextFloodCheck;
	CapsFloodCheck myCapsFloodCheck;
	LetterRepeatingCheck myLetterRepeatingCheck;
	UrlCheck myUrlCheck;

	VectorMap<unsigned, ClientWarnInfos> myClientWarnLevelList;
	VectorMap<String, ClientKickInfos> myClientKickCounterList;

	int warnLevelToKick = 3;
	int kickNumberToBan = 3;

	class ConfigFile *config;
	TimeStop timer;
};

END_UPP_NAMESPACE

#endif
