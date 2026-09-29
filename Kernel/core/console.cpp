/*
	This file is part of Fennix Kernel.

	Fennix Kernel is free software: you can redistribute it and/or
	modify it under the terms of the GNU General Public License as
	published by the Free Software Foundation, either version 3 of
	the License, or (at your option) any later version.

	Fennix Kernel is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Fennix Kernel. If not, see <https://www.gnu.org/licenses/>.
*/

#include <kcon.hpp>

#include <memory.hpp>
#include <stropts.h>
#include <string.h>
#include <log.hpp>
#include <thread>
#include <ini.h>

#include "../kernel.h"

using namespace std::chrono_literals;

namespace KernelConsole
{
	static int TermColors[] = {
		[TerminalColor::BLACK] = 0x000000,
		[TerminalColor::RED] = 0xAA0000,
		[TerminalColor::GREEN] = 0x00AA00,
		[TerminalColor::YELLOW] = 0xAA5500,
		[TerminalColor::BLUE] = 0x0000AA,
		[TerminalColor::MAGENTA] = 0xAA00AA,
		[TerminalColor::CYAN] = 0x00AAAA,
		[TerminalColor::GREY] = 0xAAAAAA,
	};

	static int TermBrightColors[] = {
		[TerminalColor::BLACK] = 0x858585,
		[TerminalColor::RED] = 0xFF5555,
		[TerminalColor::GREEN] = 0x55FF55,
		[TerminalColor::YELLOW] = 0xFFFF55,
		[TerminalColor::BLUE] = 0x5555FF,
		[TerminalColor::MAGENTA] = 0xFF55FF,
		[TerminalColor::CYAN] = 0x55FFFF,
		[TerminalColor::GREY] = 0xFFFFFF,
	};

	struct GlyphCache
	{
		Video::Font *Font = nullptr;
		uint32_t Width = 0;
		uint32_t Height = 0;
		uint32_t *Pixels = nullptr;
	};

	static GlyphCache CachedGlyphs;

	static void ResetGlyphCache()
	{
		if (CachedGlyphs.Pixels)
			delete[] CachedGlyphs.Pixels;
		CachedGlyphs.Pixels = nullptr;
		CachedGlyphs.Font = nullptr;
		CachedGlyphs.Width = 0;
		CachedGlyphs.Height = 0;
	}

	static bool LookupForeground(uint32_t color, uint8_t &index, bool &bright)
	{
		for (uint8_t i = 0; i < 8; ++i)
		{
			if (color == (uint32_t)TermColors[i])
			{
				index = i;
				bright = false;
				return true;
			}
		}
		for (uint8_t i = 0; i < 8; ++i)
		{
			if (color == (uint32_t)TermBrightColors[i])
			{
				index = i;
				bright = true;
				return true;
			}
		}
		return false;
	}

