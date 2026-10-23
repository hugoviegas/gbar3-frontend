#ifndef COVER_H
#define COVER_H

#include <string>

// Top screen: shows /_gba/covers/<rom name without extension>.bmp
// BMP must be uncompressed 24-bit, up to 256x192 pixels.
void coverInit(void);
void coverUpdate(bool isDir, const std::string& name);

#endif
