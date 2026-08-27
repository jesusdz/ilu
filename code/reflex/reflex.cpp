#define CAST_IMPLEMENTATION
#include "cast.h"
#include "../ilu_core.h"

#define StringPrintfArgs(string) string.size, string.str

// Macros used in the parsed files to tag reflected structs, properties and enums
static const CastConfig castConfig = {
	.structTag = "ILU_STRUCT",
	.fieldTag = "ILU_PROPERTY",
	.enumTag = "ILU_ENUM",
};

// Returns the type name of a struct member when its type is an identifier
// (as opposed to a trivial type or an inline struct/enum), or an empty string.
static String GetMemberTypeName(const CastStructDeclaration *structDeclaration)
{
	const CastSpecifierQualifierList *specifierList = CAST_CHILD(structDeclaration, specifierQualifierList);
	while (specifierList)
	{
		const CastTypeSpecifier *typeSpecifier = specifierList->typeSpecifier;
		if (typeSpecifier && typeSpecifier->type == CAST_IDENTIFIER) {
			return typeSpecifier->identifier;
		}
		specifierList = specifierList->next;
	}
	return MakeString("");
}

static bool IsPointerMember(const CastStructDeclaration *structDeclaration)
{
	const CastStructDeclaratorList *structDeclaratorList = CAST_CHILD(structDeclaration, structDeclaratorList);
	const CastDeclarator *declarator = CAST_CHILD(structDeclaratorList, structDeclarator);
	const CastPointer *pointer = CAST_CHILD(declarator, pointer);
	return pointer != NULL;
}

