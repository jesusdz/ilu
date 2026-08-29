/*
 * cast.h
 * Author: Jesus Diaz Garcia
 *
 * Single file utility to parse C files and generate their AST structure.
 * It should serve to parse most type and data definitions, yet it is not complete.
 * The AST structure has been created with the rules available in the following
 * ANSI C Yacc grammar that is based on the 2011 ISO C standard:
 *
 * https://www.quut.com/c/ANSI-C-grammar-y.html
 *
 * The file can be included in your project multiple times. Once and only once,
 * however, it needs to be told to embed its implementation like this:
 *
 * #define CAST_IMPLEMENTATION
 * #include "cast.h"
 *
 * Additionally `#define CAST_PRINT` may be added before including `cast.h` in
 * case we want to have functionality to print the AST.
 *
 * Structs, struct fields and enums can be tagged with user-defined macros (a la
 * Unreal's UCLASS/UPROPERTY). The names of these macros are not hardcoded, but
 * provided by the user of this file through a `CastConfig` object:
 *
 * const CastConfig config = {
 *   .structTag = "ILU_STRUCT", .fieldTag = "ILU_PROPERTY", .enumTag = "ILU_ENUM",
 * };
 * const Cast *cast = Cast_Create(arena, text, textSize, config);
 *
 * Tags are then available in `CastStructSpecifier::tag`, `CastStructDeclaration::tag`
 * and `CastEnumSpecifier::tag`.
 *
 * These are the main API functions used to create the C AST and start inspecting it:
 *
 * const Cast *Cast_Create( CastArena &arena, const char *text, cast_u64 textSize);
 * const Cast *Cast_Create( CastArena &arena, const char *text, cast_u64 textSize, const CastConfig &config);
 * const char *Cast_GetError();
 * int Cast_EvaluateInt( const CastExpression *ast ); // And for all trivial types
 * void Cast_Print( const Cast *cast ); // Only if CAST_PRINT was defined
 */

#ifndef CAST_H
#define CAST_H

////////////////////////////////////////////////////////////////////////
// Helper macros and types

// NOTE: We can get rid of the dependency on "ilu_core.h" by providing an
// implementation for the following macros to support:
// - Logging
// - Compile-time and run-time assertions
// - Sized types
// - Strings
// - Memory
// - Linear arena allocators

#ifndef CAST_USE_TOOLS
#define CAST_USE_TOOLS 1
#endif // #ifndef CAST_USE_TOOLS

#if CAST_USE_TOOLS
#include "../ilu_core.h"
#define CAST_LOG LOG
#define CAST_ASSERT ASSERT
#define CAST_ARRAY_COUNT ARRAY_COUNT
#define CAST_CT_ASSERT CT_ASSERT
#define cast_byte byte
#define cast_i32 i32
#define cast_u32 u32
#define cast_u64 u64
#define cast_f32 f32
#define CastString String
#define CastArena Arena
#define CastMemSet MemSet
#define CastPushStruct PushStruct
#define CastPushZeroStruct PushZeroStruct
#define CastStrToBool StrToBool
#define CastStrToChar StrToChar
#define CastStrToInt StrToInt
//#define CastStrToShortInt StrToShortInt
//#define CastStrToLongInt StrToLongInt
//#define CastStrToLongLongInt StrToLongLongInt
#define CastStrToUnsignedInt StrToUnsignedInt
//#define CastStrToUnsignedShortInt StrToUnsignedShortInt
//#define CastStrToUnsignedLongInt StrToUnsignedLongInt
//#define CastStrToUnsignedLongLongInt StrToUnsignedLongLongInt
#define CastStrToFloat StrToFloat
//#define CastStrToDouble StrToFloat
#define CastStrEq StrEq
#endif // #if CAST_USE_TOOLS

#include <stdarg.h> // va_args
#include <stdio.h> // printf


////////////////////////////////////////////////////////////////////////
// Cast API

// Cast Enums

enum CastStorageClassSpecifierType
{
	CAST_TYPEDEF,
	CAST_EXTERN,
	CAST_STATIC,
	CAST_THREAD_LOCAL,
	CAST_AUTO,
	CAST_REGISTER,
};

enum CastTypeSpecifierType
{
	CAST_VOID,
	CAST_BOOL,
	CAST_CHAR,
	CAST_INT,
	CAST_FLOAT,
	CAST_DOUBLE,
	CAST_SHORT,
	CAST_LONG,
	CAST_SIGNED,
	CAST_UNSIGNED,
	CAST_IDENTIFIER,
	CAST_STRUCT,
	CAST_ENUM,
};

enum CastTypeQualifierType
{
	CAST_CONST,
	CAST_RESTRICT,
	CAST_VOLATILE,
};

enum CastExpressionType
{
	CAST_EXPR_NUMBER,
	CAST_EXPR_STRING,
	CAST_EXPR_IDENTIFIER,
	CAST_EXPR_ARRAY_COUNT,
};

// Cast struct forward declarations

struct CastTag;
struct CastStorageClassSpecifier;
struct CastSpecifierQualifierList;
struct CastStructDeclaratorList;
struct CastStructDeclaration;
struct CastStructDeclarationList;
struct CastStructSpecifier;
struct CastEnumerator;
struct CastEnumeratorList;
struct CastEnumSpecifier;
struct CastTypeSpecifier;
struct CastTypeQualifier;
struct CastTypeQualifierList;
struct CastFunctionSpecifier;
struct CastDeclarationSpecifiers;
struct CastExpression;
struct CastDirectDeclarator;
struct CastPointer;
struct CastDeclarator;
struct CastParameterDeclaration;
struct CastParameterList;
struct CastParameterTypeList;
struct CastDesignator;
struct CastDesignatorList;
struct CastDesignation;
struct CastInitializer;
struct CastInitializerList;
struct CastInitDeclarator;
struct CastInitDeclaratorList;
struct CastDeclaration;
struct CastFunctionDefinition;
struct CastExternalDeclaration;
struct CastTranslationUnit;
struct Cast;

// Cast tags

// A tag is a user-defined macro placed right before a struct, a struct field or
// an enum definition (e.g. ILU_STRUCT() / ILU_PROPERTY(...)). Its arguments, if
// any, are captured verbatim and are not interpreted by the parser.
struct CastTag
{
	CastString name;
	CastString arguments;
};

// Names of the tag macros the parser has to look for. Tags whose name is NULL
// (or empty) are simply not parsed, which is the default behaviour.
struct CastConfig
{
	const char *structTag; // Tag preceding struct definitions
	const char *fieldTag;  // Tag preceding struct field definitions
	const char *enumTag;   // Tag preceding enum definitions
};

// Cast functions

const Cast *Cast_Create( CastArena &arena, const char *text, cast_u64 textSize );
const Cast *Cast_Create( CastArena &arena, const char *text, cast_u64 textSize, const CastConfig &config );
const char *Cast_GetError();
#ifdef CAST_PRINT
void Cast_Print( const Cast *cast );
#endif

#define CAST_DECLARE_EVALUATE_FUNCTION(Type, Name) \
	Type Cast_Evaluate##Name( const CastExpression *ast );

CAST_DECLARE_EVALUATE_FUNCTION(bool, Bool);
CAST_DECLARE_EVALUATE_FUNCTION(char, Char);
CAST_DECLARE_EVALUATE_FUNCTION(unsigned char, UnsignedChar);
CAST_DECLARE_EVALUATE_FUNCTION(int, Int);
CAST_DECLARE_EVALUATE_FUNCTION(short int, ShortInt);
CAST_DECLARE_EVALUATE_FUNCTION(long int, LongInt);
CAST_DECLARE_EVALUATE_FUNCTION(long long int, LongLongInt);
CAST_DECLARE_EVALUATE_FUNCTION(unsigned int, UnsignedInt);
CAST_DECLARE_EVALUATE_FUNCTION(short unsigned int, UnsignedShortInt);
CAST_DECLARE_EVALUATE_FUNCTION(long unsigned int, UnsignedLongInt);
CAST_DECLARE_EVALUATE_FUNCTION(long long unsigned int, UnsignedLongLongInt);
CAST_DECLARE_EVALUATE_FUNCTION(float, Float);
CAST_DECLARE_EVALUATE_FUNCTION(double, Double);

// Cast structs

struct CastStorageClassSpecifier
{
	CastStorageClassSpecifierType type;
};

struct CastSpecifierQualifierList
{
	CastTypeSpecifier *typeSpecifier;
	CastTypeQualifier *typeQualifier;
	CastSpecifierQualifierList *next;
};

struct CastStructDeclaratorList
{
	CastDeclarator *structDeclarator; // pass through struct-declarator in the grammar
	CastStructDeclaratorList *next;
};

struct CastStructDeclaration
{
	CastTag *tag; // optional
	CastSpecifierQualifierList *specifierQualifierList;
	CastStructDeclaratorList *structDeclaratorList;
};

struct CastStructDeclarationList
{
	CastStructDeclaration *structDeclaration;
	CastStructDeclarationList *next;
};

struct CastStructSpecifier
{
	CastTag *tag; // optional
	CastString name;
	CastStructDeclarationList *structDeclarationList;
};

struct CastEnumerator
{
	CastString name;
	// TODO: Could have a constant expression
};

struct CastEnumeratorList
{
	CastEnumerator *enumerator;
	CastEnumeratorList *next;
};

struct CastEnumSpecifier
{
	CastTag *tag; // optional
	CastString name;
	CastEnumeratorList *enumeratorList;
};

struct CastTypeSpecifier
{
	CastTypeSpecifierType type;
	union
	{
		CastString identifier;
		CastStructSpecifier *structSpecifier;
		CastEnumSpecifier *enumSpecifier;
	};
};

struct CastTypeQualifier
{
	CastTypeQualifierType type;
};

struct CastTypeQualifierList
{
	CastTypeQualifier *typeQualifier;
	CastTypeQualifierList *next;
};

struct CastFunctionSpecifier
{
// : INLINE
// | NORETURN
};

struct CastDeclarationSpecifiers
{
	CastStorageClassSpecifier *storageClassSpecifier;
	CastTypeSpecifier *typeSpecifier;
	CastTypeQualifier *typeQualifier;
	CastFunctionSpecifier *functionSpecifier;
	//CastAlignmentSpecifier *alignmentSpecifier;
	CastDeclarationSpecifiers *next;
};

struct CastExpression
{
	CastExpressionType type;
	CastString constant; // TODO: Expressions can be much more complex
	bool negative;
};

struct CastDirectDeclarator
{
	CastString name;
	CastExpression *expression;
	CastParameterTypeList *parameterTypeList; // present when isFunction is set
	bool isArray : 1;
	bool isFunction : 1;
};

struct CastPointer
{
	CastTypeQualifierList *typeQualifierList;
	CastPointer *next;
};

struct CastDeclarator
{
	bool isReference; // Incomplete, but it works for the simple case object references
	CastPointer *pointer;
	CastDirectDeclarator *directDeclarator;
};

struct CastParameterDeclaration
{
	CastDeclarationSpecifiers *declarationSpecifiers;
	CastDeclarator *declarator; // optional (e.g. unnamed parameter, or "void")
};

struct CastParameterList
{
	CastParameterDeclaration *parameterDeclaration;
	CastParameterList *next;
};

struct CastParameterTypeList
{
	CastParameterList *parameterList;
};

struct CastDesignator
{
	CastString identifier;
	// TODO: Add missing array element designator: [ constant-expression ]
};

struct CastDesignatorList
{
	CastDesignator *designator;
	CastDesignatorList *next;
};

struct CastDesignation
{
	CastDesignatorList *designatorList;
};

struct CastInitializer
{
	CastExpression *expression;
	CastInitializerList *initializerList;
	cast_u32 initializerCount;
};

struct CastInitializerList
{
	CastDesignation *designation; // optional
	CastInitializer *initializer;
	CastInitializerList *next;
};

struct CastInitDeclarator
{
	CastDeclarator *declarator;
	CastInitializer *initializer;
};

struct CastInitDeclaratorList
{
	CastInitDeclarator *initDeclarator;
	CastInitDeclaratorList *next;
};