	static void EnsureGlyphCache(FontRenderer &renderer)
	{
		if (!renderer.CurrentFont)
			return;

		Video::FontInfo info = renderer.CurrentFont->GetInfo();
		if (info.Type != Video::FontType::PCScreenFont2)
			return;

		if (CachedGlyphs.Font == renderer.CurrentFont &&
			CachedGlyphs.Width == info.Width &&
			CachedGlyphs.Height == info.Height &&
			CachedGlyphs.Pixels != nullptr)
			return;

		ResetGlyphCache();

		const uint32_t width = info.Width;
		const uint32_t height = info.Height;
		if (width == 0 || height == 0)
			return;

		const size_t glyphPixels = (size_t)width * (size_t)height;
		const size_t totalPixels = 256u * 16u * glyphPixels;
		CachedGlyphs.Pixels = new uint32_t[totalPixels];
		memset(CachedGlyphs.Pixels, 0, totalPixels * sizeof(uint32_t));

		CachedGlyphs.Font = renderer.CurrentFont;
		CachedGlyphs.Width = width;
		CachedGlyphs.Height = height;

		char *fontAddress = (char *)info.StartAddress;
		uint32_t headerSize = info.PSF2Font->Header->headersize;
		uint32_t charSize = info.PSF2Font->Header->charsize;
		uint32_t fontLength = info.PSF2Font->Header->length;
		uint32_t bytesPerLine = (width + 7) / 8;

		for (uint32_t colorIndex = 0; colorIndex < 16; ++colorIndex)
		{
			uint32_t fg = (colorIndex < 8) ? (uint32_t)TermColors[colorIndex]
										   : (uint32_t)TermBrightColors[colorIndex - 8];

			for (uint32_t ch = 0; ch < 256; ++ch)
			{
				uint32_t glyphIndex = (ch < fontLength) ? ch : 0;
				char *glyph = fontAddress + headerSize + (glyphIndex * charSize);
				uint32_t *dst = CachedGlyphs.Pixels + ((colorIndex * 256u + ch) * glyphPixels);

				for (uint32_t y = 0; y < height; ++y)
				{
					for (uint32_t x = 0; x < width; ++x)
					{
						uint8_t b = glyph[y * bytesPerLine + (x / 8)];
						uint8_t mask = (uint8_t)(0x80 >> (x % 8));
						dst[y * width + x] = (b & mask) ? fg : 0x000000;
					}
				}
			}
		}
	}

	static bool PaintCached(FontRenderer &renderer, long CellX, long CellY, char Char, uint32_t Foreground, uint32_t Background)
	{
		if (!Display || Display->GetBitsPerPixel() != 32)
			return false;
		if (Background != 0x000000)
			return false;

		uint8_t fgIndex = 0;
		bool bright = false;
		if (!LookupForeground(Foreground, fgIndex, bright))
			return false;

		EnsureGlyphCache(renderer);
		if (CachedGlyphs.Pixels == nullptr || CachedGlyphs.Font != renderer.CurrentFont)
			return false;

		const uint32_t width = CachedGlyphs.Width;
		const uint32_t height = CachedGlyphs.Height;
		const size_t glyphPixels = (size_t)width * (size_t)height;
		const uint32_t colorIndex = (bright ? 8u : 0u) + fgIndex;
		const uint8_t ch = (uint8_t)Char;
		const uint32_t *src = CachedGlyphs.Pixels + ((colorIndex * 256u + ch) * glyphPixels);

		const uint32_t x = (uint32_t)CellX * width;
		const uint32_t y = (uint32_t)CellY * height;
		uint8_t *dst = reinterpret_cast<uint8_t *>(Display->GetBuffer);
		const size_t pitch = Display->GetPitch();

		for (uint32_t row = 0; row < height; ++row)
		{
			memcpy(dst + ((y + row) * pitch) + (x * sizeof(uint32_t)),
				   src + (row * width),
				   width * sizeof(uint32_t));
		}

		return true;
	}

