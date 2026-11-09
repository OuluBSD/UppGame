#include "ChatFilters.h"
#include <EditorCommon/ConfigFile.h>

NAMESPACE_UPP

bool BadWordCheck::Run(const String& msg)
{
	String lmsg = ToLower(msg);
	bool bad = false;
	for (const String& bw : badWords) {
		if (lmsg.Find(bw) >= 0) {
			bad = true;
			for (const String& bwe : badWordsException) {
				if (bwe.Find(bw) >= 0 && lmsg.Find(bwe) >= 0) {
					bad = false;
					break;
				}
			}
			if (bad) return true;
		}
	}
	return false;
}

bool CapsFloodCheck::Run(const String& msg)
{
	if (capsNumberToTrigger <= 0) return false;
	String s = msg;
	s.Replace(" ", "");
	int count = 0;
	for (int i = 0; i < s.GetLength(); i++) {
		if (IsUpper(s[i])) {
			count++;
			if (count >= capsNumberToTrigger) return true;
		} else {
			count = 0;
		}
	}
	return false;
}

bool LetterRepeatingCheck::Run(const String& msg)
{
	if (letterNumberToTrigger <= 1) return false;
	String s = msg;
	s.Replace(" ", "");
	if (s.IsEmpty()) return false;
	int count = 1;
	for (int i = 1; i < s.GetLength(); i++) {
		if (s[i] == s[i-1]) {
			count++;
			if (count >= letterNumberToTrigger) return true;
		} else {
			count = 1;
		}
	}
	return false;
}

TextFloodCheck::TextFloodCheck()
{
	timer.Reset();
}

bool TextFloodCheck::Run(unsigned playerId)
{
	CleanMsgTimesList();
	int64 now = timer.Elapsed();
	int i = msgTimesList.Find(playerId);
	if (i < 0) {
		TextFloodInfos info;
		info.floodLevel = 0;
		info.timeStamp = now;
		msgTimesList.Add(playerId, info);
	} else {
		TextFloodInfos& info = msgTimesList[i];
		if (now - info.timeStamp <= 1000) {
			if (info.floodLevel >= textFloodLevelToTrigger) {
				info.floodLevel--;
				info.timeStamp = now;
				return true;
			} else {
				info.floodLevel++;
			}
		}
		info.timeStamp = now;
	}
	return false;
}

void TextFloodCheck::RemovePlayer(unsigned playerId)
{
	msgTimesList.RemoveKey(playerId);
}

void TextFloodCheck::CleanMsgTimesList()
{
	int64 now = timer.Elapsed();
	for (int i = 0; i < msgTimesList.GetCount(); i++) {
		if (now - msgTimesList[i].timeStamp > 3000) {
			if (msgTimesList[i].floodLevel <= 0) {
				msgTimesList.Remove(i);
				i--;
			} else {
				msgTimesList[i].floodLevel--;
			}
		}
	}
}

bool UrlCheck::Run(const String& msg)
{
	String lmsg = ToLower(msg);
	for (const String& url : urlStrings) {
		if (lmsg.Find(url) >= 0) {
			bool exception = false;
			for (const String& bwe : urlExceptionStrings) {
				if (lmsg.Find(bwe) >= 0) {
					exception = true;
					break;
				}
			}
			if (!exception) return true;
		}
	}
	return false;
}

enum ActionType { NOTHING, WARN, KICK, KICKBAN, MUTE };
enum OffenceType { OFFENCE_NONE, BAD_WORD, TEXT_FLOOD_LINES, CAPS_FLOOD, LETTER_REPEATING, URL };

MessageFilter::MessageFilter(class ConfigFile *c) : config(c) { timer.Reset(); }
MessageFilter::~MessageFilter() {}