struct CastDeclaration
{
	CastDeclarationSpecifiers *declarationSpecifiers;
	CastInitDeclaratorList *initDeclaratorList;
	//CastStaticAssertDeclaration *staticAssertDeclaration;
};

struct CastFunctionDefinition
{
	CastDeclarationSpecifiers *declarationSpecifiers;
	CastDeclarator *declarator;
};

struct CastExternalDeclaration
{
	CastDeclaration *declaration;
	CastFunctionDefinition *functionDefinition;
};

struct CastTranslationUnit
{
	CastExternalDeclaration *externalDeclaration;
	CastTranslationUnit *next;
};

struct Cast
{
	CastTranslationUnit *translationUnit;
};


#ifdef CAST_IMPLEMENTATION

////////////////////////////////////////////////////////////////////////
// CScanner and CParser types

enum CTokenId
{
	// Single character tokens
	TOKEN_LEFT_PAREN,
	TOKEN_RIGHT_PAREN,
	TOKEN_LEFT_BRACE,
	TOKEN_RIGHT_BRACE,
	TOKEN_LEFT_BRACKET,
	TOKEN_RIGHT_BRACKET,
	TOKEN_COMMA,
	TOKEN_DOT,
	TOKEN_MINUS,
	TOKEN_PLUS,
	TOKEN_COLON,
	TOKEN_SEMICOLON,
	TOKEN_TERNARY,
	TOKEN_MODULO,
	TOKEN_SLASH,
	TOKEN_STAR,
	TOKEN_TILDE,
	// One or two-character tokens
	TOKEN_NOT,
	TOKEN_NOT_EQUAL,
	TOKEN_EQUAL,
	TOKEN_EQUAL_EQUAL,
	TOKEN_GREATER,
	TOKEN_GREATER_EQUAL,
	TOKEN_LESS,
	TOKEN_LESS_EQUAL,
	TOKEN_AND,
	TOKEN_ANDAND,
	TOKEN_OR,
	TOKEN_OROR,
	// Literals
	TOKEN_IDENTIFIER,
	TOKEN_STRING,
	TOKEN_CHARACTER,
	TOKEN_NUMBER,
	// Keywords
	TOKEN_STRUCT,
	TOKEN_ENUM,
	TOKEN_TRUE,
	TOKEN_FALSE,
	TOKEN_TYPEDEF,
	TOKEN_EXTERN,
	TOKEN_STATIC,
	TOKEN_REGISTER,
	TOKEN_AUTO,
	TOKEN_THREAD_LOCAL,
	TOKEN_CONST,
	TOKEN_RESTRICT,
	TOKEN_VOLATILE,
	TOKEN_SIGNED,
	TOKEN_UNSIGNED,
	TOKEN_SHORT,
	TOKEN_LONG,
	TOKEN_VOID,
	TOKEN_BOOL,
	TOKEN_CHAR,
	TOKEN_INT,
	TOKEN_FLOAT,
	TOKEN_DOUBLE,
	TOKEN_NULL, // ad-hoc value
	TOKEN_ARRAY_COUNT,
	TOKEN_EOF,
	TOKEN_COUNT,
};

static const char *CTokenIdNames[] =
{
	// Single character tokens
	"TOKEN_LEFT_PAREN",
	"TOKEN_RIGHT_PAREN",
	"TOKEN_LEFT_BRACE",
	"TOKEN_RIGHT_BRACE",
	"TOKEN_LEFT_BRACKET",
	"TOKEN_RIGHT_BRACKET",
	"TOKEN_COMMA",
	"TOKEN_DOT",
	"TOKEN_MINUS",
	"TOKEN_PLUS",
	"TOKEN_COLON",
	"TOKEN_SEMICOLON",
	"TOKEN_TERNARY",
	"TOKEN_MODULO",
	"TOKEN_SLASH",
	"TOKEN_STAR",
	"TOKEN_TILDE",
	// One or two-character tokens
	"TOKEN_NOT",
	"TOKEN_NOT_EQUAL",
	"TOKEN_EQUAL",
	"TOKEN_EQUAL_EQUAL",
	"TOKEN_GREATER",
	"TOKEN_GREATER_EQUAL",
	"TOKEN_LESS",
	"TOKEN_LESS_EQUAL",
	"TOKEN_AND",
	"TOKEN_ANDAND",
	"TOKEN_OR",
	"TOKEN_OROR",
	// Literals
	"TOKEN_IDENTIFIER",
	"TOKEN_STRING",
	"TOKEN_CHARACTER",
	"TOKEN_NUMBER",
	// Keywords
	"TOKEN_STRUCT",
	"TOKEN_ENUM",
	"TOKEN_TRUE",
	"TOKEN_FALSE",
	"TOKEN_TYPEDEF",
	"TOKEN_EXTERN",
	"TOKEN_STATIC",
	"TOKEN_REGISTER",
	"TOKEN_AUTO",
	"TOKEN_THREAD_LOCAL",
	"TOKEN_CONST",
	"TOKEN_RESTRICT",
	"TOKEN_VOLATILE",
	"TOKEN_SIGNED",
	"TOKEN_UNSIGNED",
	"TOKEN_SHORT",
	"TOKEN_LONG",
	"TOKEN_VOID",
	"TOKEN_BOOL",
	"TOKEN_CHAR",
	"TOKEN_INT",
	"TOKEN_FLOAT",
	"TOKEN_DOUBLE",
	"TOKEN_NULL",
	"TOKEN_ARRAY_COUNT",
	"TOKEN_EOF",
};

CAST_CT_ASSERT(CAST_ARRAY_COUNT(CTokenIdNames) == TOKEN_COUNT);

enum ValueType
{
	VALUE_TYPE_BOOL,
	//VALUE_TYPE_INT,
	VALUE_TYPE_FLOAT,
	VALUE_TYPE_STRING,
	VALUE_TYPE_CHAR,
	VALUE_TYPE_NULL,
};

struct CValue
{
	ValueType type;
	union
	{
		bool b;
		char c;
		cast_i32 i;
		cast_f32 f;
		CastString s;
	};
};

typedef CValue CLiteral;

struct CToken
{
	CTokenId id;
	cast_u32 line;
	CastString lexeme;
	CLiteral literal;
};

static CToken gNullToken = {
	.id = TOKEN_COUNT,
	.line = 0,
	.lexeme = { "", 0 },
	.literal = { .type = VALUE_TYPE_NULL, .i = 0 },
};

struct CTokenList
{
	CastArena *arena;
	CToken *tokens;
	cast_i32 count;
	bool valid;
};

struct CScanner
{
	cast_i32 start;
	cast_i32 current;
	cast_i32 currentInLine;
	cast_u32 line;
	bool hasErrors;

	const char *text;
	cast_u32 textSize;
};

struct CIdentifierList
{
	CastString identifier;
	CIdentifierList *next;
};

struct CParser
{
	const CTokenList *tokenList;
	CastArena *arena;
	CastConfig config;
	cast_u32 nextToken;
	cast_u32 lastToken;
	cast_u32 structDepth;
	bool hasErrors;
	bool hasFinished;

	CIdentifierList *identifiers;
};

#define CAST_CHILD1(parent, child) ( parent ? parent->child : 0 )
#define CAST_CHILD2(p, c1, c2) CAST_CHILD1(CAST_CHILD1(p, c1), c2)
#define CAST_CHILD3(p, c1, c2, c3) CAST_CHILD1(CAST_CHILD2(p, c1, c2), c3)
#define CAST_CHILD4(p, c1, c2, c3, c4) CAST_CHILD1(CAST_CHILD3(p, c1, c2, c3), c4)
#define CAST_CHILD(parent, child) CAST_CHILD1(parent, child)

// Trick so CAST_CHILD ends up using the appropriate CAST_CHILDX macro depending on its number of arguments
// NOTE: Removed until fixed. It seems to generate wrong code by the Visual Studio C preprocessor.
//#define CAST_CHILD_SELECT(parent, _1, _2, _3, _4,CAST_CHILDX,...) CAST_CHILDX
//#define CAST_CHILD(parent, ...) CAST_CHILD_SELECT(parent, __VA_ARGS__, CAST_CHILD4, CAST_CHILD3, CAST_CHILD2, CAST_CHILD1)(parent, __VA_ARGS__)



////////////////////////////////////////////////////////////////////////
// CScanner

static bool Char_IsEOL(char character)
{
	return character == '\n';
}

static bool Char_IsAlpha(char character)
{
	return
		( character >= 'a' && character <= 'z' ) ||
		( character >= 'A' && character <= 'Z' ) ||
		( character == '_' );
}

static bool Char_IsDigit(char character)
{
	return character >= '0' && character <= '9';
}

static bool Char_IsAlphaNumeric(char character)
{
	return Char_IsAlpha(character) || Char_IsDigit(character);
}

static bool CScanner_IsAtEnd(const CScanner &scanner)
{
	return scanner.current >= scanner.textSize;
}

static char CScanner_Advance(CScanner &scanner)
{
	CAST_ASSERT(scanner.current < scanner.textSize);
	char currentChar = scanner.text[scanner.current];
	scanner.current++;
	scanner.currentInLine++;
	return currentChar;
}

static bool CScanner_Consume(CScanner &scanner, char expected)
{
	if ( CScanner_IsAtEnd(scanner) ) return false;
	if ( scanner.text[scanner.current] != expected ) return false;
	scanner.current++;
	scanner.currentInLine++;
	return true;
}

static char CScanner_Peek(const CScanner &scanner)
{
	return CScanner_IsAtEnd(scanner) ? '\0' : scanner.text[ scanner.current ];
}

static char CScanner_PeekNext(const CScanner &scanner)
{
	if (scanner.current + 1 >= scanner.textSize) return '\0';
	return scanner.text[ scanner.current + 1 ];
}

static CastString CScanner_ScannedString(const CScanner &scanner)
{
	const char *lexemeStart = scanner.text + scanner.start;
	cast_u32 lexemeSize = scanner.current - scanner.start;
	CastString scannedString = { lexemeStart, lexemeSize };
	return scannedString;
}

static void CScanner_AddToken(const CScanner &scanner, CTokenList &tokenList, CTokenId tokenId, CLiteral literal)
{
	CToken newToken = {};
	newToken.id = tokenId;
	newToken.lexeme = CScanner_ScannedString(scanner);
	newToken.literal = literal;
	newToken.line = scanner.line;

	CastPushStruct(*tokenList.arena, CToken);
	tokenList.tokens[tokenList.count++] = newToken;
}

static void CScanner_AddToken(const CScanner &scanner, CTokenList &tokenList, CTokenId tokenId)
{
	CLiteral literal = {};

	if ( tokenId == TOKEN_STRING )
	{
		literal.type = VALUE_TYPE_STRING;
		literal.s = CScanner_ScannedString(scanner);
		literal.s.str += 1; // start after the first " char
		literal.s.size -= 2; // remove both " characters
	}
	if ( tokenId == TOKEN_CHARACTER )
	{
		literal.type = VALUE_TYPE_CHAR;
		String lexeme = CScanner_ScannedString(scanner);
		literal.c = *(lexeme.str+1); // start after the ' char
	}
	else if ( tokenId == TOKEN_NUMBER )
	{
		literal.type = VALUE_TYPE_FLOAT;
		literal.f = CastStrToFloat( CScanner_ScannedString(scanner) );
	}
	else if ( tokenId == TOKEN_TRUE || tokenId == TOKEN_FALSE )
	{
		literal.type = VALUE_TYPE_BOOL;
		literal.b = tokenId == TOKEN_TRUE;
	}
	else if ( tokenId == TOKEN_NULL )
	{
		literal.type = VALUE_TYPE_NULL;
	}

	CScanner_AddToken(scanner, tokenList, tokenId, literal);
}

static void CScanner_SetError(CScanner &scanner, const char *format, ...)
{
	va_list vaList;
	va_start(vaList, format);
	printf("ERROR: %d:%d: ", scanner.line, scanner.currentInLine);
	vprintf(format, vaList);
	printf("\n");
	va_end(vaList);
	scanner.hasErrors = true;
}

