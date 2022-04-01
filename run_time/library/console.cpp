// Copyright Danyil Melnytskyi 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "console.hpp"
#include <cstdio>
#include <string>

namespace console {
#ifdef _WIN64
#include <Windows.h>
	//enable ANSI escape codes in console
	const bool is_loaded = SetConsoleMode(GetConsoleWindow(), ENABLE_VIRTUAL_TERMINAL_INPUT);
	HANDLE hout = GetStdHandle(-11);
#else
	constexpr bool is_loaded = true;
#endif
	//use ANSI escape codes
	extern "C" {
		void printLine(const char* chars) {

#ifdef _WIN64
			size_t str_len = strlen(chars);
			char* str = (char*)alloca(str_len + 2);
			memcpy(str, chars, str_len);
			str[str_len] = '\n';
			str[str_len+1] = 0;
			WriteConsoleA(hout, str, str_len+2, nullptr, nullptr);
#elif
			puts(chars);
#endif
		}
		void print(const char* chars) {
#ifdef _WIN64
			WriteConsoleA(hout, chars, strlen(chars), nullptr, nullptr);
#elif
			fputs(chars, stdout);
#endif
		}
		void resetModifiers() {
			print("\033[0m");
		}
		void boldText() {
			print("\033[1m");
		}
		void italicText() {
			print("\033[3m");
		}
		void underlineText() {
			print("\033[4m");
		}
		void slowBlink() {
			print("\033[5m");
		}
		void rapidBlink() {
			print("\033[6m");
		}
		void invertColors() {
			print("\033[7m");
		}
		void notBoldText() {
			print("\033[22m");
		}
		void notUnderlinedText() {
			print("\033[24m");
		}
		void notBlinkText() {
			print("\033[25m");
		}

		void resetTextColor() {
			print("\033[39m");
		}
		void resetBgColor() {
			print("\033[49m");
		}
		void setTextColor(uint8_t r, uint8_t g, uint8_t b) {
			printf("\033[38;2;%d;%d;%dm", r, g, b);
		}
		void setBgColor(uint8_t r, uint8_t g, uint8_t b) {
			printf("\033[48;2;%d;%d;%dm", r, g, b);
		}
		void setPos(uint16_t row, uint16_t col) {
			printf("\033[%d;%dH", row + 1, col+1);
		}
		void saveCurPos() {
			print("\033[s");
		}
		void loadCurPos() {
			print("\033[u");
		}
		void setLine(uint32_t y) {
			if (y == 0)
				print("\033[999999D");
			else
				printf("\033[999999D\033[%dC",y);
		}
		void showCursor(uint32_t y) {
			print("\033[?25h");
		}
		void hideCursor(uint32_t y) {
			print("\033[?25l");
		}
	}
}

