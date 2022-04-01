//stackoverflow
#pragma once
#include <string>
#include <locale>
namespace stringC {
	namespace multibyte {
		std::string  convert(const wchar_t* s);
		std::wstring convert(const char* s);
		std::string  convert(const std::wstring& s);
		std::wstring convert(const std::string& s);
	}
	namespace stdlocal {
		std::string convert(
			const wchar_t* s,
			const size_t len,
			const std::locale& loc = std::locale(),
			const char default_char = '?'
		);
		std::string convert(
			const wchar_t* s,
			const std::locale& loc = std::locale(""),
			const char default_char = '?'
		);
		std::string convert(
			const std::wstring& s,
			const std::locale& loc = std::locale(""),
			const char default_char = '?'
		);
		std::wstring convert(
			const char* s,
			const std::locale& loc = std::locale("")
		);
		std::wstring convert(
			const std::string& s,
			const std::locale& loc = std::locale("")
		);
		std::wstring convert(
			const char* s,
			const size_t len,
			const std::locale& loc = std::locale()
		);
	}
	namespace utf8 {
		std::string  convert(const std::wstring& s);
		std::wstring convert(const std::string& s);
	}
	namespace utf16 {
		std::string convert(const std::u16string& s);
		std::u16string convert(const std::string& s);
	}
	namespace utf32 {
		std::string convert(const std::u32string& s);
		std::u32string convert(const std::string& s);
	}
}
