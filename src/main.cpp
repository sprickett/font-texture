#include <ft2build.h>
#include FT_FREETYPE_H

#include <iostream>
#include <vector>
#include <algorithm>
#include <sstream>

#include "bitmap.hpp"
#include "box_packing.hpp"
#include "pixmap.hpp"


void flipy(PixMap& bmp)
{
	int w = bmp.width()*bmp.step();
	int h = bmp.height() / 2;
	int ymx = bmp.height() - 1;

	for (int y = 0; y < h; ++y)
	{
		auto p = bmp.ptr(y);
		auto q = bmp.ptr(ymx-y);
		for (int x = 0; x < w; ++x)
			std::swap(p[x], q[x]);
	}
}
void transpose(PixMap src, PixMap& tgt)
{
	tgt.create(src.height(), src.width(), src.step());
	if (src.isOverlapping(tgt))
		src = src.clone();
	for (int y = 0; y < src.height(); ++y)
	{
		const uint8* ps = src.ptr(y);
		const uint8* pse = ps + src.width()*src.step();		
		uint8* pt = tgt.ptr(0,y);
		while (ps < pse)
		{	
			switch (src.step())
			{
			case 8: *pt = *ps; ++pt; ++ps;
			case 7: *pt = *ps; ++pt; ++ps;
			case 6: *pt = *ps; ++pt; ++ps;
			case 5: *pt = *ps; ++pt; ++ps;
			case 4: *pt = *ps; ++pt; ++ps;
			case 3: *pt = *ps; ++pt; ++ps;
			case 2: *pt = *ps; ++pt; ++ps;
			case 1: *pt = *ps; ++pt; ++ps;
				break;
			default:
				for(int i=0;i< src.step();++i)
				{ 
					*pt = *ps; ++pt; ++ps;
				}
			}
		}
	}
}
 
struct XY
{
	float x, y;
};
struct LayoutAdvance
{
	XY bearing;
	XY advance;	
};
struct GlyphMetrics
{
	XY size;
	LayoutAdvance horizontal;
	LayoutAdvance vertical;
};
struct Glyph
{
	unsigned char_code;
	GlyphMetrics metrics;
	PixMap bitmap;
};


void show(const PixMap& p)
{
	if (p.step() != 1)
		return;
	int W = p.width();
	int H = p.height();
	std::cout << std::hex;
	for (int y = 0; y < H; ++y)
	{
		const uint8 *R = p.ptr(y);
		for (int x = 0; x < W; ++x)
			std::cout << (int(R[x])>>4) ;
		std::cout << "\n";
	}
	std::cout << std::dec;
	
}
void show(const FT_GlyphSlot& slot)
{
	printf("glyph slot:\n");
	printf("  advance: %d %d\n", slot->advance.x, slot->advance.y);
	printf("  linearHoriAdvance: %d\n", slot->linearHoriAdvance);
	printf("  linearVertAdvance: %d\n", slot->linearVertAdvance);
	printf("  metrics:\n");
	printf("    size: %d %d\n", slot->metrics.width, slot->metrics.height);
	printf("    horiAdvance: %d\n", slot->metrics.horiAdvance);
	printf("    vertAdvance: %d\n", slot->metrics.vertAdvance);
	printf("  bitmap_left: %d\n", slot->bitmap_left);
	printf("  bitmap_top: %d\n", slot->bitmap_top);
	printf("  bitmap:\n");
	printf("    width: %d\n", slot->bitmap.width);
	printf("    rows: %d\n", slot->bitmap.rows);
	printf("    pixel mode: %d\n", slot->bitmap.pixel_mode);
}
std::ostream& operator<<(std::ostream& os, const XY& v)
{
	os << '{' << v.x << ", " << v.y << '}';
	return os;
}
std::ostream& operator<<(std::ostream& os, const LayoutAdvance& a)
{
	os << '{' << a.bearing <<  ", "<< a.advance << '}';
	return os;
}
std::ostream& operator<<(std::ostream& os, const FT_Encoding& e)
{
	os << char(e>>24) << char(e >> 16) << char(e >> 8) << char(e);
	return os;
}
std::ostream& operator<<(std::ostream& os, const GlyphMetrics& m)
{
	
	os << "glyph metrics:\n"
		<< "  size: " << m.size << "\n" 
	    << "  hori: " << m.horizontal << "\n"
	    << "  vert: " << m.vertical << "\n";
	return os;
}
void showCharMap(const FT_CharMap& cm)
{
	std::cout
		<< cm->encoding << ", "
		<< cm->platform_id << ", "
		<< cm->encoding_id << "\n";
}
void showCharMaps(const FT_Face& f)
{
	FT_CharMap* cm = f->charmaps;
	int ct = f->num_charmaps;
	for (int i=0; i<ct;++i)
		showCharMap(cm[i]);
}
void showFaces(const FT_Face& f)
{
	std::cout << "num faces: " << f->num_faces << "\n";
	std::cout << "num fixed sizes: " << f->num_fixed_sizes << "\n";
	std::cout << "num glyphs: " << f->num_glyphs << "\n";
	std::cout << "family name: " << f->family_name<< "\n";
	std::cout << "char map: ";
	showCharMap(f->charmap);
}
void showAvailableCharacters(const FT_Face& face)
{
	FT_UInt i;
	FT_ULong c = FT_Get_First_Char(face, &i);
	while (i)
	{
		std::cout << i << ", 0x" << std::hex << c << std::dec << " (" << c << ")\n";
		c = FT_Get_Next_Char(face, c, &i);
	}
}