	__no_sanitize("undefined") char FontRenderer::Paint(long CellX, long CellY, char Char, uint32_t Foreground, uint32_t Background)
	{
		if (PaintCached(*this, CellX, CellY, Char, Foreground, Background))
			return Char;

		uint64_t x = CellX * CurrentFont->GetInfo().Width;
		uint64_t y = CellY * CurrentFont->GetInfo().Height;

		switch (CurrentFont->GetInfo().Type)
		{
		case Video::FontType::PCScreenFont1:
		{
			uint32_t *PixelPtr = (uint32_t *)Display->GetBuffer;
			char *FontPtr = (char *)CurrentFont->GetInfo().PSF1Font->GlyphBuffer + (Char * CurrentFont->GetInfo().PSF1Font->Header->charsize);
			for (uint64_t Y = 0; Y < 16; Y++)
			{
				for (uint64_t X = 0; X < 8; X++)
				{
					if ((*FontPtr & (0b10000000 >> X)) > 0)
						*(unsigned int *)(PixelPtr + (x + X) + ((y + Y) * Display->GetWidth)) = Foreground;
					else
						*(unsigned int *)(PixelPtr + (x + X) + ((y + Y) * Display->GetWidth)) = Background;
				}
				FontPtr++;
			}
			break;
		}
		case Video::FontType::PCScreenFont2:
		{
			Video::FontInfo fInfo = CurrentFont->GetInfo();

			int BytesPerLine = (fInfo.PSF2Font->Header->width + 7) / 8;
			char *FontAddress = (char *)fInfo.StartAddress;
			uint32_t FontHeaderSize = fInfo.PSF2Font->Header->headersize;
			uint32_t FontCharSize = fInfo.PSF2Font->Header->charsize;
			uint32_t FontLength = fInfo.PSF2Font->Header->length;
			char *FontPtr = FontAddress + FontHeaderSize + (Char > 0 && (uint32_t)Char < FontLength ? Char : 0) * FontCharSize;

			uint32_t FontHdrWidth = fInfo.PSF2Font->Header->width;
			uint32_t FontHdrHeight = fInfo.PSF2Font->Header->height;

			for (uint32_t Y = 0; Y < FontHdrHeight; Y++)
			{
				for (uint32_t X = 0; X < FontHdrWidth; X++)
				{
					void *FramebufferAddress = (void *)((uintptr_t)Display->GetBuffer +
														((y + Y) * Display->GetWidth + (x + X)) *
															(Display->GetFramebufferStruct().BitsPerPixel / 8));

					if ((*FontPtr & (0b10000000 >> (X % 8))) > 0)
						*(uint32_t *)FramebufferAddress = Foreground;
					else
						*(uint32_t *)FramebufferAddress = Background;
				}
				FontPtr += BytesPerLine;
			}
			break;
		}
		default:
			warn("Unsupported font type");
			break;
		}
		return Char;
	}

	FontRenderer Renderer;

	ConsoleTerminal *Terminals[16] = {nullptr};
	std::atomic<ConsoleTerminal *> CurrentTerminal = nullptr;

	void paint_blinker(bool Enable)
	{
		if (CurrentTerminal == nullptr)
			return;

		ConsoleTerminal *term = CurrentTerminal.load();
		ConsoleTerminal::Blinker &blinker = term->Blink;
		size_t cellIndex = Renderer.Cursor.Y * term->Term->GetWinsize()->ws_col + Renderer.Cursor.X;
		TerminalCell *cell = term->Term->GetCell(cellIndex);
		uint32_t bgColor = cell->attr.Bright ? TermBrightColors[cell->attr.Background] : TermColors[cell->attr.Background];
		Renderer.Paint(Renderer.Cursor.X, Renderer.Cursor.Y, blinker.Character, Enable ? blinker.Color : bgColor, bgColor);
	}

	void paint_blinker_thread()
	{
		bool blink = false;
		while (true)
		{
			paint_blinker(blink);
			blink = !blink;
			std::this_thread::sleep_for(std::chrono::milliseconds(CurrentTerminal.load()->Blink.Delay));
		}
	}

	void paint_callback(TerminalCell *cell, long x, long y)
	{
		if (cell->attr.Bright)
			Renderer.Paint(x, y, cell->c, TermBrightColors[cell->attr.Foreground], TermColors[cell->attr.Background]);
		else
			Renderer.Paint(x, y, cell->c, TermColors[cell->attr.Foreground], TermColors[cell->attr.Background]);
	}

	void cursor_callback(TerminalCursor *cur)
	{
		Renderer.Cursor = {cur->X, cur->Y};
		paint_blinker(false);
	}

