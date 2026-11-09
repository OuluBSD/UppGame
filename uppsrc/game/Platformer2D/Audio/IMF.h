#ifndef _Platformer2D_Audio_IMF_h_
#define _Platformer2D_Audio_IMF_h_

#include "Audio.h"


// ============================================================================
// IMFPlayer Implementation
// ============================================================================

inline IMFPlayer::IMFPlayer(OPLEmulator* opl_) : opl(opl_)
{
}

inline IMFPlayer::~IMFPlayer()
{
}

inline bool IMFPlayer::Load(const uint8_t* data, int length)
{
	commands.Clear();
	cmdIndex = 0;
	nextDelay = 0;
	
	// IMF format: [delay_low][delay_high][reg][val]... terminated by 0x00 0x00
	const uint8_t* ptr = data;
	const uint8_t* end = data + length;
	
	while (ptr + 3 < end)
	{
		uint16_t delay = ptr[0] | (ptr[1] << 8);
		if (delay == 0 && ptr[2] == 0 && ptr[3] == 0)
			break; // End marker
		
		IMFCommand cmd;
		cmd.delay = delay;
		cmd.reg = ptr[2];
		cmd.val = ptr[3];
		commands.Add(cmd);
		ptr += 4;
	}
	
	return !commands.IsEmpty();
}

inline void IMFPlayer::Play(bool loop_)
{
	loop = loop_;
	playing = true;
	paused = false;
	cmdIndex = 0;
	nextDelay = commands.GetCount() > 0 ? commands[0].delay : 0;
}

inline void IMFPlayer::Stop()
{
	playing = false;
	paused = false;
	cmdIndex = 0;
}

inline void IMFPlayer::Pause(bool pause_)
{
	paused = pause_;
}

inline void IMFPlayer::Tick()
{
	if (!playing || paused || commands.IsEmpty()) return;
	
	if (nextDelay > 0)
	{
		nextDelay--;
		return;
	}
	
	// Execute commands with zero delay
	while (cmdIndex < commands.GetCount() && commands[cmdIndex].delay == 0)
	{
		opl->WriteReg(commands[cmdIndex].reg, commands[cmdIndex].val);
		cmdIndex++;
	}
	
	if (cmdIndex >= commands.GetCount())
	{
		if (loop)
		{
			cmdIndex = 0;
			nextDelay = commands[0].delay;
		}
		else
		{
			playing = false;
		}
	}
	else
	{
		nextDelay = commands[cmdIndex].delay;
	}
}


#endif // _Platformer2D_Audio_IMF_h_