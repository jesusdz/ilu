/*
 * unit_test_ilu.cpp
 * Unit tests for code/ilu_core.h
 */

#define STB_IMAGE_IMPLEMENTATION
#define TOOLS_IMAGE_PIXELS
#include "../ilu_core.h"
#include "../handle_pool.h"
#include "../ilu_id.h"

// The ID pool the compaction tests bind their elements to. The engine keeps its own in
// the Engine struct, ilu_id.h only asks for one to be named.
static IDPool gTestIDPool;
#define ILU_ID_POOL gTestIDPool
#define ILU_ID_IMPLEMENTATION
#include "../ilu_id.h"

// ANSI color codes
#ifdef _WIN32
#include <windows.h>
#define ANSI_RESET   "\x1b[0m"
#define ANSI_BOLD    "\x1b[1m"
#define ANSI_RED     "\x1b[31m"
#define ANSI_GREEN   "\x1b[32m"
#else
#define ANSI_RESET
#define ANSI_BOLD
#define ANSI_RED
#define ANSI_GREEN
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
// Test framework

static u32 gTestsPassed = 0;
static u32 gTestsFailed = 0;

#define TEST(name, expression) \
    do { \
        if (expression) { \
            LOG(Info, ANSI_GREEN "[PASS]" ANSI_RESET " %s\n", name); \
            gTestsPassed++; \
        } else { \
            LOG(Error, ANSI_RED "[FAIL]" ANSI_RESET " %s  (line %d)\n", name, __LINE__); \
            gTestsFailed++; \
        } \
    } while(0)

#define TEST_SECTION(name) LOG(Info, ANSI_BOLD "\n--- %s ---\n" ANSI_RESET, name)

////////////////////////////////////////////////////////////////////////////////////////////////////
// Truncation tests