void GenerateReflex(const Cast *cast, Arena arena)
{
	printf("\n");
	printf("////////////////////////////////////////////////////////////////////////\n");
	printf("// Includes\n");
	printf("#include <stddef.h> // for offsetof macro\n");

	const CastStructSpecifier *structs[128];
	u32 structCount = 0;

	const CastStructDeclaration *structDeclarations[128]; // members
	u32 structDeclarationCount = 0;

	const CastEnumSpecifier *enums[128];
	u32 enumCount = 0;

	// Get all the global struct and enum specifiers from the AST.
	// Only structs and enums tagged with their tag macro are reflected.
	const CastTranslationUnit *translationUnit = cast->translationUnit;
	while (translationUnit)
	{
		if (translationUnit->externalDeclaration &&
			translationUnit->externalDeclaration->declaration &&
			translationUnit->externalDeclaration->declaration->declarationSpecifiers &&
			translationUnit->externalDeclaration->declaration->declarationSpecifiers->typeSpecifier)
		{
			const CastTypeSpecifier *typeSpecifier =
				translationUnit->externalDeclaration->declaration->declarationSpecifiers->typeSpecifier;

			if (typeSpecifier->type == CAST_STRUCT && typeSpecifier->structSpecifier && typeSpecifier->structSpecifier->tag) {
				ASSERT(structCount < ARRAY_COUNT(structs));
				structs[structCount++] = typeSpecifier->structSpecifier;
			} else if (typeSpecifier->type == CAST_ENUM && typeSpecifier->enumSpecifier && typeSpecifier->enumSpecifier->tag) {
				ASSERT(enumCount < ARRAY_COUNT(enums));
				enums[enumCount++] = typeSpecifier->enumSpecifier;
			}
		}
		translationUnit = translationUnit->next;
	}

	// Reflected members can point to not reflected types declared in other files.
	// These are opaque and we call them custom types
	String customTypes[128];
	bool customTypeIsValue[128];
	u32 customTypeCount = 0;

	for (u32 index = 0; index < structCount; ++index)
	{
		const CastStructDeclarationList *structDeclarationList = structs[index]->structDeclarationList;
		while (structDeclarationList)
		{
			const CastStructDeclaration *structDeclaration = structDeclarationList->structDeclaration;
			structDeclarationList = structDeclarationList->next;

			if (!structDeclaration || !structDeclaration->tag) {
				continue;
			}

			const String typeName = GetMemberTypeName(structDeclaration);
			if (typeName.size == 0) {
				continue;
			}

			// Types reflected in this file already have a ReflexID
			bool isReflected = false;
			for (u32 i = 0; i < structCount && !isReflected; ++i) {
				isReflected = StrEq(structs[i]->name, typeName);
			}
			for (u32 i = 0; i < enumCount && !isReflected; ++i) {
				isReflected = StrEq(enums[i]->name, typeName);
			}
			if (isReflected) {
				continue;
			}

			const bool isValue = !IsPointerMember(structDeclaration);

			const String tagName = structDeclaration->tag->arguments;

			char customTypeName[128];
			SPrintf(customTypeName, "%.*s%.*s", StringPrintfArgs(typeName), StringPrintfArgs(tagName));

			u32 customIndex = 0;
			while (customIndex < customTypeCount && !StrEq(customTypes[customIndex], customTypeName)) {
				customIndex++;
			}
			if (customIndex == customTypeCount) {
				ASSERT(customTypeCount < ARRAY_COUNT(customTypes));
				const char *str = PushString(arena, customTypeName);
				customTypes[customTypeCount] = MakeString(str);
				customTypeIsValue[customTypeCount] = false;
				customTypeCount++;
			}
			customTypeIsValue[customIndex] = customTypeIsValue[customIndex] || isValue;
		}
	}

	if (customTypeCount > 0)
	{
		printf("\n");
		printf("\n");
		printf("////////////////////////////////////////////////////////////////////////\n");
		printf("// Custom Types: used by reflected members but not reflected themselves\n");

		for (u32 index = 0; index < customTypeCount; ++index)
		{
			const String typeName = customTypes[index];

			printf("\n");
			printf("// ReflexCustom info\n");
			printf("static const ReflexCustom reflexCustom_%.*s =\n", StringPrintfArgs(typeName));
			printf("{\n");
			printf("  .name = \"%.*s\",\n", StringPrintfArgs(typeName));
			printf("  .size = sizeof(%.*s),\n", StringPrintfArgs(typeName));
			printf("};\n");

			printf("\n");
			printf("// ReflexCustom registration\n");
			printf("static const ReflexID ReflexIDStub_%.*s = ReflexRegisterCustom(&reflexCustom_%.*s, ReflexID_%.*s);\n", StringPrintfArgs(typeName), StringPrintfArgs(typeName), StringPrintfArgs(typeName));
			printf("\n");
		}
	}

	// ReflexID enum
	printf("\n");
	for (u32 index = 0; index < enumCount; ++index)
	{
		const CastEnumSpecifier *cenum = enums[index];

		printf("\n");
		printf("////////////////////////////////////////////////////////////////////////\n");
		printf("// enum %.*s\n", StringPrintfArgs(cenum->name));

		printf("\n");
		printf("// ReflexEnumerator info\n");
		printf("static const ReflexEnumerator reflexEnumerators_%.*s[] = {\n", StringPrintfArgs(cenum->name));
		i32 enumeratorValue = 0;
		const CastEnumeratorList *enumeratorList = CAST_CHILD(cenum, enumeratorList);
		while (enumeratorList) {
			const CastEnumerator *enumerator = CAST_CHILD(enumeratorList, enumerator);
			if (enumerator) {
				printf("  { ");
				printf(".name = \"%.*s\", ", StringPrintfArgs(enumerator->name));
				printf(".value = %d, ", enumeratorValue++);
				printf("},\n");
			}
			enumeratorList = enumeratorList->next;
		}
		printf("};\n");

		printf("\n");
		printf("// ReflexEnum info\n");
		printf("static const ReflexEnum reflexEnum_%.*s =\n", StringPrintfArgs(cenum->name));
		printf("{\n");
		printf("  .name = \"%.*s\",\n", StringPrintfArgs(cenum->name));
		if (cenum->tag && cenum->tag->arguments.size > 0) {
			printf("  .hint = \"%.*s\",\n", StringPrintfArgs(cenum->tag->arguments));
		} else {
			printf("  .hint = NULL,\n");
		}
		printf("  .enumerators = reflexEnumerators_%.*s,\n", StringPrintfArgs(cenum->name));
		printf("  .enumeratorCount = ARRAY_COUNT(reflexEnumerators_%.*s),\n", StringPrintfArgs(cenum->name));
		printf("};\n");

		printf("\n");
		printf("// ReflexEnum registration\n");
		printf("static const ReflexID ReflexID_%.*s = ReflexRegisterEnum(&reflexEnum_%.*s);\n", StringPrintfArgs(cenum->name), StringPrintfArgs(cenum->name));
		printf("\n");
	}

	for (u32 index = 0; index < structCount; ++index)
	{
		const CastStructSpecifier *cstruct = structs[index];

		printf("\n");
		printf("////////////////////////////////////////////////////////////////////////\n");
		printf("// struct %.*s\n", StringPrintfArgs(cstruct->name));

		// Only members tagged with the field tag macro are reflected
		structDeclarationCount = 0;
		const CastStructDeclarationList *structDeclarationList = cstruct->structDeclarationList;
		while (structDeclarationList) {
			const CastStructDeclaration *structDeclaration = structDeclarationList->structDeclaration;
			if (structDeclaration && structDeclaration->tag) {
				ASSERT(structDeclarationCount < ARRAY_COUNT(structDeclarations));
				structDeclarations[structDeclarationCount++] = structDeclaration;
			}
			structDeclarationList = structDeclarationList->next;
		}

		if (structDeclarationCount > 0)
		{
			printf("\n");
			printf("// ReflexMember info\n");
			printf("static const ReflexMember reflexMembers_%.*s[] = {\n", StringPrintfArgs(cstruct->name));
		}

		for ( u32 memberIndex = 0; memberIndex < structDeclarationCount; ++memberIndex)
		{
			const CastStructDeclaration *structDeclaration = structDeclarations[memberIndex];

			// Type qualifiers

			const CastSpecifierQualifierList *specifierQualifierList = NULL;
			if (structDeclaration && structDeclaration->specifierQualifierList) {
				specifierQualifierList = structDeclaration->specifierQualifierList;
			}

			const CastTypeQualifier *typeQualifier = NULL;
			if (specifierQualifierList && specifierQualifierList->typeQualifier) {
				typeQualifier = specifierQualifierList->typeQualifier;
			}

			bool isConst = false;
			if (typeQualifier) {
				isConst = typeQualifier->type == CAST_CONST;
			}

			// Type specifiers

			bool isVoid = false;
			bool isBool = false;
			bool isChar = false;
			bool isInt = false;
			bool isFloat = false;
			bool isDouble = false;
			bool isShort = false;
			bool isLong = false;
			bool isLongLong = false;
			bool isUnsigned = false;
			bool isIdentifier = false;
			String identifier = MakeString("");
			char typeNameBuffer[MAX_PATH_LENGTH];
			String typeName = MakeString("<none>");
			//bool isStruct = false;
			//bool isEnum = false;

			const CastTypeSpecifier *typeSpecifier = NULL;
			if (specifierQualifierList)
			{
				const CastSpecifierQualifierList *specifierList = specifierQualifierList;
				while (specifierList)
				{
					typeSpecifier = specifierList->typeSpecifier;
					if (typeSpecifier) {
						if (typeSpecifier->type == CAST_VOID) {
							isVoid = true; break;
						} else if (typeSpecifier->type == CAST_BOOL) {
							isBool = true; break;
						} else if (typeSpecifier->type == CAST_CHAR) {
							isChar = true; break;
						} else if (typeSpecifier->type == CAST_FLOAT) {
							isFloat = true; break;
						} else if (typeSpecifier->type == CAST_DOUBLE) {
							isDouble = true; break;
						} else if (typeSpecifier->type == CAST_IDENTIFIER) {
							isIdentifier = true; identifier = typeSpecifier->identifier; break;
						} else if (typeSpecifier->type == CAST_INT) {
							isInt = true;
						} else if (typeSpecifier->type == CAST_UNSIGNED) {
							isUnsigned = true;
						} else if (typeSpecifier->type == CAST_SHORT) {
							isShort = true;
						} else if (typeSpecifier->type == CAST_LONG) {
							isLongLong = isLong; isLong = true;
						} else {
							typeName = MakeString("<error>"); break;
						}
					}
					specifierList = specifierList->next;
				}
			}

			if (isVoid) {
				typeName = MakeString("ReflexID_Void");
			} else if (isBool) {
				typeName = MakeString("ReflexID_Bool");
			} else if (isChar) {
				if (isUnsigned) typeName = MakeString("ReflexID_UnsignedChar");
				else typeName = MakeString("ReflexID_Char");
			} else if (isInt) {
				if (isUnsigned) {
					if (isLongLong) typeName = MakeString("ReflexID_UnsignedLongLongInt");
					else if (isLong) typeName = MakeString("ReflexID_UnsignedLongInt");
					else if (isShort) typeName = MakeString("ReflexID_UnsignedShortInt");
					else typeName = MakeString("ReflexID_UnsignedInt");
				} else {
					if (isLongLong) typeName = MakeString("ReflexID_LongLongInt");
					else if (isLong) typeName = MakeString("ReflexID_LongInt");
					else if (isShort) typeName = MakeString("ReflexID_ShortInt");
					else typeName = MakeString("ReflexID_Int");
				}
			} else if (isFloat) {
				typeName = MakeString("ReflexID_Float");
			} else if (isDouble) {
				typeName = MakeString("ReflexID_Double");
			} else if (isIdentifier) {
				StrCopy(typeNameBuffer, "ReflexID_");
				StrCat(typeNameBuffer, identifier);
				typeName = MakeString(typeNameBuffer);
			}

			// Declarator

			const CastDeclarator *declarator = NULL;
			if (structDeclaration->structDeclaratorList &&
				structDeclaration->structDeclaratorList->structDeclarator) {
				declarator = structDeclaration->structDeclaratorList->structDeclarator;
			}

			String memberName = MakeString("<none>");
			if (declarator &&
				declarator->directDeclarator) {
				memberName = declarator->directDeclarator->name;
			}

			u32 pointerCount = 0;
			CastPointer *pointer = declarator->pointer;
			while (pointer) {
				pointerCount++;
				pointer = pointer->next;
			}

			bool isArray = false;
			u32 arrayDim = 0;
			if (declarator && declarator->directDeclarator) {
				isArray = declarator->directDeclarator->isArray;
				CastExpression *expression = declarator->directDeclarator->expression;
				arrayDim = expression ? Cast_EvaluateInt(expression) : 0;
			}

			const CastTag *memberTag = structDeclaration->tag;
			const bool hasHint = memberTag && memberTag->arguments.size > 0;

			printf("  { ");
			printf(".name = \"%.*s\", ", StringPrintfArgs(memberName));
			if (hasHint) {
				printf(".hint = \"%.*s\", ", StringPrintfArgs(memberTag->arguments));
			} else {
				printf(".hint = NULL, ");
			}
			printf(".isConst = %s, ", isConst ? "true" : "false");
			printf(".pointerCount = %u, ", pointerCount);
			printf(".isArray = %s, ", isArray ? "true" : "false");
			printf(".arrayDim = %u, ", arrayDim);
			printf(".reflexId = %.*s%.*s, ", StringPrintfArgs(typeName), StringPrintfArgs(memberTag->arguments));
			printf(".offset = offsetof(%.*s, %.*s) ", StringPrintfArgs(cstruct->name), StringPrintfArgs(memberName));
			printf("},\n");
		}

		if (structDeclarationCount > 0)
		{
			printf("};\n");
		}

		printf("\n");
		printf("// ReflexStruct info\n");
		printf("static const ReflexStruct reflexStruct_%.*s =\n", StringPrintfArgs(cstruct->name));
		printf("{\n");
		printf("  .name = \"%.*s\",\n", StringPrintfArgs(cstruct->name));
		if (cstruct->tag && cstruct->tag->arguments.size > 0) {
			printf("  .hint = \"%.*s\",\n", StringPrintfArgs(cstruct->tag->arguments));
		} else {
			printf("  .hint = NULL,\n");
		}
		if (structDeclarationCount > 0) {
			printf("  .members = reflexMembers_%.*s,\n", StringPrintfArgs(cstruct->name));
			printf("  .memberCount = ARRAY_COUNT(reflexMembers_%.*s),\n", StringPrintfArgs(cstruct->name));
		} else {
			// A tagged struct with no tagged members: an empty array is not valid C++
			printf("  .members = NULL,\n");
			printf("  .memberCount = 0,\n");
		}
		printf("  .size = sizeof(%.*s),\n", StringPrintfArgs(cstruct->name));
		printf("};\n");

		printf("\n");
		printf("// ReflexStruct registration\n");
		printf("static const ReflexID ReflexID_%.*s = ReflexRegisterStruct(&reflexStruct_%.*s);\n", StringPrintfArgs(cstruct->name), StringPrintfArgs(cstruct->name));
		printf("\n");
	}
}