	static bool scroll_callback(unsigned short lines)
	{
		if (!Display || !Renderer.CurrentFont || lines == 0)
			return false;

		const uint32_t cellH = Renderer.CurrentFont->GetInfo().Height;
		const uint32_t shiftPX = cellH * lines;
		if (shiftPX == 0)
			return true;

		uint8_t *buf = reinterpret_cast<uint8_t *>(Display->GetBuffer);
		const size_t pitch = Display->GetPitch();
		const uint32_t height = Display->GetHeight;

		if (!buf || pitch == 0 || height == 0)
			return false;

		size_t moveRows = 0;
		if (shiftPX < height)
			moveRows = height - shiftPX;

		size_t move_bytes = moveRows * pitch;
		if (moveRows > 0)
			memmove(buf, buf + (shiftPX * pitch), move_bytes);

		const uint32_t bg = TermColors[TerminalColor::BLACK];
		uint8_t *clearPointer = buf + move_bytes;
		size_t clearBytes = (height * pitch) - move_bytes;

		if (Display->GetBitsPerPixel() == 32)
		{
			uint32_t *dst = reinterpret_cast<uint32_t *>(clearPointer);
			size_t pixels = clearBytes / sizeof(uint32_t);
			for (size_t i = 0; i < pixels; ++i)
				dst[i] = bg;
		}
		else
		{
			memset(clearPointer, 0, clearBytes);
		}

		return true;
	}

	bool SetTheme(std::string Theme)
	{
		Node rn = fs->Lookup(thisProcess->Info.RootNode, "/sys/cfg/term");
		if (rn == nullptr)
			return false;

		kstat st;
		fs->Stat(rn, &st);

		char *sh = new char[st.Size];
		fs->Read(rn, sh, st.Size, 0);

		ini_t *ini = ini_load(sh, NULL);
		int themeSection, c0, c1, c2, c3, c4, c5, c6, c7, colorsIdx;
		const char *colors[8];

		debug("Loading terminal theme: \"%s\"", Theme.c_str());
		themeSection = ini_find_section(ini, Theme.c_str(), NULL);
		if (themeSection == INI_NOT_FOUND)
		{
			ini_destroy(ini);
			delete[] sh;
			return false;
		}

		auto getColorComponent = [](const char *str, int &index) -> int
		{
			int value = 0;
			while (str[index] >= '0' && str[index] <= '9')
			{
				value = value * 10 + (str[index] - '0');
				++index;
			}
			return value;
		};

		auto parseColor = [getColorComponent](const char *colorStr) -> unsigned int
		{
			int index = 0;
			int r = getColorComponent(colorStr, index);
			if (colorStr[index] == ',')
				++index;
			int g = getColorComponent(colorStr, index);
			if (colorStr[index] == ',')
				++index;
			int b = getColorComponent(colorStr, index);
			return (r << 16) | (g << 8) | b;
		};

		c0 = ini_find_property(ini, themeSection, "color0", NULL);
		c1 = ini_find_property(ini, themeSection, "color1", NULL);
		c2 = ini_find_property(ini, themeSection, "color2", NULL);
		c3 = ini_find_property(ini, themeSection, "color3", NULL);
		c4 = ini_find_property(ini, themeSection, "color4", NULL);
		c5 = ini_find_property(ini, themeSection, "color5", NULL);
		c6 = ini_find_property(ini, themeSection, "color6", NULL);
		c7 = ini_find_property(ini, themeSection, "color7", NULL);

		colors[0] = ini_property_value(ini, themeSection, c0);
		colors[1] = ini_property_value(ini, themeSection, c1);
		colors[2] = ini_property_value(ini, themeSection, c2);
		colors[3] = ini_property_value(ini, themeSection, c3);
		colors[4] = ini_property_value(ini, themeSection, c4);
		colors[5] = ini_property_value(ini, themeSection, c5);
		colors[6] = ini_property_value(ini, themeSection, c6);
		colors[7] = ini_property_value(ini, themeSection, c7);

		colorsIdx = 0;
		for (auto color : colors)
		{
			colorsIdx++;
			if (color == 0)
				continue;

			char *delimiterPos = strchr(color, ':');
			if (delimiterPos == NULL)
				continue;

			char colorStr[20], colorBrightStr[20];
			strncpy(colorStr, color, delimiterPos - color);
			colorStr[delimiterPos - color] = '\0';
			strcpy(colorBrightStr, delimiterPos + 1);

			TermColors[colorsIdx - 1] = parseColor(colorStr);
			TermBrightColors[colorsIdx - 1] = parseColor(colorBrightStr);
		}

		ini_destroy(ini);
		delete[] sh;
		ResetGlyphCache();
		return true;
	}

#ifdef DEBUG
	void __test_themes()
	{
		printf("\x1b[H\x1b[2J");
		auto testTheme = [](const char *theme)
		{
			KernelConsole::SetTheme("vga");
			printf("== Theme: \"%s\" ==\n", theme);
			KernelConsole::SetTheme(theme);
			const char *txtColors[] = {
				"30", "31", "32", "33", "34", "35", "36", "37", "39"};
			const char *bgColors[] = {
				"40", "41", "42", "43", "44", "45", "46", "47", "49"};
			const int numTxtColors = sizeof(txtColors) / sizeof(txtColors[0]);
			const int numBgColors = sizeof(bgColors) / sizeof(bgColors[0]);

			const char *msg = "H";
			for (int i = 0; i < numTxtColors; i++)
				for (int j = 0; j < numBgColors; j++)
					printf("\x1b[%sm\x1b[%sm%s\x1b[0m", txtColors[i], bgColors[j], msg);

			for (int i = 0; i < numTxtColors; i++)
				for (int j = 0; j < numBgColors; j++)
					printf("\x1b[1;%sm\x1b[1;%sm%s\x1b[0m", txtColors[i], bgColors[j], msg);

			KernelConsole::SetTheme("vga");
			printf("\n");
		};
		testTheme("vga");
		testTheme("breeze");
		testTheme("coolbreeze");
		testTheme("softlight");
		testTheme("calmsea");
		testTheme("warmember");
		// CPU::Stop();
	}
#endif