static void CScanner_ScanToken(CScanner &scanner, CTokenList &tokenList)
{
	scanner.start = scanner.current;

	char c = CScanner_Advance(scanner);

	switch (c)
	{
		case '(': CScanner_AddToken(scanner, tokenList, TOKEN_LEFT_PAREN); break;
		case ')': CScanner_AddToken(scanner, tokenList, TOKEN_RIGHT_PAREN); break;
		case '{': CScanner_AddToken(scanner, tokenList, TOKEN_LEFT_BRACE); break;
		case '}': CScanner_AddToken(scanner, tokenList, TOKEN_RIGHT_BRACE); break;
		case '[': CScanner_AddToken(scanner, tokenList, TOKEN_LEFT_BRACKET); break;
		case ']': CScanner_AddToken(scanner, tokenList, TOKEN_RIGHT_BRACKET); break;
		case ',': CScanner_AddToken(scanner, tokenList, TOKEN_COMMA); break;
		case '.': CScanner_AddToken(scanner, tokenList, TOKEN_DOT); break;
		case '-': CScanner_AddToken(scanner, tokenList, TOKEN_MINUS); break;
		case '+': CScanner_AddToken(scanner, tokenList, TOKEN_PLUS); break;
		case '*': CScanner_AddToken(scanner, tokenList, TOKEN_STAR); break;
		case '~': CScanner_AddToken(scanner, tokenList, TOKEN_TILDE); break;
		case ':': CScanner_AddToken(scanner, tokenList, TOKEN_COLON); break;
		case ';': CScanner_AddToken(scanner, tokenList, TOKEN_SEMICOLON); break;
		case '?': CScanner_AddToken(scanner, tokenList, TOKEN_TERNARY); break;
		case '%': CScanner_AddToken(scanner, tokenList, TOKEN_MODULO); break;
		case '!': CScanner_AddToken(scanner, tokenList, CScanner_Consume(scanner, '=') ? TOKEN_NOT_EQUAL : TOKEN_NOT); break;
		case '=': CScanner_AddToken(scanner, tokenList, CScanner_Consume(scanner, '=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL); break;
		case '<': CScanner_AddToken(scanner, tokenList, CScanner_Consume(scanner, '=') ? TOKEN_LESS_EQUAL : TOKEN_LESS); break;
		case '>': CScanner_AddToken(scanner, tokenList, CScanner_Consume(scanner, '=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER); break;
		case '&': CScanner_AddToken(scanner, tokenList, CScanner_Consume(scanner, '&') ? TOKEN_ANDAND : TOKEN_AND); break;
		case '|': CScanner_AddToken(scanner, tokenList, CScanner_Consume(scanner, '|') ? TOKEN_OROR : TOKEN_OR); break;
		case '/':
			if ( CScanner_Consume(scanner, '/') )
			{
				// Discard all chars until the end of line is reached
				while ( !Char_IsEOL( CScanner_Peek(scanner) ) && !CScanner_IsAtEnd(scanner) )
				{
					CScanner_Advance(scanner);
				}
			}
			else if ( CScanner_Consume(scanner, '*') )
			{
				while( !(CScanner_Consume(scanner, '*') && CScanner_Consume(scanner, '/')) || CScanner_IsAtEnd(scanner) )
				{
					if ( Char_IsEOL( CScanner_Peek(scanner) ) ) {
						scanner.line++;
						scanner.currentInLine = 0;
					}
					CScanner_Advance(scanner);
				}
			}
			else
			{
				CScanner_AddToken(scanner, tokenList, TOKEN_SLASH);
			}
			break;

		case '#':
			// Discard all chars until the end of line is reached
			while ( !Char_IsEOL( CScanner_Peek(scanner) ) && !CScanner_IsAtEnd(scanner) ) CScanner_Advance(scanner);
			break;

		// Skip whitespaces
		case ' ':
		case '\r':
		case '\t':
			break;

		// End of line counter
		case '\n':
			scanner.line++;
			scanner.currentInLine = 0;
			break;

		case '\"':
			while ( CScanner_Peek(scanner) != '\"' && !CScanner_IsAtEnd(scanner) )
			{
				if ( Char_IsEOL( CScanner_Peek(scanner) ) ) {
					scanner.line++;
					scanner.currentInLine = 0;
				}
				CScanner_Advance(scanner);
			}

			if ( CScanner_IsAtEnd(scanner) )
			{
				CScanner_SetError( scanner, "Unterminated string." );
				return;
			}

			CScanner_Advance(scanner);

			CScanner_AddToken(scanner, tokenList, TOKEN_STRING);
			break;

		case '\'':
			{
				if ( CScanner_IsAtEnd(scanner) )
				{
					CScanner_SetError( scanner, "Unterminated character" );
					return;
				}
				char character = CScanner_Advance(scanner);
				if ( CScanner_Advance(scanner) != '\'' )
				{
					CScanner_SetError( scanner, "Invalid char literal '%c'", character);
					return;
				}
				CScanner_AddToken(scanner, tokenList, TOKEN_CHARACTER);
			}
			break;

		default:
			if ( Char_IsDigit(c) )
			{
				while ( Char_IsDigit( CScanner_Peek(scanner) ) ) CScanner_Advance(scanner);

				if ( CScanner_Peek(scanner) == '.' && Char_IsDigit( CScanner_PeekNext(scanner) ) )
				{
					CScanner_Advance(scanner);
					while ( Char_IsDigit(CScanner_Peek(scanner)) ) CScanner_Advance(scanner);
					CScanner_Consume(scanner, 'f');
				}

				CScanner_AddToken(scanner, tokenList, TOKEN_NUMBER);
			}
			else if ( Char_IsAlpha(c) )
			{
				while ( Char_IsAlphaNumeric( CScanner_Peek(scanner) ) ) CScanner_Advance(scanner);

				CastString word = CScanner_ScannedString(scanner);

				// Keywords // TODO: This keyword search could be much more efficient
				if      ( CastStrEq( word, "struct" ) )       CScanner_AddToken(scanner, tokenList, TOKEN_STRUCT);
				else if ( CastStrEq( word, "enum" ) )         CScanner_AddToken(scanner, tokenList, TOKEN_ENUM);
				else if ( CastStrEq( word, "true" ) )         CScanner_AddToken(scanner, tokenList, TOKEN_TRUE);
				else if ( CastStrEq( word, "false" ) )        CScanner_AddToken(scanner, tokenList, TOKEN_FALSE);
				else if ( CastStrEq( word, "typedef" ) )      CScanner_AddToken(scanner, tokenList, TOKEN_TYPEDEF);
				else if ( CastStrEq( word, "extern" ) )       CScanner_AddToken(scanner, tokenList, TOKEN_EXTERN);
				else if ( CastStrEq( word, "static" ) )       CScanner_AddToken(scanner, tokenList, TOKEN_STATIC);
				else if ( CastStrEq( word, "register" ) )     CScanner_AddToken(scanner, tokenList, TOKEN_REGISTER);
				else if ( CastStrEq( word, "auto" ) )         CScanner_AddToken(scanner, tokenList, TOKEN_AUTO);
				else if ( CastStrEq( word, "thread_local" ) ) CScanner_AddToken(scanner, tokenList, TOKEN_THREAD_LOCAL);
				else if ( CastStrEq( word, "const" ) )        CScanner_AddToken(scanner, tokenList, TOKEN_CONST);
				else if ( CastStrEq( word, "restrict" ) )     CScanner_AddToken(scanner, tokenList, TOKEN_RESTRICT);
				else if ( CastStrEq( word, "volatile" ) )     CScanner_AddToken(scanner, tokenList, TOKEN_VOLATILE);
				else if ( CastStrEq( word, "signed" ) )       CScanner_AddToken(scanner, tokenList, TOKEN_SIGNED);
				else if ( CastStrEq( word, "unsigned" ) )     CScanner_AddToken(scanner, tokenList, TOKEN_UNSIGNED);
				else if ( CastStrEq( word, "short" ) )        CScanner_AddToken(scanner, tokenList, TOKEN_SHORT);
				else if ( CastStrEq( word, "long" ) )         CScanner_AddToken(scanner, tokenList, TOKEN_LONG);
				else if ( CastStrEq( word, "void" ) )         CScanner_AddToken(scanner, tokenList, TOKEN_VOID);
				else if ( CastStrEq( word, "bool" ) )         CScanner_AddToken(scanner, tokenList, TOKEN_BOOL);
				else if ( CastStrEq( word, "char" ) )         CScanner_AddToken(scanner, tokenList, TOKEN_CHAR);
				else if ( CastStrEq( word, "int" ) )          CScanner_AddToken(scanner, tokenList, TOKEN_INT);
				else if ( CastStrEq( word, "float" ) )        CScanner_AddToken(scanner, tokenList, TOKEN_FLOAT);
				else if ( CastStrEq( word, "double" ) )       CScanner_AddToken(scanner, tokenList, TOKEN_DOUBLE);
				else if ( CastStrEq( word, "NULL" ) )         CScanner_AddToken(scanner, tokenList, TOKEN_NULL);
				else if ( CastStrEq( word, "ARRAY_COUNT" ) )  CScanner_AddToken(scanner, tokenList, TOKEN_ARRAY_COUNT);
				else                                          CScanner_AddToken(scanner, tokenList, TOKEN_IDENTIFIER);
			}
			else
			{
				CScanner_SetError( scanner, "Unexpected character '%c'.", c );
			}
	}
}

static CTokenList CScan(CastArena &arena, const char *text, cast_u32 textSize)
{
	CTokenList tokenList = {};
	tokenList.arena = &arena;
	tokenList.tokens = (CToken*)(arena.base + arena.used);

	CScanner scanner = {};
	scanner.line = 1;
	scanner.hasErrors = false;
	scanner.text = text;
	scanner.textSize = textSize;

	while ( !CScanner_IsAtEnd(scanner) )
	{
		CScanner_ScanToken(scanner, tokenList);
	}

	CScanner_AddToken(scanner, tokenList, TOKEN_EOF);

	tokenList.valid = !scanner.hasErrors;

	return tokenList;
}

#if 0
static void PrintTokenList(const CTokenList &tokenList)
{
	static const char *paddingStr =
		"                                                  "
		"                                                  ";
	printf("List of tokens:\n");
	for (cast_u32 i = 0; i < tokenList.count; ++i)
	{
		const CToken& token = tokenList.tokens[i];
		const int tokenIdLen = strlen(CTokenIdNames[token.id]);
		const int paddingSize = 32 - tokenIdLen;
		printf("%s:%.*s%.*s\n", CTokenIdNames[token.id], paddingSize, paddingStr, token.lexeme.size, token.lexeme.str);
	}
}
#endif


////////////////////////////////////////////////////////////////////////
// CParser

static cast_u32 CParser_RemainingTokens(const CParser &parser)
{
	return parser.tokenList->count - parser.nextToken;
}

static const CToken &CParser_GetPreviousToken( const CParser &parser )
{
	CAST_ASSERT(parser.nextToken > 0);
	return parser.tokenList->tokens[parser.nextToken-1];
}

static const CToken &CParser_GetNextToken( const CParser &parser )
{
	CAST_ASSERT(parser.nextToken < parser.tokenList->count);
	return parser.tokenList->tokens[parser.nextToken];
}

static bool CParser_HasFinished(const CParser &parser)
{
	const bool hasErrors = parser.hasErrors;
	const bool hasFinished = CParser_GetNextToken(parser).id == TOKEN_EOF;
	return hasErrors || hasFinished;
}

static bool CParser_IsNextToken( const CParser &parser, CTokenId tokenId )
{
	if ( CParser_HasFinished( parser ) ) {
		return tokenId == TOKEN_EOF;
	} else {
		CAST_ASSERT(parser.nextToken < parser.tokenList->count);
		return tokenId == parser.tokenList->tokens[parser.nextToken].id;
	}
}

static bool CParser_AreNextTokens( const CParser &parser, CTokenId tokenId0, CTokenId tokenId1 )
{
	if ( CParser_HasFinished( parser ) || CParser_RemainingTokens(parser) < 2 ) {
		return false;
	} else {
		CAST_ASSERT(parser.nextToken+1 < parser.tokenList->count);
		return
			tokenId0 == parser.tokenList->tokens[parser.nextToken].id &&
			tokenId1 == parser.tokenList->tokens[parser.nextToken+1].id;
	}
}

static bool CParser_AreNextTokens( const CParser &parser, CTokenId tokenId0, CTokenId tokenId1, CTokenId tokenId2 )
{
	if ( CParser_HasFinished( parser ) || CParser_RemainingTokens(parser) < 3 ) {
		return false;
	} else {
		CAST_ASSERT(parser.nextToken+2 < parser.tokenList->count);
		return
			tokenId0 == parser.tokenList->tokens[parser.nextToken].id &&
			tokenId1 == parser.tokenList->tokens[parser.nextToken+1].id &&
			tokenId2 == parser.tokenList->tokens[parser.nextToken+2].id;
	}
}

static const CToken &CParser_GetLastToken( const CParser &parser )
{
	return parser.tokenList->tokens[parser.lastToken];
}

static void CParser_SetError(CParser &parser, const char *message)
{
	CToken token = CParser_GetNextToken(parser);
	printf("ERROR: %d:%.*s %s\n", token.line, token.lexeme.size, token.lexeme.str, message);
	parser.hasErrors = true;
}

static const CToken &CParser_Consume( CParser &parser )
{
	if ( !CParser_HasFinished( parser ) ) {
		parser.nextToken++;
		parser.lastToken = parser.nextToken > parser.lastToken ? parser.nextToken : parser.lastToken;
		return CParser_GetPreviousToken(parser);
	} else {
		CParser_SetError(parser, "Reached end of file");
		return gNullToken;
	}
}

static bool CParser_TryConsume( CParser &parser, CTokenId tokenId )
{
	if ( CParser_IsNextToken( parser, tokenId ) ) {
		CParser_Consume( parser );
		return true;
	}
	return false;
}

static bool CParser_TryConsume( CParser &parser, CTokenId tokenId0, CTokenId tokenId1 )
{
	if ( CParser_AreNextTokens( parser, tokenId0, tokenId1 ) ) {
		CParser_Consume( parser );
		CParser_Consume( parser );
		return true;
	}
	return false;
}

static bool CParser_TryConsume( CParser &parser, CTokenId tokenId0, CTokenId tokenId1, CTokenId tokenId2 )
{
	if ( CParser_AreNextTokens( parser, tokenId0, tokenId1, tokenId2 ) ) {
		CParser_Consume( parser );
		CParser_Consume( parser );
		CParser_Consume( parser );
		return true;
	}
	return false;
}

static bool CParser_IsNextTokenNamed( const CParser &parser, const char *name )
{
	if ( !name || *name == '\0' || !CParser_IsNextToken( parser, TOKEN_IDENTIFIER ) ) {
		return false;
	}
	return CastStrEq( CParser_GetNextToken(parser).lexeme, name );
}

static bool CParser_SkipExternalDeclaration( CParser &parser )
{
	cast_u32 skippedTokens = 0;
	cast_u32 braceDepth = 0;
	while ( !CParser_HasFinished(parser) )
	{
		const CToken &token = CParser_Consume(parser);
		skippedTokens++;

		if ( token.id == TOKEN_LEFT_BRACE )
		{
			braceDepth++;
		}
		else if ( token.id == TOKEN_RIGHT_BRACE )
		{
			if ( braceDepth > 0 ) braceDepth--;
			if ( braceDepth == 0 ) {
				CParser_TryConsume(parser, TOKEN_SEMICOLON);
				break;
			}
		}
		else if ( token.id == TOKEN_SEMICOLON && braceDepth == 0 )
		{
			break;
		}
	}
	return skippedTokens > 0;
}

static void CParser_AddIdentifier( CParser &parser, CastString identifier )
{
	CIdentifierList *previousFirst = parser.identifiers;
	parser.identifiers = CastPushZeroStruct( *parser.arena, CIdentifierList );
	parser.identifiers->identifier = identifier;
	parser.identifiers->next = previousFirst;
}

static bool CParser_FindIdentifier( const CParser &parser, CastString identifier)
{
	CIdentifierList *node = parser.identifiers;
	while (node)
	{
		if (CastStrEq(node->identifier, identifier))
		{
			return true;
		}
		node = node->next;
	}
	return false;
}


////////////////////////////////////////////////////////////////////////
// Cast helpers

static char gCastError[4096] = {};

const char *Cast_GetError()
{
	return gCastError;
}

static const void Cast_SetError(const char *format, ...)
{
	va_list vaList;
	va_start(vaList, format);
	vsnprintf(gCastError, CAST_ARRAY_COUNT(gCastError), format, vaList);
	va_end(vaList);
}


////////////////////////////////////////////////////////////////////////
// Cast enum names

static const char *CastStorageClassSpecifierTypeNames[] =
{
	"typedef",
	"extern",
	"static",
	"thread_local",
	"auto",
	"register"
};

CAST_CT_ASSERT(CAST_ARRAY_COUNT(CastStorageClassSpecifierTypeNames) == CAST_REGISTER + 1);

static const char *CastTypeSpecifierTypeNames[]
{
	"void",
	"bool",
	"char",
	"int",
	"float",
	"double",
	"short",
	"long",
	"signed",
	"unsigned",
	"identifier",
	"struct",
	"enum",
};

CAST_CT_ASSERT(CAST_ARRAY_COUNT(CastTypeSpecifierTypeNames) == CAST_ENUM + 1);


////////////////////////////////////////////////////////////////////////
// Cast Function definitions

static bool Cast_IsTypedef( const CastExternalDeclaration *externalDeclaration, CastString &symbol )
{
	const CastDeclaration *declaration = externalDeclaration->declaration;
	if ( declaration ) {
		bool isTypedef = false;
		const CastDeclarationSpecifiers *specifiers = declaration->declarationSpecifiers;
		while (specifiers && !isTypedef)
		{
			const CastStorageClassSpecifier *storageClass = specifiers->storageClassSpecifier;
			isTypedef = storageClass && storageClass->type == CAST_TYPEDEF;
			specifiers = specifiers->next;
		}
		if ( isTypedef )
		{
			const CastInitDeclaratorList *initDeclaratorList = declaration->initDeclaratorList;
			if (initDeclaratorList)
			{
				const CastInitDeclarator *initDeclarator = initDeclaratorList->initDeclarator;
				if (initDeclarator)
				{
					const CastDeclarator *declarator = initDeclarator->declarator;
					if (declarator)
					{
						const CastDirectDeclarator *directDeclarator = declarator->directDeclarator;
						if (directDeclarator)
						{
							symbol = directDeclarator->name;
							return true;
						}
					}
				}
			}
		}
	}
	return false;
}

#define CAST_BACKUP() \
	CastArena backupArena = *parser.arena; \
	cast_u32 nextTokenBackup = parser.nextToken;

#define CAST_RESTORE() \
	*parser.arena = backupArena; \
	parser.nextToken = nextTokenBackup;

#define CAST_NODE( TypeName ) \
	CastPushZeroStruct( *parser.arena, TypeName )

static CastTag *Cast_ParseTag( CParser &parser, const char *tagName )
{
	if ( !tagName || *tagName == '\0' ) {
		return NULL;
	}
	if ( !CParser_IsNextToken( parser, TOKEN_IDENTIFIER ) ) {
		return NULL;
	}
	const CastString name = CParser_GetNextToken(parser).lexeme;
	if ( !CastStrEq( name, tagName ) ) {
		return NULL;
	}

	CAST_BACKUP();
	CParser_Consume(parser);

	CastString arguments = {};
	if ( CParser_TryConsume(parser, TOKEN_LEFT_PAREN) )
	{
		const char *argumentsBegin = CParser_GetNextToken(parser).lexeme.str;
		const char *argumentsEnd = argumentsBegin;
		cast_u32 depth = 1;
		while ( depth > 0 )
		{
			if ( CParser_HasFinished(parser) ) { // Unterminated tag
				CAST_RESTORE();
				return NULL;
			}
			const CToken &token = CParser_Consume(parser);
			if ( token.id == TOKEN_LEFT_PAREN ) depth++;
			else if ( token.id == TOKEN_RIGHT_PAREN ) depth--;
			if ( depth > 0 ) {
				argumentsEnd = token.lexeme.str + token.lexeme.size;
			}
		}
		arguments.str = argumentsBegin;
		arguments.size = (cast_u32)(argumentsEnd - argumentsBegin);
	}

	CastTag *tag = CAST_NODE( CastTag );
	tag->name = name;
	tag->arguments = arguments;
	return tag;
}

static CastStorageClassSpecifier *Cast_ParseStorageClassSpecifier( CParser &parser, CTokenList &tokenList )
{
	bool match = true;
	CastStorageClassSpecifierType type = CAST_TYPEDEF;
	if ( CParser_TryConsume( parser, TOKEN_TYPEDEF ) ) type = CAST_TYPEDEF;
	else if ( CParser_TryConsume( parser, TOKEN_EXTERN ) ) type = CAST_EXTERN;
	else if ( CParser_TryConsume( parser, TOKEN_STATIC ) ) type = CAST_STATIC;
	else if ( CParser_TryConsume( parser, TOKEN_THREAD_LOCAL ) ) type = CAST_THREAD_LOCAL;
	else if ( CParser_TryConsume( parser, TOKEN_AUTO ) ) type = CAST_AUTO;
	else if ( CParser_TryConsume( parser, TOKEN_REGISTER ) ) type = CAST_REGISTER;
	else { match = false; }

	CastStorageClassSpecifier *castStorageSpecifier = NULL;
	if (match) {
		castStorageSpecifier = CAST_NODE( CastStorageClassSpecifier );
		castStorageSpecifier->type = type;
	}
	return castStorageSpecifier;
}

static CastTypeSpecifier *Cast_ParseTypeSpecifier( CParser &parser, CTokenList &tokenList );
static CastTypeQualifier *Cast_ParseTypeQualifier( CParser &parser, CTokenList &tokenList );

static CastSpecifierQualifierList *Cast_ParseSpecifierQualifierList( CParser &parser, CTokenList &tokenList )
{
	CastTypeSpecifier *typeSpecifier = NULL;
	CastTypeQualifier *typeQualifier = NULL;
	CastSpecifierQualifierList *firstSpecifierQualifierList = NULL;
	CastSpecifierQualifierList *prevSpecifierQualifierList = NULL;
	do
	{
		typeSpecifier = Cast_ParseTypeSpecifier(parser, tokenList);

		typeQualifier = Cast_ParseTypeQualifier(parser, tokenList);

		if ( typeSpecifier || typeQualifier )
		{
			CastSpecifierQualifierList *specifierQualifierList = CAST_NODE( CastSpecifierQualifierList );
			specifierQualifierList->typeSpecifier = typeSpecifier;
			specifierQualifierList->typeQualifier = typeQualifier;
			if (!firstSpecifierQualifierList) {
				firstSpecifierQualifierList = specifierQualifierList;
			}
			if (prevSpecifierQualifierList) {
				prevSpecifierQualifierList->next = specifierQualifierList;
			}
			prevSpecifierQualifierList = specifierQualifierList;
		}
	}
	while ( typeSpecifier || typeQualifier );
	return firstSpecifierQualifierList;
}

static CastTypeQualifierList *Cast_ParseTypeQualifierList( CParser &parser, CTokenList &tokenList );

static CastPointer *Cast_ParsePointer( CParser &parser, CTokenList &tokenList )
{
	CastPointer *pointer = NULL;
	if (CParser_TryConsume(parser, TOKEN_STAR))
	{
		CastTypeQualifierList *typeQualifierList = Cast_ParseTypeQualifierList(parser, tokenList);

		pointer = CAST_NODE( CastPointer );
		pointer->typeQualifierList = typeQualifierList;
	}
	return pointer;
}

static CastExpression *Cast_ParseExpression( CParser &parser, CTokenList &tokenList )
{
	CAST_BACKUP();
	// TODO: Expressions can be much more complex than simple constants
	CastExpression *expression = NULL;
	const bool negative = CParser_TryConsume(parser, TOKEN_MINUS);
	if ( CParser_TryConsume(parser, TOKEN_NUMBER) )
	{
		const CToken &token = CParser_GetPreviousToken(parser);
		expression = CAST_NODE( CastExpression );
		expression->type = CAST_EXPR_NUMBER;
		expression->negative = negative;
		expression->constant = token.lexeme;
	}
	else if ( CParser_TryConsume(parser, TOKEN_STRING ) )
	{
		const CToken &token = CParser_GetPreviousToken(parser);
		expression = CAST_NODE( CastExpression );
		expression->type = CAST_EXPR_STRING;
		expression->constant = token.literal.s;
	}
	else if ( CParser_TryConsume(parser, TOKEN_IDENTIFIER) )
	{
		const CToken &token = CParser_GetPreviousToken(parser);
		expression = CAST_NODE( CastExpression );
		expression->type = CAST_EXPR_IDENTIFIER;
		expression->constant = token.lexeme;
	}
	else if ( CParser_TryConsume(parser, TOKEN_ARRAY_COUNT) )
	{
		if ( CParser_TryConsume(parser, TOKEN_LEFT_PAREN) )
		{
			const CToken &tokenArgument = CParser_Consume(parser);
			if ( CParser_TryConsume(parser, TOKEN_RIGHT_PAREN) )
			{
				expression = CAST_NODE( CastExpression );
				expression->type = CAST_EXPR_ARRAY_COUNT;
				expression->constant = tokenArgument.lexeme;
			}
		}
	}
	if (!expression) {
		CAST_RESTORE();
	}
	return expression;
}

static CastDeclarationSpecifiers *Cast_ParseDeclarationSpecifiers( CParser &parser, CTokenList &tokenList );
static CastDeclarator *Cast_ParseDeclarator( CParser &parser, CTokenList &tokenList );

static CastParameterDeclaration *Cast_ParseParameterDeclaration( CParser &parser, CTokenList &tokenList )
{
	CAST_BACKUP();
	CastParameterDeclaration *parameterDeclaration = NULL;
	CastDeclarationSpecifiers *declarationSpecifiers = Cast_ParseDeclarationSpecifiers(parser, tokenList);
	if (declarationSpecifiers)
	{
		// The declarator is optional (e.g. an unnamed parameter, or "void").
		CastDeclarator *declarator = Cast_ParseDeclarator(parser, tokenList);

		parameterDeclaration = CAST_NODE( CastParameterDeclaration );
		parameterDeclaration->declarationSpecifiers = declarationSpecifiers;
		parameterDeclaration->declarator = declarator;
	}
	if (!parameterDeclaration) {
		CAST_RESTORE();
	}
	return parameterDeclaration;
}

static CastParameterList *Cast_ParseParameterList( CParser &parser, CTokenList &tokenList )
{
	bool comma = false;
	CastParameterDeclaration *parameterDeclaration = NULL;
	CastParameterList *firstParameterList = NULL;
	CastParameterList *prevParameterList = NULL;
	do
	{
		parameterDeclaration = Cast_ParseParameterDeclaration(parser, tokenList);
		if (parameterDeclaration)
		{
			CastParameterList *parameterList = CAST_NODE( CastParameterList );
			parameterList->parameterDeclaration = parameterDeclaration;
			if (!firstParameterList) {
				firstParameterList = parameterList;
			}
			if (prevParameterList) {
				prevParameterList->next = parameterList;
			}
			prevParameterList = parameterList;
			comma = CParser_TryConsume(parser, TOKEN_COMMA);
		}
	} while (parameterDeclaration && comma);
	return firstParameterList;
}

static CastParameterTypeList *Cast_ParseParameterTypeList( CParser &parser, CTokenList &tokenList )
{
	CastParameterTypeList *parameterTypeList = NULL;
	CastParameterList *parameterList = Cast_ParseParameterList(parser, tokenList);
	if (parameterList)
	{
		parameterTypeList = CAST_NODE( CastParameterTypeList );
		parameterTypeList->parameterList = parameterList;
	}
	return parameterTypeList;
}

static CastDirectDeclarator *Cast_ParseDirectDeclarator( CParser &parser, CTokenList &tokenList )
{
	CAST_BACKUP();
	if (!CParser_TryConsume(parser, TOKEN_IDENTIFIER)) {
		return NULL;
	}
	const CToken &token = CParser_GetPreviousToken(parser);

	bool isArray = false;
	bool isFunction = false;
	CastExpression *expression = NULL;
	CastParameterTypeList *parameterTypeList = NULL;
	if ( CParser_TryConsume(parser, TOKEN_LEFT_BRACKET) )
	{
		isArray = true;
		expression = Cast_ParseExpression(parser, tokenList);
		if (!CParser_TryConsume(parser, TOKEN_RIGHT_BRACKET)) {
			CAST_RESTORE();
			return NULL;
		}
	}
	else if ( CParser_TryConsume(parser, TOKEN_LEFT_PAREN) )
	{
		isFunction = true;
		parameterTypeList = Cast_ParseParameterTypeList(parser, tokenList);
		if (!CParser_TryConsume(parser, TOKEN_RIGHT_PAREN)) {
			CAST_RESTORE();
			return NULL;
		}
	}

	CastDirectDeclarator *directDeclarator = CAST_NODE( CastDirectDeclarator );
	directDeclarator->name = token.lexeme;
	directDeclarator->expression = expression;
	directDeclarator->parameterTypeList = parameterTypeList;
	directDeclarator->isArray = isArray;
	directDeclarator->isFunction = isFunction;
	return directDeclarator;
}

static CastDeclarator *Cast_ParseDeclarator( CParser &parser, CTokenList &tokenList )
{
	CAST_BACKUP();
	CastDeclarator *declarator =  NULL;
	const bool isReference = CParser_TryConsume(parser, TOKEN_AND);
	CastPointer *pointer = Cast_ParsePointer(parser, tokenList);
	CastDirectDeclarator *directDeclarator = Cast_ParseDirectDeclarator(parser, tokenList);
	if (directDeclarator) {
		declarator = CAST_NODE( CastDeclarator );
		declarator->isReference = isReference;
		declarator->pointer = pointer;
		declarator->directDeclarator = directDeclarator;
	}
	if (!declarator) {
		CAST_RESTORE();
	}
	return declarator;
}

static CastStructDeclaratorList *Cast_ParseStructDeclaratorList( CParser &parser, CTokenList &tokenList )
{
	CastDeclarator *declarator = NULL;
	CastStructDeclaratorList *firstStructDeclaratorList = NULL;
	CastStructDeclaratorList *prevStructDeclaratorList = NULL;
	do
	{
		declarator = Cast_ParseDeclarator(parser, tokenList);
		if (declarator)
		{
			CastStructDeclaratorList *structDeclaratorList = CAST_NODE( CastStructDeclaratorList );
			structDeclaratorList->structDeclarator = declarator;
			if (!firstStructDeclaratorList) {
				firstStructDeclaratorList = structDeclaratorList;
			}
			if (prevStructDeclaratorList) {
				prevStructDeclaratorList->next = structDeclaratorList;
			}
			prevStructDeclaratorList = structDeclaratorList;
		}
	}
	while (declarator);
	
	return firstStructDeclaratorList;
}

static CastStructDeclaration *Cast_ParseStructDeclaration( CParser &parser, CTokenList &tokenList )
{
	CAST_BACKUP();
	CastStructDeclaration *structDeclaration = NULL;
	CastTag *tag = Cast_ParseTag(parser, parser.config.fieldTag);
	CastSpecifierQualifierList *specifierQualifierList = Cast_ParseSpecifierQualifierList(parser, tokenList);
	if ( specifierQualifierList )
	{
		CastStructDeclaratorList *structDeclaratorList = Cast_ParseStructDeclaratorList(parser, tokenList);
		if ( structDeclaratorList ) {
			if (CParser_TryConsume(parser, TOKEN_SEMICOLON)) {
				structDeclaration = CAST_NODE( CastStructDeclaration );
				structDeclaration->tag = tag;
				structDeclaration->specifierQualifierList = specifierQualifierList;
				structDeclaration->structDeclaratorList = structDeclaratorList;
			}
		}
	}
	if ( !structDeclaration ) {
		CAST_RESTORE();
	}
	return structDeclaration;
}

static CastStructDeclarationList *Cast_ParseStructDeclarationList( CParser &parser, CTokenList &tokenList )
{
	CastStructDeclaration *structDeclaration = NULL;
	CastStructDeclarationList *firstDeclarationList = NULL;
	CastStructDeclarationList *previousDeclarationList = NULL;
	do
	{
		structDeclaration = Cast_ParseStructDeclaration(parser, tokenList);
		if ( structDeclaration )
		{
			CastStructDeclarationList *structDeclarationList = CAST_NODE( CastStructDeclarationList );
			structDeclarationList->structDeclaration = structDeclaration;
			if (!firstDeclarationList) {
				firstDeclarationList = structDeclarationList;
			}
			if (previousDeclarationList) {
				previousDeclarationList->next = structDeclarationList;
			}
			previousDeclarationList = structDeclarationList;
		}
	}
	while ( structDeclaration );
	return firstDeclarationList;
}

static CastStructSpecifier *Cast_ParseStructSpecifier( CParser &parser, CTokenList &tokenList )
{
	CAST_BACKUP();
	if ( !CParser_TryConsume(parser, TOKEN_STRUCT) ) {
		CAST_RESTORE();
		return NULL;
	}
	if ( !CParser_TryConsume(parser, TOKEN_IDENTIFIER) ) {
		CAST_RESTORE();
		return NULL;
	}
	const CToken &tokenIdentifier = CParser_GetPreviousToken(parser);
	CastString identifier = tokenIdentifier.lexeme;
	CParser_AddIdentifier(parser, identifier);

	CastStructDeclarationList* structDeclarationList = NULL;
	if ( CParser_TryConsume(parser, TOKEN_LEFT_BRACE) )
	{
		parser.structDepth++;
		structDeclarationList = Cast_ParseStructDeclarationList(parser, tokenList);
		parser.structDepth--;

		if ( !CParser_TryConsume(parser, TOKEN_RIGHT_BRACE) )
		{
			CAST_RESTORE();
			return NULL;
		}
	}
	CastStructSpecifier *structSpecifier = CAST_NODE( CastStructSpecifier );
	structSpecifier->name = identifier;
	structSpecifier->structDeclarationList = structDeclarationList;
	return structSpecifier;
}

static CastEnumerator *Cast_ParseEnumerator( CParser &parser, CTokenList &tokenList )
{
	CastEnumerator *enumerator = NULL;
	if ( CParser_TryConsume(parser, TOKEN_IDENTIFIER) ) {
		const CToken &identifier = CParser_GetPreviousToken(parser);
		enumerator = CAST_NODE( CastEnumerator );
		enumerator->name = identifier.lexeme;

		// Ignore expressions after the assignment token
		if ( CParser_TryConsume(parser, TOKEN_EQUAL) ) {
			while ( !CParser_IsNextToken(parser, TOKEN_COMMA) &&
				!CParser_IsNextToken(parser, TOKEN_RIGHT_BRACE) &&
				!CParser_HasFinished(parser) ) {
				CParser_Consume(parser);
			}
		}
	}
	return enumerator;
}

static CastEnumeratorList *Cast_ParseEnumeratorList( CParser &parser, CTokenList &tokenList )
{
	CastEnumerator *enumerator = NULL;
	CastEnumeratorList *firstEnumeratorList = NULL;
	CastEnumeratorList *prevEnumeratorList = NULL;
	do
	{
		enumerator = Cast_ParseEnumerator(parser, tokenList);
		if (enumerator)
		{
			CParser_TryConsume(parser, TOKEN_COMMA);

			CastEnumeratorList *enumeratorList = CAST_NODE( CastEnumeratorList );
			enumeratorList->enumerator = enumerator;
			if (!firstEnumeratorList) {
				firstEnumeratorList = enumeratorList;
			}
			if (prevEnumeratorList) {
				prevEnumeratorList->next = enumeratorList;
			}
			prevEnumeratorList = enumeratorList;
		}
	} while (enumerator);
	return firstEnumeratorList;
}

static CastEnumSpecifier *Cast_ParseEnumSpecifier( CParser &parser, CTokenList &tokenList )
{
	CAST_BACKUP();
	if ( !CParser_TryConsume(parser, TOKEN_ENUM) ) {
		return NULL;
	}

	CastString name = {};
	if ( CParser_TryConsume(parser, TOKEN_IDENTIFIER) ) {
		const CToken &tokenIdentifier = CParser_GetPreviousToken(parser);
		CastString identifier = tokenIdentifier.lexeme;
		CParser_AddIdentifier(parser, identifier);
		name = identifier;
	}

	// Ignore the underlying type after the colon token (e.g. enum Foo : u8)
	if ( CParser_TryConsume(parser, TOKEN_COLON) ) {
		while ( !CParser_IsNextToken(parser, TOKEN_LEFT_BRACE) &&
			!CParser_IsNextToken(parser, TOKEN_SEMICOLON) &&
			!CParser_HasFinished(parser) ) {
			CParser_Consume(parser);
		}
	}

	CastEnumeratorList *enumeratorList = NULL;
	if ( CParser_TryConsume(parser, TOKEN_LEFT_BRACE) )
	{
		enumeratorList = Cast_ParseEnumeratorList(parser, tokenList);
		CParser_TryConsume(parser, TOKEN_COMMA);
		if (!CParser_TryConsume(parser, TOKEN_RIGHT_BRACE))
		{
			CAST_RESTORE();
			return NULL;
		}
	}

	CastEnumSpecifier *enumSpecifier = CAST_NODE( CastEnumSpecifier );
	enumSpecifier->name = name;
	enumSpecifier->enumeratorList = enumeratorList;
	return enumSpecifier;
}

static CastTypeSpecifier *Cast_ParseTypeSpecifier( CParser &parser, CTokenList &tokenList )
{
	CAST_BACKUP();

	bool match = true;
	CastTypeSpecifierType type = CAST_VOID;
	CastString identifier = {};
	CastStructSpecifier *structSpecifier = NULL;
	CastEnumSpecifier *enumSpecifier = NULL;
	CastTag *structTag = Cast_ParseTag(parser, parser.config.structTag);
	CastTag *enumTag = structTag ? NULL : Cast_ParseTag(parser, parser.config.enumTag);
	if ( structTag )
	{
		// A struct tag was found, so a struct specifier must follow
		type = CAST_STRUCT;
		structSpecifier = Cast_ParseStructSpecifier(parser, tokenList);
		if ( structSpecifier ) {
			structSpecifier->tag = structTag;
		} else {
			CAST_RESTORE();
			match = false;
		}
	}
	else if ( enumTag )
	{
		// An enum tag was found, so an enum specifier must follow
		type = CAST_ENUM;
		enumSpecifier = Cast_ParseEnumSpecifier(parser, tokenList);
		if ( enumSpecifier ) {
			enumSpecifier->tag = enumTag;
		} else {
			CAST_RESTORE();
			match = false;
		}
	}
	else if ( CParser_TryConsume(parser, TOKEN_VOID) ) type = CAST_VOID;
	else if ( CParser_TryConsume(parser, TOKEN_BOOL) ) type = CAST_BOOL;
	else if ( CParser_TryConsume(parser, TOKEN_CHAR) ) type = CAST_CHAR;
	else if ( CParser_TryConsume(parser, TOKEN_INT) ) type = CAST_INT;
	else if ( CParser_TryConsume(parser, TOKEN_FLOAT) ) type = CAST_FLOAT;
	else if ( CParser_TryConsume(parser, TOKEN_DOUBLE) ) type = CAST_DOUBLE;
	else if ( CParser_TryConsume(parser, TOKEN_SHORT) ) type = CAST_SHORT;
	else if ( CParser_TryConsume(parser, TOKEN_LONG) ) type = CAST_LONG;
	else if ( CParser_TryConsume(parser, TOKEN_SIGNED) ) type = CAST_SIGNED;
	else if ( CParser_TryConsume(parser, TOKEN_UNSIGNED) ) type = CAST_UNSIGNED;
	else if ( CParser_TryConsume(parser, TOKEN_IDENTIFIER) )
	{
		const CToken &tokenIdentifier = CParser_GetPreviousToken(parser);
		identifier = tokenIdentifier.lexeme;
		const bool isKnownIdentifier = CParser_FindIdentifier(parser, identifier);
		// Types declared in other files are unknown to the parser. Within a struct
		// body, though, `identifier identifier` and `identifier *identifier` can
		// only be member declarations, so unknown types are accepted there.
		const bool isMemberType = parser.structDepth > 0 &&
			( CParser_IsNextToken(parser, TOKEN_IDENTIFIER) || CParser_IsNextToken(parser, TOKEN_STAR) );
		if ( isKnownIdentifier || isMemberType ) {
			type = CAST_IDENTIFIER;
		} else {
			CAST_RESTORE();
			match = false;
		}
	}
	else
	{
		type = CAST_STRUCT;
		structSpecifier = Cast_ParseStructSpecifier(parser, tokenList);
		if ( !structSpecifier )
		{
			type = CAST_ENUM;
			enumSpecifier = Cast_ParseEnumSpecifier(parser, tokenList);
			if ( !enumSpecifier )
			{
				match = false;
			}
		}
	}

	CastTypeSpecifier *castTypeSpecifier = NULL;
	if (match)
	{
		castTypeSpecifier = CAST_NODE( CastTypeSpecifier );
		castTypeSpecifier->type = type;
		switch (type) {
			case CAST_IDENTIFIER: castTypeSpecifier->identifier = identifier; break;
			case CAST_STRUCT: castTypeSpecifier->structSpecifier = structSpecifier; break;
			case CAST_ENUM: castTypeSpecifier->enumSpecifier = enumSpecifier; break;
			default:;
		};
	}
	return castTypeSpecifier;
}

static CastTypeQualifier *Cast_ParseTypeQualifier( CParser &parser, CTokenList &tokenList )
{
	bool match = true;
	CastTypeQualifierType type = CAST_CONST;
	if ( CParser_TryConsume( parser, TOKEN_CONST) ) type = CAST_CONST;
	else if ( CParser_TryConsume( parser, TOKEN_RESTRICT) ) type = CAST_RESTRICT;
	else if ( CParser_TryConsume( parser, TOKEN_VOLATILE) ) type = CAST_VOLATILE;
	else { match = false; }

	CastTypeQualifier *typeQualifier = NULL;
	if (match) {
		typeQualifier = CAST_NODE( CastTypeQualifier );
		typeQualifier->type = type;
	}
	return typeQualifier;
}

static CastTypeQualifierList *Cast_ParseTypeQualifierList( CParser &parser, CTokenList &tokenList )
{
	CastTypeQualifier *typeQualifier = NULL;
	CastTypeQualifierList *firstTypeQualifierList = NULL;
	CastTypeQualifierList *prevTypeQualifierList = NULL;
	do
	{
		typeQualifier = Cast_ParseTypeQualifier(parser, tokenList);

		if ( typeQualifier )
		{
			CastTypeQualifierList *typeQualifierList = CAST_NODE( CastTypeQualifierList );
			typeQualifierList->typeQualifier = typeQualifier;
			if (!firstTypeQualifierList) {
				firstTypeQualifierList = typeQualifierList;
			}
			if (prevTypeQualifierList) {
				prevTypeQualifierList->next = typeQualifierList;
			}
			prevTypeQualifierList = typeQualifierList;
		}
	}
	while ( typeQualifier );
	return firstTypeQualifierList;
}

static CastFunctionSpecifier *Cast_ParseFunctionSpecifier( CParser &parser, CTokenList &tokenList )
{
	// TODO
	return NULL;
}

static CastDeclarationSpecifiers *Cast_ParseDeclarationSpecifiers( CParser &parser, CTokenList &tokenList )
{
	CastStorageClassSpecifier *storageClassSpecifier = Cast_ParseStorageClassSpecifier(parser, tokenList);
	CastTypeSpecifier *typeSpecifier = Cast_ParseTypeSpecifier(parser, tokenList);
	CastTypeQualifier *typeQualifier = Cast_ParseTypeQualifier(parser, tokenList);
	CastFunctionSpecifier *functionSpecifier = Cast_ParseFunctionSpecifier(parser, tokenList);

	CastDeclarationSpecifiers *declarationSpecifiers =  NULL;
	if (storageClassSpecifier || typeSpecifier || typeQualifier || functionSpecifier) {
		declarationSpecifiers = CAST_NODE( CastDeclarationSpecifiers );
		declarationSpecifiers->storageClassSpecifier = storageClassSpecifier;
		declarationSpecifiers->typeSpecifier = typeSpecifier;
		declarationSpecifiers->typeQualifier = typeQualifier;
		declarationSpecifiers->functionSpecifier = functionSpecifier;
		declarationSpecifiers->next = Cast_ParseDeclarationSpecifiers(parser, tokenList);
	}
	return declarationSpecifiers;
}

static CastDesignator *Cast_ParseDesignator( CParser &parser, CTokenList &tokenList )
{
	CAST_BACKUP();
	CastDesignator *designator = NULL;
	if ( CParser_TryConsume(parser, TOKEN_DOT) ) {
		if ( CParser_TryConsume(parser, TOKEN_IDENTIFIER) ) {
			const CToken &tokenIdentifier = CParser_GetPreviousToken(parser);
			designator = CAST_NODE( CastDesignator );
			designator->identifier = tokenIdentifier.lexeme;
		}
	}
	// TODO: Parse array element designator: [ constant-expression ]
	if (!designator) {
		CAST_RESTORE();
	}
	return designator;
}

static CastDesignatorList *Cast_ParseDesignatorList( CParser &parser, CTokenList &tokenList )
{
	CastDesignatorList *firstDesignatorList = NULL;
	CastDesignatorList *prevDesignatorList = NULL;
	CastDesignator *designator = NULL;
	do
	{
		designator = Cast_ParseDesignator(parser, tokenList);
		if (designator)
		{
			CastDesignatorList *designatorList = CAST_NODE( CastDesignatorList );
			designatorList->designator = designator;
			if (!firstDesignatorList) {
				firstDesignatorList = designatorList;
			}
			if (prevDesignatorList) {
				prevDesignatorList->next = designatorList;
			}
			prevDesignatorList = designatorList;
		}
	} while (designator);
	return firstDesignatorList;
}

static CastDesignation *Cast_ParseDesignation( CParser &parser, CTokenList &tokenList )
{
	CAST_BACKUP();
	CastDesignation *designation = NULL;
	CastDesignatorList *designatorList = Cast_ParseDesignatorList(parser, tokenList);
	if ( designatorList && CParser_TryConsume(parser, TOKEN_EQUAL) )
	{
		designation = CAST_NODE( CastDesignation );
		designation->designatorList = designatorList;
	}
	if (!designation) {
		CAST_RESTORE();
	}
	return designation;
}

static CastInitializerList *Cast_ParseInitializerList ( CParser &parser, CTokenList &tokenList );

static CastInitializer *Cast_ParseInitializer( CParser &parser, CTokenList &tokenList )
{
	CAST_BACKUP();
	CastInitializer *castInitializer = NULL;
	if (CParser_TryConsume(parser, TOKEN_LEFT_BRACE))
	{
		CastInitializerList *initializerList = Cast_ParseInitializerList(parser, tokenList);
		if (initializerList)
		{
			if ( CParser_TryConsume(parser, TOKEN_RIGHT_BRACE) )
			{
				castInitializer = CAST_NODE( CastInitializer );
				castInitializer->initializerList = initializerList;

				// Count number of initializers in the list
				castInitializer->initializerCount = 0;
				CastInitializerList *listIterator = initializerList;
				while (listIterator) {
					castInitializer->initializerCount++;
					listIterator = listIterator->next;
				}
			}
		}
	}
	else
	{
		CastExpression *expression = Cast_ParseExpression(parser, tokenList);
		if (expression)
		{
			castInitializer = CAST_NODE( CastInitializer );
			castInitializer->expression = expression;
		}
	}
	if (!castInitializer) {
		CAST_RESTORE();
	}
	return castInitializer;
}

static CastInitializerList *Cast_ParseInitializerList ( CParser &parser, CTokenList &tokenList )
{
	CAST_BACKUP();
	bool comma = false;
	CastInitializer *initializer = NULL;
	CastInitializerList *firstInitalizerList = NULL;
	CastInitializerList *prevInitalizerList = NULL;
	do
	{
		CastDesignation *designation = Cast_ParseDesignation(parser, tokenList);
		initializer = Cast_ParseInitializer(parser, tokenList);
		if (initializer)
		{
			CastInitializerList *initializerList = CAST_NODE( CastInitializerList );
			initializerList->designation = designation;
			initializerList->initializer = initializer;
			if ( !firstInitalizerList ) {
				firstInitalizerList = initializerList;
			}
			if ( prevInitalizerList ) {
				prevInitalizerList->next = initializerList;
			}
			prevInitalizerList = initializerList;
			comma = CParser_TryConsume(parser, TOKEN_COMMA);
		}
		else if ( designation )
		{
			CAST_RESTORE();
			firstInitalizerList = NULL;
		}
	} while (initializer && comma);
	return firstInitalizerList;
}

static CastInitDeclarator *Cast_ParseInitDeclarator( CParser &parser, CTokenList &tokenList )
{
	CAST_BACKUP();
	CastInitDeclarator *initDeclarator = NULL;
	CastInitializer *initializer = NULL;
	CastDeclarator *declarator = Cast_ParseDeclarator(parser, tokenList);
	if ( declarator )
	{
		if ( CParser_TryConsume(parser, TOKEN_EQUAL) )
		{
			initializer = Cast_ParseInitializer(parser, tokenList);
			if (!initializer) {
				CAST_RESTORE();
				return NULL;
			}
		}

		initDeclarator = CAST_NODE( CastInitDeclarator );
		initDeclarator->declarator = declarator;
		initDeclarator->initializer = initializer;
	}
	return initDeclarator;
}

static CastInitDeclaratorList *Cast_ParseInitDeclaratorList( CParser &parser, CTokenList &tokenList )
{
	CAST_BACKUP();
	bool comma = false;
	CastInitDeclarator *initDeclarator = NULL;
	CastInitDeclaratorList *firstInitDeclaratorList = NULL;
	CastInitDeclaratorList *prevInitDeclaratorList = NULL;
	do
	{
		initDeclarator = Cast_ParseInitDeclarator(parser, tokenList);
		if (initDeclarator)
		{
			CastInitDeclaratorList *initDeclaratorList = CAST_NODE( CastInitDeclaratorList );
			initDeclaratorList->initDeclarator = initDeclarator;
			if ( !firstInitDeclaratorList ) {
				firstInitDeclaratorList = initDeclaratorList;
			}
			if ( prevInitDeclaratorList ) {
				prevInitDeclaratorList->next = initDeclaratorList;
			}
			prevInitDeclaratorList = initDeclaratorList;
			comma = CParser_TryConsume(parser, TOKEN_COMMA);
		}
		else if ( comma )
		{
			CAST_RESTORE();
			firstInitDeclaratorList = NULL;
		}
	} while ( initDeclarator );
	return firstInitDeclaratorList;
}

static CastDeclaration *Cast_ParseDeclaration( CParser &parser, CTokenList &tokenList )
{
	CAST_BACKUP();

	CastDeclarationSpecifiers *declarationSpecifiers = Cast_ParseDeclarationSpecifiers(parser, tokenList);
	if (!declarationSpecifiers)
	{
		return NULL;
	}

	CastInitDeclaratorList *initDeclaratorList = Cast_ParseInitDeclaratorList(parser, tokenList);
	// initDeclaratorList may be null (e.g. typedef statements only need declarationSpecifiers).

	CastDeclaration *declaration = NULL;
	if ( CParser_TryConsume(parser, TOKEN_SEMICOLON) )
	{
		declaration = CAST_NODE( CastDeclaration );
		declaration->declarationSpecifiers = declarationSpecifiers;
		declaration->initDeclaratorList = initDeclaratorList;
	}

	if (!declaration) {
		CAST_RESTORE();
	}
	return declaration;
}

static CastFunctionDefinition *Cast_ParseFunctionDefinition( CParser &parser, CTokenList &tokenList )
{
	CAST_BACKUP();
	CastFunctionDefinition *functionDefinition = NULL;
	
	CastDeclarationSpecifiers *declarationSpecifiers = Cast_ParseDeclarationSpecifiers(parser, tokenList);
	if (declarationSpecifiers)
	{
		CastDeclarator *declarator = Cast_ParseDeclarator(parser, tokenList);
		if ( declarator && declarator->directDeclarator && declarator->directDeclarator->isFunction && CParser_TryConsume(parser, TOKEN_LEFT_BRACE) )
		{
			// Function bodies (compound statements) aren't represented in the AST yet;
			// skip over the tokens up to (and including) the matching closing brace.
			cast_u32 braceDepth = 1;
			while ( braceDepth > 0 && !CParser_HasFinished(parser) )
			{
				const CToken &token = CParser_Consume(parser);
				if ( token.id == TOKEN_LEFT_BRACE ) braceDepth++;
				else if ( token.id == TOKEN_RIGHT_BRACE ) braceDepth--;
			}

			if ( braceDepth == 0 )
			{
				functionDefinition = CAST_NODE( CastFunctionDefinition );
				functionDefinition->declarationSpecifiers = declarationSpecifiers;
				functionDefinition->declarator = declarator;
			}
		}
	}

	if (!functionDefinition) {
		CAST_RESTORE();
	}
	return functionDefinition;
}

static CastExternalDeclaration *Cast_ParseExternalDeclaration( CParser &parser, CTokenList &tokenList )
{
	CastExternalDeclaration *externalDeclaration = NULL;
	CastDeclaration *declaration = Cast_ParseDeclaration(parser, tokenList);
	if (declaration)
	{
		externalDeclaration = CAST_NODE( CastExternalDeclaration );
		externalDeclaration->declaration = declaration;
	}
	else
	{
		CastFunctionDefinition *functionDefinition = Cast_ParseFunctionDefinition(parser, tokenList);
		if (functionDefinition)
		{
			externalDeclaration = CAST_NODE( CastExternalDeclaration );
			externalDeclaration->functionDefinition = functionDefinition;
		}
	}
	return externalDeclaration;
}

static CastTranslationUnit *Cast_ParseTranslationUnit( CParser &parser, CTokenList &tokenList )
{
	CastTranslationUnit *firstTranslationUnit = NULL;
	CastTranslationUnit *previousTranslationUnit = NULL;
	while ( !CParser_HasFinished(parser) )
	{
		CastExternalDeclaration *externalDeclaration = Cast_ParseExternalDeclaration(parser, tokenList);
		if ( externalDeclaration )
		{
			CastTranslationUnit *translationUnit = CAST_NODE( CastTranslationUnit );
			translationUnit->externalDeclaration = externalDeclaration;
			if (!firstTranslationUnit) {
				firstTranslationUnit = translationUnit;
			}
			if (previousTranslationUnit) {
				previousTranslationUnit->next = translationUnit;
			}
			previousTranslationUnit = translationUnit;

			CastString symbol;
			if ( Cast_IsTypedef( externalDeclaration, symbol ) ) {
				//printf("%.*s\n", symbol.size, symbol.str);
				CParser_AddIdentifier(parser, symbol);
			}
		}
		else
		{
			CToken token = CParser_GetNextToken(parser);

			// Tagged definitions are explicitly requested by the user, so failing
			// to parse one is an error rather than something to silently skip.
			const bool isTagged =
				CParser_IsNextTokenNamed(parser, parser.config.structTag) ||
				CParser_IsNextTokenNamed(parser, parser.config.enumTag);

			if ( isTagged || !CParser_SkipExternalDeclaration(parser) )
			{
				CToken lastToken = CParser_GetLastToken(parser);
				Cast_SetError("Invalid grammar between lines: %u-%u\n", token.line, lastToken.line);
				firstTranslationUnit = NULL;
				break;
			}
		}
	}
	if ( !firstTranslationUnit && !parser.hasErrors && !*gCastError )
	{
		Cast_SetError("No declarations could be parsed\n");
	}
	return firstTranslationUnit;
}


static Cast *Cast_Create( CParser &parser, CTokenList &tokenList )
{
	Cast *cast = NULL;
	CastTranslationUnit *translationUnit = Cast_ParseTranslationUnit(parser, tokenList);
	if ( translationUnit ) {
		cast = CAST_NODE( Cast );
		cast->translationUnit = translationUnit;
	}
	return cast;
}

const Cast *Cast_Create( CastArena &arena, const char *text, cast_u64 textSize, const CastConfig &config )
{
	gCastError[0] = '\0';

	CTokenList tokenList = CScan(arena, text, textSize);

	if ( tokenList.valid )
	{
		CParser parser = {};
		parser.tokenList = &tokenList;
		parser.arena = &arena;
		parser.config = config;
		Cast *cast = Cast_Create(parser, tokenList);
		return cast;
	}

	return NULL;
}

const Cast *Cast_Create( CastArena &arena, const char *text, cast_u64 textSize)
{
	const CastConfig config = {}; // No tags are parsed by default
	return Cast_Create(arena, text, textSize, config);
}


////////////////////////////////////////////////////////////////////////
// Evaluate utils

#define CAST_DEFINE_EVALUATE_FUNCTION(Type, Name, StrToType) \
	Type Cast_Evaluate##Name( const CastExpression *ast ) \
	{ \
		CAST_ASSERT( ast ); \
		Type res = (Type)0; \
		if ( ast->type == CAST_EXPR_NUMBER ) { \
			res = StrToType(ast->constant); \
		} \
		res = CAST_EVALUATE_SIGN(ast, res); \
		return res; \
	}

#undef CAST_EVALUATE_SIGN
#define CAST_EVALUATE_SIGN( ast, value ) res

CAST_DEFINE_EVALUATE_FUNCTION(bool, Bool, CastStrToBool);

#undef CAST_EVALUATE_SIGN
#define CAST_EVALUATE_SIGN( ast, value ) ast->negative ? -res : res

CAST_DEFINE_EVALUATE_FUNCTION(char, Char, CastStrToChar);
CAST_DEFINE_EVALUATE_FUNCTION(unsigned char, UnsignedChar, CastStrToUnsignedInt);
CAST_DEFINE_EVALUATE_FUNCTION(int, Int, CastStrToInt);
CAST_DEFINE_EVALUATE_FUNCTION(short int, ShortInt, CastStrToInt); // TODO
CAST_DEFINE_EVALUATE_FUNCTION(long int, LongInt, CastStrToInt);
CAST_DEFINE_EVALUATE_FUNCTION(long long int, LongLongInt, CastStrToInt);
CAST_DEFINE_EVALUATE_FUNCTION(unsigned int, UnsignedInt,CastStrToUnsignedInt );
CAST_DEFINE_EVALUATE_FUNCTION(short unsigned int, UnsignedShortInt, CastStrToUnsignedInt);
CAST_DEFINE_EVALUATE_FUNCTION(long unsigned int, UnsignedLongInt, CastStrToUnsignedInt);
CAST_DEFINE_EVALUATE_FUNCTION(long long unsigned int, UnsignedLongLongInt, CastStrToUnsignedInt);
CAST_DEFINE_EVALUATE_FUNCTION(float, Float, CastStrToFloat);
CAST_DEFINE_EVALUATE_FUNCTION(double, Double, CastStrToFloat);

#undef CAST_EVALUATE_SIGN


////////////////////////////////////////////////////////////////////////
// Print utils

#define CastPrintfN( format, ... ) CAST_LOG(Info, "%s" format, Indentation(), ##__VA_ARGS__ ); CastPrintNewLine();
#define CastPrintBeginScope( format, ... ) CAST_LOG(Info, "%s" format, Indentation(), ##__VA_ARGS__ ); IndentationIncrease(); CastPrintNewLine();
#define CastPrintEndScope() IndentationDecrease();
#define CastPrintNewLine() CAST_LOG(Info, "\n"); indent.apply = true;

struct IndentationState
{
	bool apply;
	cast_u32 pos;
	char padding[64];
};

static IndentationState indent = {
	.apply = true,
	.pos = 0,
	.padding = "",
};

static void IndentationIncrease()
{
	indent.padding[indent.pos++] = ' ';
}

static void IndentationDecrease()
{
	CAST_ASSERT(indent.pos > 0);
	indent.padding[--indent.pos] = '\0';
}

static const char *Indentation()
{
	const char *padding = indent.apply ? indent.padding : "";
	indent.apply = false;
	return padding;
}


#ifdef CAST_PRINT

////////////////////////////////////////////////////////////////////////
// Print functions

static void Cast_Print( const CastTag *ast )
{
	CastPrintfN("Tag -> %.*s(%.*s)", ast->name.size, ast->name.str, ast->arguments.size, ast->arguments.str);
}

static void Cast_Print( const CastExpression *ast )
{
	// TODO: Expressions can be much more complex
	if (ast->type == CAST_EXPR_ARRAY_COUNT) {
		CastPrintfN("Expression -> ARRAY_COUNT(%.*s)", ast->constant.size, ast->constant.str);
	} else {
		CastPrintfN("Expression -> %.*s", ast->constant.size, ast->constant.str);
	}
}

static void Cast_Print( const CastDesignator *ast )
{
	CastPrintfN("Designator -> .%.*s", ast->identifier.size, ast->identifier.str);
}

static void Cast_Print( const CastDesignatorList *ast )
{
	CastPrintBeginScope("DesignatorList");
	while (ast)
	{
		if (ast->designator) {
			Cast_Print(ast->designator);
		}
		ast = ast->next;
	}
	CastPrintEndScope();
}

static void Cast_Print( const CastDesignation *ast )
{
	CastPrintBeginScope("Designation");
	if (ast->designatorList) {
		Cast_Print(ast->designatorList);
	}
	CastPrintEndScope();
}

static void Cast_Print( const CastInitializer *ast );

static void Cast_Print( const CastInitializerList *ast )
{
	CastPrintBeginScope("InitializerList");
	while (ast)
	{
		if (ast->designation) {
			Cast_Print(ast->designation);
		}
		if (ast->initializer) {
			Cast_Print(ast->initializer);
		}
		ast = ast->next;
	}
	CastPrintEndScope();
}

static void Cast_Print( const CastStorageClassSpecifier *ast )
{
	CAST_ASSERT(ast->type < CAST_ARRAY_COUNT(CastStorageClassSpecifierTypeNames));
	CastPrintBeginScope("StorageClassSpecifier -> %s", CastStorageClassSpecifierTypeNames[ast->type]);
	CastPrintEndScope();
}

static void Cast_Print( const CastTypeSpecifier *ast );
static void Cast_Print( const CastTypeQualifier *ast );

static void Cast_Print( const CastSpecifierQualifierList *ast )
{
	CastPrintBeginScope("SpecifierQualifierList");
	while (ast)
	{
		if (ast->typeSpecifier) {
			Cast_Print(ast->typeSpecifier);
		}
		if (ast->typeQualifier) {
			Cast_Print(ast->typeQualifier);
		}
		ast = ast->next;
	}
	CastPrintEndScope();
}

static void Cast_Print( const CastDeclarator *ast );

static void Cast_Print( const CastStructDeclaratorList *ast )
{
	CastPrintBeginScope("StructDeclaratorList");
	while (ast)
	{
		if (ast->structDeclarator) {
			Cast_Print(ast->structDeclarator);
		}
		ast = ast->next;
	}
	CastPrintEndScope();
}

static void Cast_Print( const CastStructDeclaration *ast )
{
	CastPrintBeginScope("StructDeclaration");
	if (ast->tag) {
		Cast_Print(ast->tag);
	}
	if (ast->specifierQualifierList) {
		Cast_Print(ast->specifierQualifierList);
	}
	if (ast->structDeclaratorList) {
		Cast_Print(ast->structDeclaratorList);
	}
	CastPrintEndScope();
}

static void Cast_Print( const CastStructDeclarationList *ast )
{
	CastPrintBeginScope("StructDeclarationList");
	while (ast)
	{
		if (ast->structDeclaration) {
			Cast_Print(ast->structDeclaration);
		}
		ast = ast->next;
	}
	CastPrintEndScope();
}

static void Cast_Print( const CastStructSpecifier *ast )
{
	CastPrintBeginScope("StructSpecifier -> %.*s", ast->name.size, ast->name.str);
	if (ast->tag) {
		Cast_Print(ast->tag);
	}
	Cast_Print(ast->structDeclarationList);
	CastPrintEndScope();
}

static void Cast_Print( const CastEnumerator *ast )
{
	CastPrintBeginScope("Enumerator -> %.*s", ast->name.size, ast->name.str);
	// TODO: Could have a constant expression
	CastPrintEndScope();
}

static void Cast_Print( const CastEnumeratorList *ast )
{
	CastPrintBeginScope("EnumeratorList");
	while (ast)
	{
		if (ast->enumerator) {
			Cast_Print(ast->enumerator);
		}
		ast = ast->next;
	}
	CastPrintEndScope();
}

static void Cast_Print( const CastEnumSpecifier *ast )
{
	CastPrintBeginScope("EnumSpecifier -> %.*s", ast->name.size, ast->name.str);
	if (ast->tag) {
		Cast_Print(ast->tag);
	}
	if (ast->enumeratorList)
	{
		Cast_Print(ast->enumeratorList);
	}
	CastPrintEndScope();
}

static void Cast_Print( const CastTypeSpecifier *ast )
{
	if (ast->type < CAST_IDENTIFIER)
	{
		CastPrintfN("TypeSpecifier -> %s", CastTypeSpecifierTypeNames[ast->type]);
	}
	else if (ast->type == CAST_IDENTIFIER)
	{
		CastPrintfN("TypeSpecifier -> %.*s", ast->identifier.size, ast->identifier.str);
	}
	else
	{
		CastPrintBeginScope("TypeSpecifier");
		CAST_ASSERT(ast->type <= CAST_ENUM);
		switch (ast->type)
		{
			case CAST_STRUCT: Cast_Print(ast->structSpecifier); break;
			case CAST_ENUM: Cast_Print(ast->enumSpecifier); break;
		}
		CastPrintEndScope();
	}
}

static void Cast_Print( const CastTypeQualifier *ast )
{
	const char *typeQualifierNames[] =
	{ "const", "restrict", "volatile" };
	CAST_ASSERT(ast->type < CAST_ARRAY_COUNT(typeQualifierNames));
	CastPrintBeginScope("TypeQualifier -> %s", typeQualifierNames[ast->type]);
	CastPrintEndScope();
}

static void Cast_Print( const CastFunctionSpecifier *ast )
{
	CastPrintBeginScope("FunctionSpecifier");
	// TODO
	CastPrintEndScope();
}

static void Cast_Print( const CastDeclarationSpecifiers *ast );

static void Cast_Print( const CastFunctionDefinition *ast )
{
	CastPrintBeginScope("FunctionDefinition");
	if (ast->declarationSpecifiers) {
		Cast_Print(ast->declarationSpecifiers);
	}
	if (ast->declarator) {
		Cast_Print(ast->declarator);
	}
	CastPrintEndScope();
}

static void Cast_Print( const CastDeclarationSpecifiers *ast )
{
	CastPrintBeginScope("DeclarationSpecifiers");
	while (ast)
	{
		if (ast->storageClassSpecifier) {
			Cast_Print(ast->storageClassSpecifier);
		}
		if (ast->typeSpecifier) {
			Cast_Print(ast->typeSpecifier);
		}
		if (ast->typeQualifier) {
			Cast_Print(ast->typeQualifier);
		}
		if (ast->functionSpecifier) {
			Cast_Print(ast->functionSpecifier);
		}
		ast = ast->next;
	}
	CastPrintEndScope();
}

static void Cast_Print( const CastTypeQualifierList *ast )
{
	CastPrintBeginScope("TypeQualifierList");
	while (ast)
	{
		if (ast->typeQualifier) {
			Cast_Print(ast->typeQualifier);
		}
		ast = ast->next;
	}
	CastPrintEndScope();
}

static void Cast_Print( const CastPointer *ast )
{
	CastPrintBeginScope("Pointer");
	if (ast->typeQualifierList) {
		Cast_Print(ast->typeQualifierList);
	}
	if (ast->next) {
		Cast_Print(ast->next);
	}
	CastPrintEndScope();
}

static void Cast_Print( const CastParameterDeclaration *ast )
{
	CastPrintBeginScope("ParameterDeclaration");
	if (ast->declarationSpecifiers) {
		Cast_Print(ast->declarationSpecifiers);
	}
	if (ast->declarator) {
		Cast_Print(ast->declarator);
	}
	CastPrintEndScope();
}

static void Cast_Print( const CastParameterList *ast )
{
	CastPrintBeginScope("ParameterList");
	while (ast) {
		Cast_Print(ast->parameterDeclaration);
		ast = ast->next;
	}
	CastPrintEndScope();
}

static void Cast_Print( const CastParameterTypeList *ast )
{
	CastPrintBeginScope("ParameterTypeList");
	if (ast->parameterList) {
		Cast_Print(ast->parameterList);
	}
	CastPrintEndScope();
}

static void Cast_Print( const CastDirectDeclarator *ast )
{
	CastPrintBeginScope("DirectDeclarator -> %.*s%s",ast->name.size, ast->name.str,ast->isArray?"[]":(ast->isFunction?"()":""));
	if (ast->expression) {
		Cast_Print(ast->expression);
	}
	if (ast->parameterTypeList) {
		Cast_Print(ast->parameterTypeList);
	}
	CastPrintEndScope();
}

static void Cast_Print( const CastDeclarator *ast )
{
	CastPrintBeginScope("Declarator");
	if (ast->pointer) {
		Cast_Print(ast->pointer);
	}
	if (ast->directDeclarator) {
		Cast_Print(ast->directDeclarator);
	}
	CastPrintEndScope();
}

static void Cast_Print( const CastInitializer *ast )
{
	CastPrintBeginScope("Initializer");
	if (ast->expression) {
		Cast_Print(ast->expression);
	}
	if (ast->initializerList) {
		Cast_Print(ast->initializerList);
	}
	CastPrintEndScope();
}

static void Cast_Print( const CastInitDeclarator *ast )
{
	CastPrintBeginScope("InitDeclarator");
	if (ast->declarator) {
		Cast_Print(ast->declarator);
	}
	if (ast->initializer) {
		Cast_Print(ast->initializer);
	}
	CastPrintEndScope();
}

static void Cast_Print( const CastInitDeclaratorList *ast )
{
	CastPrintBeginScope("InitDeclaratorList");
	while (ast) {
		Cast_Print(ast->initDeclarator);
		ast = ast->next;
	}
	CastPrintEndScope();
}

static void Cast_Print( const CastDeclaration *ast )
{
	CastPrintBeginScope("Declaration");
	if (ast->declarationSpecifiers) {
		Cast_Print(ast->declarationSpecifiers);
	}
	if (ast->initDeclaratorList) {
		Cast_Print(ast->initDeclaratorList);
	}
	CastPrintEndScope();
}

static void Cast_Print( const CastExternalDeclaration *ast )
{
	CastPrintBeginScope("ExternalDeclaration");
	if (ast->declaration) {
		Cast_Print(ast->declaration);
	} else if (ast->functionDefinition) {
		Cast_Print(ast->functionDefinition);
	}
	CastPrintEndScope();
}

static void Cast_Print( const CastTranslationUnit *ast )
{
	CastPrintBeginScope("TranslationUnit");
	while (ast) {
		Cast_Print(ast->externalDeclaration);
		ast = ast->next;
	}
	CastPrintEndScope();
}

void Cast_Print( const Cast *cast )
{
	CastPrintBeginScope("Cast");
	Cast_Print(cast->translationUnit);
	CastPrintEndScope();
}

#endif // #ifdef CAST_PRINT

#endif // #ifdef CAST_IMPLEMENTATION

#endif // #ifndef CAST_H