int main(int argc, char **argv)
{
	if (argc != 2 )
	{
		LOG(Info, "Usage: %s <c file>\n", argv[0]);
		return -1;
	}

	const char *filename = argv[1];

	u64 fileSize;
	if ( GetFileSize(filename, fileSize) && fileSize > 0 )
	{
		u32 globalArenaSize = MB(4);
		byte *globalArenaBase = (byte*)AllocateVirtualMemory(globalArenaSize);
		Arena globalArena = MakeArena(globalArenaBase, globalArenaSize, "globalArena");

		char* bytes = PushArray(globalArena, char, fileSize + 1);
		if ( ReadEntireFile(filename, bytes, fileSize) )
		{
			bytes[fileSize] = 0;

			const Cast *cast = Cast_Create(globalArena, bytes, fileSize, castConfig);
			if (cast)
			{
				GenerateReflex(cast, globalArena);
			}
			else
			{
				LOG(Error, "Cast_Create() failed:\n");
				LOG(Error, "- file: %s\n", filename);
				LOG(Error, "- message: %s\n", Cast_GetError());
				return -1;
			}
		}
		else
		{
			LOG(Error, "ReadEntireFile() failed reading %s\n", filename);
			return -1;
		}
	}
	else
	{
		LOG(Error, "GetFileSize() failed reading %s\n", filename);
		return -1;
	}
	
	return 0;
}