	static void WriteFromLog(const Log::LogRecord *record)
	{
		char prefixBuf[128];

		std::chrono::nanoseconds nano = std::chrono::nanoseconds(record->TimestampNs);
		std::chrono::seconds sec = nano;
		// uint64_t frac = nano.count() % 10000000;
		uint64_t frac = nano.count() % 1'000'000'000ULL;
		frac /= 100;

#if defined(__amd64__) || defined(__aarch64__)
		snprintf(prefixBuf, sizeof(prefixBuf),
				 "\x1b[1;30m[\x1b[1;34m%lu.%07lu ", sec.count(), frac);
#else
		snprintf(prefixBuf, sizeof(prefixBuf),
				 "\x1b[1;30m[\x1b[1;34m%llu.%07llu ", sec.count(), frac);
#endif

		size_t prefixLen = strlen(prefixBuf);

		switch (record->Level)
		{
		case Log::LogLevel::LogLevelNone:
			snprintf(prefixBuf + prefixLen, sizeof(prefixBuf) - prefixLen, "\x1b[0mL");
			break;
		case Log::LogLevel::LogLevelError:
			snprintf(prefixBuf + prefixLen, sizeof(prefixBuf) - prefixLen, "\x1b[1;31mE");
			break;
		case Log::LogLevel::LogLevelWarning:
			snprintf(prefixBuf + prefixLen, sizeof(prefixBuf) - prefixLen, "\x1b[1;33mW");
			break;
		case Log::LogLevel::LogLevelFixme:
			snprintf(prefixBuf + prefixLen, sizeof(prefixBuf) - prefixLen, "\x1b[1;35mF");
			break;
		case Log::LogLevel::LogLevelInfo:
			snprintf(prefixBuf + prefixLen, sizeof(prefixBuf) - prefixLen, "\x1b[1;32mI");
			break;
		case Log::LogLevel::LogLevelStub:
			snprintf(prefixBuf + prefixLen, sizeof(prefixBuf) - prefixLen, "\x1b[1;36mS");
			break;
		case Log::LogLevel::LogLevelDebug:
			snprintf(prefixBuf + prefixLen, sizeof(prefixBuf) - prefixLen, "\x1b[1;34mD");
			break;
		case Log::LogLevel::LogLevelUbsan:
			snprintf(prefixBuf + prefixLen, sizeof(prefixBuf) - prefixLen, "\x1b[1;31mU");
			break;
		case Log::LogLevel::LogLevelFunction:
			snprintf(prefixBuf + prefixLen, sizeof(prefixBuf) - prefixLen, "\x1b[1;36mF");
			break;
		default:
			snprintf(prefixBuf + prefixLen, sizeof(prefixBuf) - prefixLen, "\x1b[0m?");
			break;
		}
		prefixLen = strlen(prefixBuf);

		snprintf(prefixBuf + prefixLen, sizeof(prefixBuf) - prefixLen, "\x1b[0m%u", record->CpuID);
		prefixLen = strlen(prefixBuf);

		snprintf(prefixBuf + prefixLen, sizeof(prefixBuf) - prefixLen, "\x1b[1;30m]\x1b[0m ");

		if (KernelConsole::CurrentTerminal.load(std::memory_order_acquire)->Term != nullptr)
		{
			if (DebuggerIsAttached == true &&
				(record->Level == Log::LogLevel::LogLevelUbsan ||
				 record->Level == Log::LogLevel::LogLevelFunction ||
				 record->Level == Log::LogLevel::LogLevelStub ||
				 record->Level == Log::LogLevel::LogLevelDebug ||
				 record->Level == Log::LogLevel::LogLevelInfo ||
				 record->Level == Log::LogLevel::LogLevelFixme))
				return;

			static std::atomic_flag in_dispatch = ATOMIC_FLAG_INIT;

			while (in_dispatch.test_and_set(std::memory_order_acquire))
				CPU::Pause();

			KernelConsole::VirtualTerminal *vt = KernelConsole::CurrentTerminal.load(std::memory_order_acquire)->Term;

			for (char *p = prefixBuf; *p; ++p)
				vt->Process(*p);

			for (uint32_t i = 0; i < record->MessageLength; ++i)
				vt->Process(record->Message[i]);

			vt->Process('\x1b');
			vt->Process('[');
			vt->Process('0');
			vt->Process('m');
			vt->Process('\n');

			if (!Config.Quiet && Display)
				Display->UpdateBuffer();

			in_dispatch.clear(std::memory_order_release);
		}
		else
		{
			for (char *p = prefixBuf; *p; ++p)
				uart.DebugWrite(*p);

			for (uint32_t i = 0; i < record->MessageLength; ++i)
				uart.DebugWrite(record->Message[i]);

			uart.DebugWrite('\x1b');
			uart.DebugWrite('[');
			uart.DebugWrite('0');
			uart.DebugWrite('m');
			uart.DebugWrite('\n');
		}
	}

