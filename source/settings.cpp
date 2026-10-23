#include "settings.h"

#include <nds.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

static const char* const CONFIG_PATHS[] = { "/_gba/gbarunner3.json", "/gbarunner3.json" };

static std::string configPath() {
	for (const char* p : CONFIG_PATHS) {
		if (access(p, F_OK) == 0) return p;
	}
	return CONFIG_PATHS[0];
}

static bool readFile(const std::string& path, std::string& out) {
	FILE* f = fopen(path.c_str(), "rb");
	if (!f) return false;
	char buf[256];
	size_t n;
	out.clear();
	while ((n = fread(buf, 1, sizeof(buf), f)) > 0) out.append(buf, n);
	fclose(f);
	return true;
}

static bool writeFile(const std::string& path, const std::string& data) {
	FILE* f = fopen(path.c_str(), "wb");
	if (!f) return false;
	size_t w = fwrite(data.data(), 1, data.size(), f);
	fclose(f);
	return w == data.size();
}

static bool findValue(const std::string& j, const char* key, size_t& vs, size_t& ve) {
	std::string k = std::string("\"") + key + "\"";
	size_t p = j.find(k);
	if (p == std::string::npos) return false;
	p = j.find(':', p + k.size());
	if (p == std::string::npos) return false;
	p++;
	while (p < j.size() && isspace((unsigned char)j[p])) p++;
	vs = p;
	if (p < j.size() && j[p] == '"') {
		size_t q = j.find('"', p + 1);
		if (q == std::string::npos) return false;
		ve = q + 1;
	} else {
		while (p < j.size() && j[p] != ',' && j[p] != '}' && !isspace((unsigned char)j[p])) p++;
		ve = p;
	}
	return true;
}

static bool objectIsEmptyAfter(const std::string& j, size_t brace) {
	size_t n = brace + 1;
	while (n < j.size() && isspace((unsigned char)j[n])) n++;
	return n < j.size() && j[n] == '}';
}

static void setValue(std::string& j, const char* key, const std::string& value) {
	size_t vs, ve;
	if (findValue(j, key, vs, ve)) {
		j.replace(vs, ve - vs, value);
		return;
	}

	std::string entry = std::string("\"") + key + "\": " + value;

	size_t d = j.find("\"displaySettings\"");
	if (d != std::string::npos) {
		size_t b = j.find('{', d);
		if (b != std::string::npos) {
			j.insert(b + 1, "\n    " + entry + (objectIsEmptyAfter(j, b) ? "" : ","));
			return;
		}
	}

	size_t b = j.find('{');
	if (b == std::string::npos) {
		j = "{\n  \"displaySettings\": {\n    " + entry + "\n  }\n}\n";
		return;
	}
	j.insert(b + 1, "\n  \"displaySettings\": {\n    " + entry + "\n  }" + (objectIsEmptyAfter(j, b) ? "" : ","));
}

void settingsLoad(GbaSettings& s) {
	s.bottom = false;
	s.brightness = 16;
	s.centerMask = true;

	std::string j;
	if (!readFile(configPath(), j)) return;

	size_t vs, ve;
	if (findValue(j, "gbaScreen", vs, ve)) {
		std::string v = j.substr(vs, ve - vs);
		s.bottom = (strcasestr(v.c_str(), "bottom") != nullptr);
	}
	if (findValue(j, "gbaScreenBrightness", vs, ve)) {
		int b = atoi(j.substr(vs, ve - vs).c_str());
		if (b < 1) b = 1;
		if (b > 16) b = 16;
		s.brightness = b;
	}
	if (findValue(j, "enableCenterAndMask", vs, ve)) {
		s.centerMask = (j.compare(vs, 4, "true") == 0);
	}
}

bool settingsSave(const GbaSettings& s) {
	std::string path = configPath();
	std::string j;
	readFile(path, j);

	char num[16];
	snprintf(num, sizeof(num), "%d", s.brightness);

	setValue(j, "gbaScreen", s.bottom ? "\"bottom\"" : "\"top\"");
	setValue(j, "gbaScreenBrightness", num);
	setValue(j, "enableCenterAndMask", s.centerMask ? "true" : "false");

	mkdir("/_gba", 0777);
	return writeFile(path, j);
}

static void changeValue(GbaSettings& s, int idx, int dir, bool wrap) {
	if (idx == 0) {
		s.bottom = !s.bottom;
	} else if (idx == 1) {
		s.brightness += dir;
		if (s.brightness > 16) s.brightness = wrap ? 1 : 16;
		if (s.brightness < 1) s.brightness = wrap ? 16 : 1;
	} else {
		s.centerMask = !s.centerMask;
	}
}

static void drawMenu(const GbaSettings& s, int sel) {
	consoleClear();
	iprintf("DEFINICOES GBARUNNER3");
	iprintf("\x1b[1;0H------------------------------");

	const char* names[3] = { "Ecra da GBA", "Brilho", "Centrar/mascara" };
	char values[3][12];
	snprintf(values[0], sizeof(values[0]), "%s", s.bottom ? "Baixo" : "Cima");
	snprintf(values[1], sizeof(values[1]), "%d/16", s.brightness);
	snprintf(values[2], sizeof(values[2]), "%s", s.centerMask ? "Sim" : "Nao");

	for (int i = 0; i < 3; i++) {
		iprintf("\x1b[%d;0H%c %-15s %s", 3 + i, i == sel ? '*' : ' ', names[i], values[i]);
	}

	iprintf("\x1b[8;0H Cima/Baixo: escolher");
	iprintf("\x1b[9;0H Esq/Dir ou toque: mudar");
	iprintf("\x1b[10;0H R ou B: guardar e voltar");
}

void settingsMenu(void) {
	GbaSettings s;
	settingsLoad(s);

	int sel = 0;
	bool dirty = false;
	bool redraw = true;

	while (pmMainLoop()) {
		if (redraw) {
			drawMenu(s, sel);
			redraw = false;
		}

		swiWaitForVBlank();
		scanKeys();
		int k = keysDownRepeat();
		int d = keysDown();

		if (d & KEY_TOUCH) {
			touchPosition t;
			touchRead(&t);
			int idx = t.py / 8 - 1 - 3;
			if (idx >= 0 && idx < 3) {
				sel = idx;
				changeValue(s, idx, 1, true);
				dirty = true;
				redraw = true;
			}
		}

		if (k & KEY_UP) { sel = (sel + 2) % 3; redraw = true; }
		if (k & KEY_DOWN) { sel = (sel + 1) % 3; redraw = true; }
		if (k & KEY_RIGHT) { changeValue(s, sel, 1, false); dirty = true; redraw = true; }
		if (k & KEY_LEFT) { changeValue(s, sel, -1, false); dirty = true; redraw = true; }
		if (k & KEY_A) { changeValue(s, sel, 1, true); dirty = true; redraw = true; }

		if (k & (KEY_B | KEY_R | KEY_START)) break;
	}

	if (dirty && !settingsSave(s)) {
		consoleClear();
		iconsole_dummy_placeholder
	}

	consoleClear();
}