void TestTruncations()
{
    TEST_SECTION("Truncations");

    // U64ToU32
    {
        u64 val = 42;
        u32 res = U64ToU32(val);
        TEST("U64ToU32 basic", res == 42);
    }
    {
        u64 val = U32_MAX;
        u32 res = U64ToU32(val);
        TEST("U64ToU32 max", res == U32_MAX);
    }

    // U32ToI32
    {
        u32 val = 100;
        i32 res = U32ToI32(val);
        TEST("U32ToI32 basic", res == 100);
    }
    {
        u32 val = 0;
        i32 res = U32ToI32(val);
        TEST("U32ToI32 zero", res == 0);
    }

    // I32ToI16
    {
        i32 val = 1000;
        i16 res = I32ToI16(val);
        TEST("I32ToI16 basic", res == 1000);
    }
    {
        i32 val = -1000;
        i16 res = I32ToI16(val);
        TEST("I32ToI16 negative", res == -1000);
    }

    // I32ToU16
    {
        i32 val = 500;
        u16 res = I32ToU16(val);
        TEST("I32ToU16 basic", res == 500);
    }
    {
        i32 val = 0;
        u16 res = I32ToU16(val);
        TEST("I32ToU16 zero", res == 0);
    }

    // I32ToU8
    {
        i32 val = 200;
        u8 res = I32ToU8(val);
        TEST("I32ToU8 basic", res == 200);
    }
    {
        i32 val = 0;
        u8 res = I32ToU8(val);
        TEST("I32ToU8 zero", res == 0);
    }

    // ClipI32ToI16
    {
        i32 val = 1000;
        i16 res = ClipI32ToI16(val);
        TEST("ClipI32ToI16 in range", res == 1000);
    }
    {
        i32 val = 100000;
        i16 res = ClipI32ToI16(val);
        TEST("ClipI32ToI16 clip high", res == I16_MAX);
    }
    {
        i32 val = -100000;
        i16 res = ClipI32ToI16(val);
        TEST("ClipI32ToI16 clip low", res == I16_MIN);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Intrinsics tests

void TestIntrinsics()
{
    TEST_SECTION("Intrinsics");

    // CTZ - Count Trailing Zeros
    {
        u32 res = CTZ(1);
        TEST("CTZ(1) == 0", res == 0);
    }
    {
        u32 res = CTZ(2);
        TEST("CTZ(2) == 1", res == 1);
    }
    {
        u32 res = CTZ(4);
        TEST("CTZ(4) == 2", res == 2);
    }
    {
        u32 res = CTZ(8);
        TEST("CTZ(8) == 3", res == 3);
    }
    {
        u32 res = CTZ(0x80000000);
        TEST("CTZ(0x80000000) == 31", res == 31);
    }
    {
        u32 res = CTZ(0b1100);
        TEST("CTZ(0b1100) == 2", res == 2);
    }

    // CLZ - Count Leading Zeros
    {
        u32 res = CLZ(0x80000000);
        TEST("CLZ(0x80000000) == 0", res == 0);
    }
    {
        u32 res = CLZ(0x40000000);
        TEST("CLZ(0x40000000) == 1", res == 1);
    }
    {
        u32 res = CLZ(1);
        TEST("CLZ(1) == 31", res == 31);
    }
    {
        u32 res = CLZ(2);
        TEST("CLZ(2) == 30", res == 30);
    }

    // FBS - First Bit Set
    {
        u32 res = FBS(1);
        TEST("FBS(1) == 0", res == 0);
    }
    {
        u32 res = FBS(0b1010);
        TEST("FBS(0b1010) == 1", res == 1);
    }
    {
        u32 res = FBS(0b1100);
        TEST("FBS(0b1100) == 2", res == 2);
    }

    // FBZ - First Bit Zero
    {
        u32 res = FBZ(0b1110);
        TEST("FBZ(0b1110) == 0", res == 0);
    }
    {
        u32 res = FBZ(0b1101);
        TEST("FBZ(0b1101) == 1", res == 1);
    }
    {
        u32 res = FBZ(0b0011);
        TEST("FBZ(0b0011) == 2", res == 2);
    }
    {
        u32 res = FBZ(0xFFFFFFFE);
        TEST("FBZ(0xFFFFFFFE) == 0", res == 0);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// String tests

void TestStrings()
{
    TEST_SECTION("Strings");

    // StrLen
    {
        u32 len = StrLen("hello");
        TEST("StrLen hello == 5", len == 5);
    }
    {
        u32 len = StrLen("");
        TEST("StrLen empty == 0", len == 0);
    }
    {
        u32 len = StrLen("a");
        TEST("StrLen single char == 1", len == 1);
    }

    // MakeString
    {
        String s = MakeString("hello", 5);
        TEST("MakeString size", s.size == 5);
        TEST("MakeString str", StrEq(s, "hello"));
    }
    {
        String s = MakeString("world");
        TEST("MakeString auto size", s.size == 5);
    }

    // StrCopy
    {
        char buf[32] = {};
        StrCopy(buf, "hello");
        TEST("StrCopy from cstr", StrEq(buf, "hello"));
    }
    {
        char buf[32] = {};
        String s = MakeString("world", 5);
        StrCopy(buf, s);
        TEST("StrCopy from String", StrEq(buf, "world"));
    }

    // StrCopyN
    {
        char buf[32] = {};
        StrCopyN(buf, "hello world", 5);
        TEST("StrCopyN copies N chars", StrEq(buf, "hello"));
    }

    // StrCat
    {
        char buf[32] = {};
        StrCopy(buf, "hello");
        StrCat(buf, " world");
        TEST("StrCat cstr", StrEq(buf, "hello world"));
    }
    {
        char buf[32] = {};
        StrCopy(buf, "hello");
        String s = MakeString(" world", 6);
        StrCat(buf, s);
        TEST("StrCat String", StrEq(buf, "hello world"));
    }

    // StrEq
    {
        String s1 = MakeString("hello", 5);
        String s2 = MakeString("hello", 5);
        TEST("StrEq String String equal", StrEq(s1, s2));
    }
    {
        String s1 = MakeString("hello", 5);
        String s2 = MakeString("world", 5);
        TEST("StrEq String String not equal", !StrEq(s1, s2));
    }
    {
        String s1 = MakeString("hello", 5);
        String s2 = MakeString("hell", 4);
        TEST("StrEq String String different size", !StrEq(s1, s2));
    }
    {
        String s = MakeString("hello", 5);
        TEST("StrEq String cstr equal", StrEq(s, "hello"));
    }
    {
        String s = MakeString("hello", 5);
        TEST("StrEq String cstr not equal", !StrEq(s, "world"));
    }
    {
        TEST("StrEq cstr cstr equal", StrEq("hello", "hello"));
    }
    {
        TEST("StrEq cstr cstr not equal", !StrEq("hello", "world"));
    }
    {
        TEST("StrEq cstr cstr empty", StrEq("", ""));
    }

    // StrEqN
    {
        TEST("StrEqN first N chars equal", StrEqN("hello world", "hello", 5));
    }
    {
        TEST("StrEqN first N chars not equal", !StrEqN("hello world", "world", 5));
    }

    // StrConsume
    {
        const char *res = StrConsume("hello world", "hello ");
        TEST("StrConsume match", res != NULL && StrEq(res, "world"));
    }
    {
        const char *res = StrConsume("hello world", "world");
        TEST("StrConsume no match returns NULL", res == NULL);
    }

    // StrToBool
    {
        TEST("StrToBool '1'", StrToBool("1"));
    }
    {
        TEST("StrToBool 'true'", StrToBool("true"));
    }
    {
        TEST("StrToBool '0' is false", !StrToBool("0"));
    }
    {
        TEST("StrToBool 'false' is false", !StrToBool("false"));
    }
    {
        String s = MakeString("true", 4);
        TEST("StrToBool String true", StrToBool(s));
    }
    {
        String s = MakeString("1", 1);
        TEST("StrToBool String 1", StrToBool(s));
    }

    // StrToChar
    {
        TEST("StrToChar basic", StrToChar("A") == 'A');
    }
    {
        TEST("StrToChar zero len returns ?", StrToChar("A", 0) == '?');
    }
    {
        String s = MakeString("Z", 1);
        TEST("StrToChar String", StrToChar(s) == 'Z');
    }

    // StrChar
    {
        const char *res = StrChar("hello world", 'w');
        TEST("StrChar found", res != nullptr && *res == 'w');
    }
    {
        const char *res = StrChar("hello", 'z');
        TEST("StrChar not found returns nullptr", res == nullptr);
    }

    // StrCharR
    {
        const char *res = StrCharR("hello/world/foo", '/');
        TEST("StrCharR finds last", res != nullptr && StrEq(res + 1, "foo"));
    }
    {
        const char *res = StrCharR("hello", '/');
        TEST("StrCharR not found returns nullptr", res == nullptr);
    }

    // StrReplace
    {
        char buf[32] = {};
        StrCopy(buf, "hello/world");
        StrReplace(buf, '/', '\\');
        TEST("StrReplace replaces char", StrEq(buf, "hello\\world"));
    }
    {
        char buf[32] = {};
        StrCopy(buf, "aaa");
        StrReplace(buf, 'a', 'b');
        TEST("StrReplace replaces all occurrences", StrEq(buf, "bbb"));
    }

    // StrToInt
    {
        TEST("StrToInt positive", StrToInt("42") == 42);
    }
    {
        TEST("StrToInt negative", StrToInt("-42") == -42);
    }
    {
        TEST("StrToInt zero", StrToInt("0") == 0);
    }
    {
        String s = MakeString("123", 3);
        TEST("StrToInt String", StrToInt(s) == 123);
    }

    // StrToUnsignedInt
    {
        TEST("StrToUnsignedInt basic", StrToUnsignedInt("42") == 42u);
    }
    {
        TEST("StrToUnsignedInt zero", StrToUnsignedInt("0") == 0u);
    }
    {
        String s = MakeString("999", 3);
        TEST("StrToUnsignedInt String", StrToUnsignedInt(s) == 999u);
    }

    // StrToFloat
    {
        f32 val = StrToFloat("3.14");
        TEST("StrToFloat pi approx", val > 3.13f && val < 3.15f);
    }
    {
        f32 val = StrToFloat("-1.5");
        TEST("StrToFloat negative", val > -1.51f && val < -1.49f);
    }
    {
        f32 val = StrToFloat("42");
        TEST("StrToFloat integer", val == 42.0f);
    }
    {
        String s = MakeString("2.5", 3);
        f32 val = StrToFloat(s);
        TEST("StrToFloat String", val > 2.49f && val < 2.51f);
    }

    // SPrintf
    {
        char buf[64] = {};
        SPrintf(buf, "Hello %s %d", "world", 42);
        TEST("SPrintf basic", StrEq(buf, "Hello world 42"));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Hashing tests

void TestHashing()
{
    TEST_SECTION("Hashing");

    // HashFNV
    {
        const char *data = "hello";
        u32 h1 = HashFNV(data, 5);
        u32 h2 = HashFNV(data, 5);
        TEST("HashFNV deterministic", h1 == h2);
    }
    {
        u32 h1 = HashFNV("hello", 5);
        u32 h2 = HashFNV("world", 5);
        TEST("HashFNV different inputs differ", h1 != h2);
    }
    {
        u32 h1 = HashFNV("hello", 5, 0);
        u32 h2 = HashFNV("hello", 5, 1);
        TEST("HashFNV different seeds differ", h1 != h2);
    }

    // HashStringFNV
    {
        u32 h1 = HashStringFNV("hello");
        u32 h2 = HashStringFNV("hello");
        TEST("HashStringFNV deterministic", h1 == h2);
    }
    {
        u32 h1 = HashStringFNV("hello");
        u32 h2 = HashStringFNV("world");
        TEST("HashStringFNV different inputs differ", h1 != h2);
    }
    {
        // HashFNV and HashStringFNV should agree on same data
        u32 h1 = HashFNV("hello", 5);
        u32 h2 = HashStringFNV("hello");
        TEST("HashFNV and HashStringFNV agree", h1 == h2);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Memory tests

void TestMemory()
{
    TEST_SECTION("Memory");

    // MemSet
    {
        byte buf[8] = {};
        MemSet(buf, 8, 0xAB);
        bool allSet = true;
        for (int i = 0; i < 8; i++) allSet = allSet && (buf[i] == 0xAB);
        TEST("MemSet sets all bytes", allSet);
    }
    {
        byte buf[8] = {1,2,3,4,5,6,7,8};
        MemSet(buf, 8, 0);
        bool allZero = true;
        for (int i = 0; i < 8; i++) allZero = allZero && (buf[i] == 0);
        TEST("MemSet zeros all bytes", allZero);
    }

    // MemCopy
    {
        byte src[4] = {1, 2, 3, 4};
        byte dst[4] = {};
        MemCopy(dst, src, 4);
        TEST("MemCopy copies bytes", dst[0]==1 && dst[1]==2 && dst[2]==3 && dst[3]==4);
    }

    // MemCompare
    {
        byte a[4] = {1, 2, 3, 4};
        byte b[4] = {1, 2, 3, 4};
        TEST("MemCompare equal returns 0", MemCompare(a, b, 4) == 0);
    }
    {
        byte a[4] = {1, 2, 3, 5};
        byte b[4] = {1, 2, 3, 4};
        TEST("MemCompare a > b returns positive", MemCompare(a, b, 4) > 0);
    }
    {
        byte a[4] = {1, 2, 3, 3};
        byte b[4] = {1, 2, 3, 4};
        TEST("MemCompare a < b returns negative", MemCompare(a, b, 4) < 0);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Arena tests

void TestArena()
{
    TEST_SECTION("Arena");

    const u32 arenaSize = MB(1);
    byte *memory = (byte*)AllocateVirtualMemory(arenaSize);

    // MakeArena
    {
        Arena arena = MakeArena(memory, arenaSize, "test");
        TEST("MakeArena base", arena.base == memory);
        TEST("MakeArena size", arena.size == arenaSize);
        TEST("MakeArena used == 0", arena.used == 0);
        TEST("MakeArena name", StrEq(arena.name, "test"));
    }

    // PushSize
    {
        Arena arena = MakeArena(memory, arenaSize, "test");
        byte *ptr = PushSize(arena, 64);
        TEST("PushSize returns non-null", ptr != nullptr);
        TEST("PushSize advances used", arena.used == 64);
        TEST("PushSize returns base", ptr == memory);
    }

    // PushZeroSize
    {
        Arena arena = MakeArena(memory, arenaSize, "test");
        byte *ptr = PushZeroSize(arena, 64);
        bool allZero = true;
        for (int i = 0; i < 64; i++) allZero = allZero && (ptr[i] == 0);
        TEST("PushZeroSize zeroes memory", allZero);
    }

    // PushStruct
    {
        Arena arena = MakeArena(memory, arenaSize, "test");
        int2 *v = PushStruct(arena, int2);
        TEST("PushStruct returns non-null", v != nullptr);
        TEST("PushStruct advances used", arena.used == sizeof(int2));
    }

    // PushZeroStruct
    {
        Arena arena = MakeArena(memory, arenaSize, "test");
        int2 *v = PushZeroStruct(arena, int2);
        TEST("PushZeroStruct x == 0", v->x == 0);
        TEST("PushZeroStruct y == 0", v->y == 0);
    }

    // PushArray
    {
        Arena arena = MakeArena(memory, arenaSize, "test");
        i32 *arr = PushArray(arena, i32, 10);
        TEST("PushArray returns non-null", arr != nullptr);
        TEST("PushArray advances used", arena.used == sizeof(i32) * 10);
    }

    // PushZeroArray
    {
        Arena arena = MakeArena(memory, arenaSize, "test");
        i32 *arr = PushZeroArray(arena, i32, 10);
        bool allZero = true;
        for (int i = 0; i < 10; i++) allZero = allZero && (arr[i] == 0);
        TEST("PushZeroArray zeroes memory", allZero);
    }

    // PushString
    {
        Arena arena = MakeArena(memory, arenaSize, "test");
        const char *str = PushString(arena, "hello");
        TEST("PushString content", StrEq(str, "hello"));
        TEST("PushString advances used", arena.used == 6); // 5 chars + null terminator
    }

    // PushStringN
    {
        Arena arena = MakeArena(memory, arenaSize, "test");
        const char *str = PushStringN(arena, "hello world", 5);
        TEST("PushStringN content", StrEq(str, "hello"));
        TEST("PushStringN advances used", arena.used == 6); // 5 chars + null terminator
    }

    // PushChar
    {
        Arena arena = MakeArena(memory, arenaSize, "test");
        char *c = PushChar(arena, 'X');
        TEST("PushChar content", *c == 'X');
        TEST("PushChar advances used", arena.used == sizeof(char));
    }

    // MakeSubArena
    {
        Arena arena = MakeArena(memory, arenaSize, "parent");
        Arena sub = MakeSubArena(arena, 1024, "child");
        TEST("MakeSubArena base", sub.base == memory);
        TEST("MakeSubArena size", sub.size == 1024);
        TEST("MakeSubArena used == 0", sub.used == 0);
        TEST("MakeSubArena parent used unchanged", arena.used == 0);
    }

    // PushSubArena
    {
        Arena arena = MakeArena(memory, arenaSize, "parent");
        Arena sub = PushSubArena(arena, 1024, "child");
        TEST("PushSubArena base", sub.base == memory);
        TEST("PushSubArena size", sub.size == 1024);
        TEST("PushSubArena parent used advanced", arena.used == 1024);
    }

    // ResetArena
    {
        Arena arena = MakeArena(memory, arenaSize, "test");
        PushSize(arena, 64);
        ResetArena(arena);
        TEST("ResetArena resets used to 0", arena.used == 0);
    }

    // ZeroStruct
    {
        int2 v = {10, 20};
        ZeroStruct(&v);
        TEST("ZeroStruct zeroes struct", v.x == 0 && v.y == 0);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// String interning tests

void TestStringInterning()
{
    TEST_SECTION("String Interning");

    const u32 arenaSize = MB(1);
    byte *memory = (byte*)AllocateVirtualMemory(arenaSize);
    Arena arena = MakeArena(memory, arenaSize, "interning");

    StringInterning interning = StringInterningCreate(&arena);

    // Same string returns same pointer
    {
        const char *a = MakeStringIntern(&interning, "hello");
        const char *b = MakeStringIntern(&interning, "hello");
        TEST("StringIntern same string same pointer", a == b);
    }

    // Different strings return different pointers
    {
        const char *a = MakeStringIntern(&interning, "hello");
        const char *b = MakeStringIntern(&interning, "world");
        TEST("StringIntern different strings different pointers", a != b);
    }

    // Content is correct
    {
        const char *a = MakeStringIntern(&interning, "foobar");
        TEST("StringIntern content correct", StrEq(a, "foobar"));
    }

    // With explicit length
    {
        const char *a = MakeStringIntern(&interning, "hello world", 5);
        TEST("StringIntern with length content", StrEq(a, "hello"));
    }

    // Same interned string with length
    {
        const char *a = MakeStringIntern(&interning, "hello world", 5);
        const char *b = MakeStringIntern(&interning, "hello", 5);
        TEST("StringIntern with length same pointer", a == b);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// File path tests

void TestFilePaths()
{
    TEST_SECTION("File Paths");

    // MakePath
    {
        FilePath path = MakePath("base", "relative/file.txt");
        TEST("MakePath combines paths", StrEq(path.str, "base/relative/file.txt"));
    }
    {
        FilePath path = MakePath("/root", "sub/file.txt");
        TEST("MakePath with absolute base", StrEq(path.str, "/root/sub/file.txt"));
    }

    // HasFileExtension
    {
        TEST("HasFileExtension match", HasFileExtension("file.txt", "txt"));
    }
    {
        TEST("HasFileExtension no match", !HasFileExtension("file.txt", "png"));
    }
    {
        TEST("HasFileExtension cpp", HasFileExtension("main.cpp", "cpp"));
    }
    {
        TEST("HasFileExtension h", HasFileExtension("ilu_core.h", "h"));
    }
    {
        TEST("HasFileExtension no extension", !HasFileExtension("noextension", "txt"));
    }
    {
        TEST("HasFileExtension empty string", !HasFileExtension("", "txt"));
    }
    {
        TEST("HasFileExtension longer ext than filename", !HasFileExtension("a.t", "txt"));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Math tests

void TestMath()
{
    TEST_SECTION("Math");

    // Min / Max
    {
        TEST("Max i32", Max(3, 5) == 5);
        TEST("Max i32 equal", Max(5, 5) == 5);
        TEST("Min i32", Min(3, 5) == 3);
        TEST("Min i32 equal", Min(5, 5) == 5);
        TEST("Max u32", Max(3u, 5u) == 5u);
        TEST("Min u32", Min(3u, 5u) == 3u);
        TEST("Max f32", Max(3.0f, 5.0f) == 5.0f);
        TEST("Min f32", Min(3.0f, 5.0f) == 3.0f);
    }

    // int2 Min/Max
    {
        int2 a = {1, 5};
        int2 b = {3, 2};
        int2 mx = Max(a, b);
        int2 mn = Min(a, b);
        TEST("Max int2 x", mx.x == 3);
        TEST("Max int2 y", mx.y == 5);
        TEST("Min int2 x", mn.x == 1);
        TEST("Min int2 y", mn.y == 2);
    }

    // uint2 Min/Max
    {
        uint2 a = {1u, 5u};
        uint2 b = {3u, 2u};
        uint2 mx = Max(a, b);
        uint2 mn = Min(a, b);
        TEST("Max uint2 x", mx.x == 3u);
        TEST("Max uint2 y", mx.y == 5u);
        TEST("Min uint2 x", mn.x == 1u);
        TEST("Min uint2 y", mn.y == 2u);
    }

    // float2 Min/Max
    {
        float2 a = {1.0f, 5.0f};
        float2 b = {3.0f, 2.0f};
        float2 mx = Max(a, b);
        float2 mn = Min(a, b);
        TEST("Max float2 x", mx.x == 3.0f);
        TEST("Max float2 y", mx.y == 5.0f);
        TEST("Min float2 x", mn.x == 1.0f);
        TEST("Min float2 y", mn.y == 2.0f);
    }

    // Clamp
    {
        TEST("Clamp i32 in range", Clamp(5, 0, 10) == 5);
        TEST("Clamp i32 below min", Clamp(-5, 0, 10) == 0);
        TEST("Clamp i32 above max", Clamp(15, 0, 10) == 10);
        TEST("Clamp u32 in range", Clamp(5u, 0u, 10u) == 5u);
        TEST("Clamp f32 in range", Clamp(5.0f, 0.0f, 10.0f) == 5.0f);
        TEST("Clamp f32 below min", Clamp(-1.0f, 0.0f, 1.0f) == 0.0f);
        TEST("Clamp f32 above max", Clamp(2.0f, 0.0f, 1.0f) == 1.0f);
    }

    // Rgba
    {
        float4 color = {1.0f, 0.0f, 0.5f, 1.0f};
        rgba c = Rgba(color);
        TEST("Rgba r", c.r == 255);
        TEST("Rgba g", c.g == 0);
        TEST("Rgba b", c.b == 127);
        TEST("Rgba a", c.a == 255);
    }

    // Float3
    {
        float3 v = Float3(1.0f);
        TEST("Float3 uniform x", v.x == 1.0f);
        TEST("Float3 uniform y", v.y == 1.0f);
        TEST("Float3 uniform z", v.z == 1.0f);
    }
    {
        float3 v = Float3(1.0f, 2.0f, 3.0f);
        TEST("Float3 xyz x", v.x == 1.0f);
        TEST("Float3 xyz y", v.y == 2.0f);
        TEST("Float3 xyz z", v.z == 3.0f);
    }

    // Float4
    {
        float3 xyz = {1.0f, 2.0f, 3.0f};
        float4 v = Float4(xyz, 4.0f);
        TEST("Float4 x", v.x == 1.0f);
        TEST("Float4 y", v.y == 2.0f);
        TEST("Float4 z", v.z == 3.0f);
        TEST("Float4 w", v.w == 4.0f);
    }

    // Sin, Cos, Tan
    {
        f32 s = Sin(0.0f);
        TEST("Sin(0) == 0", s > -0.001f && s < 0.001f);
    }
    {
        f32 s = Sin(Pi / 2.0f);
        TEST("Sin(Pi/2) == 1", s > 0.999f && s < 1.001f);
    }
    {
        f32 c = Cos(0.0f);
        TEST("Cos(0) == 1", c > 0.999f && c < 1.001f);
    }
    {
        f32 c = Cos(Pi);
        TEST("Cos(Pi) == -1", c > -1.001f && c < -0.999f);
    }
    {
        f32 t = Tan(0.0f);
        TEST("Tan(0) == 0", t > -0.001f && t < 0.001f);
    }

    // Sqrt
    {
        f32 s = Sqrt(4.0f);
        TEST("Sqrt(4) == 2", s > 1.999f && s < 2.001f);
    }
    {
        f32 s = Sqrt(9.0f);
        TEST("Sqrt(9) == 3", s > 2.999f && s < 3.001f);
    }
    {
        f32 s = Sqrt(0.0f);
        TEST("Sqrt(0) == 0", s == 0.0f);
    }

    // Round
    {
        TEST("Round 1.4 -> 1", Round(1.4f) == 1.0f);
        TEST("Round 1.5 -> 2", Round(1.5f) == 2.0f);
        TEST("Round -1.4 -> -1", Round(-1.4f) == -1.0f);
        TEST("Round -1.5 -> -2", Round(-1.5f) == -2.0f);
    }

    // Floor
    {
        TEST("Floor 1.9 -> 1", Floor(1.9f) == 1);
        TEST("Floor 1.0 -> 1", Floor(1.0f) == 1);
        TEST("Floor -1.1 -> -2", Floor(-1.1f) == -2);
    }

    // Log2
    {
        f32 l = Log2(1.0f);
        TEST("Log2(1) == 0", l > -0.001f && l < 0.001f);
    }
    {
        f32 l = Log2(2.0f);
        TEST("Log2(2) == 1", l > 0.999f && l < 1.001f);
    }
    {
        f32 l = Log2(8.0f);
        TEST("Log2(8) == 3", l > 2.999f && l < 3.001f);
    }

    // FromTo
    {
        float3 a = {0.0f, 0.0f, 0.0f};
        float3 b = {1.0f, 2.0f, 3.0f};
        float3 res = FromTo(a, b);
        TEST("FromTo x", res.x == 1.0f);
        TEST("FromTo y", res.y == 2.0f);
        TEST("FromTo z", res.z == 3.0f);
    }

    // Add float2
    {
        float2 a = {1.0f, 2.0f};
        float2 b = {3.0f, 4.0f};
        float2 res = Add(a, b);
        TEST("Add float2 x", res.x == 4.0f);
        TEST("Add float2 y", res.y == 6.0f);
    }

    // Add float3
    {
        float3 a = {1.0f, 2.0f, 3.0f};
        float3 b = {4.0f, 5.0f, 6.0f};
        float3 res = Add(a, b);
        TEST("Add float3 x", res.x == 5.0f);
        TEST("Add float3 y", res.y == 7.0f);
        TEST("Add float3 z", res.z == 9.0f);
    }

    // Sub float3
    {
        float3 a = {4.0f, 5.0f, 6.0f};
        float3 b = {1.0f, 2.0f, 3.0f};
        float3 res = Sub(a, b);
        TEST("Sub float3 x", res.x == 3.0f);
        TEST("Sub float3 y", res.y == 3.0f);
        TEST("Sub float3 z", res.z == 3.0f);
    }

    // Mul float3 scalar
    {
        float3 a = {1.0f, 2.0f, 3.0f};
        float3 res = Mul(a, 2.0f);
        TEST("Mul float3 scalar x", res.x == 2.0f);
        TEST("Mul float3 scalar y", res.y == 4.0f);
        TEST("Mul float3 scalar z", res.z == 6.0f);
    }

    // Negate
    {
        float3 v = {1.0f, -2.0f, 3.0f};
        float3 res = Negate(v);
        TEST("Negate x", res.x == -1.0f);
        TEST("Negate y", res.y == 2.0f);
        TEST("Negate z", res.z == -3.0f);
    }

    // Dot float3
    {
        float3 a = {1.0f, 0.0f, 0.0f};
        float3 b = {0.0f, 1.0f, 0.0f};
        TEST("Dot perpendicular == 0", Dot(a, b) == 0.0f);
    }
    {
        float3 a = {1.0f, 0.0f, 0.0f};
        float3 b = {1.0f, 0.0f, 0.0f};
        TEST("Dot parallel == 1", Dot(a, b) == 1.0f);
    }
    {
        float3 a = {1.0f, 2.0f, 3.0f};
        float3 b = {4.0f, 5.0f, 6.0f};
        TEST("Dot general", Dot(a, b) == 32.0f);
    }

    // Dot float4
    {
        float4 a = {1.0f, 0.0f, 0.0f, 0.0f};
        float4 b = {1.0f, 0.0f, 0.0f, 0.0f};
        TEST("Dot float4 parallel", Dot(a, b) == 1.0f);
    }

    // Length2
    {
        float3 v = {3.0f, 4.0f, 0.0f};
        TEST("Length2 3-4-0", Length2(v) == 25.0f);
    }

    // Length
    {
        float3 v = {3.0f, 4.0f, 0.0f};
        f32 len = Length(v);
        TEST("Length 3-4-0 == 5", len > 4.999f && len < 5.001f);
    }

    // IsZero
    {
        float3 v = {0.0f, 0.0f, 0.0f};
        TEST("IsZero zero vector", IsZero(v));
    }
    {
        float3 v = {1.0f, 0.0f, 0.0f};
        TEST("IsZero non-zero vector", !IsZero(v));
    }

    // Normalize
    {
        float3 v = {3.0f, 0.0f, 0.0f};
        float3 n = Normalize(v);
        TEST("Normalize x", n.x > 0.999f && n.x < 1.001f);
        TEST("Normalize y", n.y > -0.001f && n.y < 0.001f);
        TEST("Normalize z", n.z > -0.001f && n.z < 0.001f);
    }
    {
        float3 v = {1.0f, 1.0f, 1.0f};
        float3 n = Normalize(v);
        f32 len = Length(n);
        TEST("Normalize length == 1", len > 0.999f && len < 1.001f);
    }

    // NormalizeIfNotZero
    {
        float3 v = {0.0f, 0.0f, 0.0f};
        float3 n = NormalizeIfNotZero(v);
        TEST("NormalizeIfNotZero zero vector unchanged x", n.x == 0.0f);
        TEST("NormalizeIfNotZero zero vector unchanged y", n.y == 0.0f);
        TEST("NormalizeIfNotZero zero vector unchanged z", n.z == 0.0f);
    }
    {
        float3 v = {2.0f, 0.0f, 0.0f};
        float3 n = NormalizeIfNotZero(v);
        TEST("NormalizeIfNotZero non-zero normalizes", n.x > 0.999f && n.x < 1.001f);
    }

    // Cross
    {
        float3 x = {1.0f, 0.0f, 0.0f};
        float3 y = {0.0f, 1.0f, 0.0f};
        float3 z = Cross(x, y);
        TEST("Cross x*y = z x", z.x > -0.001f && z.x < 0.001f);
        TEST("Cross x*y = z y", z.y > -0.001f && z.y < 0.001f);
        TEST("Cross x*y = z z", z.z > 0.999f && z.z < 1.001f);
    }
    {
        float3 a = {1.0f, 0.0f, 0.0f};
        float3 b = {1.0f, 0.0f, 0.0f};
        float3 res = Cross(a, b);
        TEST("Cross parallel == zero x", res.x > -0.001f && res.x < 0.001f);
        TEST("Cross parallel == zero y", res.y > -0.001f && res.y < 0.001f);
        TEST("Cross parallel == zero z", res.z > -0.001f && res.z < 0.001f);
    }

    // Eye
    {
        float4x4 m = Eye();
        TEST("Eye m00", m.m00 == 1.0f);
        TEST("Eye m11", m.m11 == 1.0f);
        TEST("Eye m22", m.m22 == 1.0f);
        TEST("Eye m33", m.m33 == 1.0f);
        TEST("Eye m01", m.m01 == 0.0f);
        TEST("Eye m10", m.m10 == 0.0f);
    }

    // Translate
    {
        float3 t = {1.0f, 2.0f, 3.0f};
        float4x4 m = Translate(t);
        TEST("Translate m30", m.m30 == 1.0f);
        TEST("Translate m31", m.m31 == 2.0f);
        TEST("Translate m32", m.m32 == 3.0f);
        TEST("Translate m00", m.m00 == 1.0f);
        TEST("Translate m11", m.m11 == 1.0f);
        TEST("Translate m22", m.m22 == 1.0f);
        TEST("Translate m33", m.m33 == 1.0f);
    }

    // Mul float4x4 * float4
    {
        float4x4 m = Eye();
        float4 v = {1.0f, 2.0f, 3.0f, 1.0f};
        float4 res = Mul(m, v);
        TEST("Mul Eye*v x", res.x == 1.0f);
        TEST("Mul Eye*v y", res.y == 2.0f);
        TEST("Mul Eye*v z", res.z == 3.0f);
        TEST("Mul Eye*v w", res.w == 1.0f);
    }

    // MulPoint
    {
        float3 t = {5.0f, 6.0f, 7.0f};
        float4x4 m = Translate(t);
        float3 p = {1.0f, 2.0f, 3.0f};
        float3 res = MulPoint(m, p);
        TEST("MulPoint x", res.x > 5.999f && res.x < 6.001f);
        TEST("MulPoint y", res.y > 7.999f && res.y < 8.001f);
        TEST("MulPoint z", res.z > 9.999f && res.z < 10.001f);
    }

    // MulVector
    {
        float3 t = {5.0f, 6.0f, 7.0f};
        float4x4 m = Translate(t);
        float3 v = {1.0f, 2.0f, 3.0f};
        float3 res = MulVector(m, v);
        // Vectors are not affected by translation
        TEST("MulVector x", res.x > 0.999f && res.x < 1.001f);
        TEST("MulVector y", res.y > 1.999f && res.y < 2.001f);
        TEST("MulVector z", res.z > 2.999f && res.z < 3.001f);
    }

    // Mul float4x4 * float4x4
    {
        float4x4 a = Eye();
        float4x4 b = Eye();
        float4x4 res = Mul(a, b);
        TEST("Mul Eye*Eye m00", res.m00 == 1.0f);
        TEST("Mul Eye*Eye m11", res.m11 == 1.0f);
        TEST("Mul Eye*Eye m22", res.m22 == 1.0f);
        TEST("Mul Eye*Eye m33", res.m33 == 1.0f);
        TEST("Mul Eye*Eye m01", res.m01 == 0.0f);
    }

    // Transpose float3x3
    {
        float3x3 m = {
            1, 2, 3,
            4, 5, 6,
            7, 8, 9,
        };
        float3x3 t = Transpose(m);
        TEST("Transpose3x3 m00", t.m00 == 1.0f);
        TEST("Transpose3x3 m01", t.m01 == 4.0f);
        TEST("Transpose3x3 m02", t.m02 == 7.0f);
        TEST("Transpose3x3 m10", t.m10 == 2.0f);
        TEST("Transpose3x3 m11", t.m11 == 5.0f);
        TEST("Transpose3x3 m12", t.m12 == 8.0f);
        TEST("Transpose3x3 m20", t.m20 == 3.0f);
        TEST("Transpose3x3 m21", t.m21 == 6.0f);
        TEST("Transpose3x3 m22", t.m22 == 9.0f);
    }

    // Transpose float4x4
    {
        float4x4 m = {
            1,  2,  3,  4,
            5,  6,  7,  8,
            9,  10, 11, 12,
            13, 14, 15, 16,
        };
        float4x4 t = Transpose(m);
        TEST("Transpose4x4 m00", t.m00 == 1.0f);
        TEST("Transpose4x4 m01", t.m01 == 5.0f);
        TEST("Transpose4x4 m10", t.m10 == 2.0f);
        TEST("Transpose4x4 m11", t.m11 == 6.0f);
        TEST("Transpose4x4 m33", t.m33 == 16.0f);
    }

    // Float3x3 from float4x4
    {
        float4x4 m = {
            1, 2, 3, 4,
            5, 6, 7, 8,
            9, 10, 11, 12,
            13, 14, 15, 16,
        };
        float3x3 r = Float3x3(m);
        TEST("Float3x3 from float4x4 m00", r.m00 == 1.0f);
        TEST("Float3x3 from float4x4 m01", r.m01 == 2.0f);
        TEST("Float3x3 from float4x4 m02", r.m02 == 3.0f);
        TEST("Float3x3 from float4x4 m10", r.m10 == 5.0f);
        TEST("Float3x3 from float4x4 m22", r.m22 == 11.0f);
    }

    // Float4x4 from float3x3
    {
        float3x3 m = {
            1, 2, 3,
            4, 5, 6,
            7, 8, 9,
        };
        float4x4 r = Float4x4(m);
        TEST("Float4x4 from float3x3 m00", r.m00 == 1.0f);
        TEST("Float4x4 from float3x3 m33", r.m33 == 1.0f);
        TEST("Float4x4 from float3x3 m03", r.m03 == 0.0f);
        TEST("Float4x4 from float3x3 m30", r.m30 == 0.0f);
    }

    // Operator overloads
    {
        float2 a = {1.0f, 2.0f};
        float2 b = {3.0f, 4.0f};
        float2 res = a + b;
        TEST("float2 operator+ x", res.x == 4.0f);
        TEST("float2 operator+ y", res.y == 6.0f);
    }
    {
        float3 a = {1.0f, 2.0f, 3.0f};
        float3 b = {4.0f, 5.0f, 6.0f};
        float3 res = a + b;
        TEST("float3 operator+ x", res.x == 5.0f);
        TEST("float3 operator+ y", res.y == 7.0f);
        TEST("float3 operator+ z", res.z == 9.0f);
    }
    {
        float2 a = {5.0f, 6.0f};
        float2 b = {3.0f, 4.0f};
        float2 res = a - b;
        TEST("float2 operator- x", res.x == 2.0f);
        TEST("float2 operator- y", res.y == 2.0f);
    }
    {
        float2 a = {1.0f, 2.0f};
        float2 res = 2.0f * a;
        TEST("float scalar * float2 x", res.x == 2.0f);
        TEST("float scalar * float2 y", res.y == 4.0f);
    }
    {
        float3 a = {1.0f, 2.0f, 3.0f};
        float3 res = 2.0f * a;
        TEST("float scalar * float3 x", res.x == 2.0f);
        TEST("float scalar * float3 y", res.y == 4.0f);
        TEST("float scalar * float3 z", res.z == 6.0f);
    }
    {
        float2 a = {2.0f, 3.0f};
        float2 b = {4.0f, 5.0f};
        float2 res = a * b;
        TEST("float2 operator* x", res.x == 8.0f);
        TEST("float2 operator* y", res.y == 15.0f);
    }
    {
        float2 a = {1.0f, 2.0f};
        float2 b = {3.0f, 4.0f};
        a += b;
        TEST("float2 operator+= x", a.x == 4.0f);
        TEST("float2 operator+= y", a.y == 6.0f);
    }
    {
        int2 pos = {1, 2};
        uint2 size = {3u, 4u};
        int2 res = pos + size;
        TEST("int2 + uint2 x", res.x == 4);
        TEST("int2 + uint2 y", res.y == 6);
    }
    {
        int2 a = {5, 6};
        int2 b = {3, 4};
        int2 res = a - b;
        TEST("int2 - int2 x", res.x == 2);
        TEST("int2 - int2 y", res.y == 2);
    }
    {
        int2 a = {1, 2};
        int2 b = {3, 4};
        int2 res = a + b;
        TEST("int2 + int2 x", res.x == 4);
        TEST("int2 + int2 y", res.y == 6);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Alignment tests

void TestAlignment()
{
    TEST_SECTION("Alignment");

    // IsPowerOfTwo
    {
        TEST("IsPowerOfTwo 1", IsPowerOfTwo(1));
        TEST("IsPowerOfTwo 2", IsPowerOfTwo(2));
        TEST("IsPowerOfTwo 4", IsPowerOfTwo(4));
        TEST("IsPowerOfTwo 8", IsPowerOfTwo(8));
        TEST("IsPowerOfTwo 16", IsPowerOfTwo(16));
        TEST("IsPowerOfTwo 1024", IsPowerOfTwo(1024));
        TEST("IsPowerOfTwo 0 is false", !IsPowerOfTwo(0));
        TEST("IsPowerOfTwo 3 is false", !IsPowerOfTwo(3));
        TEST("IsPowerOfTwo 5 is false", !IsPowerOfTwo(5));
        TEST("IsPowerOfTwo 6 is false", !IsPowerOfTwo(6));
        TEST("IsPowerOfTwo 7 is false", !IsPowerOfTwo(7));
    }

    // AlignUp
    {
        TEST("AlignUp already aligned", AlignUp(8, 4) == 8);
        TEST("AlignUp not aligned", AlignUp(5, 4) == 8);
        TEST("AlignUp 1 to 4", AlignUp(1, 4) == 4);
        TEST("AlignUp 0 to 4", AlignUp(0, 4) == 0);
        TEST("AlignUp 15 to 16", AlignUp(15, 16) == 16);
        TEST("AlignUp 16 to 16", AlignUp(16, 16) == 16);
        TEST("AlignUp 17 to 16", AlignUp(17, 16) == 32);
        TEST("AlignUp 1 to 256", AlignUp(1, 256) == 256);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Ticks tests

void TestTicks()
{
    TEST_SECTION("Ticks");

    // GetTicksPerSecond
    {
        u64 freq = GetTicksPerSecond();
        TEST("GetTicksPerSecond > 0", freq > 0);
        TEST("GetTicksPerSecond stable", freq == GetTicksPerSecond());
    }

    // GetTicks monotonicity
    {
        u64 t1 = GetTicks();
        u64 t2 = GetTicks();
        TEST("GetTicks monotonic", t1 <= t2);
    }

    // SecondsFromTicks
    {
        TEST("SecondsFromTicks(0) == 0", SecondsFromTicks(0) == 0.0);
        TEST("SecondsFromTicks(freq) == 1s", SecondsFromTicks(GetTicksPerSecond()) == 1.0);
        double halfSecond = SecondsFromTicks(GetTicksPerSecond() / 2);
        TEST("SecondsFromTicks(freq/2) ~= 0.5s", halfSecond > 0.49 && halfSecond < 0.51);
    }

    // Elapsed ticks over a real wait measure wall-clock time
    {
        u64 t1 = GetTicks();
        SleepMillis(50);
        u64 t2 = GetTicks();
        double elapsed = SecondsFromTicks(t2 - t1);
        // Generous bounds: sleep granularity is coarse (~16ms on Windows) and may overshoot
        TEST("Elapsed over 50ms sleep >= 40ms", elapsed >= 0.040);
        TEST("Elapsed over 50ms sleep < 500ms", elapsed < 0.500);
    }

    // GetClock/GetSecondsElapsed agree with GetTicks/SecondsFromTicks
    {
        Clock c1 = GetClock();
        u64 t1 = GetTicks();
        SleepMillis(20);
        f32 clockElapsed = GetSecondsElapsed(c1, GetClock());
        double tickElapsed = SecondsFromTicks(GetTicks() - t1);
        double diff = tickElapsed - (double)clockElapsed;
        TEST("Clock and ticks agree", clockElapsed > 0.0f && diff > -0.010 && diff < 0.010);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Clock tests

void TestClock()
{
    TEST_SECTION("Clock");

    // GetClock / GetSecondsElapsed
    {
        Clock start = GetClock();
        // Do some work
        volatile u32 sum = 0;
        for (u32 i = 0; i < 1000000; i++) sum += i;
        Clock end = GetClock();
        f32 elapsed = GetSecondsElapsed(start, end);
        TEST("GetSecondsElapsed >= 0", elapsed >= 0.0f);
        TEST("GetSecondsElapsed < 10s", elapsed < 10.0f);
    }

    // Two consecutive clocks should have elapsed >= 0
    {
        Clock c1 = GetClock();
        Clock c2 = GetClock();
        f32 elapsed = GetSecondsElapsed(c1, c2);
        TEST("Consecutive clocks elapsed >= 0", elapsed >= 0.0f);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Sized types tests

void TestSizedTypes()
{
    TEST_SECTION("Sized Types");

    TEST("sizeof i8 == 1", sizeof(i8) == 1);
    TEST("sizeof i16 == 2", sizeof(i16) == 2);
    TEST("sizeof i32 == 4", sizeof(i32) == 4);
    TEST("sizeof i64 == 8", sizeof(i64) == 8);
    TEST("sizeof u8 == 1", sizeof(u8) == 1);
    TEST("sizeof u16 == 2", sizeof(u16) == 2);
    TEST("sizeof u32 == 4", sizeof(u32) == 4);
    TEST("sizeof u64 == 8", sizeof(u64) == 8);
    TEST("sizeof f32 == 4", sizeof(f32) == 4);
    TEST("sizeof f64 == 8", sizeof(f64) == 8);
    TEST("sizeof byte == 1", sizeof(byte) == 1);

    TEST("I8_MIN == -128", I8_MIN == -128);
    TEST("I8_MAX == 127", I8_MAX == 127);
    TEST("U8_MIN == 0", U8_MIN == 0);
    TEST("U8_MAX == 255", U8_MAX == 255);
    TEST("I16_MIN == -32768", I16_MIN == -32768);
    TEST("I16_MAX == 32767", I16_MAX == 32767);
    TEST("U16_MIN == 0", U16_MIN == 0);
    TEST("U16_MAX == 65535", U16_MAX == 65535);
    TEST("I32_MIN == -2147483648", I32_MIN == -2147483648);
    TEST("I32_MAX == 2147483647", I32_MAX == 2147483647);
    TEST("U32_MIN == 0", U32_MIN == 0);
    TEST("U32_MAX == 4294967295", U32_MAX == 4294967295u);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Handle pool tests

struct TestPayload
{
    u32 elements[8];
    u16 moveCount;
    u16 moveDst[8];
    u16 moveSrc[8];
};

static MOVE_ELEMENT(TestMoveElement)
{
    TestPayload &payload = *(TestPayload*)data;
    payload.elements[dstIndex] = payload.elements[srcIndex];
    if (payload.moveCount < ARRAY_COUNT(payload.moveDst)) {
        payload.moveDst[payload.moveCount] = dstIndex;
        payload.moveSrc[payload.moveCount] = srcIndex;
        payload.moveCount++;
    }
}

void TestHandlePool()
{
    TEST_SECTION("HandlePool");

    const u32 arenaSize = MB(1);
    byte *memory = (byte*)AllocateVirtualMemory(arenaSize);

    // Initialize
    {
        Arena arena = MakeArena(memory, arenaSize, "test");
        HandlePool pool = {};
        Initialize(pool, arena, 8);
        TEST("Initialize count == 0", pool.count == 0);
        TEST("Initialize HandleCount == 0", HandleCount(pool) == 0);
        TEST("Initialize is not full", !IsFull(pool));
        TEST("InvalidHandle is not valid", !IsValidHandle(pool, InvalidHandle));
    }

    // NewHandle hands out consecutive element indices
    {
        Arena arena = MakeArena(memory, arenaSize, "test");
        HandlePool pool = {};
        Initialize(pool, arena, 8);

        const Handle a = NewHandle(pool);
        const Handle b = NewHandle(pool);
        const Handle c = NewHandle(pool);

        TEST("NewHandle returns a valid handle", IsValidHandle(pool, a));
        TEST("NewHandle indices are consecutive",
             GetHandleIndex(pool, a) == 0 && GetHandleIndex(pool, b) == 1 && GetHandleIndex(pool, c) == 2);
        TEST("NewHandle handles are distinct", a != b && b != c);
        TEST("NewHandle advances HandleCount", HandleCount(pool) == 3);
        TEST("GetHandleAt matches the issued handle", GetHandleAt(pool, 1) == b);
    }

    // FreeHandle only marks, the element lives out the frame
    {
        Arena arena = MakeArena(memory, arenaSize, "test");
        HandlePool pool = {};
        Initialize(pool, arena, 8);

        const Handle a = NewHandle(pool);
        const Handle b = NewHandle(pool);
        FreeHandle(pool, b);

        TEST("FreeHandle keeps the handle valid", IsValidHandle(pool, b));
        TEST("FreeHandle marks the handle", IsPendingRemoval(pool, b));
        TEST("FreeHandle leaves other handles unmarked", !IsPendingRemoval(pool, a));
        TEST("FreeHandle keeps the element addressable", GetHandleIndex(pool, b) == 1);
        TEST("FreeHandle keeps the array whole", HandleCount(pool) == 2);
        TEST("FreeHandle does not move the survivors", GetHandleIndex(pool, a) == 0);
    }

    // Freeing twice in the same frame removes one element
    {
        Arena arena = MakeArena(memory, arenaSize, "test");
        HandlePool pool = {};
        Initialize(pool, arena, 8);

        const Handle a = NewHandle(pool);
        const Handle b = NewHandle(pool);
        FreeHandle(pool, a);
        FreeHandle(pool, a);

        TEST("Double free removes one element", CompactPool(pool, nullptr, nullptr, nullptr) == 1);
        TEST("Double free leaves the rest alone", HandleCount(pool) == 1 && IsValidHandle(pool, b));
    }

    // CompactPool closes the holes and reports the moves
    {
        Arena arena = MakeArena(memory, arenaSize, "test");
        HandlePool pool = {};
        Initialize(pool, arena, 8);

        TestPayload payload = {};
        Handle handles[4];
        for (u16 i = 0; i < 4; ++i) {
            handles[i] = NewHandle(pool);
            payload.elements[GetHandleIndex(pool, handles[i])] = 10 * (i + 1);
        }

        FreeHandle(pool, handles[1]);
        const u16 removed = CompactPool(pool, TestMoveElement, nullptr, &payload);

        TEST("CompactPool returns the removed count", removed == 1);
        TEST("CompactPool shrinks the array", HandleCount(pool) == 3);
        TEST("CompactPool invalidates the removed handle", !IsValidHandle(pool, handles[1]));
        TEST("CompactPool moves every element after the hole", payload.moveCount == 2);
        TEST("CompactPool moves towards the front",
             payload.moveDst[0] == 1 && payload.moveSrc[0] == 2 &&
             payload.moveDst[1] == 2 && payload.moveSrc[1] == 3);
        TEST("CompactPool keeps the payload compact",
             payload.elements[0] == 10 && payload.elements[1] == 30 && payload.elements[2] == 40);
        TEST("CompactPool preserves the relative order",
             GetHandleIndex(pool, handles[0]) == 0 &&
             GetHandleIndex(pool, handles[2]) == 1 &&
             GetHandleIndex(pool, handles[3]) == 2);

        // Nothing to do the second time around
        payload.moveCount = 0;
        TEST("CompactPool is a no-op without holes", CompactPool(pool, TestMoveElement, nullptr, &payload) == 0);
        TEST("CompactPool moves nothing without holes", payload.moveCount == 0);
    }

    // A recycled slot never answers to the old handle
    {
        Arena arena = MakeArena(memory, arenaSize, "test");
        HandlePool pool = {};
        Initialize(pool, arena, 8);

        const Handle a = NewHandle(pool);
        FreeHandle(pool, a);
        CompactPool(pool, nullptr, nullptr, nullptr);

        const Handle b = NewHandle(pool);
        TEST("Freed slot is recycled", b.slot == a.slot);
        TEST("Recycled slot bumps the generation", b.gen != a.gen);
        TEST("Recycled slot rejects the stale handle", !IsValidHandle(pool, a));
        TEST("Recycled slot accepts the new handle", IsValidHandle(pool, b));
    }

    // A slot freed this frame is not reusable until the compaction
    {
        Arena arena = MakeArena(memory, arenaSize, "test");
        HandlePool pool = {};
        Initialize(pool, arena, 8);

        const Handle a = NewHandle(pool);
        FreeHandle(pool, a);
        const Handle b = NewHandle(pool);

        TEST("Pending removal does not hand the slot back", b.slot != a.slot);
        TEST("Pending removal does not share the element", GetHandleIndex(pool, b) != GetHandleIndex(pool, a));
    }

    // A plain loop stays a valid traversal across a removal
    {
        Arena arena = MakeArena(memory, arenaSize, "test");
        HandlePool pool = {};
        Initialize(pool, arena, 8);

        Handle handles[3];
        for (u16 i = 0; i < 3; ++i) {
            handles[i] = NewHandle(pool);
        }

        // Removing from inside the loop must not disturb it
        u16 visited = 0;
        bool allResolve = true;
        for (u16 i = 0; i < HandleCount(pool); ++i) {
            const Handle handle = GetHandleAt(pool, i);
            allResolve = allResolve && IsValidHandle(pool, handle) && GetHandleIndex(pool, handle) == i;
            if (i == 0) {
                FreeHandle(pool, handles[0]);
            }
            visited++;
        }
        TEST("Removing mid-loop visits every element", visited == 3);
        TEST("Removing mid-loop keeps every element addressable", allResolve);

        CompactPool(pool, nullptr, nullptr, nullptr);
        TEST("The removal lands on the next compaction",
             HandleCount(pool) == 2 && !IsValidHandle(pool, handles[0]));
        TEST("Compaction moves the survivors up",
             GetHandleAt(pool, 0) == handles[1] && GetHandleAt(pool, 1) == handles[2]);
    }

    // An empty pool iterates zero times
    {
        Arena arena = MakeArena(memory, arenaSize, "test");
        HandlePool pool = {};
        Initialize(pool, arena, 8);

        u16 visited = 0;
        for (u16 i = 0; i < HandleCount(pool); ++i) {
            visited++;
        }
        TEST("Empty pool iterates zero times", visited == 0);
    }

    // Filling the pool
    {
        Arena arena = MakeArena(memory, arenaSize, "test");
        HandlePool pool = {};
        Initialize(pool, arena, 4);

        for (u16 i = 0; i < 4; ++i) {
            NewHandle(pool);
        }
        TEST("Pool reports full at the limit", IsFull(pool));
        TEST("Full pool counts every element", HandleCount(pool) == 4);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Array compaction tests

// The ID sits inside a nested struct, the way the engine's arrays carry theirs
struct TestCompactDesc
{
    ID id;
    u32 value;
};

struct TestCompactElement
{
    TestCompactDesc desc;
    u32 payload;
};

struct TestCompactPayload
{
    TestCompactElement *elements;
    u32 parallel[8]; // Parallel to the elements, dragged along by the move callback
    u32 moveCount;
    u32 moveDst[8];
    u32 moveSrc[8];
    u32 removeCount;
    u32 removedIndices[8];
    u32 removedPayloads[8];
};

static COMPACT_MOVE(TestCompactMove)
{
    TestCompactPayload &payload = *(TestCompactPayload*)data;
    payload.parallel[dstIndex] = payload.parallel[srcIndex];
    if (payload.moveCount < ARRAY_COUNT(payload.moveDst)) {
        payload.moveDst[payload.moveCount] = dstIndex;
        payload.moveSrc[payload.moveCount] = srcIndex;
        payload.moveCount++;
    }
}

// Reads the element it is handed, which is only legal because the compaction has not
// overwritten it yet
static COMPACT_REMOVE(TestCompactRemove)
{
    TestCompactPayload &payload = *(TestCompactPayload*)data;
    if (payload.removeCount < ARRAY_COUNT(payload.removedIndices)) {
        payload.removedIndices[payload.removeCount] = index;
        payload.removedPayloads[payload.removeCount] = payload.elements[index].payload;
        payload.removeCount++;
    }
}

// Fills the array with live elements, each bound to a fresh ID
static void MakeTestElements(TestCompactElement *elements, u32 count, TestCompactPayload &payload)
{
    payload = {};
    payload.elements = elements;

    for (u32 i = 0; i < count; ++i) {
        elements[i] = {};
        elements[i].desc.value = 10 * (i + 1);
        elements[i].payload = 100 * (i + 1);
        payload.parallel[i] = 1000 * (i + 1);
        BindID(&elements[i].desc.id, &elements[i]);
    }
}

// Marks an element the way the engine's Remove functions do: clear the ID, drop the slot
static void MarkTestElementRemoved(TestCompactElement &element)
{
    const ID id = element.desc.id;
    element.desc.id = {};
    Invalidate(id);
}

static bool IsZeroedTestElement(const TestCompactElement &element)
{
    const bool zeroed = element.desc.id.slot == 0 && element.desc.value == 0 && element.payload == 0;
    return zeroed;
}

void TestCompactArray()
{
    TEST_SECTION("CompactArrayByID");

    InitializeIDPool();

    // The offset is the whole of what the compaction knows about the element type
    TEST("OFFSET_OF finds a leading member", OFFSET_OF(TestCompactElement, desc.id) == 0);
    TEST("OFFSET_OF walks into nested members",
         OFFSET_OF(TestCompactElement, payload) == sizeof(TestCompactDesc));

    // Nothing to do without holes
    {
        TestCompactElement elements[4];
        TestCompactPayload payload;
        MakeTestElements(elements, 4, payload);

        u32 count = 4;
        const bool compacted = COMPACT_ARRAY_BY_ID(TestCompactElement, elements, count, desc.id,
                TestCompactMove, TestCompactRemove, &payload);

        TEST("Compaction without holes reports nothing done", !compacted);
        TEST("Compaction without holes keeps the count", count == 4);
        TEST("Compaction without holes moves nothing", payload.moveCount == 0);
        TEST("Compaction without holes removes nothing", payload.removeCount == 0);
        TEST("Compaction without holes leaves the elements alone",
             elements[0].desc.value == 10 && elements[3].desc.value == 40);
    }

    // A hole in the middle closes, and everything after it comes forward
    {
        TestCompactElement elements[4];
        TestCompactPayload payload;
        MakeTestElements(elements, 4, payload);

        const ID removedId = elements[1].desc.id;
        const ID survivorId = elements[2].desc.id;
        MarkTestElementRemoved(elements[1]);

        u32 count = 4;
        const bool compacted = COMPACT_ARRAY_BY_ID(TestCompactElement, elements, count, desc.id,
                TestCompactMove, TestCompactRemove, &payload);

        TEST("Compaction reports what it did", compacted);
        TEST("Compaction shrinks the array", count == 3);
        TEST("Compaction preserves the relative order",
             elements[0].desc.value == 10 && elements[1].desc.value == 30 && elements[2].desc.value == 40);
        TEST("Compaction moves every element after the hole", payload.moveCount == 2);
        TEST("Compaction moves towards the front",
             payload.moveDst[0] == 1 && payload.moveSrc[0] == 2 &&
             payload.moveDst[1] == 2 && payload.moveSrc[1] == 3);
        TEST("Compaction drags the parallel array along",
             payload.parallel[0] == 1000 && payload.parallel[1] == 3000 && payload.parallel[2] == 4000);
        TEST("Compaction re-points the surviving IDs",
             GetObject(survivorId) == &elements[1] &&
             GetObject(elements[0].desc.id) == &elements[0] &&
             GetObject(elements[2].desc.id) == &elements[2]);
        TEST("Compaction leaves the removed ID invalid", !Valid(removedId));
        TEST("Compaction zeroes the vacated tail", IsZeroedTestElement(elements[3]));
    }

    // Holes at the front, in the middle and at the back
    {
        TestCompactElement elements[5];
        TestCompactPayload payload;
        MakeTestElements(elements, 5, payload);

        MarkTestElementRemoved(elements[0]);
        MarkTestElementRemoved(elements[2]);
        MarkTestElementRemoved(elements[4]);

        u32 count = 5;
        COMPACT_ARRAY_BY_ID(TestCompactElement, elements, count, desc.id,
                TestCompactMove, TestCompactRemove, &payload);

        TEST("Compaction drops every hole", count == 2);
        TEST("Compaction keeps the survivors in order",
             elements[0].desc.value == 20 && elements[1].desc.value == 40);
        TEST("Compaction re-points every survivor",
             GetObject(elements[0].desc.id) == &elements[0] &&
             GetObject(elements[1].desc.id) == &elements[1]);
        TEST("Compaction zeroes everything past the survivors",
             IsZeroedTestElement(elements[2]) && IsZeroedTestElement(elements[3]) &&
             IsZeroedTestElement(elements[4]));
    }

    // onRemove runs for each dropped element, while it is still readable
    {
        TestCompactElement elements[5];
        TestCompactPayload payload;
        MakeTestElements(elements, 5, payload);

        MarkTestElementRemoved(elements[0]);
        MarkTestElementRemoved(elements[2]);
        MarkTestElementRemoved(elements[4]);

        u32 count = 5;
        COMPACT_ARRAY_BY_ID(TestCompactElement, elements, count, desc.id,
                nullptr, TestCompactRemove, &payload);

        TEST("onRemove runs once per dropped element", count == 2 && payload.removeCount == 3);
        TEST("onRemove gets the index the element was at",
             payload.removedIndices[0] == 0 && payload.removedIndices[1] == 2 &&
             payload.removedIndices[2] == 4);
        TEST("onRemove sees the dropped element intact",
             payload.removedPayloads[0] == 100 && payload.removedPayloads[1] == 300 &&
             payload.removedPayloads[2] == 500);
    }

    // A hole at the back displaces nobody
    {
        TestCompactElement elements[3];
        TestCompactPayload payload;
        MakeTestElements(elements, 3, payload);

        MarkTestElementRemoved(elements[2]);

        u32 count = 3;
        const bool compacted = COMPACT_ARRAY_BY_ID(TestCompactElement, elements, count, desc.id,
                TestCompactMove, TestCompactRemove, &payload);

        TEST("A trailing hole still counts as compacted", compacted);
        TEST("A trailing hole shrinks the array", count == 2);
        TEST("A trailing hole moves nothing", payload.moveCount == 0);
        TEST("A trailing hole leaves the survivors put",
             elements[0].desc.value == 10 && elements[1].desc.value == 20);
        TEST("A trailing hole is zeroed", IsZeroedTestElement(elements[2]));
    }

    // Removing everything empties the array
    {
        TestCompactElement elements[3];
        TestCompactPayload payload;
        MakeTestElements(elements, 3, payload);

        for (u32 i = 0; i < 3; ++i) {
            MarkTestElementRemoved(elements[i]);
        }

        u32 count = 3;
        COMPACT_ARRAY_BY_ID(TestCompactElement, elements, count, desc.id,
                TestCompactMove, TestCompactRemove, &payload);

        TEST("Removing everything empties the array", count == 0);
        TEST("Removing everything drops every element", payload.removeCount == 3);
        TEST("Removing everything moves nothing", payload.moveCount == 0);
        TEST("Removing everything zeroes the array",
             IsZeroedTestElement(elements[0]) && IsZeroedTestElement(elements[1]) &&
             IsZeroedTestElement(elements[2]));
    }

    // An empty array, and callbacks nobody passed
    {
        TestCompactElement elements[1];
        TestCompactPayload payload;
        MakeTestElements(elements, 1, payload);

        u32 empty = 0;
        TEST("An empty array has nothing to compact",
             !COMPACT_ARRAY_BY_ID(TestCompactElement, elements, empty, desc.id,
                     nullptr, nullptr, nullptr));
        TEST("An empty array stays empty", empty == 0);

        MarkTestElementRemoved(elements[0]);

        u32 count = 1;
        TEST("The callbacks are optional",
             COMPACT_ARRAY_BY_ID(TestCompactElement, elements, count, desc.id,
                     nullptr, nullptr, nullptr));
        TEST("The only element is gone", count == 0 && IsZeroedTestElement(elements[0]));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Main

int main()
{
#ifdef _WIN32
    // Enable colors
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
#endif

    LOG(Info, ANSI_BOLD "====================================\n" ANSI_RESET);
    LOG(Info, ANSI_BOLD "  ilu_core.h Unit Tests\n" ANSI_RESET);
    LOG(Info, ANSI_BOLD "====================================\n" ANSI_RESET);

    TestSizedTypes();
    TestTruncations();
    TestIntrinsics();
    TestStrings();
    TestHashing();
    TestMemory();
    TestArena();
    TestStringInterning();
    TestFilePaths();
    TestMath();
    TestAlignment();
    TestTicks();
    TestClock();
    TestHandlePool();
    TestCompactArray();

    LOG(Info, "\n" ANSI_BOLD "====================================\n" ANSI_RESET);
    if (gTestsFailed == 0) {
        LOG(Info, ANSI_BOLD ANSI_GREEN "  Results: %u passed, %u failed\n" ANSI_RESET, gTestsPassed, gTestsFailed);
    } else {
        LOG(Info, ANSI_BOLD "  Results: " ANSI_GREEN "%u passed" ANSI_RESET ANSI_BOLD ", " ANSI_RED "%u failed\n" ANSI_RESET, gTestsPassed, gTestsFailed);
    }
    LOG(Info, ANSI_BOLD "====================================\n" ANSI_RESET);


    return gTestsFailed > 0 ? 1 : 0;
}
