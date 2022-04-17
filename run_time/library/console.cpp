// Copyright Danyil Melnytskyi 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#define _CRT_SECURE_NO_WARNINGS

#ifdef _WIN64
#include <Windows.h>
#endif

#include "console.hpp"
#include "exceptions.hpp"
#include "../../library/string_convert.hpp"
#include "../AttachA_CXX.hpp"
#include <cstdio>
#include <string>
namespace console {
	auto _stdin = stdin;
	auto _stdout = stdout;
	auto _stderr = stderr;
#ifdef _WIN64
	bool enableUtf8Support() {
		HANDLE hout = GetStdHandle(-11);
		if (setvbuf(_stderr, nullptr, _IOFBF, 1024))return false;
		if (setvbuf(_stdout, nullptr, _IOFBF, 1024))return false;
		if (!SetConsoleOutputCP(65001)) return false;

		CONSOLE_FONT_INFOEX info;
		memset(&info, 0, sizeof(CONSOLE_FONT_INFOEX));
		info.cbSize = sizeof(CONSOLE_FONT_INFOEX);
		if (GetCurrentConsoleFontEx(hout, false, &info)) {
			info.FontFamily = 0;//FF_DONTCARE (0 << 4) == 0
			info.dwFontSize.X = 0;
			info.dwFontSize.Y = 14;
			info.FontWeight = 400;
			wcscpy_s(info.FaceName, L"Lucida Console");
			if (SetCurrentConsoleFontEx(hout, false, &info))
				return true;
		}
		return false;
	}
	//enable ANSI escape codes in console
	const bool is_loaded = SetConsoleMode(GetConsoleWindow(), ENABLE_VIRTUAL_TERMINAL_INPUT) && enableUtf8Support();
#else
	constexpr bool is_loaded = true;
#endif
	enum class RW_CONS : bool {
		R,
		W
	};
	thread_local RW_CONS rw_switcher = RW_CONS::R;
	void was_w_mode() {
		if (rw_switcher == RW_CONS::R)
			fflush(_stdin);
	}
	void was_r_mode() {
		if (rw_switcher == RW_CONS::W) {
			fflush(_stdout);
			fflush(_stderr);
		}
	}

	template <size_t N>
	inline void print(const char(&chars)[N]) {
		if (rw_switcher != RW_CONS::W) 
			fflush(_stdin);
		fwrite(chars, 1, N, _stdout);
	};



	extern "C" {
		ValueItem* printLine(list_array<ValueItem>* args) {
			was_w_mode();
			if (args == nullptr)
				fwrite("\n", 1, 1, _stdout);
			else {
				for (auto& it : *args) {
					std::string tmp = ABI_IMPL::Scast(it.val, it.meta) + '\n';
					fwrite(tmp.c_str(), 1, tmp.size(), _stdout);
				}
			}
			return nullptr;
		}
		ValueItem* print(list_array<ValueItem>* args) {
			was_w_mode();
			if (args != nullptr)
				for (auto& it : *args) {
					std::string tmp = ABI_IMPL::Scast(it.val, it.meta);
					fwrite(tmp.c_str(), 1, tmp.size(), _stdout);
				}
			return nullptr;
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

		ValueItem* readWord(list_array<ValueItem>* args) {
			was_r_mode();
			std::string str;
			char c;
			while ((c = getchar()) != EOF) {
				switch (c) {
				case '\n':
				case '\b':
				case '\t':
				case '\r':
				case ' ':
					continue;
				default:
					str += c;
				}
			}
			while ((c = getchar()) != EOF) {
				switch (c)
				{
				case '\n':
				case '\b':
				case '\t':
				case '\r':
				case ' ':
					goto end;
				default:
					str += c;
				}
			}
		end:
			return new ValueItem(ABI_IMPL::SBcast(str));
		}
		ValueItem* readLine(list_array<ValueItem>* args) {
			was_r_mode();
			std::string str;
			char c;
			bool do_continue = true;
			while (do_continue) {
				switch (c = getchar()) {
				case EOF: case '\n':
					do_continue = false;
				}
				str += c;
			}
			return new ValueItem(new std::string(std::move(str)), ValueMeta(VType::string, false, true));
		}
		ValueItem* readInput(list_array<ValueItem>*) {
			was_r_mode();
			std::string str;
			char c;
			while ((c = getchar()) != EOF)
				str += c;
			
			return new ValueItem(new std::string(std::move(str)), ValueMeta(VType::string, false, true));

		}
		ValueItem* readValue(list_array<ValueItem>*) {
			was_r_mode();
			std::string str;
			char c;
			while ((c = getchar()) != EOF) {
				switch (c)
				{
				case '\n':
				case '\b':
				case '\t':
				case '\r':
				case ' ':
					break;
				default:
					str += c;
				}
			}
			return new ValueItem(ABI_IMPL::SBcast(str));
		}
	}
}

