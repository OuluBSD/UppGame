#include "Adventure.h"

namespace Adventure {






/*
void ProgramDraw::LoadBuiltinGfxStr(const char* s, Image& out) {
	if (*s == '\n') s++;
	
	int line = 0;
	int max_col = 0;
	int col = 0;
	while (1) {
		int chr = *s++;
		
		if (chr == '\n') {
			line++;
			ASSERT(max_col == 0 || col == max_col);
			ASSERT(col > 0);
			max_col = max(max_col, col);
			col = -1;
		}
		else if (chr >= 'a' && chr <= 'f') {
			int n = chr - 'a' + 10;
			Color c = GetPaletteColor(n);
			RGBA r = c;
			data.Add(r);
		}
		else if (chr >= '0' && chr <= '9') {
			int n = chr - '0';
			RGBA r;
			if (n > 0)
				r = GetPaletteColor(n);
			else {
				r.r = 0;
				r.g = 0;
				r.b = 0;
				r.a = 0;
			}
			data.Add(r);
		}
		else if (!chr)
			break;
		else Panic("Invalid data");
		
		col++;
	}
	
}
*/
void Program::LoadBuiltinGfxStr(const char* s, Vector<byte>& out, Size& sz) {
	if (*s == '\n') s++;
	
	out.SetCount(0);
	out.Reserve(10000);
	
	int line = 0;
	int max_col = 0;
	int col = 0;
	while (1) {
		int chr0 = *s++;
		
		if (chr0 == '\n') {
			line++;
			ASSERT(max_col == 0 || col == max_col);
			ASSERT(col > 0);
			max_col = max(max_col, col);
			col = 0;
		}
		else if (chr0 == 0)
			break;
		else {
			int n = ctoi(chr0);
			out.Add(n);
			
			col++;
		}
	}
	
	sz = Size(max_col, line);
}

void Program::LoadBuiltinGfxStr(const char* s, Vector<uint16>& out, Size& sz) {
	if (*s == '\n') s++;
	
	out.SetCount(0);
	out.Reserve(10000);
	
	int line = 0;
	int max_col = 0;
	int col = 0;
	while (1) {
		int chr0 = *s++;
		
		if (chr0 == '\n') {
			line++;
			ASSERT(max_col == 0 || col == max_col);
			ASSERT(col > 0);
			max_col = max(max_col, col);
			col = 0;
		}
		else if (chr0 == 0)
			break;
		else {
			int chr1 = *s++;
			
			int n0 = ctoi(chr0);
			int n1 = ctoi(chr1);
			ASSERT(n0 >= 0 && n0 < 0x10);
			ASSERT(n1 >= 0 && n1 < 0x10);
			int n = (n0 << 4) | n1;
			out.Add(n);
			
			col++;
		}
	}
	
	sz = Size(max_col, line);
}



}
