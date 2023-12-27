#include "Color.h"
#include "Script/LuaFunc.h"
#include "Script/ScriptManager.h"

namespace app {

Color::Color() : mVal(0xFF) {
}

Color::Color(u32 it) : mVal(it) {
}

Color::~Color() {
}

void Color::setColor(u32 it) {
    mVal = it;
}

u32 Color::getColor() const {
    return mVal;
}

s32 Color::getRed() const {
    return (0xFFu & mVal);
}

s32 Color::getGreen() const {
    return (0xFF00u & mVal) >> 8;
}

s32 Color::getBlue() const {
    return (0xFF0000u & mVal) >> 16;
}

s32 Color::getAlpha() const {
    return (0xFF000000u & mVal) >> 24;
}

void Color::setRed(u32 it) {
    mVal = (mVal & 0xFFFFFF00u) | (0xFFu & it);
}

void Color::setGreen(u32 it) {
    mVal = (mVal & 0xFFFF00FFu) | ((0xFFu & it) << 8);
}

void Color::setBlue(u32 it) {
    mVal = (mVal & 0xFF00FFFFu) | ((0xFFu & it) << 16);
}

void Color::setAlpha(u32 it) {
    mVal = (mVal & 0x00FFFFFFu) | ((0xFFu & it) << 24);
}


String Color::toStr() const {
    s8 buf[128];
    snprintf(buf, sizeof(buf), "(R=%d, G=%d, B=%d, A=%d)", getRed(), getGreen(), getBlue(), getAlpha());
    return buf;
}

} // namespace app
