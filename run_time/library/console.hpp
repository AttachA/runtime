// Copyright Danyil Melnytskyi 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <cstdint>
namespace console {
	extern const bool is_loaded;
	extern "C" {
		void printLine(const char* chars);
		void print(const char* chars);
		void resetModifiers();
		void boldText();
		void italicText();
		void underlineText();
		void slowBlink();
		void rapidBlink();
		void invertColors();
		void notBoldText();
		void notUnderlinedText();
		void notBlinkText();

		void resetTextColor();
		void resetBgColor();
		void setTextColor(uint8_t r, uint8_t g, uint8_t b);
		void setBgColor(uint8_t r, uint8_t g, uint8_t b);
		void setPos(uint16_t row, uint16_t col);
		void saveCurPos();
		void loadCurPos();
		void setLine(uint32_t y);
		void showCursor(uint32_t y);
		void hideCursor(uint32_t y);
	}
}