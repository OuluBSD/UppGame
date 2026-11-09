#ifndef _Platformer2D_Audio_OPL_h_
#define _Platformer2D_Audio_OPL_h_

#include "Audio.h"


// ============================================================================
// DBOPL Emulator (DOSBox OPL2)
// ============================================================================

class DBOPL : public OPLEmulator
{
public:
	DBOPL();
	~DBOPL();
	
	void Reset() override;
	void WriteReg(uint8_t reg, uint8_t val) override;
	void Generate(int16_t* buffer, int frames) override;
	void SetSampleRate(int rate) override;
	const char* GetName() const override { return "DBOPL"; }
	
private:
	// OPL2 state (matching DOSBox DBOPL)
	struct Channel
	{
		// Operator 1 (modulator)
		uint8_t op1_AM, op1_VIB, op1_EGT, op1_KSR, op1_MUL;
		uint8_t op1_KSL, op1_TL, op1_AR, op1_DR, op1_SL, op1_RR, op1_WS;
		// Operator 2 (carrier)
		uint8_t op2_AM, op2_VIB, op2_EGT, op2_KSR, op2_MUL;
		uint8_t op2_KSL, op2_TL, op2_AR, op2_DR, op2_SL, op2_RR, op2_WS;
		// Channel
		uint8_t ksl_base, freq_low, freq_high;
		uint8_t fb_conn;
		
		// Runtime state
		int32_t phase[2];
		int32_t phase_inc[2];
		int32_t env_phase[2];
		uint8_t env_state[2];
		int32_t tremolo_phase;
		int32_t vibrato_phase;
	};
	
	Channel channels[OPL_CHANNELS];
	uint8_t rhythm_reg;
	int sampleRate;
	
	// Envelope generator
	void UpdateChannel(int ch);
	int32_t CalcPhaseInc(int freq, int block, int mul);
	void GenerateChannel(int ch, int16_t* buffer, int frames);
};

// ============================================================================
// Nuked OPL3 Emulator (wrapper around nuked_opl3.c)
// ============================================================================

class NukedOPL3 : public OPLEmulator
{
public:
	NukedOPL3();
	~NukedOPL3();
	
	void Reset() override;
	void WriteReg(uint8_t reg, uint8_t val) override;
	void WriteReg16(uint16_t reg, uint8_t val); // OPL3 extended regs
	void Generate(int16_t* buffer, int frames) override;
	void SetSampleRate(int rate) override;
	const char* GetName() const override { return "NukedOPL3"; }
	
private:
	// Opaque pointer to nuked_opl3 context
	void* opl3_ctx = nullptr;
	int sampleRate;
};


#endif // _Platformer2D_Audio_OPL_h_