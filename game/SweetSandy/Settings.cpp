#include "SweetSandy.h"

NAMESPACE_UPP

static AppRegistry& Registry()
{
	static AppRegistry reg;
	static bool init = false;
	if(!init) {
		reg.Vendor("AiUpp").AppId("SweetSandy").Profile("default");
		reg.Load();
		init = true;
	}
	return reg;
}

KeyBindings& GetKeyBindings()
{
	static KeyBindings kb = [] {
		KeyBindings k;
		AppRegistry& reg = Registry();
		k.throttle    = (dword)(int)reg.Get("input.key.throttle", (int)k.throttle);
		k.brake       = (dword)(int)reg.Get("input.key.brake", (int)k.brake);
		k.steer_left  = (dword)(int)reg.Get("input.key.steer_left", (int)k.steer_left);
		k.steer_right = (dword)(int)reg.Get("input.key.steer_right", (int)k.steer_right);
		k.back        = (dword)(int)reg.Get("input.key.back", (int)k.back);
		return k;
	}();
	return kb;
}

void SaveKeyBindings()
{
	const KeyBindings& k = GetKeyBindings();
	AppRegistry& reg = Registry();
	reg.Set("input.key.throttle", (int)k.throttle);
	reg.Set("input.key.brake", (int)k.brake);
	reg.Set("input.key.steer_left", (int)k.steer_left);
	reg.Set("input.key.steer_right", (int)k.steer_right);
	reg.Set("input.key.back", (int)k.back);
	if(!reg.Save())
		LOG("SaveKeyBindings: AppRegistry::Save failed");
}

String KeyName(dword key)
{
	switch(key) {
	case K_UP:     return "Up";
	case K_DOWN:   return "Down";
	case K_LEFT:   return "Left";
	case K_RIGHT:  return "Right";
	case K_ESCAPE: return "Esc";
	case K_SPACE:  return "Space";
	case K_ENTER:  return "Enter";
	}
	if(key >= 32 && key < 127)
		return String(ToUpper((int)key), 1);
	return Format("Key %d", (int)key);
}

END_UPP_NAMESPACE
