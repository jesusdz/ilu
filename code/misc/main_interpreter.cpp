#include "../ilu_core.h"



////////////////////////////////////////////////////////////////////////////////////////////////////
// Scanner


#define MAX_LINE_SIZE KB(1)
#define MAX_TOKEN_COUNT KB(10)


#define TOKEN_LIST(X) \
	/* Single character tokens */ \
	X(TOKEN_LEFT_PAREN) \
	X(TOKEN_RIGHT_PAREN) \
	X(TOKEN_LEFT_BRACE) \
	X(TOKEN_RIGHT_BRACE) \
	X(TOKEN_COMMA) \
	X(TOKEN_DOT) \
	X(TOKEN_MINUS) \
	X(TOKEN_PLUS) \
	X(TOKEN_SEMICOLON) \
	X(TOKEN_SLASH) \
	X(TOKEN_STAR) \
	/* One or two-character tokens */ \
	X(TOKEN_NOT) \
	X(TOKEN_NOT_EQUAL) \
	X(TOKEN_EQUAL) \
	X(TOKEN_EQUAL_EQUAL) \
	X(TOKEN_GREATER) \
	X(TOKEN_GREATER_EQUAL) \
	X(TOKEN_LESS) \
	X(TOKEN_LESS_EQUAL) \
	X(TOKEN_AND) \
	X(TOKEN_ANDAND) \
	X(TOKEN_OR) \
	X(TOKEN_OROR) \
	/* Literals */ \
	X(TOKEN_IDENTIFIER) \
	X(TOKEN_STRING) \
	X(TOKEN_NUMBER) \
	/* Keywords */ \
	X(TOKEN_IF) \
	X(TOKEN_ELSE) \
	X(TOKEN_FOR) \
	X(TOKEN_WHILE) \
	X(TOKEN_CLASS) \
	X(TOKEN_SUPER) \
	X(TOKEN_THIS) \
	X(TOKEN_FUN) \
	X(TOKEN_RETURN) \
	X(TOKEN_TRUE) \
	X(TOKEN_FALSE) \
	X(TOKEN_NIL) \
	X(TOKEN_VAR) \
	X(TOKEN_PRINT) \
	X(TOKEN_EOF)

// Enum values
#define TOKEN_ENUM_VALUE(tokenName) tokenName,
enum TokenId {
	TOKEN_LIST(TOKEN_ENUM_VALUE)
};
#undef TOKEN_ENUM_VALUE

// Enum names
#define TOKEN_ENUM_NAME(tokenName) #tokenName,
const char *TokenNames[] = {
	TOKEN_LIST(TOKEN_ENUM_NAME)
};
#undef TOKEN_ENUM_NAME



enum ValueType
{
	VALUE_TYPE_BOOL,
	//VALUE_TYPE_INT,
	VALUE_TYPE_FLOAT,
	VALUE_TYPE_STRING,
	VALUE_TYPE_NIL,
};

struct Value
{
	ValueType type;
	union
	{
		bool b;
		i32 i;
		f32 f;
		String s;
	};
};

typedef Value Literal;

struct Token
{
	TokenId type;
	String lexeme;
	Literal literal;
	i32 line;
};

struct TokenList
{
	Token *tokens;
	i32 count;
};

struct ScanState
{
	i32 start;
	i32 current;
	i32 line;
	bool hasErrors;

	const char *script;
	u32 scriptSize;
};

const char* TokenName(TokenId type)
{
	return TokenNames[type];
}

bool IsEOL(char character)
{
	return character == '\n';
}

bool IsAlpha(char character)
{
	return
		( character >= 'a' && character <= 'z' ) ||
		( character >= 'A' && character <= 'Z' ) ||
		( character == '_' );
}

bool IsDigit(char character)
{
	return character >= '0' && character <= '9';
}

bool IsAlphaNumeric(char character)
{
	return IsAlpha(character) || IsDigit(character);
}

bool IsAtEnd(const ScanState &scanState)
{
	return scanState.current >= scanState.scriptSize;
}

char Advance(ScanState &scanState)
{
	ASSERT(scanState.current < scanState.scriptSize);
	return scanState.script[scanState.current++];
}

bool Consume(ScanState &scanState, char expected)
{
	if ( IsAtEnd(scanState) ) return false;
	if ( scanState.script[scanState.current] != expected ) return false;
	scanState.current++;
	return true;
}

char Peek(const ScanState &scanState)
{
	return IsAtEnd(scanState) ? '\0' : scanState.script[ scanState.current ];
}

char PeekNext(const ScanState &scanState)
{
	if (scanState.current + 1 >= scanState.scriptSize) return '\0';
	return scanState.script[ scanState.current + 1 ];
}

String ScannedString(const ScanState &scanState)
{
	const char *lexemeStart = scanState.script + scanState.start;
	u32 lexemeSize = scanState.current - scanState.start;
	String scannedString = { lexemeStart, lexemeSize };
	return scannedString;
}

