/*
 * ilu_core.h
 * Author: Jesus Diaz Garcia
 *
 * Single file library with several utilities among the following:
 * - Platform identification
 * - Assertions, debugging, errors, logging
 * - Aliases for sized types
 * - Intrinsics
 * - Strings
 * - Hashing
 * - Memory allocators
 * - Image loading
 * - File/directory/path management
 * - Process execution
 * - Dynamic library loading
 * - Mathematics
 * - Clock / timing
 */

#ifndef ILU_CORE_H
#define ILU_CORE_H

////////////////////////////////////////////////////////////////////////////////////////////////////
// Platform definitions

#if _WIN32
#	define PLATFORM_WINDOWS 1
#elif __ANDROID__
#	define PLATFORM_ANDROID 1
#elif __linux__
#	define PLATFORM_LINUX 1
#elif __APPLE__
#	define PLATFORM_APPLE 1
#else
#	error "Unsupported platform"
#endif


#if PLATFORM_LINUX
#	define USE_XCB 1
#elif PLATFORM_ANDROID
#	define USE_ANDROID 1
#elif PLATFORM_WINDOWS
#	define USE_WINAPI 1
#endif


#if PLATFORM_WINDOWS
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#undef Yield          // Empty macro defined in winbase.h
#include <intrin.h>   // _WriteBarrier
#elif PLATFORM_LINUX || PLATFORM_ANDROID
#include <time.h>     // TODO: Find out if this header belongs to the C runtime library...
#include <sys/stat.h> // stat
#include <fcntl.h>    // open
#include <unistd.h>   // read, close, getcwd
#include <string.h>   // strerror_r
#include <errno.h>    // errno
#include <sys/mman.h> // mmap
#include <dirent.h>   // opendir/readdir/closedir
#include <pthread.h>
#include <semaphore.h>
#endif

#if PLATFORM_LINUX || PLATFORM_ANDROID
#include <sys/wait.h>
#include <spawn.h>
#include <dlfcn.h>
#endif

#if PLATFORM_LINUX
#include <linux/ioctl.h> // ioctl
#include <linux/input.h> // input_event
#endif

#if PLATFORM_ANDROID
#include <android/sensor.h>
#include <android/log.h>
#include <android_native_app_glue.h>
#include <aaudio/AAudio.h>
#endif


////////////////////////////////////////////////////////////////////////////////////////////////////
// Useful defines

#define internal static

#define KB(x) (1024ul * x)
#define MB(x) (1024ul * KB(x))
#define GB(x) (1024ul * MB(x))
#define TB(x) (1024ul * GB(x))

#define MAX_PATH_LENGTH 512

enum LogVerbosityLevel
{
	Error,
	Warning,
	Info,
	Debug,
	LogVerbosityLevelCount,
};

#define LOG_VERBOSITY Info

#if PLATFORM_ANDROID
static const int __android_log_channel[] = {
	ANDROID_LOG_ERROR,
	ANDROID_LOG_WARN,
	ANDROID_LOG_INFO,
	ANDROID_LOG_DEBUG,
};
#define LOG(channel, fmt, ...) if ( channel <= LOG_VERBOSITY ) { ((void)__android_log_print(__android_log_channel[channel], "tools", fmt, ##__VA_ARGS__)); }
#else
#define LOG(channel, fmt, ...) if ( channel <= LOG_VERBOSITY ) { Printf(fmt, ##__VA_ARGS__); }
#endif

#if PLATFORM_ANDROID
#define QUIT_ABNORMALLY() __builtin_trap();
#else
#define QUIT_ABNORMALLY() *((int*)0) = 0;
#endif

#define ASSERT(expression) if ( !(expression) ) { \
		LOG(Error, "%s", "Assertion failed: ASSERT( " #expression " )\n" ); \
		QUIT_ABNORMALLY(); \
	}
#define ASSERTMSG(expression, message, ...) if ( !(expression) ) { \
		LOG(Error, "Assertion failed: ASSERT(" #expression " )\nAssertion message: " message, ##__VA_ARGS__); \
		QUIT_ABNORMALLY(); \
	}

#define INVALID_CODE_PATH() ASSERT(0 && "Invalid code path")
#define INVALID_CODE_PATH_MSG(message) ASSERT(0 && message)
#define ARRAY_COUNT(array) (sizeof(array)/sizeof(array[0]))

#define CT_ASSERT3(expression, number) static int ct_assert_##number[(expression) ? 1 : -1]
#define CT_ASSERT2(expression, number) CT_ASSERT3(expression, number)
#define CT_ASSERT(expression) CT_ASSERT2(expression, __COUNTER__)



////////////////////////////////////////////////////////////////////////////////////////////////////
// Sized types

typedef char i8;
typedef short int i16;
typedef int i32;
typedef long long int i64;
typedef unsigned char u8;
typedef unsigned short int u16;
typedef unsigned int u32;
typedef unsigned long long int u64;
typedef float f32;
typedef double f64;
typedef	unsigned char byte;

CT_ASSERT(sizeof(i8) == 1);
CT_ASSERT(sizeof(i16) == 2);
CT_ASSERT(sizeof(i32) == 4);
CT_ASSERT(sizeof(i64) == 8);
CT_ASSERT(sizeof(u8) == 1);
CT_ASSERT(sizeof(u16) == 2);
CT_ASSERT(sizeof(u32) == 4);
CT_ASSERT(sizeof(u64) == 8);
CT_ASSERT(sizeof(f32) == 4);
CT_ASSERT(sizeof(f64) == 8);
CT_ASSERT(sizeof(byte) == 1);

#define I8_MIN -128
#define I8_MAX 127
#define U8_MIN 0
#define U8_MAX 255
#define I16_MIN -32768
#define I16_MAX 32767
#define U16_MIN 0
#define U16_MAX 65535
#define I32_MIN -2147483648
#define I32_MAX 2147483647
#define U32_MIN 0
#define U32_MAX 4294967295

#if PLATFORM_WINDOWS
CT_ASSERT(sizeof(long) == sizeof(i32));
typedef volatile LONG volatile_i32;
typedef volatile ULONG volatile_u32;
typedef volatile LONG64 volatile_i64;
#else
typedef volatile i32 volatile_i32;
typedef volatile u32 volatile_u32;
typedef volatile i64 volatile_i64;
#endif



////////////////////////////////////////////////////////////////////////////////////////////////////
// Forward-declared instead of including <math.h>

#if PLATFORM_WINDOWS
extern "C"
{
	float sinf(float value);
	float cosf(float value);
	float tanf(float value);
	float sqrtf(float value);
	float ceilf(float value);
	float floorf(float value);
	float log2f(float value);
	float powf(float base, float exponent);
}
#else
#include <math.h>
#endif


////////////////////////////////////////////////////////////////////////////////////////////////////
// IO: printf, vprintf, sprintf, vsprintf, etc

// Real (non-CRT) implementation, so that printing text never links printf/vprintf.
#define STB_SPRINTF_IMPLEMENTATION
#include "libs/stb/stb_sprintf.h"

#if PLATFORM_WINDOWS

// Forward-declared instead of including <stdio.h>
#ifndef _FILE_DEFINED
#define _FILE_DEFINED
struct _iobuf;
typedef struct _iobuf FILE;
#endif // _FILE_DEFINED

#define SEEK_SET 0
#define SEEK_CUR 1

extern "C"
{
	FILE *fopen(const char *filename, const char *mode);
	int fclose(FILE *stream);
	size_t fread(void *buffer, size_t size, size_t count, FILE *stream);
	size_t fwrite(const void *buffer, size_t size, size_t count, FILE *stream);
	int fseek(FILE *stream, long offset, int origin);
	int feof(FILE *stream);
}

static char *Win32PrintCallback(const char *buffer, void *user, int len)
{
	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD bytesWritten;
	WriteFile(handle, buffer, (DWORD)len, &bytesWritten, NULL);
	return (char *)buffer;
}

int VPrintf(const char *format, va_list vaList)
{
	char buffer[STB_SPRINTF_MIN];
	const int res = stbsp_vsprintfcb(Win32PrintCallback, NULL, buffer, format, vaList);
	return res;
}

int Printf(const char *format, ...)
{
	va_list vaList;
	va_start(vaList, format);
	const int res = VPrintf(format, vaList);
	va_end(vaList);
	return res;
}

#else

#include <stdio.h>
#define Printf printf
#define VPrintf vprintf

#endif

i32 VSPrintf(char *buffer, const char *format, va_list vaList)
{
	const i32 res = stbsp_vsprintf(buffer, format, vaList);
	return res;
}

i32 VSNPrintf(char *buffer, u32 size, const char *format, va_list vaList)
{
	const i32 res = stbsp_vsnprintf(buffer, size, format, vaList);
	return res;
}

