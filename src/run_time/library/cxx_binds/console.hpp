#ifndef SRC_RUN_TIME_LIBRARY_CXX_BINDS_CONSOLE
#define SRC_RUN_TIME_LIBRARY_CXX_BINDS_CONSOLE
#include <run_time/AttachA_CXX.hpp>
#include <run_time/library/console.hpp>

namespace art_lib {
    namespace console {
        template <class... Arguments>
        void printLine(Arguments... arguments) {
            art::CXX::cxxCall(art::console::printLine, std::forward<Arguments>(arguments)...);
        }

        template <class... Arguments>
        void print(Arguments... arguments) {
            art::CXX::cxxCall(art::console::print, std::forward<Arguments>(arguments)...);
        }

        template <class... Arguments>
        void printf(const art::ustring& format, Arguments... arguments) {
            art::CXX::cxxCall(art::console::printf, format, std::forward<Arguments>(arguments)...);
        }

        constexpr void (*resetModifiers)() = art::console::resetModifiers;
        constexpr void (*boldText)() = art::console::boldText;
        constexpr void (*italicText)() = art::console::italicText;
        constexpr void (*underlineText)() = art::console::underlineText;
        constexpr void (*slowBlink)() = art::console::slowBlink;
        constexpr void (*rapidBlink)() = art::console::rapidBlink;
        constexpr void (*invertColors)() = art::console::invertColors;
        constexpr void (*notBoldText)() = art::console::notBoldText;
        constexpr void (*notUnderlinedText)() = art::console::notUnderlinedText;
        constexpr void (*hideBlinkText)() = art::console::hideBlinkText;
        constexpr void (*resetTextColor)() = art::console::resetTextColor;
        constexpr void (*resetBgColor)() = art::console::resetBgColor;
        constexpr void (*setTextColor)(uint8_t r, uint8_t g, uint8_t b) = art::console::setTextColor;
        constexpr void (*setBgColor)(uint8_t r, uint8_t g, uint8_t b) = art::console::setBgColor;
        constexpr void (*setPos)(uint16_t row, uint16_t col) = art::console::setPos;
        constexpr void (*saveCurPos)() = art::console::saveCurPos;
        constexpr void (*loadCurPos)() = art::console::loadCurPos;
        constexpr void (*setLine)(uint32_t y) = art::console::setLine;
        constexpr void (*showCursor)() = art::console::showCursor;
        constexpr void (*hideCursor)() = art::console::hideCursor;

        inline art::ustring readWord() {
            return (art::ustring)art::CXX::cxxCall(art::console::readWord);
        }

        inline art::ustring readLine() {
            return (art::ustring)art::CXX::cxxCall(art::console::readLine);
        }

        inline art::ustring readInput() {
            return (art::ustring)art::CXX::cxxCall(art::console::readInput);
        }

        inline art::ValueItem readValue() {
            return (art::ValueItem)art::CXX::cxxCall(art::console::readValue);
        }

        inline int64_t readInt() {
            return (int64_t)art::CXX::cxxCall(art::console::readInt);
        }
    }
}
#endif /* SRC_RUN_TIME_LIBRARY_CXX_BINDS_CONSOLE */
