#include "console.hpp"
#include <cstdio>
#include <string>

namespace console {
#ifdef _WIN64
#include <Windows.h>
	//enable ANSI escape codes in console
	const bool is_loaded = SetConsoleMode(GetConsoleWindow(), ENABLE_VIRTUAL_TERMINAL_INPUT);
#else
	constexpr bool is_loaded = true;
#endif
	//use ANSI escape codes
	extern "C" {
		void printLine(const char* chars) {
			puts(chars);
		}
		void print(const char* chars) {
			fputs(chars, stdout);
		}
		void resetModifiers() {
			fputs("\033[0m", stdout);
		}
		void boldText() {
			fputs("\033[1m", stdout);
		}
		void italicText() {
			fputs("\033[3m", stdout);
		}
		void underlineText() {
			fputs("\033[4m", stdout);
		}
		void slowBlink() {
			fputs("\033[5m", stdout);
		}
		void rapidBlink() {
			fputs("\033[6m", stdout);
		}
		void invertColors() {
			fputs("\033[7m", stdout);
		}
		void notBoldText() {
			fputs("\033[22m", stdout);
		}
		void notUnderlinedText() {
			fputs("\033[24m", stdout);
		}
		void notBlinkText() {
			fputs("\033[25m", stdout);
		}

		void resetTextColor() {
			fputs("\033[39m", stdout);
		}
		void resetBgColor() {
			fputs("\033[49m", stdout);
		}
		void setTextColor(uint8_t r, uint8_t g, uint8_t b) {
			printf("\033[38;2;%d;%d;%dm", r, g, b);
			//fputs(("\033[38;2;" + std::to_string(r) + ';' + std::to_string(g) + ';' + std::to_string(b) + 'm').c_str(), stdout);
		}
		void setBgColor(uint8_t r, uint8_t g, uint8_t b) {
			printf("\033[48;2;%d;%d;%dm", r, g, b);
			//fputs(("\033[48;2;" + std::to_string(r) + ';' + std::to_string(g) + ';' + std::to_string(b) + 'm').c_str(), stdout);
		}
		void setPos(uint16_t row, uint16_t col) {
			printf("\033[%d;%dH", row + 1, col+1);
			//std::string tmp = "\033[" + std::to_string(row + 1) +';' + std::to_string(col + 1) +'H';
			//fputs(tmp.c_str(), stdout);
		}
		void saveCurPos() {
			fputs("\033[s", stdout);
		}
		void loadCurPos() {
			fputs("\033[u", stdout);
		}
		void setLine(uint32_t y) {
			if (y == 0)
				fputs("\033[999999D", stdout);
			else
				printf("\033[999999D\033[%dC",y);
		}
		void showCursor(uint32_t y) {
			fputs("\033[?25h", stdout);
		}
		void hideCursor(uint32_t y) {
			fputs("\033[?25l", stdout);
		}
	}
}

