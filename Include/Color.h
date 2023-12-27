#ifndef APP_COLOR_H
#define APP_COLOR_H

#include "Strings.h"

namespace app {

/**
 * @brief Color for lua test
 */
class Color {
public:
    Color();
    Color(u32 it);
    ~Color();

    void setColor(u32 it);
    u32 getColor() const;

    s32 getRed() const;
    s32 getGreen() const;
    s32 getBlue() const;
    s32 getAlpha() const;

    void setRed(u32 it);
    void setGreen(u32 it);
    void setBlue(u32 it);
    void setAlpha(u32 it);

    String toStr() const;

protected:
    u32 mVal;
};

} // namespace app

#endif // APP_COLOR_H
