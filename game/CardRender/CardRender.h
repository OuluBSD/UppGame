#ifndef _game_CardRender_CardRender_h_
#define _game_CardRender_CardRender_h_

#include <Draw/Draw.h>

NAMESPACE_UPP

Image LoadCardArt(const String& file_name, Size target_size = Size(), const String& theme = "default");
Image FitCardArt(const Image& img, Size target_size);
Image RotateCardArt(const Image& img, int rotation_deg);

END_UPP_NAMESPACE

#endif
