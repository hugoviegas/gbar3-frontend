#ifndef SETTINGS_H
#define SETTINGS_H

struct GbaSettings {
	bool bottom;       // gbaScreen: false = top, true = bottom
	int brightness;    // gbaScreenBrightness: 1..16
	bool centerMask;   // enableCenterAndMask
};

void settingsLoad(GbaSettings& s);
bool settingsSave(const GbaSettings& s);

// Runs on the bottom screen console. Returns when the user leaves the menu.
void settingsMenu(void);

#endif