i32 SPrintf(char *buffer, const char *format, ...)
{
	va_list vaList;
	va_start(vaList, format);
	const i32 res = VSPrintf(buffer, format, vaList);
	va_end(vaList);
	return res;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Truncations

u32 U64ToU32(u64 value)
{
	ASSERT(value <= U32_MAX);
	const u32 res = (u32)value;
	return res;
}

i32 U32ToI32(u32 value)
{
	ASSERT( value <= I32_MAX );
	const i32 res = (i32)value;
	return res;
}

i16 I32ToI16(i32 value)
{
	ASSERT( value >= I16_MIN && value <= I16_MAX );
	const i16 res = (i16)value;
	return res;
}

u16 I32ToU16(i32 value)
{
	ASSERT( value >= 0 && value <= U16_MAX );
	const u16 res = (u16)value;
	return res;
}

u8 I32ToU8(i32 value)
{
	ASSERT( value >= 0 && value <= U8_MAX );
	const u8 res = (u8)value;
	return res;
}

i16 ClipI32ToI16(i32 value)
{
	if ( value < I16_MIN ) { return I16_MIN; }
	if ( value > I16_MAX ) { return I16_MAX; }
	return value;
}




////////////////////////////////////////////////////////////////////////////////////////////////////
// System helpers

#if PLATFORM_WINDOWS

void Win32ReportError(const char *context)
{
	DWORD errorCode = ::GetLastError();
	if ( errorCode != ERROR_SUCCESS )
	{
		LPVOID messageBuffer;
		FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER |
				FORMAT_MESSAGE_FROM_SYSTEM |
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				errorCode,
				MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
				(LPTSTR)&messageBuffer,
				0, NULL );

		LOG(Error, "Error (%s): %s\n", context, (char*)messageBuffer);
		LocalFree( messageBuffer );
	}
}

#elif PLATFORM_LINUX || PLATFORM_ANDROID

void LinuxReportError(const char *context)
{
	char buffer[1024] = {};
	const char *errorString = strerror_r(errno, buffer, ARRAY_COUNT(buffer));
	LOG(Error, "Error (%s): %s\n", context, errorString);
}

#endif



////////////////////////////////////////////////////////////////////////////////////////////////////
// Intrinsics

#define TOOLS_USE_INTRINSICS 1

#if TOOLS_USE_INTRINSICS
#if PLATFORM_WINDOWS
// Count trailing zeros
u32 CTZ(u32 bitMask)
{
	ASSERT(bitMask != 0);
	unsigned long firstBitSetIndex;
	_BitScanForward(&firstBitSetIndex, bitMask);
	return firstBitSetIndex;
}

// Count leading zeros
u32 CLZ(u32 bitMask)
{
	ASSERT(bitMask != 0);
	unsigned long lastBitSetIndex;
	_BitScanReverse(&lastBitSetIndex, bitMask);
	return 31 - lastBitSetIndex;
}
#else // #if PLATFORM_WINDOWS
// Count trailing zeros
u32 CTZ(u32 bitMask)
{
	ASSERT(bitMask != 0);
	u32 count = __builtin_ctz(bitMask);
	return count;
}

// Count leading zeros
u32 CLZ(u32 bitMask)
{
	ASSERT(bitMask != 0);
	u32 count = __builtin_clz(bitMask);
	return count;
}
#endif // #else // #if PLATFORM_WINDOWS
#else // #if TOOLS_USE_INTRINSICS
// Count trailing zeros
u32 CTZ(u32 bitMask)
{
	ASSERT(bitMask != 0);
	u32 count = 0;
	while ((bitMask & 1) == 0) {
		bitMask >>= 1;
		++count;
	}
	return count;
}

// Count leading zeros
u32 CLZ(u32 bitMask)
{
	// TODO: This has not been tested
	ASSERT(bitMask != 0);
	u32 count = 0;
	while ((bitMask & 0x80000000) == 0) {
		bitMask <<= 1;
		++count;
	}
	return count;
}
#endif // #else // #if TOOLS_USE_INTRINSICS

// First bit set
u32 FBS(u32 bitMask)
{
	const u32 res = CTZ(bitMask);
	return res;
}

// First bit zero
u32 FBZ(u32 bitMask)
{
	const u32 res = CTZ(~bitMask);
	return res;
}

bool AtomicSwap(volatile_u32 *currValue, u32 oldValue, u32 newValue)
{
#if PLATFORM_WINDOWS
		const bool swapped = InterlockedCompareExchange(currValue, newValue, oldValue) != *currValue;
#elif PLATFORM_LINUX || PLATFORM_ANDROID
		const bool swapped = __sync_bool_compare_and_swap(currValue, oldValue, newValue);
#else
#error "Missing implementation"
#endif
	return swapped;
}

bool AtomicSwap(volatile_i64 *currValue, i64 oldValue, i64 newValue)
{
#if PLATFORM_WINDOWS
		const bool swapped = InterlockedCompareExchange64(currValue, newValue, oldValue) != *currValue;
#elif PLATFORM_LINUX || PLATFORM_ANDROID
		const bool swapped = __sync_bool_compare_and_swap(currValue, oldValue, newValue);
#else
#error "Missing implementation"
#endif
	return swapped;
}

i64 AtomicPreIncrement(volatile_i64 *value)
{
#if PLATFORM_WINDOWS
	const i64 oldValue = InterlockedIncrement64(value) - 1;
#elif PLATFORM_LINUX || PLATFORM_ANDROID
	const i64 oldValue = __sync_fetch_and_add(value, 1);
#else
#error "Missing implementation"
#endif
	return oldValue;
}

