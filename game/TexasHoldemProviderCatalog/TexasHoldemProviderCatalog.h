#ifndef _TexasHoldemProviderCatalog_TexasHoldemProviderCatalog_h_
#define _TexasHoldemProviderCatalog_TexasHoldemProviderCatalog_h_

#include <Core/Core.h>

NAMESPACE_UPP

struct TexasHoldemProviderInfo : Moveable<TexasHoldemProviderInfo> {
	String id;
	String label;
	String form_file;
	String layout_profile;
	String table_profile;
	int default_players = 10;
	Vector<String> aliases;
};

const Vector<TexasHoldemProviderInfo>& TexasHoldemListProviders();
const TexasHoldemProviderInfo *TexasHoldemFindProvider(const String& provider);
String TexasHoldemCanonicalProvider(const String& provider);
bool TexasHoldemIsKnownProvider(const String& provider);
String TexasHoldemDefaultProviderId();

END_UPP_NAMESPACE

#endif