void AddToken(const ScanState &scanState, TokenList &tokenList, TokenId tokenId, Literal literal)
{
	Token newToken = {};
	newToken.type = tokenId;
	newToken.lexeme = ScannedString(scanState);
	newToken.literal = literal;
	newToken.line = scanState.line;

	ASSERT( tokenList.count < MAX_TOKEN_COUNT );
	tokenList.tokens[tokenList.count++] = newToken;
}

void AddToken(const ScanState &scanState, TokenList &tokenList, TokenId tokenId)
{
	Literal literal = {};

	if ( tokenId == TOKEN_STRING )
	{
		literal.type = VALUE_TYPE_STRING;
		literal.s = ScannedString(scanState);
	}
	else if ( tokenId == TOKEN_NUMBER )
	{
		literal.type = VALUE_TYPE_FLOAT;
		literal.f = StrToFloat( ScannedString(scanState) );
	}
	else if ( tokenId == TOKEN_TRUE || tokenId == TOKEN_FALSE )
	{
		literal.type = VALUE_TYPE_BOOL;
		literal.b = tokenId == TOKEN_TRUE;
	}
	else if ( tokenId == TOKEN_NIL )
	{
		literal.type = VALUE_TYPE_NIL;
	}

	AddToken(scanState, tokenList, tokenId, literal);
}

void ReportError(ScanState &scanState, const char *message)
{
	printf("ERROR: %d:%d: %s\n", scanState.line, scanState.current, message);
	scanState.hasErrors = true;
}