	void EarlyInit()
	{
		Renderer.CurrentFont = new Video::Font(&_binary_files_tamsyn_font_1_11_Tamsyn7x14r_psf_start,
											   &_binary_files_tamsyn_font_1_11_Tamsyn7x14r_psf_end,
											   Video::FontType::PCScreenFont2);

		size_t Rows = Display->GetWidth / Renderer.CurrentFont->GetInfo().Width;
		size_t Cols = Display->GetHeight / Renderer.CurrentFont->GetInfo().Height;
		debug("Terminal size: %ux%u", Rows, Cols);
		Terminals[0] = new ConsoleTerminal;
		Terminals[0]->Term = new VirtualTerminal(Rows, Cols, Display->GetWidth, Display->GetHeight, paint_callback, cursor_callback, scroll_callback);
		Terminals[0]->Term->Clear(0, 0, Rows, Cols - 1);
		CurrentTerminal.store(Terminals[0], std::memory_order_release);
		Log::RegisterSink(WriteFromLog);
	}

	void LoadConsoleConfig(std::string &Config)
	{
		ini_t *ini = ini_load(Config.c_str(), NULL);
		int general = ini_find_section(ini, "general", NULL);
		int cursor = ini_find_section(ini, "cursor", NULL);
		assert(general != INI_NOT_FOUND && cursor != INI_NOT_FOUND);

		int themeIndex = ini_find_property(ini, general, "theme", NULL);
		assert(themeIndex != INI_NOT_FOUND);

		int cursorBlink = ini_find_property(ini, cursor, "blink", NULL);
		int cursorColor = ini_find_property(ini, cursor, "color", NULL);
		int cursorChar = ini_find_property(ini, cursor, "char", NULL);
		int cursorDelay = ini_find_property(ini, cursor, "delay", NULL);
		assert(cursorBlink != INI_NOT_FOUND && cursorColor != INI_NOT_FOUND && cursorChar != INI_NOT_FOUND && cursorDelay != INI_NOT_FOUND);

		const char *colorThemeStr = ini_property_value(ini, general, themeIndex);
		const char *cursorColorStr = ini_property_value(ini, cursor, cursorColor);
		const char *cursorBlinkStr = ini_property_value(ini, cursor, cursorBlink);
		const char *cursorCharStr = ini_property_value(ini, cursor, cursorChar);
		const char *cursorDelayStr = ini_property_value(ini, cursor, cursorDelay);
		debug("colorThemeStr=%s", colorThemeStr);
		debug("cursorBlinkStr=%s", cursorBlinkStr);
		debug("cursorColorStr=%s", cursorColorStr);
		debug("cursorCharStr=%s", cursorCharStr);
		debug("cursorDelayStr=%s", cursorDelayStr);

		auto getColorComponent = [](const char *str, int &index) -> int
		{
			int value = 0;
			while (str[index] >= '0' && str[index] <= '9')
			{
				value = value * 10 + (str[index] - '0');
				++index;
			}
			return value;
		};

		auto parseColor = [getColorComponent](const char *colorStr) -> unsigned int
		{
			int index = 0;
			int r = getColorComponent(colorStr, index);
			if (colorStr[index] == ',')
				++index;
			int g = getColorComponent(colorStr, index);
			if (colorStr[index] == ',')
				++index;
			int b = getColorComponent(colorStr, index);
			return (r << 16) | (g << 8) | b;
		};

		if (colorThemeStr != 0)
			SetTheme(colorThemeStr);

		if (cursorBlinkStr != 0 && strncmp(cursorBlinkStr, "true", 4) == 0)
		{
			uint32_t blinkColor = 0xFFFFFF;
			if (cursorColorStr != 0)
				blinkColor = parseColor(cursorColorStr);
			debug("cursor blink with colors %X and char '%s' and delay %s", blinkColor, cursorCharStr, cursorDelayStr);
			Terminals[0]->Blink.Enabled = true;
			Terminals[0]->Blink.Color = blinkColor;
			Terminals[0]->Blink.Character = *cursorCharStr;
			Terminals[0]->Blink.Delay = atoi(cursorDelayStr);
		}

		ini_destroy(ini);
	}

	void LateInit()
	{
		Node rn = fs->Lookup(thisProcess->Info.RootNode, "/sys/cfg/term");
		if (rn == nullptr)
			return;

		{
			kstat st;
			fs->Stat(rn, &st);
			std::string cfg;
			cfg.resize(st.Size);
			fs->Read(rn, cfg.data(), st.Size, 0);
			LoadConsoleConfig(cfg);
		}

		if (Terminals[0]->Blink.Enabled)
		{
			std::thread t = std::thread(paint_blinker_thread);
			t.detach();
		}

#ifdef DEBUG
		// __test_themes();
#endif
	}
}
