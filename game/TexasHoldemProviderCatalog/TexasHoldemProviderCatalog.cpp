#include "TexasHoldemProviderCatalog.h"

#include <initializer_list>

NAMESPACE_UPP

static String NormalizeTexasHoldemProviderKey(const String& provider)
{
	String key = ToLower(TrimBoth(provider));
	key.Replace("_", "");
	key.Replace("-", "");
	key.Replace(" ", "");
	return key;
}

static TexasHoldemProviderInfo MakeTexasHoldemProvider(const char *id, const char *label,
                                                       const char *form_file,
                                                       const char *layout_profile,
                                                       const char *table_profile,
                                                       int default_players,
                                                       std::initializer_list<const char *> aliases)
{
	TexasHoldemProviderInfo info;
	info.id = id;
	info.label = label;
	info.form_file = form_file;
	info.layout_profile = layout_profile;
	info.table_profile = table_profile;
	info.default_players = default_players;
	for(const char *alias : aliases)
		info.aliases.Add(alias);
	return info;
}

const Vector<TexasHoldemProviderInfo>& TexasHoldemListProviders()
{
	static Vector<TexasHoldemProviderInfo> providers;
	ONCELOCK {
		providers.Add(MakeTexasHoldemProvider("Original", "Original", "GameTable.form",
		                                      "texas-holdem", "texas-holdem-classic", 10,
		                                      {"original"}));
		providers.Add(MakeTexasHoldemProvider("PS_6p", "PS 6-player", "GameTable_PS_6p.form",
		                                      "ps-6p", "ps-6p", 6,
		                                      {"ps6p", "ps_6p", "ps-6p", "ps 6-player", "pokerstars-6p"}));
		providers.Add(MakeTexasHoldemProvider("Classic", "Classic", "GameTable.form",
		                                      "texas-holdem", "texas-holdem-classic", 10,
		                                      {"classic"}));
		providers.Add(MakeTexasHoldemProvider("Minimal", "Minimal", "GameTable.form",
		                                      "texas-holdem", "texas-holdem-classic", 10,
		                                      {"minimal"}));
	}
	return providers;
}

const TexasHoldemProviderInfo *TexasHoldemFindProvider(const String& provider)
{
	String key = NormalizeTexasHoldemProviderKey(provider);
	if(key.IsEmpty())
		key = NormalizeTexasHoldemProviderKey(TexasHoldemDefaultProviderId());
	for(const TexasHoldemProviderInfo& info : TexasHoldemListProviders()) {
		if(NormalizeTexasHoldemProviderKey(info.id) == key)
			return &info;
		for(const String& alias : info.aliases)
			if(NormalizeTexasHoldemProviderKey(alias) == key)
				return &info;
	}
	return nullptr;
}

String TexasHoldemCanonicalProvider(const String& provider)
{
	const TexasHoldemProviderInfo *info = TexasHoldemFindProvider(provider);
	return info ? info->id : String();
}

bool TexasHoldemIsKnownProvider(const String& provider)
{
	return TexasHoldemFindProvider(provider);
}

String TexasHoldemDefaultProviderId()
{
	return "PS_6p";
}

END_UPP_NAMESPACE
