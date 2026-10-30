#ifdef DEBUG

#include <fs/ustar.hpp>
#include <memory.hpp>
#include <convert.h>
#include <irq.hpp>
#include <printf.h>
#include <lock.hpp>
#include <kcon.hpp>
#include <debug.h>
#include <smp.hpp>
#include <cargs.h>
#include <cstring>
#include <io.h>

#include "../kernel.h"

void test_framebuffer()
{
#if defined(__amd64__) || defined(__i386__)
	const BootInfo::FramebufferInfo framebuffer = Display->GetFramebufferStruct();
	if (framebuffer.BaseAddress == nullptr || framebuffer.BitsPerPixel != 32 ||
		framebuffer.Pitch < framebuffer.Width * sizeof(uint32_t))
	{
		warn("Skipping framebuffer benchmark: requires a valid 32-bpp framebuffer");
		return;
	}

	constexpr size_t Rounds = 9;
	const size_t rowBytes = framebuffer.Width * sizeof(uint32_t);
	const size_t frameSize = framebuffer.Pitch * framebuffer.Height;
	uint8_t *source = static_cast<uint8_t *>(malloc(frameSize));
	if (source == nullptr)
	{
		warn("Skipping framebuffer benchmark: source buffer allocation failed");
		return;
	}

	volatile uint8_t *destination = static_cast<volatile uint8_t *>(framebuffer.BaseAddress);
	uint64_t pixelSamples[Rounds] = {};
	uint64_t bulkSamples[Rounds] = {};
	auto readTSC = []() -> uint64_t
	{
		uint32_t low, high;
		asmv("mfence\n\tlfence\n\trdtsc" : "=a"(low), "=d"(high) : : "memory");
		return (static_cast<uint64_t>(high) << 32) | low;
	};
	auto writePixels = [&]()
	{
		for (uint32_t y = 0; y < framebuffer.Height; ++y)
		{
			volatile uint32_t *row =
				reinterpret_cast<volatile uint32_t *>(destination + y * framebuffer.Pitch);
			for (uint32_t x = 0; x < framebuffer.Width; ++x)
				row[x] = 0xFF336699;
		}
	};
	auto writeBulk = [&]()
	{
		for (uint32_t y = 0; y < framebuffer.Height; ++y)
			memcpy(const_cast<uint8_t *>(destination + y * framebuffer.Pitch),
				   source + y * framebuffer.Pitch, rowBytes);
	};

	for (uint32_t y = 0; y < framebuffer.Height; ++y)
	{
		uint32_t *row = reinterpret_cast<uint32_t *>(source + y * framebuffer.Pitch);
		for (uint32_t x = 0; x < framebuffer.Width; ++x)
			row[x] = 0xFF336699;
	}

	writePixels();
	writeBulk();

	for (size_t round = 0; round < Rounds; ++round)
	{
		if (round & 1)
		{
			uint64_t start = readTSC();
			writePixels();
			pixelSamples[round] = readTSC() - start;

			start = readTSC();
			writeBulk();
			bulkSamples[round] = readTSC() - start;
		}
		else
		{
			uint64_t start = readTSC();
			writeBulk();
			bulkSamples[round] = readTSC() - start;

			start = readTSC();
			writePixels();
			pixelSamples[round] = readTSC() - start;
		}
	}

	free(source);
	auto sortSamples = [](uint64_t *samples)
	{
		for (size_t i = 1; i < Rounds; ++i)
		{
			uint64_t value = samples[i];
			size_t j = i;
			while (j > 0 && samples[j - 1] > value)
			{
				samples[j] = samples[j - 1];
				--j;
			}
			samples[j] = value;
		}
	};
	sortSamples(pixelSamples);
	sortSamples(bulkSamples);
	const uint64_t pixelMedian = pixelSamples[Rounds / 2];
	const uint64_t bulkMedian = bulkSamples[Rounds / 2];
	const uint64_t ratioPercent = bulkMedian
									  ? (pixelMedian * 100 + bulkMedian / 2) / bulkMedian
									  : 0;
	klog("Framebuffer %ux%u: pixel median %llu Kcycles/frame (range %llu-%llu), bulk median %llu Kcycles/frame (range %llu-%llu), ratio %llu%%",
		 framebuffer.Width, framebuffer.Height,
		 (pixelMedian + 500) / 1000,
		 (pixelSamples[0] + 500) / 1000,
		 (pixelSamples[Rounds - 1] + 500) / 1000,
		 (bulkMedian + 500) / 1000,
		 (bulkSamples[0] + 500) / 1000,
		 (bulkSamples[Rounds - 1] + 500) / 1000,
		 ratioPercent);
#else
	warn("Skipping framebuffer benchmark: RDTSC is only available on x86");
#endif
}

uint64_t test_framebuffer_rainbow()
{
#if defined(__amd64__) || defined(__i386__)
	static uint16_t hueStep = 0;
	const BootInfo::FramebufferInfo framebuffer = Display->GetFramebufferStruct();
	constexpr uint32_t ReservedRows = 32;
	if (framebuffer.BaseAddress == nullptr || framebuffer.BitsPerPixel != 32 ||
		framebuffer.Pitch < framebuffer.Width * sizeof(uint32_t) ||
		framebuffer.Height <= ReservedRows)
		return 0;

	constexpr uint8_t StepsPerSector = 16;
	const uint8_t sector = hueStep / StepsPerSector;
	const uint8_t step = (hueStep % StepsPerSector) * (255 / StepsPerSector);
	uint8_t red, green, blue;
	switch (sector)
	{
	case 0:
		red = 255;
		green = step;
		blue = 0;
		break;
	case 1:
		red = 255 - step;
		green = 255;
		blue = 0;
		break;
	case 2:
		red = 0;
		green = 255;
		blue = step;
		break;
	case 3:
		red = 0;
		green = 255 - step;
		blue = 255;
		break;
	case 4:
		red = step;
		green = 0;
		blue = 255;
		break;
	default:
		red = 255;
		green = 0;
		blue = 255 - step;
		break;
	}
	uint32_t color = 0xFF000000 |
					 (uint32_t(red) << framebuffer.RedMaskShift) |
					 (uint32_t(green) << framebuffer.GreenMaskShift) |
					 (uint32_t(blue) << framebuffer.BlueMaskShift);

	auto readTSC = []() -> uint64_t
	{
		uint32_t low, high;
		asmv("mfence\n\tlfence\n\trdtsc" : "=a"(low), "=d"(high) : : "memory");
		return (static_cast<uint64_t>(high) << 32) | low;
	};

	volatile uint8_t *framebufferBytes = static_cast<volatile uint8_t *>(framebuffer.BaseAddress);
	uint64_t start = readTSC();
	for (uint32_t y = ReservedRows; y < framebuffer.Height; ++y)
	{
		volatile uint32_t *row = reinterpret_cast<volatile uint32_t *>(
			framebufferBytes + static_cast<size_t>(y) * framebuffer.Pitch);
		for (uint32_t x = 0; x < framebuffer.Width; ++x)
			row[x] = color;
	}
	uint64_t elapsed = readTSC() - start;

	hueStep = (hueStep + 1) % (6 * StepsPerSector);
	return elapsed;
#else
	return 0;
#endif
}

void test_kernel_fb_speed()
{
	using namespace std::chrono_literals;
	while (1)
	{
		// thisClock->Sleep(1s);
		printf("\x1b[H%llu", (unsigned long long)test_framebuffer_rainbow());
		thisClock->Sleep(32ms);
	}

	while (1)
	{
		thisClock->Sleep(1s);
		printf("\x1b[H");
		test_framebuffer();
	}
}

#endif