MessageFilter::Result MessageFilter::Check(unsigned gameId, unsigned playerId, const String& nick, const String& msg)
{
	Result res;
	OffenceType offence = OFFENCE_NONE;

	if(myBadWordCheck.Run(msg)) offence = BAD_WORD;
	else if(myCapsFloodCheck.Run(msg)) offence = CAPS_FLOOD;
	else if(myLetterRepeatingCheck.Run(msg)) offence = LETTER_REPEATING;
	else if(myUrlCheck.Run(msg)) offence = URL;
	else if(myTextFloodCheck.Run(playerId)) offence = TEXT_FLOOD_LINES;

	if(offence != OFFENCE_NONE) {
		ActionType action = NOTHING;
		int i = myClientWarnLevelList.Find(playerId);

		if(i < 0) {
			ClientWarnInfos info;
			info.warnLevel = 1;
			info.lastWarnType = offence;
			info.nick = nick;
			myClientWarnLevelList.Add(playerId, info);
			action = WARN;
		} else {
			ClientWarnInfos& info = myClientWarnLevelList[i];
			if(info.warnLevel == warnLevelToKick || info.lastWarnType == offence) {
				if(gameId != 0) {
					action = MUTE;
					myTextFloodCheck.RemovePlayer(playerId);
					myClientWarnLevelList.Remove(i);
				} else {
					action = KICK;
					myTextFloodCheck.RemovePlayer(playerId);
					myClientWarnLevelList.Remove(i);
					int j = myClientKickCounterList.Find(nick);
					if(j < 0) {
						ClientKickInfos kinfo;
						kinfo.kickNumber = 1;
						kinfo.lastKickTimestamp = timer.Elapsed();
						myClientKickCounterList.Add(nick, kinfo);
					} else {
						ClientKickInfos& kinfo = myClientKickCounterList[j];
						if(kinfo.kickNumber >= kickNumberToBan) {
							action = KICKBAN;
							myClientKickCounterList.Remove(j);
						} else {
							kinfo.kickNumber++;
							kinfo.lastKickTimestamp = timer.Elapsed();
						}
					}
				}
			} else {
				info.warnLevel++;
				info.lastWarnType = offence;
				info.nick = nick;
				action = WARN;
			}
		}

		if(action == WARN) {
			res.action = "warn";
			switch(offence) {
				case BAD_WORD: res.message = nick + ": Warning! No racial, religious, sexually inflammatory or otherwise insulting language\n"; break;
				case TEXT_FLOOD_LINES: res.message = nick + ": Warning! You've triggered text flood (lines) protection, slow down your typing!\n"; break;
				case CAPS_FLOOD: res.message = nick + ": Warning: You've triggered caps flood protection, release your caps!\n"; break;
				case LETTER_REPEATING: res.message = nick + ": Warning: You've triggered letter repeating protection, stop repeating!\n"; break;
				case URL: res.message = nick + ": Warning: You've triggered url spam protection, stop posting urls!\n"; break;
				default: break;
			}
		} else if(action == KICK) {
			res.action = "kick";
			res.message = nick + " kicked! Please respect the table chat rules.\n";
		} else if(action == KICKBAN) {
			res.action = "kickban";
			res.message = nick + " kicked and banned! Please respect the table chat rules.\n";
		} else if(action == MUTE) {
			res.action = "mute";
			res.message = nick + " muted! Please respect the table chat rules.\n";
		}
	}
	return res;
}

void MessageFilter::RefreshConfig()
{
	if (!config) return;
	warnLevelToKick = config->readConfigInt("WarnLevelToKick");
	kickNumberToBan = config->readConfigInt("KickNumberToBan");

	myBadWordCheck.SetBadWords(config->readConfigStringList("BadWordsList"));
	myBadWordCheck.SetBadWordsException(config->readConfigStringList("BadWordsException"));
	myUrlCheck.SetUrlStrings(config->readConfigStringList("UrlStringsList"));
	myUrlCheck.SetUrlExceptionStrings(config->readConfigStringList("UrlExceptionStringsList"));

	myTextFloodCheck.SetTextFloodLevelToTrigger(config->readConfigInt("TextFloodLevelToTrigger"));
	myCapsFloodCheck.SetCapsNumberToTrigger(config->readConfigInt("CapsFloodCapsNumberToTrigger"));
	myLetterRepeatingCheck.SetLetterNumberToTrigger(config->readConfigInt("LetterRepeatingNumberToTrigger"));
}

void MessageFilter::ProcessCleanup()
{
	int64 now = timer.Elapsed();
	int64 forgetSec = (int64)config->readConfigInt("SecondsToForgetAboutKick") * 1000;
	for (int i = 0; i < myClientKickCounterList.GetCount(); i++) {
		if (now - myClientKickCounterList[i].lastKickTimestamp > forgetSec) {
			myClientKickCounterList.Remove(i);
			i--;
		}
	}
}

END_UPP_NAMESPACE
