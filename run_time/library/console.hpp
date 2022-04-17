// Copyright Danyil Melnytskyi 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <cstdint>
#include "../../library/list_array.hpp">
struct ValueItem;
namespace console {
	extern const bool is_loaded;
	extern "C" {
		ValueItem* printLine(list_array<ValueItem>*);//print all arguments, convert all non string values to string
		ValueItem* print(list_array<ValueItem>*);//print all arguments, convert all non string values to string
		
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



		ValueItem* readWord(list_array<ValueItem>*);//return string, optional argument with buffer len
		ValueItem* readLine(list_array<ValueItem>*);//return string, no arguments
		ValueItem* readInput(list_array<ValueItem>*);//return string, no arguments
		ValueItem* readValue(list_array<ValueItem>*);//return any value, no arguments
	}
}