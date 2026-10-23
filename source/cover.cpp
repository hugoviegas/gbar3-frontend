#include "cover.h"

#include <nds.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <string>

static u16* gfx = nullptr;
static std::string lastKey;

static const u16 BACKDROP = RGB15(2, 3, 7) | BIT(15);

static void fillScreen(u16 color) {
	for (int i = 0; i < 256 * 256; i++) gfx[i] = color;
}

void coverInit(void) {
	lcdMainOnTop();
	videoSetMode(MODE_5_2D);
	vramSetBankA(VRAM_A_MAIN_BG);
	int bg = bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
	gfx = bgGetGfxPtr(bg);
	fillScreen(BACKDROP);
}

static bool loadBmp(const std::string& path) {
	FILE* f = fopen(path.c_str(), "rb");
	if (!f) return false;

	uint8_t h[54];
	if (fread(h, 1, 54, f) != 54 || h[0] != 'B' || h[1] != 'M') {
		fclose(f);
		return false;
	}

	auto rd32 = [&](int o) -> uint32_t {
		return (uint32_t)h[o] | ((uint32_t)h[o + 1] << 8) | ((uint32_t)h[o + 2] << 16) | ((uint32_t)h[o + 3] << 24);
	};

	uint32_t dataOffset = rd32(10);
	int32_t w = (int32_t)rd32(18);
	int32_t ht = (int32_t)rd32(22);
	uint16_t bpp = (uint16_t)(h[28] | (h[29] << 8));
	uint32_t compression = rd32(30);

	bool topDown = ht < 0;
	if (topDown) ht = -ht;

	if (bpp != 24 || compression != 0 || w <= 0 || w > 256 || ht <= 0 || ht > 192) {
		fclose(f);
		return false;
	}

	fillScreen(BACKDROP);

	int xo = (256 - w) / 2;
	int yo = (192 - ht) / 2;
	uint32_t stride = ((uint32_t)w * 3 + 3) & ~3u;
	static uint8_t row[256 * 3 + 4];

	for (int y = 0; y < ht; y++) {
		uint32_t srcRow = topDown ? (uint32_t)y : (uint32_t)(ht - 1 - y);
		fseek(f, dataOffset + srcRow * stride, SEEK_SET);
		if (fread(row, 1, stride, f) != stride) break;
		u16* dst = gfx + (yo + y) * 256 + xo;
		for (int x = 0; x < w; x++) {
			dst[x] = RGB15(row[x * 3 + 2] >> 3, row[x * 3 + 1] >> 3, row[x * 3] >> 3) | BIT(15);
		}
	}

	fclose(f);
	return true;
}

void coverUpdate(bool isDir, const std::string& name) {
	std::string key = isDir ? std::string() : name;
	if (key == lastKey) return;
	lastKey = key;

	if (isDir || name.empty()) {
		fillScreen(BACKDROP);
		return;
	}

	size_t dot = name.rfind('.');
	std::string base = (dot == std::string::npos) ? name : name.substr(0, dot);

	if (!loadBmp("/_gba/covers/" + base + ".bmp")) {
		fillScreen(BACKDROP);
	}
}