inline void AtomicIncrement(volatile_i64 *value)
{
	AtomicPreIncrement(value);
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Strings

struct String
{
	const char* str;
	u32 size;
};

u32 StrLen(const char *str)
{
	u32 len = 0;
	while (*str++) ++len;
	return len;
}

String MakeString(const char *str, u32 size)
{
	String string = { str, size };
	return string;
}

String MakeString(const char *str)
{
	u32 size = StrLen(str);
	String string = { str, size };
	return string;
}

void StrCopy(char *dst, const String& src_string)
{
	u32 size = src_string.size;
	const char *src = src_string.str;
	while (size-- > 0) *dst++ = *src++;
	*dst = '\0';
}

void StrCopy(char *dst, const char *src)
{
	while (*src) *dst++ = *src++;
	*dst = 0;
}

void StrCopyN(char *dst, const char *src, u32 size)
{
	while (*src && size-- > 0) *dst++ = *src++;
	*dst = 0;
}

void StrCat(char *dst, const char *src)
{
	while (*dst) ++dst;
	StrCopy(dst, src);
}

void StrCat(char *dst, const String &src)
{
	while (*dst) ++dst;
	StrCopy(dst, src);
}

bool StrEq(const String &s1, const String &s2)
{
	if ( s1.size != s2.size ) return false;

	const char *str1 = s1.str;
	const char *str2 = s2.str;
	const char *str2end = s2.str + s2.size;

	while ( *str1 == *str2 && str2 != str2end )
	{
		str1++;
		str2++;
	}

	return str2 == str2end;
}

bool StrEq(const String &s11, const char *s2)
{
	const char *s1 = s11.str;
	u32 count = s11.size;
	while ( count > 0 && *s1 == *s2 )
	{
		s1++;
		s2++;
		count--;
	}
	return count == 0 && *s2 == 0;
}

bool StrEq(const char *s1, const char *s2)
{
	if ( !s1 && !s2 ) return true;
	if ( s1 != s2 && ( !s1 || !s2 ) ) return false;

	while ( *s1 == *s2 && *s1 )
	{
		s1++;
		s2++;
	}
	return *s1 == *s2;
}

bool StrEqN(const char *s1, const char *s2, u32 n)
{
	while ( *s1 == *s2 && *s1 && n > 0 )
	{
		s1++;
		s2++;
		n--;
	}
	return n == 0 || *s1 == *s2;
}

// Searches str2 in str1 and returns a pointer to the last matching character '\0'.
// If the beginning of str1 does not completely match str2, then NULL is returned.
const char *StrConsume( const char *str1, const char *str2 )
{
	while ( *str2 && *str1++ == *str2++ );
	// If str does not point to the last char, there's no match so NULL is returned.
	// Otherwise the 'advanced' str1 pointer is returned.
	return *str2 ? NULL: str1;
}


bool StrToBool(const char *str, u32 len = U32_MAX)
{
	const bool value = *str == '1' || StrEqN(str, "true", len);
	return value;
}

bool StrToBool(const String &s)
{
	const bool value = StrToBool(s.str, s.size);
	return value;
}

char StrToChar(const char *str, u32 len = U32_MAX)
{
	return len > 0 ? *str : '?';
}

char StrToChar(const String &s)
{
	const char value = StrToChar(s.str, s.size);
	return value;
}

const char *StrChar(const char *str, char c)
{
	while (*str && *str != c) {
		str++;
	}
	return *str ? str : nullptr;
}

void StrReplace(char *str, char a, char b)
{
	while (*str) {
		if (*str == a) {
			*str = b;
		}
		str++;
	}
}

const char *StrCharR(const char *str, char c)
{
	const char *res = nullptr;
	while ( (str = StrChar(str, c)) != 0 ) {
		res = str;
		str++;
	}
	return res;
}

i32 StrToInt(const char *str, u32 len = U32_MAX)
{
	i32 integer = 0;

	// scan sign
	bool negative = false;
	if (*str == '-') {
		negative = true;
		str++;
		len--;
	}

	// scan integer part
	while (*str >= '0' && *str <= '9' && len > 0) {
		integer = (integer << 3) + (integer << 1); // x10
		integer += *str++ - '0';
		len--;
	}

	const i32 result = negative ? -integer : integer;
	return result;
}

i32 StrToInt(const String &s)
{
	const i32 number = StrToInt(s.str, s.size);
	return number;
}

u32 StrToUnsignedInt(const char *str, u32 len = U32_MAX)
{
	u32 integer = 0;

	// scan integer part
	while (*str >= '0' && *str <= '9' && len > 0) {
		integer = (integer << 3) + (integer << 1); // x10
		integer += *str++ - '0';
		len--;
	}

	return integer;
}

u32 StrToUnsignedInt(const String &s)
{
	const u32 number = StrToUnsignedInt(s.str, s.size);
	return number;
}

i64 StrToI64(const char *str, u32 len = U32_MAX)
{
	i64 integer = 0;

	// scan sign
	bool negative = false;
	if (*str == '-') {
		negative = true;
		str++;
		len--;
	}

	// scan integer part
	while (*str >= '0' && *str <= '9' && len > 0) {
		integer = (integer << 3) + (integer << 1); // x10
		integer += *str++ - '0';
		len--;
	}

	const i64 result = negative ? -integer : integer;
	return result;
}

i64 StrToI64(const String &s)
{
	const i64 number = StrToI64(s.str, s.size);
	return number;
}

f32 StrToFloat(const char *str, u32 len = U32_MAX)
{
	i32 integer = 0;

	// scan sign
	f32 sign = 1.0f;
	if (*str == '-') {
		sign = -1.0f;
		str++;
		len--;
	}

	// scan integer part
	while (*str >= '0' && *str <= '9' && len > 0) {
		integer = (integer << 3) + (integer << 1); // x10
		integer += *str++ - '0';
		len--;
	}

	switch (*str++) {
		case '.': break;
		default: return integer;
	}
	len--;

	// scan decimal part
	u32 tenPower = 1;
	while (*str >= '0' && *str <= '9' && len > 0) {
		tenPower = (tenPower << 3) + (tenPower << 1);
		integer = (integer << 3) + (integer << 1); // x10
		integer += *str++ - '0';
		len--;
	}

	const f32 value = sign * integer / (f32)tenPower;
	return value;
}

f32 StrToFloat(const String &s)
{
	const f32 number = StrToFloat(s.str, s.size);
	return number;
}

bool StrIsInteger(const char *str)
{
	if (*str == '-') str++;
	if (*str == '\0') return false;
	while (*str) {
		if (*str < '0' || *str > '9') return false;
		str++;
	}
	return true;
}

bool StrIsFloat(const char *str)
{
	if (*str == '-') str++;
	if (*str == '.') return false;
	bool hasDigit = false;
	bool hasDot = false;
	while (*str) {
		if (*str >= '0' && *str <= '9') {
			hasDigit = true;
		} else if (*str == '.' && !hasDot) {
			hasDot = true;
		} else {
			return false;
		}
		str++;
	}
	return hasDigit;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Hashing

#define TOOLS_HASH_FNV_SEED 16777619

u32 HashFNV(const void *data, u32 size, u32 initial = 0, u32 prime = TOOLS_HASH_FNV_SEED)
{
	u32 hash = initial;
	const unsigned char *str = (const unsigned char *)data;
	for (u32 i = 0; i < size; ++i) {
		hash = (hash * prime) ^ str[i];
	}
	return hash;
}

u32 HashStringFNV(const char *data, u32 initial = 0, u32 prime = TOOLS_HASH_FNV_SEED)
{
	u32 hash = initial;
	const unsigned char *str = (const unsigned char *)data;
	while (*str) {
		hash = (hash * prime) ^ *str++;
	}
	return hash;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Memory

#if PLATFORM_LINUX || PLATFORM_ANDROID || PLATFORM_APPLE

void* AllocateVirtualMemory(u32 size)
{
	void* baseAddress = 0;
	i32 prot = PROT_READ | PROT_WRITE;
	i32 flags = MAP_PRIVATE | MAP_ANONYMOUS;
	i32 fd = -1;
	off_t offset = 0;
	void *allocatedMemory = mmap(baseAddress, size, prot, flags, fd, offset);
	ASSERT( allocatedMemory != MAP_FAILED && "Failed to allocate memory." );
	return allocatedMemory;
}

#elif PLATFORM_WINDOWS

void* AllocateVirtualMemory(u32 size)
{
	void *data = VirtualAlloc(0, size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	return data;
}

#endif

void MemSet(void *ptr, u32 size, byte value)
{
	byte *bytePtr = (byte*)ptr;
	while (size-- > 0) *bytePtr++ = value;
}

void MemCopy(void *dst, const void *src, u32 size)
{
	const byte *pSrc = (byte*) src;
	const byte *pEnd = pSrc + size;
	byte *pDst = (byte*) dst;
	while (pSrc != pEnd) *pDst++ = *pSrc++;
}

i32 MemCompare(const void *a, const void *b, u32 size)
{
	const byte *pA = (const byte*) a;
	const byte *pB = (const byte*) b;
	const byte *pEnd = pA + size;
	while (pA != pEnd) {
		const byte valueA = *pA++;
		const byte valueB = *pB++;
		if (valueA != valueB) {
			return valueA - valueB;
		}
	}
	return 0;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Arena

struct Arena
{
	byte* base;
	u32 used;
	u32 size;
	const char *name;
};

Arena MakeArena(byte* base, u32 size, const char *name)
{
	ASSERTMSG(base != NULL, "MakeArena needs a non-null base pointer.");
	ASSERTMSG(size > 0, "MakeArena needs a size greater-than-zero.");
	Arena arena = {};
	arena.base = base;
	arena.size = size;
	arena.used = 0;
	arena.name = name;
	return arena;
}

Arena MakeSubArena(Arena &arena, u32 size, const char *name)
{
	ASSERTMSG(arena.used + size <= arena.size,
			"MakeSubArena out of bounds of the memory arena.\n"
			"- Arena: %s\n"
			"- Subarena: %s\n",
			arena.name, name);
	Arena subarena = {};
	subarena.base = arena.base + arena.used;
	subarena.size = size;
	subarena.used = 0;
	subarena.name = name;
	return subarena;
}

Arena MakeSubArena(Arena &arena, const char *name)
{
	u32 remainingSize = arena.size - arena.used;
	Arena subarena = MakeSubArena(arena, remainingSize, name);
	return subarena;
}

Arena PushSubArena(Arena &arena, u32 size, const char *name)
{
	Arena subArena = MakeSubArena(arena, size, name);
	arena.used += size;
	return subArena;
}

Arena PushSubArena(Arena &arena, const char *name)
{
	Arena subArena = MakeSubArena(arena, name);
	arena.used = arena.size;
	return subArena;
}

byte* PushSize(Arena &arena, u32 size)
{
	ASSERTMSG(arena.used + size <= arena.size,
		"PushSize out of bounds of the memory arena.\n"
		"- Arena: %s\n"
		"- push size: %u\n- arena size: %u\n- arena remaining size: %u\n).",
		arena.name,
		size, arena.size, arena.size - arena.used);
	byte* head = arena.base + arena.used;
	arena.used += size;
	return head;
}

byte* PushZeroSize(Arena &arena, u32 size)
{
	byte *bytes = PushSize(arena, size);
	MemSet(bytes, size, 0);
	return bytes;
}

char *PushStringN(Arena &arena, const char *str, u32 len)
{
	char *bytes = (char*)PushSize(arena, len+1);
	MemCopy(bytes, str, len);
	bytes[len] = 0;
	return bytes;
}

char *PushString(Arena &arena, const char *str)
{
	u32 len = StrLen(str);
	char *bytes = PushStringN(arena, str, len);
	return bytes;
}

char *PushString(Arena &arena, String str)
{
	char *bytes = PushStringN(arena, str.str, str.size);
	return bytes;
}

char *PushChar(Arena &arena, char c)
{
	char *bytes = (char*)PushSize(arena, sizeof(c));
	*bytes = c;
	return bytes;
}

void ResetArena(Arena &arena)
{
	// This tells the OS we don't need these pages
	//madvise(arena.base, arena.size, MADV_DONTNEED);
	arena.used = 0;
}

#define ZeroStruct( pointer ) MemSet(pointer, sizeof(*pointer), 0)
#define PushStruct( arena, struct_type ) (struct_type*)PushSize(arena, sizeof(struct_type))
#define PushArray( arena, type, count ) (type*)PushSize(arena, sizeof(type) * (count))
#define PushZeroStruct( arena, struct_type ) (struct_type*)PushZeroSize(arena, sizeof(struct_type))
#define PushZeroArray( arena, type, count ) (type*)PushZeroSize(arena, sizeof(type) * (count))



////////////////////////////////////////////////////////////////////////////////////////////////////
// String interning

struct StringIntern
{
	char *str;
	u32 hash;
};

struct StringInterningNode
{
	StringIntern stringIntern;
	StringInterningNode *next;
};

struct StringInterningTable
{
	StringInterningNode* bins[1024];
};

struct StringInterning
{
	Arena *arena;
	StringInterningTable *table;
};

StringInterning StringInterningCreate(Arena *arena)
{
	StringInterning interning = {
		.arena = arena,
		.table = PushZeroStruct(*arena, StringInterningTable),
	};
	return interning;
}

const char *MakeStringIntern(StringInterning *context, const char *str, u32 len)
{
	ASSERT(context && context->table);
	const u32 hash = HashFNV(str, len);
	const u32 index = hash % ARRAY_COUNT(context->table->bins);

	StringInterningNode *next = NULL;

	if (context->table->bins[index])
	{
		StringInterningNode *node = context->table->bins[index];
		while (node)
		{
			if (node->stringIntern.hash == hash) // same hash?
			{
				if (StrEq(node->stringIntern.str, str)) // same string?
				{
					return node->stringIntern.str; // found!
				}
			}
			node = node->next; // collision! keep searching...
		}
		next = context->table->bins[index];
	}

	// No coincidence found, insert a new node
	StringInterningNode *node = PushZeroStruct(*(context->arena), StringInterningNode);
	node->stringIntern.str = PushStringN(*(context->arena), str, len);
	node->stringIntern.hash = hash;
	node->next = next;
	context->table->bins[index] = node;
	return node->stringIntern.str;
}

const char *MakeStringIntern(StringInterning *context, const char *str)
{
	const u32 len = StrLen(str);
	const char *internStr = MakeStringIntern(context, str, len);
	return internStr;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Files

struct DataChunk
{
	union {
		byte *bytes;
		const char *chars;
	};
	u64 size;
};

bool GetFileSize(const char *filename, u64 &size, bool reportError = true)
{
	bool ok = true;
#if PLATFORM_WINDOWS
	WIN32_FILE_ATTRIBUTE_DATA Data;
	if( GetFileAttributesExA(filename, GetFileExInfoStandard, &Data) )
	{
		size = (u64)Data.nFileSizeLow;
		size |= (u64)Data.nFileSizeHigh << 32;
	}
	else
	{
		if ( reportError ) Win32ReportError("GetFileSize");
		ok = false;
	}
#elif PLATFORM_LINUX || PLATFORM_ANDROID
	struct stat attrib;
	if ( stat(filename, &attrib) == 0 )
	{
		size = attrib.st_size;
	}
	else
	{
		if ( reportError )
		{
			char text[MAX_PATH_LENGTH];
			SPrintf(text, "stat %s", filename);
			LinuxReportError(text);
		}
		ok = false;
	}
#endif
	return ok;
}

bool ExistsFile(const char *filename)
{
	u64 size = 0;
	const bool res = GetFileSize(filename, size, false);
	return res;
}

struct File
{
	bool isOpen;
	u64 size;
#if PLATFORM_LINUX || PLATFORM_ANDROID
	int handle;
#elif PLATFORM_WINDOWS
	HANDLE handle;
#else
#error "Missing implementation"
#endif
};

enum FileMode
{
	FileModeRead,
	FileModeWrite,
};

File OpenFile(const char *filename, FileMode mode)
{
	File file = {};
	if ( GetFileSize( filename, file.size ) && file.size > 0 )
	{
#if  PLATFORM_LINUX || PLATFORM_ANDROID
		const int openMode = mode == FileModeRead ? O_RDONLY : O_WRONLY;
		file.handle = open(filename, openMode);
		if ( file.handle == -1 ) {
			LinuxReportError("open");
		} else {
			file.isOpen = true;
		}
#else
		ASSERT(mode == FileModeRead); // TODO: Implement for write-only
		file.handle = CreateFileA( filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL );
		if ( file.handle == INVALID_HANDLE_VALUE  ) {
			Win32ReportError("CreateFileA");
		} else {
			file.isOpen = true;
		}
#endif
	}
	return file;
}

bool ReadFromFile(File file, void *buffer, u64 size)
{
	bool ok = false;
	u64 bytesToRead = size;

#if PLATFORM_LINUX || PLATFORM_ANDROID
	while ( bytesToRead > 0 )
	{
		u64 bytesRead = read(file.handle, buffer, bytesToRead);
		if ( bytesRead > 0 )
		{
			bytesToRead -= bytesRead;
			buffer = (byte*)buffer + bytesRead;
		}
		else
		{
			LinuxReportError("read");
			break;
		}
	}
	ok = (bytesToRead == 0);
#elif PLATFORM_WINDOWS
	DWORD bytesRead = 0;
	ok = ReadFile(file.handle, buffer, bytesToRead, &bytesRead, NULL);
	if ( !ok && bytesToRead == bytesRead )
	{
		Win32ReportError("ReadEntireFile - ReadFile");
	}
#else
#error "Missing implementation"
#endif

	return ok;
}

bool FileSeek(File file, u64 offset)
{
	bool ok = false;
#if PLATFORM_LINUX || PLATFORM_ANDROID
	int res = lseek(file.handle, offset, SEEK_SET);
	if ( res == -1 ) {
		LinuxReportError("lseek");
	} else {
		ok = true;
	}
#elif PLATFORM_WINDOWS
	const LARGE_INTEGER iOffset = { .QuadPart = (LONGLONG)offset };
	ok = SetFilePointerEx(file.handle, iOffset, NULL, FILE_BEGIN);
	if ( !ok ) {
		Win32ReportError("SetFilePointerEx");
	}
#else
#error "Missing implementation"
#endif
	return ok;
}

byte *PushDataFromFile(Arena &arena, File file, u64 offset, u64 size)
{
	if ( FileSeek(file, offset) ) {
		Arena modifiedArena = arena;
		byte *bytes = PushSize(modifiedArena, size);
		if ( ReadFromFile(file, bytes, size) ) {
			arena = modifiedArena;
			return bytes;
		}
	}
	return nullptr;
}

void CloseFile(File &file)
{
	if (file.isOpen)
	{
#if PLATFORM_LINUX || PLATFORM_ANDROID
		close(file.handle);
#elif PLATFORM_WINDOWS
		CloseHandle(file.handle);
#else
#error "Missing implementation"
#endif
		file.isOpen = false;
	}
}

bool ReadEntireFile(const char *filename, void *buffer, u64 bytesToRead)
{
	bool ok = false;
#if PLATFORM_WINDOWS
	HANDLE file = CreateFileA( filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL );
	if ( file == INVALID_HANDLE_VALUE  )
	{
		Win32ReportError("ReadEntireFile - CreateFileA");
	}
	else
	{
		DWORD bytesRead = 0;
		ok = ReadFile( file, buffer, bytesToRead, &bytesRead, NULL );
		if ( !ok && bytesToRead == bytesRead )
		{
			Win32ReportError("ReadEntireFile - ReadFile");
		}
		CloseHandle( file );
	}
#elif PLATFORM_LINUX || PLATFORM_ANDROID
	int fd = open(filename, O_RDONLY);
	if ( fd == -1 )
	{
		LinuxReportError("open");
	}
	else
	{
		while ( bytesToRead > 0 )
		{
			u64 bytesRead = read(fd, buffer, bytesToRead);
			if ( bytesRead > 0 )
			{
				bytesToRead -= bytesRead;
				buffer = (byte*)buffer + bytesRead;
			}
			else
			{
				LinuxReportError("read");
				break;
			}
		}
		ok = (bytesToRead == 0);
		close(fd);
	}
#else
#error "Missing implementation"
#endif
	return ok;
}

bool WriteEntireFile(const char *filename, const void *buffer, u64 bytesToWrite)
{
	// TODO: Backup the previous file?

	bool ok = false;
#if PLATFORM_WINDOWS
	HANDLE file = CreateFileA( filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( file == INVALID_HANDLE_VALUE  )
	{
		Win32ReportError("WriteEntireFile - CreateFileA");
	}
	else
	{
		DWORD bytesWritten = 0;
		ok = WriteFile( file, buffer, bytesToWrite, &bytesWritten, NULL );
		if ( !ok && bytesToWrite == bytesWritten )
		{
			Win32ReportError("WriteEntireFile - WriteFile");
		}
		CloseHandle( file );
	}
#elif PLATFORM_LINUX || PLATFORM_ANDROID
	int fd = open(filename, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR );
	if ( fd == -1 )
	{
		LinuxReportError("open");
	}
	else
	{
		while ( bytesToWrite > 0 )
		{
			const u64 bytesWritten = write(fd, buffer, bytesToWrite);
			if ( bytesWritten > 0 )
			{
				bytesToWrite -= bytesWritten;
				buffer = (byte*)buffer + bytesWritten;
			}
			else
			{
				LinuxReportError("write");
				break;
			}
		}
		ok = (bytesToWrite == 0);
		close(fd);
	}
#else
#error "Missing implementation"
#endif
	return ok;
}

DataChunk *PushFile( Arena& arena, const char *filename )
{
	DataChunk *file = 0;

	u64 fileSize;
	if ( GetFileSize( filename, fileSize ) && fileSize > 0 )
	{
		Arena backupArena = arena;
		byte *fileData = PushArray( arena, byte, fileSize + 1 );
		if ( ReadEntireFile( filename, fileData, fileSize ) )
		{
			fileData[fileSize] = 0; // final zero
			file = PushStruct( arena, DataChunk );
			file->bytes = fileData;
			file->size = fileSize;
		}
		else
		{
			// TODO: Log error here?
			arena = backupArena;
		}
	}

	return file;
}

bool GetFileLastWriteTimestamp(const char* filename, u64 &ts)
{
	bool ok = true;

#if PLATFORM_WINDOWS
	union Filetime2u64 {
		FILETIME filetime;
		u64      u64time;
	} conversor;

	WIN32_FILE_ATTRIBUTE_DATA Data;
	if( GetFileAttributesExA(filename, GetFileExInfoStandard, &Data) )
	{
		conversor.filetime = Data.ftLastWriteTime;
		ts = conversor.u64time;
	}
	else
	{
		char text[MAX_PATH_LENGTH];
		SPrintf(text, "GetFileLastWriteTimestamp %s", filename);
		Win32ReportError(text);
		ok = false;
		ts = 0;
	}
#elif PLATFORM_LINUX || PLATFORM_ANDROID
	struct stat attrib;
	if ( stat(filename, &attrib) == 0 )
	{
		ts = attrib.st_mtime;
	}
	else
	{
		char text[MAX_PATH_LENGTH];
		SPrintf(text, "stat %s", filename);
		LinuxReportError(text);
		ok = false;
		ts = 0;
	}
#endif

	return ok;
}

bool CopyFile(const char *srcPath, const char *dstPath)
{
	bool ok = false;
#if PLATFORM_LINUX || PLATFORM_ANDROID
	int fd_rd = open(srcPath, O_RDONLY);
	if ( fd_rd == -1 ) {
		LinuxReportError("CopyFile open O_RDONLY");
	} else {
		int fd_wr = open(dstPath, O_WRONLY | O_CREAT | O_TRUNC | O_EXCL, 0666);
		if ( fd_wr == -1 ) {
			LinuxReportError("CopyFile open O_WRONLY | O_CREAT | O_EXCL");
		} else {
			char buffer[4096];
			ssize_t nread;
			while (nread = read(fd_rd, buffer, sizeof(buffer)), nread > 0) {
				char *out_ptr = buffer;
				ssize_t nwritten;
				do {
					nwritten = write(fd_wr, out_ptr, nread);
					if (nwritten >= 0) {
						nread -= nwritten;
						out_ptr += nwritten;
					} else if (errno != EINTR) {
						LinuxReportError("CopyFile write");
						close(fd_rd);
						close(fd_wr);
						return false;
					}
				} while (nread > 0);
			}
			ok = true;
			close(fd_wr);
		}
		close(fd_rd);
	}
#elif PLATFORM_WINDOWS
	ok = CopyFile(srcPath, dstPath, false);
	if ( !ok ) {
		Win32ReportError("CopyFile");
	}
#else
#error "Missing implementation"
#endif
	return ok;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// File paths

struct FilePath
{
	char str[MAX_PATH_LENGTH];
};

FilePath MakePath(const char *basePath, const char *relativePath)
{
	FilePath path = {};
	StrCopy(path.str, basePath);
	StrCat(path.str, "/");
	StrCat(path.str, relativePath);
	return path;
}

bool HasFileExtension(const char *filename, const char *extension)
{
	const u32 extLen = StrLen(extension);
	const u32 filenameLen = StrLen(filename);
	bool extensionMatches = false;
	if (extLen < filenameLen)
	{
		if (filename[filenameLen - extLen - 1] == '.')
		{
			extensionMatches = StrEq(filename + filenameLen - extLen, extension);
		}
	}
	return extensionMatches;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Directories

#if PLATFORM_LINUX || PLATFORM_ANDROID
struct Dir
{
	DIR *handle;
};

struct DirEntry
{
	dirent *data;
	const char *name;
};
#elif PLATFORM_WINDOWS
struct Dir
{
    HANDLE handle;
};

struct DirEntry
{
	WIN32_FIND_DATA data;
	const char *name;
};
#else
#error "Missing implementation"
#endif

bool CreateDirectory(const char *path)
{
	bool ok = false;
#if PLATFORM_LINUX || PLATFORM_ANDROID
	int res = mkdir(path, S_IRWXU | S_IRWXG | S_IXOTH );
	if ( res != 0 ) {
		char message[MAX_PATH_LENGTH];
		SPrintf(message, "mkdir %s", path);
		perror(message);
		ok = false;
	}
#elif PLATFORM_WINDOWS
	ok = CreateDirectoryA(path, nullptr);
	if ( !ok ) {
		DWORD errorCode = ::GetLastError();
		if ( errorCode != ERROR_ALREADY_EXISTS ) {
			char message[MAX_PATH_LENGTH];
			SPrintf(message, "CreateDirectoryA %s", path);
			Win32ReportError(message);
		}
	}
#else
#error "Missing implementation"
#endif
	return ok;
}

bool OpenDir(Dir &dir, const char *path) 
{
	bool res = false;

#if PLATFORM_LINUX || PLATFORM_ANDROID
	dir.handle = opendir(path);
	res = dir.handle != nullptr;
	return dir.handle != nullptr;
#elif PLATFORM_WINDOWS
	char wildcardPath[MAX_PATH_LENGTH];
    SPrintf(wildcardPath, "%s/*.*", path);

	// NOTE: FindFirstFile/FindNextFile called here to skip . and .. directories
	WIN32_FIND_DATA entryData;
    dir.handle = FindFirstFile(wildcardPath, &entryData);
	if (dir.handle != INVALID_HANDLE_VALUE && FindNextFile(dir.handle, &entryData) ) {
		res = true;
	} else {
		LOG(Warning, "Path not found: %s\n", wildcardPath);
		Win32ReportError("FindFirstFile");
    }
#else
#error "Missing implementation"
#endif

	return res;
}

bool ReadDir(Dir &dir, DirEntry &entry)
{
	bool res = false;
#if PLATFORM_LINUX || PLATFORM_ANDROID
	if ( dir.handle )
	{
		do
		{
			entry.data = readdir(dir.handle);
			entry.name = entry.data->d_name;
			res = entry.data != nullptr;
		}
		// We skip . and .. directories
		while ( res && ( StrEq(entry.name, ".") || StrEq(entry.name, "..") ) );
	}
#elif PLATFORM_WINDOWS
	if ( dir.handle != INVALID_HANDLE_VALUE )
	{
		res = FindNextFile(dir.handle, &entry.data);
		entry.name = entry.data.cFileName;
	}
#else
#error "Missing implementation"
#endif
	return res;
}

void CloseDir(Dir &dir)
{
#if PLATFORM_LINUX || PLATFORM_ANDROID
	if ( dir.handle )
	{
		closedir(dir.handle);
		dir.handle = nullptr;
	}
#elif PLATFORM_WINDOWS
	if ( dir.handle != INVALID_HANDLE_VALUE )
	{
		FindClose(dir.handle);
		dir.handle = INVALID_HANDLE_VALUE;
	}
#else
#error "Missing implementation"
#endif
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Process execution

bool ExecuteProcess(const char *commandLine)
{
	bool success = false;

	char commandLineCopy[MAX_PATH_LENGTH];
	StrCopyN(commandLineCopy, commandLine, MAX_PATH_LENGTH - 1);

#if PLATFORM_WINDOWS
    STARTUPINFO si;
    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);

    PROCESS_INFORMATION pi;
    ZeroMemory( &pi, sizeof(pi) );

	success = CreateProcessA(
		NULL, // No module name, use command line
		commandLineCopy, // Command line
		NULL, // Process handle not inheritable
		NULL, // Thread handle not inheritable
		FALSE, // Set handle inheritance to false
		0, // No creation flags
		NULL, // Use parent's environment block
		NULL, // Use parent's starting directory
		&si, // Pointer to STARTUPINFO structure
		&pi // Pointer to PROCESS_INFORMATION structure
	);

	if (success)
	{
		// Wait until child process exits.
		WaitForSingleObject( pi.hProcess, INFINITE );

		// Close process and thread handles.
		CloseHandle( pi.hProcess );
		CloseHandle( pi.hThread );
	}
	else
	{
		LOG(Error, "CreateProcessA failed.\n");
	}
#elif PLATFORM_LINUX || PLATFORM_ANDROID
	// Split list of arguments
	char *ptr = commandLineCopy;
	int argc = 0;
	char *argv[128] = {};
	bool addNextArgument = true;
	while (*ptr) {
		if (*ptr == ' ') {
			addNextArgument = true;
			*ptr = 0;
		} else {
			if ( addNextArgument ) {
				ASSERT(argc < ARRAY_COUNT(argv) - 1);
				argv[argc++] = ptr;
				addNextArgument = false;
			}
		}
		ptr++;
	}

	// posix_spawn behaves similar to fork/exec
	pid_t pid;
	int status = posix_spawnp(&pid, argv[0], NULL, NULL, argv, environ);
	if (status == 0) {
		if (waitpid(pid, &status, 0) != -1) {
			// Log(Debug, "Child exited with status %i\n", status);
			success = true;
		} else {
			perror("waitpid");
		}
	} else {
		LOG(Error, "posix_spawnp failed: %s\n", strerror(status));
		LOG(Error, "- Command: %s\n", commandLine);
	}
#else
#error "Missing implementation"
#endif

	return success;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Dynamic library loading

#if PLATFORM_LINUX || PLATFORM_ANDROID

	typedef void* DynamicLibrary;

	DynamicLibrary OpenLibrary(const char *filepath)
	{
		DynamicLibrary library = dlopen(filepath, RTLD_NOW | RTLD_LOCAL);
		return library;
	}

	void* LoadSymbol(DynamicLibrary library, const char *symbolName)
	{
		void *symbol = dlsym(library, symbolName);
		return symbol;
	}

	void CloseLibrary(DynamicLibrary library)
	{
		dlclose(library);
	}

#elif PLATFORM_WINDOWS

	typedef HMODULE DynamicLibrary;

	DynamicLibrary OpenLibrary(const char *filepath)
	{
		DynamicLibrary library = LoadLibraryA(filepath);
		return library;
	}

	void* LoadSymbol(DynamicLibrary library, const char *symbolName)
	{
		void *symbol = GetProcAddress(library, symbolName);
		return symbol;
	}

	void CloseLibrary(DynamicLibrary library)
	{
		FreeLibrary(library);
	}

#else

	#error "Missing implementation"

#endif



////////////////////////////////////////////////////////////////////////////////////////////////////
// Math

struct rgba
{
	byte r, g, b, a;
};

typedef u32 uint;

struct int2
{
	union { i32 x, r; };
	union { i32 y, g; };
};

struct int3
{
	union
	{
		i32 vec[3];
		struct
		{
			union { i32 x, r; };
			union { i32 y, g; };
			union { i32 z, b; };
		};
		int2 xy;
	};
};

struct int4
{
	union
	{
		i32 vec[4];
		struct
		{
			union { i32 x, r; };
			union { i32 y, g; };
			union { i32 z, b; };
			union { i32 w, a; };
		};
		int3 xyz;
	};
};

struct uint2
{
	union { u32 x, r; };
	union { u32 y, g; };
};

struct rect
{
	int2 pos;
	uint2 size;
};

struct float2
{
	union { float x, r; };
	union { float y, g; };
};

struct float3
{
	union
	{
		f32 vec[3];
		struct
		{
			union { float x, r; };
			union { float y, g; };
			union { float z, b; };
		};
		float2 xy;
	};
};

struct float4
{
	union
	{
		f32 vec[4];
		struct
		{
			union { float x, r; };
			union { float y, g; };
			union { float z, b; };
			union { float w, a; };
		};
		struct
		{
			float2 xy;
			float2 zw;
		};
		float3 xyz;
	};
};

struct float3x3
{
	union
	{
		f32 mat[3][3];
		struct
		{
			f32 m00, m01, m02,
				m10, m11, m12,
				m20, m21, m22;
		};
	};
};

struct float4x4
{
	union
	{
		f32 mat[4][4];
		struct
		{
			f32 m00, m01, m02, m03,
				m10, m11, m12, m13,
				m20, m21, m22, m23,
				m30, m31, m32, m33;
		};
	};
};

static constexpr f32 Pi = 3.14159265358979323846f;
static constexpr f32 TwoPi = 2.0f * Pi;
static constexpr f32 ToRadians = Pi / 180.0f;
static constexpr f32 ToDegrees = 180.0f / Pi;

bool operator!=(uint2 a, uint2 b)
{
	const bool res = a.x != b.x || a.y != b.y;
	return res;
}

float2 operator+(float2 a, float2 b)
{
	const float2 res = { .x = a.x + b.x, .y = a.y + b.y };
	return res;
}

float3 operator+(float3 a, float3 b)
{
	const float3 res = { .x = a.x + b.x, .y = a.y + b.y, .z = a.z + b.z };
	return res;
}

int2 operator+=(int2& a, int2 b)
{
	a = { .x = a.x + b.x, .y = a.y + b.y };
	return a;
}

float2 operator+=(float2& a, float2 b)
{
	a = { .x = a.x + b.x, .y = a.y + b.y };
	return a;
}

float2 operator-(float2 a, float2 b)
{
	const float2 res = { .x = a.x - b.x, .y = a.y - b.y };
	return res;
}

float2 operator*(float a, float2 b)
{
	const float2 res = { .x = a * b.x, .y = a * b.y };
	return res;
}

float3 operator*(float a, float3 b)
{
	const float3 res = { .x = a * b.x, .y = a * b.y, .z = a * b.z };
	return res;
}

float2 operator*(float2 a, float2 b)
{
	const float2 res = { .x = a.x * b.x, .y = a.y * b.y };
	return res;
}

uint2 operator*(u32 a, uint2 b)
{
	const uint2 res = { .x = a * b.x, .y= a * b.y };
	return res;
}

float2 operator/(float2 a, float b)
{
	const float2 res = { .x = a.x / b, .y = a.y / b };
	return res;
}

float2 operator/(float2 a, float2 b)
{
	const float2 res = { .x = a.x / b.x, .y = a.y / b.y };
	return res;
}

int2 operator+(int2 pos, uint2 size)
{
	const int2 pos2 = { pos.x + (i32)size.x, pos.y + (i32)size.y };
	return pos2;
}

int2 operator+(int2 pos, int2 size)
{
	const int2 pos2 = { pos.x + size.x, pos.y + size.y };
	return pos2;
}

int2 operator-(int2 a, int2 b)
{
	const int2 size = { a.x - b.x, a.y - b.y };
	return size;
}

i32 Max(i32 a, i32 b) { return a > b ? a : b; }
u32 Max(u32 a, u32 b) { return a > b ? a : b; }
f32 Max(f32 a, f32 b) { return a > b ? a : b; }
int2 Max(int2 a, int2 b) { return { Max(a.x, b.x), Max(a.y, b.y) }; }
uint2 Max(uint2 a, uint2 b) { return { Max(a.x, b.x), Max(a.y, b.y) }; }
float2 Max(float2 a, float2 b) { return { .x = Max(a.x, b.x), .y = Max(a.y, b.y) }; }

i32 Min(i16 a, i16 b) { return a < b ? a : b; }
i32 Min(i32 a, i32 b) { return a < b ? a : b; }
u32 Min(u32 a, u32 b) { return a < b ? a : b; }
f32 Min(f32 a, f32 b) { return a < b ? a : b; }
int2 Min(int2 a, int2 b) { return { Min(a.x, b.x), Min(a.y, b.y) }; }
uint2 Min(uint2 a, uint2 b) { return { Min(a.x, b.x), Min(a.y, b.y) }; }
float2 Min(float2 a, float2 b) { return { Min(a.x, b.x), Min(a.y, b.y) }; }

i32 Clamp( i32 v, i32 min, i32 max ) { return Min( Max( v, min ), max ); }
u32 Clamp( u32 v, u32 min, u32 max ) { return Min( Max( v, min ), max ); }
f32 Clamp( f32 v, f32 min, f32 max ) { return Min( Max( v, min ), max ); }

i32 Abs(i32 v) { return v < 0 ? -v : v; }
f32 Abs(f32 v) { return v < 0.0f ? -v : v; }

// Component isolation: dX({w,h}) is the horizontal extent, dY({w,h}) the vertical one.
// Handy to offset a position along a single axis of a size vector.
float2 dX(float2 v) { return { v.x, 0.0f }; }
float2 dY(float2 v) { return { 0.0f, v.y }; }

rgba Rgba(float4 color)
{
	const rgba res = {
		(byte)(color.r * 255),
		(byte)(color.g * 255),
		(byte)(color.b * 255),
		(byte)(color.a * 255),
	};
	return res;
}

float2 Float2(int2 value)
{
	const float2 res = {(f32)value.x, (f32)value.y};
	return res;
}

float2 Float2(uint2 value)
{
	const float2 res = {(f32)value.x, (f32)value.y};
	return res;
}

float3 Float3(float value)
{
	const float3 res = {value, value, value};
	return res;
}

float3 Float3(float x, float y, float z)
{
	const float3 res = {x, y, z};
	return res;
}

float3 Float3(float2 xy, f32 z)
{
	const float3 res = {xy.x, xy.y, z};
	return res;
}

float4 Float4(float3 xyz, f32 w)
{
	const float4 res = { xyz.x, xyz.y, xyz.z, w };
	return res;
}

float4 Float4(float2 xy, float2 zw)
{
	const float4 res = { xy.x, xy.y, zw.x, zw.y };
	return res;
}

float3x3 Float3x3(const float4x4 &m)
{
	const float3x3 res = {
		m.m00, m.m01, m.m02,
		m.m10, m.m11, m.m12,
		m.m20, m.m21, m.m22,
	};
	return res;
}

float4x4 Float4x4(const float3x3 &m)
{
	const float4x4 res = {
		m.m00, m.m01, m.m02, 0.0f,
		m.m10, m.m11, m.m12, 0.0f,
		m.m20, m.m21, m.m22, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f,
	};
	return res;
}

f32 Sin(f32 value)
{
	const f32 res = ::sinf(value);
	return res;
}

f32 Cos(f32 value)
{
	const f32 res = ::cosf(value);
	return res;
}

f32 Tan(f32 value)
{
	const f32 res = ::tanf(value);
	return res;
}

f32 Sqrt(f32 value)
{
	const f32 res = ::sqrtf(value);
	return res;
}

f32 Pow(f32 base, f32 exponent)
{
	const f32 res = ::powf(base, exponent);
	return res;
}

f32 Round(f32 value)
{
	const f32 res = static_cast<f32>(static_cast<i32>((value + 0.5f - ( value < 0.0f ))));
	return res;
}

float3 FromTo(const float3 &a, const float3 &b)
{
	const float3 res = { b.x - a.x, b.y - a.y, b.z - a.z };
	return res;
}

float2 Add(const float2 &a, const float2 &b)
{
	const float2 res = { b.x + a.x, b.y + a.y };
	return res;
}

float3 Add(const float3 &a, const float3 &b)
{
	const float3 res = { b.x + a.x, b.y + a.y, b.z + a.z };
	return res;
}

float3 Sub(const float3 &a, const float3 &b)
{
	const float3 res = { a.x - b.x, a.y - b.y, a.z - b.z };
	return res;
}

float3 Mul(const float3 &a, f32 b)
{
	const float3 res = { a.x * b, a.y * b, a.z * b };
	return res;
}

float4 Mul(const float4x4 &a, const float4 &b)
{
#define USE_ROWS_TIMES_COLUMNS_MATRIX_MULTIPLICATION 1
#if USE_ROWS_TIMES_COLUMNS_MATRIX_MULTIPLICATION
	const float4 res = {
		a.m00*b.x + a.m10*b.y + a.m20*b.z + a.m30*b.w,
		a.m01*b.x + a.m11*b.y + a.m21*b.z + a.m31*b.w,
		a.m02*b.x + a.m12*b.y + a.m22*b.z + a.m32*b.w,
		a.m03*b.x + a.m13*b.y + a.m23*b.z + a.m33*b.w,
	};
	return res;
#else
#	error "Missing implementation"
#endif
}

float3 MulVector(const float4x4 &a, const float3 &b)
{
	const float3 res = Mul(a, Float4(b, 0.0)).xyz;
	return res;
}

float3 MulPoint(const float4x4 &a, const float3 &b)
{
	const float3 res = Mul(a, Float4(b, 1.0)).xyz;
	return res;
}

float4x4 Mul(const float4x4 &a, const float4x4 &b)
{
#define USE_ROWS_TIMES_COLUMNS_MATRIX_MULTIPLICATION 1
#if USE_ROWS_TIMES_COLUMNS_MATRIX_MULTIPLICATION
	const float4x4 res = {
		a.m00*b.m00 + a.m10*b.m01 + a.m20*b.m02 + a.m30*b.m03,
		a.m01*b.m00 + a.m11*b.m01 + a.m21*b.m02 + a.m31*b.m03,
		a.m02*b.m00 + a.m12*b.m01 + a.m22*b.m02 + a.m32*b.m03,
		a.m03*b.m00 + a.m13*b.m01 + a.m23*b.m02 + a.m33*b.m03,

		a.m00*b.m10 + a.m10*b.m11 + a.m20*b.m12 + a.m30*b.m13,
		a.m01*b.m10 + a.m11*b.m11 + a.m21*b.m12 + a.m31*b.m13,
		a.m02*b.m10 + a.m12*b.m11 + a.m22*b.m12 + a.m32*b.m13,
		a.m03*b.m10 + a.m13*b.m11 + a.m23*b.m12 + a.m33*b.m13,

		a.m00*b.m20 + a.m10*b.m21 + a.m20*b.m22 + a.m30*b.m23,
		a.m01*b.m20 + a.m11*b.m21 + a.m21*b.m22 + a.m31*b.m23,
		a.m02*b.m20 + a.m12*b.m21 + a.m22*b.m22 + a.m32*b.m23,
		a.m03*b.m20 + a.m13*b.m21 + a.m23*b.m22 + a.m33*b.m23,

		a.m00*b.m30 + a.m10*b.m31 + a.m20*b.m32 + a.m30*b.m33,
		a.m01*b.m30 + a.m11*b.m31 + a.m21*b.m32 + a.m31*b.m33,
		a.m02*b.m30 + a.m12*b.m31 + a.m22*b.m32 + a.m32*b.m33,
		a.m03*b.m30 + a.m13*b.m31 + a.m23*b.m32 + a.m33*b.m33,
	};
#else // columns times rows
	const float4x4 res = {
		a.m00*b.m00 + a.m01*b.m10 + a.m02*b.m20 + a.m03*b.m30,
		a.m00*b.m01 + a.m01*b.m11 + a.m02*b.m21 + a.m03*b.m31,
		a.m00*b.m02 + a.m01*b.m12 + a.m02*b.m22 + a.m03*b.m32,
		a.m00*b.m03 + a.m01*b.m13 + a.m02*b.m23 + a.m03*b.m33,

		a.m10*b.m00 + a.m11*b.m10 + a.m12*b.m20 + a.m13*b.m30,
		a.m10*b.m01 + a.m11*b.m11 + a.m12*b.m21 + a.m13*b.m31,
		a.m10*b.m02 + a.m11*b.m12 + a.m12*b.m22 + a.m13*b.m32,
		a.m10*b.m03 + a.m11*b.m13 + a.m12*b.m23 + a.m13*b.m33,

		a.m20*b.m00 + a.m21*b.m10 + a.m22*b.m20 + a.m23*b.m30,
		a.m20*b.m01 + a.m21*b.m11 + a.m22*b.m21 + a.m23*b.m31,
		a.m20*b.m02 + a.m21*b.m12 + a.m22*b.m22 + a.m23*b.m32,
		a.m20*b.m03 + a.m21*b.m13 + a.m22*b.m23 + a.m23*b.m33,

		a.m30*b.m00 + a.m31*b.m10 + a.m32*b.m20 + a.m33*b.m30,
		a.m30*b.m01 + a.m31*b.m11 + a.m32*b.m21 + a.m33*b.m31,
		a.m30*b.m02 + a.m31*b.m12 + a.m32*b.m22 + a.m33*b.m32,
		a.m30*b.m03 + a.m31*b.m13 + a.m32*b.m23 + a.m33*b.m33,
	};
#endif
	return res;
}

float3 Negate(const float3 &v)
{
	const float3 res = { -v.x, -v.y, -v.z };
	return res;
}

f32 Dot(const float2 &a, const float2 &b)
{
	const f32 res = a.x * b.x + a.y * b.y;
	return res;
}

f32 Dot(const float3 &a, const float3 &b)
{
	const f32 res = a.x * b.x + a.y * b.y + a.z * b.z;
	return res;
}

f32 Dot(const float4 &a, const float4 &b)
{
	const f32 res = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
	return res;
}

f32 Length2(const float2 &v)
{
	const f32 res = Dot(v, v);
	return res;
}

f32 Length(const float2 &v)
{
	const f32 res = Sqrt(Length2(v));
	return res;
}

f32 Length2(const float3 &v)
{
	const f32 res = Dot(v, v);
	return res;
}

f32 Length(const float3 &v)
{
	const f32 res = Sqrt(Length2(v));
	return res;
}

bool IsZero(const float3 &v)
{
	return Length2(v) == 0.0f;
}

float3 Normalize(const float3 &v)
{
	const f32 invLen = 1.0f / Length(v);
	const float3 res = {v.x * invLen, v.y * invLen, v.z * invLen};
	return res;
}

float3 NormalizeIfNotZero(const float3 &v)
{
	const float3 res = Length2(v) > 0.0f ? Normalize(v) : v;
	return res;
}

float3 Cross(const float3 &u, const float3 &v)
{
	const float3 res = {
		u.y * v.z - u.z * v.y, // x
		u.z * v.x - u.x * v.z, // y
		u.x * v.y - u.y * v.x  // z
	};
	return res;
}

float4x4 Eye()
{
	float4x4 mat = {};
	mat.m00 = mat.m11 = mat.m22 = mat.m33 = 1.0f;
	return mat;
}

float4x4 Translate(const float3 &t)
{
	const float4x4 mat = {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		t.x, t.y, t.z, 1,
	};
	return mat;
}

float4x4 Rotate(const float3 &axis, f32 degrees)
{
	// ASSERT( IsNormalized(axis) );

	const f32 radians = degrees * ToRadians;
	const f32 c = Cos(radians);
	const f32 s = Sin(radians);
	const f32 t = 1.0f - c;

	const f32 m00 = c + axis.x*axis.x*t;
	const f32 m11 = c + axis.y*axis.y*t;
	const f32 m22 = c + axis.z*axis.z*t;

	const f32 tmp1 = axis.x*axis.y*t;
	const f32 tmp2 = axis.z*s;
	const f32 m10 = tmp1 + tmp2;
	const f32 m01 = tmp1 - tmp2;

	const f32 tmp3 = axis.x*axis.z*t;
	const f32 tmp4 = axis.y*s;
	const f32 m20 = tmp3 - tmp4;
	const f32 m02 = tmp3 + tmp4;

	const f32 tmp5 = axis.y*axis.z*t;
	const f32 tmp6 = axis.x*s;
	const f32 m21 = tmp5 + tmp6;
	const f32 m12 = tmp5 - tmp6;

	const float4x4 mat = {
		m00, m01, m02, 0,
		m10, m11, m12, 0,
		m20, m21, m22, 0,
		0,     0,   0, 1,
	};
	return mat;
}

float4x4 Scale(const float3 &s)
{
	const float4x4 mat = {
		s.x, 0, 0, 0,
		0, s.y, 0, 0,
		0, 0, s.y, 0,
		0, 0, 0, 1,
	};
	return mat;
}

float4x4 LookAt(const float3 &vrp, const float3 &obs, const float3 &up)
{
	const float3 forward = Normalize(FromTo(vrp, obs));
	const float3 right = Normalize(Cross(up, forward));
	const float3 newUp = Cross(forward, right);
	const f32 tx = Dot(Negate(right) ,obs);
	const f32 ty = Dot(Negate(newUp) ,obs);
	const f32 tz = Dot(Negate(forward) ,obs);
	const float4x4 mat = {
		right.x, newUp.x, forward.x, 0,
		right.y, newUp.y, forward.y, 0,
		right.z, newUp.z, forward.z, 0,
		tx, ty, tz, 1,
	};
	return mat;
}

//#define USE_DEPTH_ZERO_TO_ONE 0 // For OpenGL
#define USE_DEPTH_ZERO_TO_ONE 1 // For DX12, Vulkan

//#define USE_REVERSE_Z 0
#define USE_REVERSE_Z 1

float4x4 Perspective(float fov, float aspect, float Near, float Far)
{
#if USE_REVERSE_Z
#if USE_DEPTH_ZERO_TO_ONE
	float tmp = Near;
	Near = Far;
	Far = tmp;
#else
#error "Check reverse-z for OpenGL implementations"
#endif
#endif

	const f32 yScale = 1.0f / Tan(fov * ToRadians / 2.0f);
	const f32 xScale = yScale / aspect;
	const f32 NearMinusFar = Near - Far;

	float4x4 mat = {};
	mat.m00 = xScale;
	mat.m11 = yScale;
#if USE_DEPTH_ZERO_TO_ONE
	mat.m22 = Far / NearMinusFar;
	mat.m32 = Far * Near / NearMinusFar;
#else
	mat.m22 = (Far + Near) / NearMinusFar;
	mat.m32 = 2.0f * Far * Near / NearMinusFar;
#endif
	mat.m23 = -1;

	return mat;
}

float4x4 Orthogonal(f32 l, f32 r, f32 b, f32 t, f32 n, f32 f)
{
#if USE_REVERSE_Z
#if USE_DEPTH_ZERO_TO_ONE
	float tmp = n;
	n = f;
	f = tmp;
#else
#error "Check reverse-z for OpenGL implementations"
#endif
#endif

	float4x4 mat = {};
	const f32 RminusL = r - l;
	const f32 TminusB = t - b;
	const f32 FminusN = f - n;

	mat.m00 = 2.0f / RminusL;
	mat.m30 = -(r + l)/RminusL;
	mat.m11 = 2.0f / TminusB;
	mat.m31 = -(t + b)/TminusB;
#if USE_DEPTH_ZERO_TO_ONE
	mat.m22 = -1.0f / FminusN;
	mat.m32 = - n /FminusN;
#else
	mat.m22 = -2.0f / FminusN;
	mat.m32 = -(f + n)/FminusN;
#endif
	mat.m33 = 1.0f;

	return mat;
}

float3x3 Transpose(const float3x3 &m)
{
	const float3x3 res = {
		m.m00, m.m10, m.m20,
		m.m01, m.m11, m.m21,
		m.m02, m.m12, m.m22,
	};
	return res;
}

float4x4 Transpose(const float4x4 &m)
{
	const float4x4 res = {
		m.m00, m.m10, m.m20, m.m30,
		m.m01, m.m11, m.m21, m.m31,
		m.m02, m.m12, m.m22, m.m32,
		m.m03, m.m13, m.m23, m.m33,
	};
	return res;
}

i32 Ceil(f32 value)
{
	const f32 res = ::ceilf(value);
	return static_cast<i32>(res);
}

i32 Floor(f32 value)
{
	const f32 res = ::floorf(value);
	return static_cast<i32>(res);
}

float2 Floor(float2 value)
{
	const float2 res = float2{(f32)Floor(value.x), (f32)Floor(value.y)};
	return res;
}

f32 Log2(f32 value)
{
	const f32 res = ::log2f(value);
	return res;
}

f32 Mod(f32 value, f32 incr)
{
	const f32 res = value - Floor(value);
	return res;
}

f32 Lerp(f32 a, f32 b, f32 t)
{
	const f32 res = (1.0f - t) * a + t * b;
	return res;
}
float2 Lerp(float2 a, float2 b, f32 t)
{
	const float2 res = {
		Lerp(a.x, b.x, t),
		Lerp(a.y, b.y, t)
	};
	return res;
}
float3 Lerp(float3 a, float3 b, f32 t)
{
	const float3 res = {
		Lerp(a.x, b.x, t),
		Lerp(a.y, b.y, t),
		Lerp(a.z, b.z, t)
	};
	return res;
}
float4 Lerp(float4 a, float4 b, f32 t)
{
	const float4 res = {
		Lerp(a.x, b.x, t),
		Lerp(a.y, b.y, t),
		Lerp(a.z, b.z, t),
		Lerp(a.w, b.w, t)
	};
	return res;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Alignment functions

bool IsPowerOfTwo(u32 value)
{
	// First value in the below expression is for the case when value is 0
	return value && (!(value & (value - 1)));
}

u32 AlignUp(u32 value, u32 alignment)
{
	ASSERT( IsPowerOfTwo(alignment) );
	const u32 alignedValue =  ((value - 1) & ~(alignment - 1)) + alignment;
	return alignedValue;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Time

#if PLATFORM_WINDOWS
internal LONGLONG Win32GetPerformanceCounterFrequency()
{
	LARGE_INTEGER PerfCountFrequencyResult;
	BOOL res = QueryPerformanceFrequency(&PerfCountFrequencyResult);
	ASSERT(res && "QueryPerformanceFrequency() failed!");
	return PerfCountFrequencyResult.QuadPart;
}
#endif

u64 GetTicks()
{
#if PLATFORM_WINDOWS
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	return (u64)counter.QuadPart;
#elif PLATFORM_LINUX || PLATFORM_ANDROID
	timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (u64)ts.tv_sec * 1000000000ull + (u64)ts.tv_nsec;
#endif
}

u64 GetTicksPerSecond()
{
#if PLATFORM_WINDOWS
	static LONGLONG frequency = Win32GetPerformanceCounterFrequency();
	return (u64)frequency;
#else
	return 1000000000ull;
#endif
}

double SecondsFromTicks(u64 ticks)
{
	return (double)ticks / (double)GetTicksPerSecond();
}

struct Clock
{
	u64 ticks;
};

#define SECONDS_FROM_NANOSECONDS( ns ) ((float)(ns) / 1000000000.0f)

Clock GetClock()
{
	Clock clock = { .ticks = GetTicks() };
	return clock;
}

float GetSecondsElapsed(Clock start, Clock end)
{
	ASSERT(start.ticks <= end.ticks);
	return (float)SecondsFromTicks(end.ticks - start.ticks);
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Threading

struct ThreadInfo
{
	u32 globalIndex;
};

#define WORK_QUEUE_CALLBACK(name) void name(const ThreadInfo &threadInfo, void *data)
typedef WORK_QUEUE_CALLBACK(WorkQueueCallback);

#if PLATFORM_WINDOWS

#define THREAD_FUNCTION(name) DWORD WINAPI name(LPVOID arguments)

#define FullWriteBarrier() _WriteBarrier(); _mm_sfence()
#define FullReadBarrier() _ReadBarrier()

typedef HANDLE Semaphore;

static bool CreateSemaphore( Semaphore &semaphore, u32 iniCount, u32 maxCount )
{
	bool success = true;
	semaphore = CreateSemaphoreEx(0, iniCount, maxCount, 0, 0, SEMAPHORE_ALL_ACCESS);
	if ( semaphore == NULL ) {
		Win32ReportError("CreateSemaphoreEx");
		success = false;
	}
	return success;
}

static bool SignalSemaphore( Semaphore semaphore )
{
	bool success = true;
	if ( !ReleaseSemaphore(semaphore, 1, 0) ) {
		Win32ReportError("ReleaseSemaphore");
		success = false;
	}
	return success;
}

static bool WaitSemaphore( Semaphore semaphore )
{
	bool success = true;
	if ( WaitForSingleObjectEx(semaphore, INFINITE, FALSE) == WAIT_FAILED ) {
		Win32ReportError("WaitForSingleObjectEx");
		success = false;
	}
	return success;
}

static bool CreateDetachedThread( THREAD_FUNCTION(threadFunc), const ThreadInfo &threadInfo )
{
	bool success = true;
	DWORD threadId;
	HANDLE threadHandle = CreateThread(0, 0, threadFunc, (LPVOID)&threadInfo, 0, &threadId);
	if ( threadHandle == NULL ) {
		Win32ReportError("CreateThread");
		success = false;
	} else {
		CloseHandle(threadHandle);
	}
	return success;
}

static void SleepMillis(u32 millis)
{
	Sleep(millis);
}

static void Yield()
{
	SwitchToThread();
}

typedef SRWLOCK Mutex;

static bool CreateMutex( Mutex &mutex )
{
	InitializeSRWLock(&mutex);
	return true;
}

static void DestroyMutex( Mutex &mutex )
{
}

static void LockMutex( Mutex &mutex )
{
	AcquireSRWLockExclusive(&mutex);
}

static void UnlockMutex( Mutex &mutex )
{
	ReleaseSRWLockExclusive(&mutex);
}


#elif PLATFORM_LINUX || PLATFORM_ANDROID

#define THREAD_FUNCTION(name) void* name(void *arguments)

#define FullWriteBarrier() __sync_synchronize()
#define FullReadBarrier() __sync_synchronize()

typedef sem_t Semaphore;

static bool CreateSemaphore( Semaphore &semaphore, u32 iniCount, u32 maxCount )
{
	bool success = true;
	if ( sem_init(&semaphore, 0, iniCount) != 0 ) { // TODO(jediaz): use sem_init_np to set maxCount
		LinuxReportError("sem_init");
		success = false;
	}
	return success;
}

static bool SignalSemaphore( Semaphore &semaphore )
{
	bool success = true;
	if ( sem_post(&semaphore) != 0 ) {
		LinuxReportError("sem_post");
		success = false;
	}
	return success;
}

static bool WaitSemaphore( Semaphore &semaphore )
{
	bool success = true;
	if ( sem_wait(&semaphore) != 0 ) {
		LinuxReportError("sem_wait");
		success = false;
	}
	return success;
}

static bool CreateDetachedThread( THREAD_FUNCTION(threadFunc), const ThreadInfo &threadInfo )
{
	bool success = false;
	pthread_t threadHandle;
	if ( pthread_create(&threadHandle, nullptr, threadFunc, (void *)&threadInfo) == 0 ) {
		if ( pthread_detach(threadHandle) == 0 ) {
			success = true;
		} else {
			LinuxReportError("pthread_detach");
		}
	} else {
		LinuxReportError("pthread_create");
	}
	return success;
}

static void SleepMillis(u32 millis)
{
	const useconds_t micros = millis * 1000;
	const int res = usleep(micros);
	if ( res == -1 )
	{
		LinuxReportError("usleep");
	}
}

static void Yield()
{
	SleepMillis(1);
}

typedef pthread_mutex_t Mutex;

static bool CreateMutex( Mutex &mutex )
{
	bool success = true;
	if ( pthread_mutex_init(&mutex, nullptr) != 0 ) {
		LinuxReportError("pthread_mutex_init");
		success = false;
	}
	return success;
}

static void DestroyMutex( Mutex &mutex )
{
	pthread_mutex_destroy(&mutex);
}

static void LockMutex( Mutex &mutex )
{
	pthread_mutex_lock(&mutex);
}

static void UnlockMutex( Mutex &mutex )
{
	pthread_mutex_unlock(&mutex);
}

#else
#error "Missing implementation"
#endif

struct MutexScope
{
	Mutex &mutex;
	MutexScope( Mutex &mutex ) : mutex(mutex) { LockMutex(mutex); }
	~MutexScope() { UnlockMutex(mutex); }
	MutexScope( const MutexScope & ) = delete;
	MutexScope &operator=( const MutexScope & ) = delete;
};



#endif // #ifndef ILU_CORE_H