void ScanToken(ScanState &scanState, TokenList &tokenList)
{
	scanState.start = scanState.current;

	char c = Advance(scanState);

	switch (c)
	{
		case '(': AddToken(scanState, tokenList, TOKEN_LEFT_PAREN); break;
		case ')': AddToken(scanState, tokenList, TOKEN_RIGHT_PAREN); break;
		case '{': AddToken(scanState, tokenList, TOKEN_LEFT_BRACE); break;
		case '}': AddToken(scanState, tokenList, TOKEN_RIGHT_BRACE); break;
		case ',': AddToken(scanState, tokenList, TOKEN_COMMA); break;
		case '.': AddToken(scanState, tokenList, TOKEN_DOT); break;
		case '-': AddToken(scanState, tokenList, TOKEN_MINUS); break;
		case '+': AddToken(scanState, tokenList, TOKEN_PLUS); break;
		case '*': AddToken(scanState, tokenList, TOKEN_STAR); break;
		case ';': AddToken(scanState, tokenList, TOKEN_SEMICOLON); break;
		case '!': AddToken(scanState, tokenList, Consume(scanState, '=') ? TOKEN_NOT_EQUAL : TOKEN_NOT); break;
		case '=': AddToken(scanState, tokenList, Consume(scanState, '=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL); break;
		case '<': AddToken(scanState, tokenList, Consume(scanState, '=') ? TOKEN_LESS_EQUAL : TOKEN_LESS); break;
		case '>': AddToken(scanState, tokenList, Consume(scanState, '=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER); break;
		case '/':
			if ( Consume(scanState, '/') )
			{
				// Discard all chars until the end of line is reached
				while ( !IsEOL( Peek(scanState) ) && !IsAtEnd(scanState) ) Advance(scanState);
			}
			else if ( Consume(scanState, '*') )
			{
				while( Peek(scanState) != '*' && PeekNext(scanState) != '/' &&  !IsAtEnd(scanState) )
				{
					if ( IsEOL( Peek(scanState) ) ) scanState.line++;
					Advance(scanState);
				}
			}
			else
			{
				AddToken(scanState, tokenList, TOKEN_SLASH);
			}
			break;

		// Skip whitespaces
		case ' ':
		case '\r':
		case '\t':
			break;

		// End of line counter
		case '\n':
			scanState.line++;
			break;

		case '\"':
			while ( Peek(scanState) != '\"' && !IsAtEnd(scanState) )
			{
				if ( IsEOL( Peek(scanState) ) ) scanState.line++;
				Advance(scanState);
			}

			if ( IsAtEnd(scanState) )
			{
				ReportError( scanState, "Unterminated string." );
				return;
			}

			Advance(scanState);

			AddToken(scanState, tokenList, TOKEN_STRING);
			break;

		default:
			if ( IsDigit(c) )
			{
				while ( IsDigit( Peek(scanState) ) ) Advance(scanState);

				if ( Peek(scanState) == '.' && IsDigit( PeekNext(scanState) ) )
				{
					Advance(scanState);
					while ( IsDigit(Peek(scanState)) ) Advance(scanState);
				}

				AddToken(scanState, tokenList, TOKEN_NUMBER);
			}
			else if ( IsAlpha(c) )
			{
				while ( IsAlphaNumeric( Peek(scanState) ) ) Advance(scanState);

				String word = ScannedString(scanState);

				// Keywords // TODO: This keyword search could be much more efficient
				if      ( StrEq( word, "if" ) )     AddToken(scanState, tokenList, TOKEN_IF);
				else if ( StrEq( word, "else" ) )   AddToken(scanState, tokenList, TOKEN_ELSE);
				else if ( StrEq( word, "for" ) )    AddToken(scanState, tokenList, TOKEN_FOR);
				else if ( StrEq( word, "while" ) )  AddToken(scanState, tokenList, TOKEN_WHILE);
				else if ( StrEq( word, "class" ) )  AddToken(scanState, tokenList, TOKEN_CLASS);
				else if ( StrEq( word, "super" ) )  AddToken(scanState, tokenList, TOKEN_SUPER);
				else if ( StrEq( word, "this" ) )   AddToken(scanState, tokenList, TOKEN_THIS);
				else if ( StrEq( word, "fun" ) )    AddToken(scanState, tokenList, TOKEN_FUN);
				else if ( StrEq( word, "return" ) ) AddToken(scanState, tokenList, TOKEN_RETURN);
				else if ( StrEq( word, "true" ) )   AddToken(scanState, tokenList, TOKEN_TRUE);
				else if ( StrEq( word, "false" ) )  AddToken(scanState, tokenList, TOKEN_FALSE);
				else if ( StrEq( word, "nil" ) )    AddToken(scanState, tokenList, TOKEN_NIL);
				else if ( StrEq( word, "var" ) )    AddToken(scanState, tokenList, TOKEN_VAR);
				else if ( StrEq( word, "print" ) )  AddToken(scanState, tokenList, TOKEN_PRINT);
				else if ( StrEq( word, "eof" ) )    AddToken(scanState, tokenList, TOKEN_EOF);
				else                                AddToken(scanState, tokenList, TOKEN_IDENTIFIER);
			}
			else
			{
				ReportError( scanState, "Unexpected character." );
			}
	}
}

TokenList Scan(Arena &arena, ScanState &scanState, const char *script, u32 scriptSize)
{
	TokenList tokenList = {};
	tokenList.tokens = PushArray(arena, Token, MAX_TOKEN_COUNT);

	scanState.line = 1;
	scanState.hasErrors = false;
	scanState.script = script;
	scanState.scriptSize = scriptSize;

	while ( !IsAtEnd(scanState) )
	{
		ScanToken(scanState, tokenList);
	}

	AddToken(scanState, tokenList, TOKEN_EOF);

	return tokenList;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Parser

// Grammar:
// program        -> declaration* EOF
// declaration    -> varDecl | statement
// statement      -> exprStatement | printStatement
// exprStatement  -> expression ";"
// printStatement -> "print" "(" expression ")" ";"
// expression     -> assignment
// assignment     -> IDENTIFIER "=" assignment | equality
// equality       -> comparison ( ( "!=" | "==") comparison )
// comparison     -> term ( ( ">" | ">=" | "<" | "<=" ) term )*
// term           -> factor ( ( "-" | "+" ) factor )*
// factor         -> unary ( ( "/" | "*" ) unary )*
// unary          -> ( "!" | "-" ) unary | primary
// primary        -> NUMBER | STRING | "true" | "false" | "nil" | "(" expression ")"

struct ParseState
{
	TokenList* tokenList;
	u32 current;
	bool hasErrors;
};

enum ExprType
{
	EXPR_IDENTIFIER,
	EXPR_LITERAL,
	EXPR_UNARY,
	EXPR_BINARY,
	EXPR_ASSIGNMENT,
};

struct Expr;

struct ExprIdentifier
{
	Token *identifierToken;
};

struct ExprLiteral
{
	Token *literalToken;
};

struct ExprUnary
{
	Token *operatorToken;
	Expr *expr;
};

struct ExprBinary
{
	Expr *left;
	Token *operatorToken;
	Expr *right;
};

struct ExprAssignment
{
	Token *nameToken;
	Expr *right;
};

struct Expr
{
	ExprType type;
	union
	{
		ExprIdentifier identifier;
		ExprLiteral literal;
		ExprUnary unary;
		ExprBinary binary;
		ExprAssignment assignment;
	};
};

struct ExprList
{
	Expr exprs[8];
	u32 exprsCount;
	ExprList *next;
};

enum StmtType
{
	STMT_PRINT,
	STMT_EXPR,
	STMT_VAR_DECL,
};

struct Stmt
{
	StmtType type;
	Expr *expr;
	Token *identifier;
};

struct StmtList
{
	Stmt stmts[8];
	u32 stmtsCount;
	StmtList *next;
};

struct Program
{
	Arena *arena;
	ExprList *expressions;
	StmtList *statements;
};

struct Var
{
	String name;
	Value value;
};

struct VarList
{
	Var vars[16];
	u32 varsCount;
	VarList *next;
};

struct Environment
{
	VarList *variables;
};

void ReportError(ParseState &parseState, const char *message)
{
	Token &token = parseState.tokenList->tokens[ parseState.current ];
	u32 line = token.line;
	printf("ERROR: %d: %s\n", line, message);
	parseState.hasErrors = true;
}

bool IsAtEnd(const ParseState &parseState)
{
	const TokenList &tokenList = *parseState.tokenList;
	const Token &currentToken = tokenList.tokens[ parseState.current ];
	return currentToken.type == TOKEN_EOF;
}

bool Consume(ParseState &parseState, TokenId tokenId)
{
	TokenList &tokenList = *parseState.tokenList;
	Token &currentToken = tokenList.tokens[ parseState.current ];
	if ( currentToken.type == tokenId )
	{
		parseState.current++;
		return true;
	}
	else
	{
		return false;
	}
}

void ConsumeForced(ParseState &parseState, TokenId tokenId, const char *context)
{
	if ( !Consume(parseState, tokenId) )
	{
		char errorMessage[1024];
		StrCopy(errorMessage, "Expected token ");
		StrCat(errorMessage, TokenNames[tokenId]);
		StrCat(errorMessage, " not found in context: ");
		StrCat(errorMessage, context);
		ReportError( parseState, errorMessage );
	}
}

Token* Consumed(ParseState &parseState)
{
	ASSERT( parseState.current > 0 );
	TokenList &tokenList = *parseState.tokenList;
	Token &consumedToken = tokenList.tokens[ parseState.current - 1 ];
	return &consumedToken;
}

Expr* AddExpression(Program &program)
{
	// Ensure the first expression list block
	if ( !program.expressions )
	{
		program.expressions = PushStruct(*program.arena, ExprList);
		program.expressions->exprsCount = 0;
		program.expressions->next = 0;
	}

	// Advance to the last expression list block
	ExprList *list = program.expressions;
	while ( list->next ) list = list->next;

	// Ensure we have a list block with space
	if ( list->exprsCount == ARRAY_COUNT(list->exprs) )
	{
		list->next = PushStruct(*program.arena, ExprList);
		list->next->exprsCount = 0;
		list->next->next = 0;
		list = list->next;
	}

	Expr* expr = &list->exprs[list->exprsCount++];
	return expr;
}

Expr* AddExpression(Program &program, Token* token)
{
	Expr* expr = AddExpression(program);
	if ( token->type == TOKEN_IDENTIFIER )
	{
		expr->type = EXPR_IDENTIFIER;
		expr->identifier.identifierToken = token;
	}
	else
	{
		expr->type = EXPR_LITERAL;
		expr->literal.literalToken = token;
	}
	return expr;
}

Expr* AddExpression(Program &program, Token* operatorToken, Expr* pExpr)
{
	Expr* expr = AddExpression(program);
	expr->type = EXPR_UNARY;
	expr->unary.operatorToken = operatorToken;
	expr->unary.expr = pExpr;
	return expr;
}

Expr* AddExpression(Program &program, Expr* left, Token* operatorToken, Expr* right)
{
	Expr* expr = AddExpression(program);
	expr->type = EXPR_BINARY;
	expr->binary.left = left;
	expr->binary.operatorToken = operatorToken;
	expr->binary.right = right;
	return expr;
}

Stmt* AddStatement(Program &program)
{
	// Ensure first list block
	if ( !program.statements )
	{
		program.statements = PushStruct(*program.arena, StmtList);
		program.statements->stmtsCount = 0;
		program.statements->next = 0;
	}

	// Advance to the last list block
	StmtList *list = program.statements;
	while ( list->next ) list = list->next;

	// Ensure a current list block with space
	if ( list->stmtsCount == ARRAY_COUNT( list->stmts ) )
	{
		list->next = PushStruct(*program.arena, StmtList);
		list->next->stmtsCount = 0;
		list->next->next = 0;
		list = list->next;
	}

	Stmt *stmt = &list->stmts[ list->stmtsCount++ ];
	return stmt;
}

Stmt* AddPrintStatement(Program &program, Expr *expr)
{
	Stmt *statement = AddStatement(program);
	statement->type = STMT_PRINT;
	statement->expr = expr;
	return statement;
}

Stmt* AddExpressionStatement(Program &program, Expr *expr)
{
	Stmt *statement = AddStatement(program);
	statement->type = STMT_EXPR;
	statement->expr = expr;
	return statement;
}

Stmt* AddVarDeclaration(Program &program, Token *tokenIdentifier, Expr *expr)
{
	Stmt *statement = AddStatement(program);
	statement->type = STMT_VAR_DECL;
	statement->expr = expr;
	statement->identifier = tokenIdentifier;
	return statement;
}

Expr* ParseExpression(ParseState &parseState, Program &program);

Expr* ParsePrimary(ParseState &parseState, Program &program)
{
	if ( Consume(parseState, TOKEN_FALSE) ) return AddExpression(program, Consumed(parseState));
	if ( Consume(parseState, TOKEN_TRUE) ) return AddExpression(program, Consumed(parseState));
	if ( Consume(parseState, TOKEN_NIL) ) return AddExpression(program, Consumed(parseState));
	if ( Consume(parseState, TOKEN_NUMBER) ) return AddExpression(program, Consumed(parseState));
	if ( Consume(parseState, TOKEN_STRING) ) return AddExpression(program, Consumed(parseState));
	if ( Consume(parseState, TOKEN_IDENTIFIER) ) return AddExpression(program, Consumed(parseState));
	if ( Consume(parseState, TOKEN_LEFT_PAREN) )
	{
		Expr* expr = ParseExpression(parseState, program);
		ConsumeForced(parseState, TOKEN_RIGHT_PAREN, __FUNCTION__);
		return expr;
	}

	ReportError(parseState, "Could not parse primary expression");
	return 0;
}

Expr* ParseUnary(ParseState &parseState, Program &program)
{
	if ( Consume(parseState, TOKEN_NOT) ||
		 Consume(parseState, TOKEN_MINUS) )
	{
		Token* op = Consumed(parseState);
		Expr* expr = ParseUnary(parseState, program);
		return AddExpression(program, op, expr);
	}
	else
	{
		return ParsePrimary(parseState, program);
	}
}

Expr* ParseFactor(ParseState &parseState, Program &program)
{
	Expr *expr = ParseUnary(parseState, program);

	while ( Consume(parseState, TOKEN_STAR) ||
			Consume(parseState, TOKEN_SLASH) )
	{
		Token* op = Consumed(parseState);
		Expr* right = ParseUnary(parseState, program);
		expr = AddExpression(program, expr, op, right);
	}

	return expr;
}

Expr* ParseTerm(ParseState &parseState, Program &program)
{
	Expr* expr = ParseFactor(parseState, program);

	while ( Consume(parseState, TOKEN_MINUS) ||
			Consume(parseState, TOKEN_PLUS) )
	{
		Token* op = Consumed(parseState);
		Expr* right = ParseFactor(parseState, program);
		expr = AddExpression(program, expr, op, right);
	}

	return expr;
}

Expr* ParseComparison(ParseState &parseState, Program &program)
{
	Expr* expr = ParseTerm(parseState, program);

	while ( Consume(parseState, TOKEN_GREATER) ||
			Consume(parseState, TOKEN_GREATER_EQUAL) ||
			Consume(parseState, TOKEN_LESS) ||
			Consume(parseState, TOKEN_LESS_EQUAL) )
	{
		Token* op = Consumed(parseState);
		Expr* right = ParseTerm(parseState, program);
		expr = AddExpression(program, expr, op, right);
	}

	return expr;
}

Expr* ParseEquality(ParseState &parseState, Program &program)
{
	Expr* expr = ParseComparison(parseState, program);

	if ( Consume(parseState, TOKEN_EQUAL_EQUAL) ||
		 Consume(parseState, TOKEN_NOT_EQUAL) )
	{
		Token* op = Consumed(parseState);
		Expr* right = ParseComparison(parseState, program);
		expr = AddExpression(program, expr, op, right);
	}

	return expr;
}

Expr* ParseAssignment(ParseState &parseState, Program &program)
{
	Expr *expr = ParseEquality(parseState, program);

	if ( Consume(parseState, TOKEN_EQUAL) )
	{
		Expr *left = expr;
		//Token *equals = Consumed(parseState);
		Expr *right = ParseAssignment(parseState, program);

		if (left->type == EXPR_IDENTIFIER)
		{
			Expr *exprAssign = AddExpression(program);
			exprAssign->type = EXPR_ASSIGNMENT;
			exprAssign->assignment.nameToken = left->identifier.identifierToken;
			exprAssign->assignment.right = right;
			return exprAssign;
		}
		else
		{
			printf("Invalid assignment target.\n");
		}
	}

	return expr;
}

Expr* ParseExpression(ParseState &parseState, Program &program)
{
	return ParseAssignment(parseState, program);
}

Stmt* ParseExpressionStatement(ParseState &parseState, Program &program)
{
	Expr *expr = ParseExpression(parseState, program);
	ConsumeForced(parseState, TOKEN_SEMICOLON, __FUNCTION__);
	Stmt *stmt = AddExpressionStatement(program, expr);
	return stmt;
}

Stmt* ParsePrintStatement(ParseState &parseState, Program &program)
{
	ConsumeForced(parseState, TOKEN_LEFT_PAREN, __FUNCTION__);
	Expr *expr = ParseExpression(parseState, program);
	ConsumeForced(parseState, TOKEN_RIGHT_PAREN, __FUNCTION__);
	ConsumeForced(parseState, TOKEN_SEMICOLON, __FUNCTION__);
	Stmt *stmt = AddPrintStatement(program, expr);
	return stmt;
}

Stmt* ParseStatement(ParseState &parseState, Program &program)
{
	if ( Consume(parseState, TOKEN_PRINT) )
	{
		return ParsePrintStatement(parseState, program);
	}
	else
	{
		return ParseExpressionStatement(parseState, program);
	}
}

Stmt* ParseVarDeclaration(ParseState &parseState, Program &program)
{
	ConsumeForced(parseState, TOKEN_IDENTIFIER, __FUNCTION__);
	Token *tokenIdentifier = Consumed( parseState );

	Expr *initExpr = 0;
	if ( Consume(parseState, TOKEN_EQUAL) )
	{
		initExpr = ParseExpression(parseState, program);
	}

	ConsumeForced(parseState, TOKEN_SEMICOLON, __FUNCTION__);

	Stmt *stmt = AddVarDeclaration(program, tokenIdentifier, initExpr);
	return stmt;
}

void ParseDeclaration(ParseState &parseState, Program &program)
{
	if ( Consume(parseState, TOKEN_VAR) )
	{
		ParseVarDeclaration(parseState, program);
	}
	else
	{
		ParseStatement(parseState, program);
	}

	// This is a good point to catch parsing errors and synchronize
	if ( parseState.hasErrors )
	{
		// TODO(jesus): Advance to the next declaration/statement here
	}
}

#if 0
void PrintExpr(Expr* expr, u32 level = 0)
{
	for (u32 i = 0; i < level; ++i) printf("  ");
	u32 space = 16 - level*2;

	if (expr->type == EXPR_LITERAL)
	{
		String lexeme = expr->literal.literalToken->lexeme;
		printf("%.&s%*s(%s)\n", lexeme.size, lexeme.str, space, "", TokenNames[expr->literal.literalToken->type] );
	}
	else if (expr->type == EXPR_UNARY)
	{
		String lexeme = expr->unary.operatorToken->lexeme;
		printf("%.*s%*s(%s)\n", lexeme.size, lexeme.str, space, "", TokenNames[expr->unary.operatorToken->type] );
		PrintExpr(expr->unary.expr, level+1);
	}
	else if (expr->type == EXPR_BINARY)
	{
		String lexeme = expr->binary.operatorToken->lexeme;
		printf("%.*s%*s(%s)\n", lexeme.size, lexeme.str, space, "", TokenNames[expr->binary.operatorToken->type] );
		PrintExpr(expr->binary.left, level+1);
		PrintExpr(expr->binary.right, level+1);
	}
}
#endif

Program Parse(Arena &arena, ParseState &parseState, TokenList &tokens)
{
	Program program = {};
	program.arena = &arena;

	parseState.tokenList = &tokens;
	parseState.current = 0;
	parseState.hasErrors = false;

	while (!IsAtEnd(parseState) && !parseState.hasErrors)
	{
		ParseDeclaration(parseState, program);
		// TODO: Maybe the declaration must be pushed here onto the arena

		//program.exprCount = 0; // Reset AST expression list
		//Expr *expr = ParseExpression(parseState, program);
#if 0
		PrintExpr(expr);
#endif
	}

	return program;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Evaluator

bool Add(Arena &arena, Environment &e, String name, Value value)
{
	VarList *varlist = e.variables;

	if ( varlist )
	{
		while ( varlist->next )
		{
			for ( u32 i = 0; i < varlist->varsCount; ++i )
			{
				if ( StrEq( name, varlist->vars[i].name ) )
				{
					printf("A variable with the same name'%s' already exists in this environment.\n", name.str);
					return false;
				}
			}

			varlist = varlist->next;
		}
	}
	else
	{
		varlist = PushStruct( arena, VarList );
		varlist->varsCount = 0;
		varlist->next = 0;
		e.variables = varlist;
	}

	if ( varlist->varsCount == ARRAY_COUNT(varlist->vars) )
	{
		varlist->next = PushStruct( arena, VarList );
		varlist = varlist->next;
		varlist->varsCount = 0;
		varlist->next = 0;
	}

	varlist->vars[ varlist->varsCount ].name = name;
	varlist->vars[ varlist->varsCount ].value = value;
	varlist->varsCount++;

	return true;
}

bool Get(Environment &e, String name, Value &value)
{
	VarList *varlist = e.variables;

	while ( varlist )
	{
		for ( u32 i = 0; i < varlist->varsCount; ++i )
		{
			if ( StrEq( name, varlist->vars[i].name ) )
			{
				value = varlist->vars[i].value;
				return true;
			}
		}

		varlist = varlist->next;
	}

	return false;
}

bool Set(Environment &e, String name, Value value)
{
	VarList *varlist = e.variables;

	while ( varlist )
	{
		for ( u32 i = 0; i < varlist->varsCount; ++i )
		{
			if ( StrEq( name, varlist->vars[i].name ) )
			{
				varlist->vars[i].value = value;
				return true;
			}
		}

		varlist = varlist->next;
	}

	return false;
}

Value Evaluate(Arena &arena, Expr *expr, Environment &env)
{
	Value value = {};

	switch (expr->type)
	{
		case EXPR_IDENTIFIER:
		{
			if ( !Get(env, expr->identifier.identifierToken->lexeme, value) )
			{
				printf("Could not find identifier %.*s\n", expr->identifier.identifierToken->lexeme.size, expr->identifier.identifierToken->lexeme.str);
			}
			break;
		}
		case EXPR_LITERAL:
		{
			value = expr->literal.literalToken->literal;
			break;
		}
		case EXPR_UNARY:
		{
			value = Evaluate( arena, expr->unary.expr, env );
			switch ( expr->unary.operatorToken->type )
			{
				case TOKEN_MINUS:
					ASSERT( value.type == VALUE_TYPE_FLOAT );
					value.f = -value.f;
					break;
				case TOKEN_NOT:
					ASSERT( value.type == VALUE_TYPE_BOOL );
					value.b = !value.b;
					break;
				default:
					INVALID_CODE_PATH();
			}
			break;
		}
		case EXPR_BINARY:
		{
			Value left = Evaluate( arena, expr->binary.left, env );
			Value right = Evaluate( arena, expr->binary.right, env );
			switch ( expr->binary.operatorToken->type )
			{
				case TOKEN_MINUS:
					ASSERT( left.type == VALUE_TYPE_FLOAT && right.type == VALUE_TYPE_FLOAT );
					value.type = VALUE_TYPE_FLOAT;
					value.f = left.f - right.f;
					break;
				case TOKEN_PLUS:
					ASSERT( left.type == VALUE_TYPE_FLOAT && right.type == VALUE_TYPE_FLOAT );
					value.type = VALUE_TYPE_FLOAT;
					value.f = left.f + right.f;
					break;
				case TOKEN_STAR:
					ASSERT( left.type == VALUE_TYPE_FLOAT && right.type == VALUE_TYPE_FLOAT );
					value.type = VALUE_TYPE_FLOAT;
					value.f = left.f * right.f;
					break;
				case TOKEN_SLASH:
					ASSERT( left.type == VALUE_TYPE_FLOAT && right.type == VALUE_TYPE_FLOAT );
					value.type = VALUE_TYPE_FLOAT;
					value.f = left.f / right.f;
					break;
				case TOKEN_LESS:
					ASSERT( left.type == VALUE_TYPE_FLOAT && right.type == VALUE_TYPE_FLOAT );
					value.type = VALUE_TYPE_BOOL;
					value.b = left.f < right.f;
					break;
				case TOKEN_LESS_EQUAL:
					ASSERT( left.type == VALUE_TYPE_FLOAT && right.type == VALUE_TYPE_FLOAT );
					value.type = VALUE_TYPE_BOOL;
					value.b = left.f <= right.f;
					break;
				case TOKEN_GREATER:
					ASSERT( left.type == VALUE_TYPE_FLOAT && right.type == VALUE_TYPE_FLOAT );
					value.type = VALUE_TYPE_BOOL;
					value.b = left.f > right.f;
					break;
				case TOKEN_GREATER_EQUAL:
					ASSERT( left.type == VALUE_TYPE_FLOAT && right.type == VALUE_TYPE_FLOAT );
					value.type = VALUE_TYPE_BOOL;
					value.b = left.f >= right.f;
					break;
				case TOKEN_NOT_EQUAL:
					value.type = VALUE_TYPE_BOOL;
					value.b = false;
					if ( left.type == VALUE_TYPE_BOOL && right.type == VALUE_TYPE_BOOL ) {
						value.b = left.b != right.b;
					} else if ( left.type == VALUE_TYPE_FLOAT && right.type == VALUE_TYPE_FLOAT ) {
						value.b = left.f != right.f;
					}
					break;
				case TOKEN_EQUAL_EQUAL:
					value.type = VALUE_TYPE_BOOL;
					value.b = false;
					if ( left.type == VALUE_TYPE_BOOL && right.type == VALUE_TYPE_BOOL ) {
						value.b = left.b == right.b;
					} else if ( left.type == VALUE_TYPE_FLOAT && right.type == VALUE_TYPE_FLOAT ) {
						value.b = left.f == right.f;
					}
					break;
				default:
					INVALID_CODE_PATH();
			}
			break;
		}
		case EXPR_ASSIGNMENT:
		{
			Token *name = expr->assignment.nameToken;
			Expr *right = expr->assignment.right;

			Value value = Evaluate( arena, right, env );
			if ( !Set( env, name->lexeme, value ) )
			{
				printf("Could not find identifier %.*s\n", name->lexeme.size, name->lexeme.str);
			}
			break;
		}
		default:
			INVALID_CODE_PATH();
	}

	return value;
}

void Execute( Arena &arena, Stmt &stmt, Environment &env)
{
	switch ( stmt.type )
	{
		case STMT_EXPR:
			{
				// Assignment statements are here by now
				Evaluate( arena, stmt.expr, env );
				break;
			}
		case STMT_PRINT:
			{
				Value val = Evaluate( arena, stmt.expr, env );
				// TODO: In case there was an evaluation erro,
				// this should not print anything

				printf("Evaluated value: ");
				switch ( val.type )
				{
					case VALUE_TYPE_FLOAT:
						printf("%f", val.f);
						break;
					case VALUE_TYPE_BOOL:
						printf("%s", val.b ? "true" : "false" );
						break;
					case VALUE_TYPE_STRING:
						char cstring[512];
						StrCopy(cstring, val.s);
						printf("%s", cstring);
						break;
					case VALUE_TYPE_NIL:
						printf("nil");
						break;
					default:
						INVALID_CODE_PATH();
				}
				printf("\n");
			}
			break;
		case STMT_VAR_DECL:
			{
				Value val;
				val.type = VALUE_TYPE_NIL;

				if ( stmt.expr )
				{
					val = Evaluate( arena, stmt.expr, env );
				}

				if ( !Add( arena, env, stmt.identifier->lexeme, val ) )
				{
					printf("Error\n");
				}
				break;
			}
		default:
			INVALID_CODE_PATH();
	}
}

void Execute(Arena &arena, Program &program, Environment &env)
{
	for (StmtList *list = program.statements; list; list = list->next)
	{
		for (u32 i = 0; i < list->stmtsCount; ++i)
		{
			Execute( arena, list->stmts[i], env );
		}
	}
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Program

#define COMMAND_NAME "jsl"

#if 0
void PrintTokenList(const TokenList &tokenList)
{
	printf("List of tokens:\n");
	for (u32 i = 0; i < tokenList.count; ++i)
	{
		const Token& token = tokenList.tokens[i];
		printf("%s\t: %.*s\n", TokenNames[token.type], token.lexeme.size, token.lexeme.str);
	}
}
#endif

void Run(Arena &arena, const char *script, u32 scriptSize)
{
	ScanState scanState = {};
	TokenList tokenList = Scan(arena, scanState, script, scriptSize);

	if ( !scanState.hasErrors )
	{
#if 0
		PrintTokenList(tokenList);
#endif

		ParseState parseState = {};
		Program program = Parse(arena, parseState, tokenList);

		if ( !parseState.hasErrors )
		{
			Environment env = {};

			Execute(arena, program, env);
		}
	}
}

void RunFile(Arena &arena, const char* filename)
{
	u64 fileSize;
	if ( GetFileSize(filename, fileSize) && fileSize > 0 )
	{
		Arena backupArena = arena;
		char* bytes = PushArray(arena, char, fileSize + 1);
		if ( ReadEntireFile(filename, bytes, fileSize) )
		{
			bytes[fileSize] = 0;
			Run(arena, bytes, fileSize);
		}
		else
		{
			arena = backupArena;
			LOG(Error, "ReadEntireFile() failed reading %s\n", filename);
		}
	}
	else
	{
		LOG(Error, "GetFileSize() failed reading %s\n", filename);
	}
}

void RunPrompt(Arena &arena)
{
	char line[1024];

	for (;;)
	{
		printf("> ");

		fgets(line, ARRAY_COUNT(line), stdin);
		u32 lineLen = StrLen(line);
		line[--lineLen] = 0; // remove trailing \n

		if ( StrEq( "", line ) )
		{
		    continue;
		}
		else if ( StrEq( "exit", line ) ||
			 StrEq( "quit", line ) ||
			 StrEq( "q", line ) )
		{
			break;
		}
		else
		{
			ResetArena(arena);
			Run(arena, line, lineLen);
		}
	}
}

int main(int argc, char **argv)
{
	u32 globalArenaSize = MB(2);
	byte *globalArenaBase = (byte*)AllocateVirtualMemory(globalArenaSize);

	Arena globalArena = MakeArena(globalArenaBase, globalArenaSize, "Global Arena");

	if ( argc > 2 )
	{
		printf("Usage: %s [script]\n", COMMAND_NAME);
		return -1;
	}
	else if ( argc == 2 )
	{
		RunFile(globalArena, argv[1]);
	}
	else
	{
		RunPrompt(globalArena);
	}

	//PrintArenaUsage(globalArena);

	return 0;
}

