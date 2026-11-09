#include <Poker/TexasGen.h>
#include <Poker/TexasRenderer.h>
#include <Core/Core.h>
#include <plugin/png/png.h>
#include <Painter/Painter.h>

namespace Upp {

int RunTexasDatasetGeneration() {
	String root = AppendFileName(GetCurrentDirectory(), "bin/data/gfx/dataset/texas_renderer");
	String rank_root = AppendFileName(root, "rank");
	String suit_root = AppendFileName(root, "suit");
	RealizeDirectory(rank_root);
	RealizeDirectory(suit_root);
	
	Vector<String> suits, ranks;
	suits << "S" << "H" << "D" << "C";
	ranks << "2" << "3" << "4" << "5" << "6" << "7" << "8" << "9" << "T" << "J" << "Q" << "K" << "A";
	
	Rect sr(2, 2, 22, 54); // Symbol area
	
	Cout() << "Generating dataset in " << root << "...\n";
	
	for (int s = 0; s < 4; s++) {
		for (int r = 0; r < 13; r++) {
			int suit_map[] = { 26, 13, 0, 39 }; // offset for S, H, D, C
			int card_id = suit_map[s] + r;
			
			Image asset = TexasRenderer::GetCardImage(card_id);
			if (asset.IsEmpty()) continue;
			
			ImageBuffer ib(20, 32);
			BufferPainter bp(ib);
			bp.Begin();
			bp.DrawImage(0, 0, 20, 32, asset, sr);
			bp.End();
			Image final_img = ib;
			
			String name = ranks[r] + "_" + suits[s] + ".png";
			PNGEncoder().SaveFile(AppendFileName(rank_root, name), final_img);
			
			String sname = suits[s] + "_" + ranks[r] + ".png";
			PNGEncoder().SaveFile(AppendFileName(suit_root, sname), final_img);
		}
	}
	Cout() << "Done.\n";
	return 0;
}

}
