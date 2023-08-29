// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <cstdint>
#include <library/list_array.hpp>

namespace art {
    struct ValueItem;

    namespace console {
        extern const bool is_loaded;
        ValueItem* printLine(ValueItem* args, uint32_t len);
        ValueItem* print(ValueItem* args, uint32_t len);

        ValueItem* register_format_operator(ValueItem* args, uint32_t len);
        ValueItem* format(ValueItem* args, uint32_t len);
        ValueItem* printf(ValueItem* args, uint32_t len);

        void resetModifiers();
        void boldText();
        void italicText();
        void underlineText();
        void slowBlink();
        void rapidBlink();
        void invertColors();
        void notBoldText();
        void notUnderlinedText();
        void hideBlinkText();

        void resetTextColor();
        void resetBgColor();
        void setTextColor(uint8_t r, uint8_t g, uint8_t b);
        void setBgColor(uint8_t r, uint8_t g, uint8_t b);
        void setPos(uint16_t row, uint16_t col);
        void saveCurPos();
        void loadCurPos();
        void setLine(uint32_t y);
        void showCursor();
        void hideCursor();


        ValueItem* readWord(ValueItem* args, uint32_t len);  //return string, no arguments
        ValueItem* readLine(ValueItem* args, uint32_t len);  //return string, no arguments
        ValueItem* readInput(ValueItem* args, uint32_t len); //return string, no arguments
        ValueItem* readValue(ValueItem* args, uint32_t len); //return any value, no arguments
        ValueItem* readInt(ValueItem* args, uint32_t len);   //return int(any size), no arguments
    }
}