int main(int argc, char** argv)
{
	FT_Library library;
	FT_Face face;
	FT_GlyphSlot slot;             
	FT_UInt index;
	FT_ULong char_code;
	FT_Error error;
	char* filename;
	

	if (argc < 2)
	{
		fprintf(stderr, "usage: %s font sample-text\n", argv[0]);
		exit(1);
	}

	filename = argv[1];                                 

	error = FT_Init_FreeType(&library);      
	if (error)
	{
		printf("FT_Init_FreeType failed %d\n", error);
		return 0;
	}

	error = FT_New_Face(library, filename, 0, &face);/* create face object */
	if (error)
	{
		printf("FT_New_Face failed\n");
		FT_Done_FreeType(library);
		return 0;
	}


	 
	constexpr auto PT = 96;
	error = FT_Set_Char_Size(face, PT * 64, 0,	100, 0);           
	if (error)
		printf("FT_Set_Char_Size failed\n");

	//showCharMaps(face);
	//showFaces(face);
	//showAvailableCharacters(face);

	slot = face->glyph;

	std::vector<Glyph> glyphs;
	glyphs.reserve(face->num_glyphs);

	for (char_code = FT_Get_First_Char(face, &index); index; char_code = FT_Get_Next_Char(face, char_code, &index))
	{
		error = FT_Load_Glyph(face, index, FT_LOAD_RENDER);
		if (error)
			continue;

		const FT_Bitmap& b = slot->bitmap;
		if (b.pixel_mode != FT_PIXEL_MODE_GRAY)
			continue;

		glyphs.emplace_back(Glyph());
		Glyph& g = glyphs.back();

		g.char_code = char_code;
		g.bitmap = PixMap(b.width, b.rows, 1, b.buffer, 0, true);			

		float s = 1.f / 64;	
		GlyphMetrics& m = g.metrics;
		const FT_Glyph_Metrics& fm = slot->metrics;		
		m.size.x = fm.width*s;
		m.size.y = fm.height*s;
		m.horizontal.bearing.x = fm.horiBearingX*s;
		m.horizontal.bearing.y = fm.horiBearingY*s;
		m.horizontal.advance.x = fm.horiAdvance*s;
		m.horizontal.advance.y = 0;
		m.vertical.bearing.x = fm.vertBearingX*s;
		m.vertical.bearing.y = fm.vertBearingY*s;
		m.vertical.advance.x = 0;
		m.vertical.advance.y = fm.vertAdvance*s;
	}


	FT_Done_Face(face);
	FT_Done_FreeType(library);
	
	//int ct = 0;
	//for (auto& g : glyphs)
	//	std::cout << ct++ << " " << g.char_code << " " << g.metrics.size << "\n";
	
	std::vector<Size> sizes(glyphs.size());
	std::transform(glyphs.begin(), glyphs.end(), sizes.begin(),
			[](const Glyph& g) {return Size(g.bitmap.width(), g.bitmap.height()); });

	size_t total_area = 0;
	for (auto& sz : sizes)
		total_area += sz.width*sz.height;

	// find a power two size big enough for area
	size_t side = 1, check = (total_area *20)/19;
	while (side*side < check )
		side <<= 1;
	PixMap bmp(side, side, 1);
	std::cout << side << "*" << side << " " << side * side << " (" << total_area << ")\n";

	int ct = 0;
	BoxPacker bp(side, side, false);
	bp.insert(sizes.begin(), sizes.end());
	bp.pack( [&](const std::vector<BoxPacker::Packing>& solution) {
		bmp.setTo(0);
		for (auto& p : solution)
		{
			auto& r = p.second;
			//auto& g = glyphs[p.first];
			//g.bitmap.copyTo(bmp(r.x, r.y, g.bitmap.width(), g.bitmap.height()));
			bmp(r.x, r.y, r.width, r.height).setTo(-1);

		}
		std::cout<< "bam\n";
		flipy(bmp);
		int w = bmp.width();
		int h = bmp.height();
		std::stringstream ss;
		ss << "font" << ct << ".bmp";
		save_bitmap(ss.str().c_str(), bmp.ptr(), w, h, w, 8);
		return ++ct >= 3;
	});
	//flipy(bmp);
	//if(!save_bitmap("font.bmp", bmp.ptr(),  bmp.width(), bmp.height(), bmp.width(), 8))
	//	std::cout << "saved bitmap failed\n";
	return 0;
}

/* EOF */
