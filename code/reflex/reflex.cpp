#define CAST_IMPLEMENTATION
#include "cast.h"
#include "../ilu_core.h"
#include "reflex.h"

#define StringPrintfArgs(string) string.size, string.str

// Macros used in the parsed files to tag reflected structs, properties and enums
static const CastConfig castConfig = {
	.tagString = "REFLEX",
};

// The trivial ReflexID enumerators, which the generated code refers to by name
static const char *TrivialReflexIDNames[] =
{
	"ReflexID_Void",
	"ReflexID_Bool",
	"ReflexID_Char",
	"ReflexID_UnsignedChar",
	"ReflexID_Int",
	"ReflexID_ShortInt",
	"ReflexID_LongInt",
	"ReflexID_LongLongInt",
	"ReflexID_UnsignedInt",
	"ReflexID_UnsignedShortInt",
	"ReflexID_UnsignedLongInt",
	"ReflexID_UnsignedLongLongInt",
	"ReflexID_Float",
	"ReflexID_Double",
};
CT_ASSERT(ARRAY_COUNT(TrivialReflexIDNames) == ReflexID_TrivialCount);

static bool SplitMetaKeyValue(String token, String &key, String &value)
{
	u32 equalsIndex = 0;
	while (equalsIndex < token.size && token.str[equalsIndex] != '=') {
		equalsIndex++;
	}
	if (equalsIndex == token.size) {
		return false;
	}
	key = MakeString(token.str, equalsIndex);
	value = MakeString(token.str + equalsIndex + 1, token.size - equalsIndex - 1);
	return true;
}

constexpr u32 MAX_META_FLAGS = 8 * sizeof(ReflexMetaFlags);
static String gMetaFlagNames[MAX_META_FLAGS];
static u32 gMetaFlagCount = 0;

static ReflexMetaFlags FindMetaFlag(String name)
{
	for (u32 i = 0; i < gMetaFlagCount; ++i) {
		if (StrEq(gMetaFlagNames[i], name)) {
			return (ReflexMetaFlags)1 << i;
		}
	}
	return 0;
}

static ReflexMetaFlags MakeMetaFlag(Arena &arena, String name)
{
	ReflexMetaFlags flag = FindMetaFlag(name);
	if (!flag) {
		if (gMetaFlagCount < MAX_META_FLAGS) {
			gMetaFlagNames[gMetaFlagCount] = MakeString(PushString(arena, name), name.size);
			flag = (ReflexMetaFlags)1 << gMetaFlagCount++;
		} else {
			LOG(Error, "Reflex: too many meta flags (%u), <%.*s> is dropped\n", MAX_META_FLAGS, StringPrintfArgs(name));
		}
	}
	return flag;
}

static ReflexMetaArg MakeReflexMetaArg(Arena &arena, String name, String value)
{
	ReflexMetaArg arg = {
		.name = MakeString(PushString(arena, name), name.size),
	};

	const char *valueStr = PushString(arena, value);
	if (value.size >= 2 && value.str[0] == '"' && value.str[value.size - 1] == '"') {
		const String unquoted = MakeString(value.str + 1, value.size - 2);
		arg.type = ReflexMetaArg_String;
		arg.stringValue = MakeString(PushString(arena, unquoted), unquoted.size);
	} else if (StrIsInteger(valueStr)) {
		arg.type = ReflexMetaArg_Int;
		arg.intValue = StrToInt(valueStr);
	} else if (StrIsFloat(valueStr)) {
		arg.type = ReflexMetaArg_Float;
		arg.floatValue = StrToFloat(valueStr);
	} else {
		arg.type = ReflexMetaArg_String;
		arg.stringValue = MakeString(valueStr, value.size);
	}

	return arg;
}

static ReflexMeta MakeReflexMeta(Arena &arena, String metaString)
{
	ReflexMeta meta = {};

	constexpr u32 MAX_META_ARGS = 16;
	String tokens[MAX_META_ARGS];
	u32 tokenCount = 0;
	if (metaString.size > 0) {
		StrSplit(metaString, ',', tokens, tokenCount, MAX_META_ARGS);
	}

	ReflexMetaArg *args = PushArray(arena, ReflexMetaArg, tokenCount);
	u8 argCount = 0;

	for (u32 i = 0; i < tokenCount; ++i)
	{
		const String &token = tokens[i];
		if (token.size == 0) {
			continue;
		}

		String key, value;
		if (SplitMetaKeyValue(token, key, value)) {
			args[argCount++] = MakeReflexMetaArg(arena, key, value);
		} else {
			meta.flags |= MakeMetaFlag(arena, token);
		}
	}

	meta.args = args;
	meta.argCount = argCount;

	return meta;
}

static String ReflexMetaToString(Arena &arena, const ReflexMeta &meta)
{
	char buffer[1024] = {};

	for (u32 i = 0; i < gMetaFlagCount; ++i) {
		if (meta.flags & ((ReflexMetaFlags)1 << i)) {
			if (buffer[0]) StrCat(buffer, ", ");
			StrCat(buffer, gMetaFlagNames[i]);
		}
	}

	for (u32 i = 0; i < meta.argCount; ++i) {
		if (buffer[0]) StrCat(buffer, ", ");
		const ReflexMetaArg &arg = meta.args[i];
		StrCat(buffer, arg.name);
		StrCat(buffer, "=");
		char number[64];
		if (arg.type == ReflexMetaArg_Int) {
			SPrintf(number, "%d", arg.intValue);
			StrCat(buffer, number);
		} else if (arg.type == ReflexMetaArg_Float) {
			SPrintf(number, "%g", arg.floatValue);
			StrCat(buffer, number);
		} else {
			StrCat(buffer, arg.stringValue);
		}
	}

	String res = MakeString(PushString(arena, buffer));
	return res;
}

static void PrintReflexMetaFlags(ReflexMetaFlags flags)
{
	if (flags == 0) {
		printf("ReflexMetaFlag_None");
		return;
	}

	bool first = true;
	for (u32 i = 0; i < gMetaFlagCount; ++i) {
		if (flags & ((ReflexMetaFlags)1 << i)) {
			printf("%sReflexMetaFlag_%.*s", first ? "" : " | ", StringPrintfArgs(gMetaFlagNames[i]));
			first = false;
		}
	}
}

static void PrintReflexMetaArgs(const char *argsName, const ReflexMeta &meta)
{
	if (meta.argCount == 0) {
		return;
	}

	printf("\n");
	printf("// ReflexMetaArg info\n");
	printf("static const ReflexMetaArg reflexMetaArgs_%s[] = {\n", argsName);
	for (u32 i = 0; i < meta.argCount; ++i)
	{
		const ReflexMetaArg &arg = meta.args[i];
		printf("  { .name = MakeString(\"%.*s\"), ", StringPrintfArgs(arg.name));
		if (arg.type == ReflexMetaArg_Int) {
			printf(".type = ReflexMetaArg_Int, .intValue = %d },\n", arg.intValue);
		} else if (arg.type == ReflexMetaArg_Float) {
			printf(".type = ReflexMetaArg_Float, .floatValue = (f32)%.9g },\n", arg.floatValue);
		} else {
			printf(".type = ReflexMetaArg_String, .stringValue = MakeString(\"%.*s\") },\n", StringPrintfArgs(arg.stringValue));
		}
	}
	printf("};\n");
}

static void PrintReflexMeta(const char *argsName, const ReflexMeta &meta, Arena &arena)
{
	const String metaString = ReflexMetaToString(arena, meta);
	if (metaString.size > 0) {
		printf("/* %.*s */ ", StringPrintfArgs(metaString));
	}
	printf(".flags = ");
	PrintReflexMetaFlags(meta.flags);
	if (meta.argCount > 0) {
		printf(", .args = reflexMetaArgs_%s, .argCount = %u", argsName, (u32)meta.argCount);
	} else {
		printf(", .args = NULL, .argCount = 0");
	}
}

static bool MakeReflexMember(Arena &arena, const CastStructDeclaration *structDeclaration, ReflexMember &member)
{
	const CastTag *tag = structDeclaration->tag;
	const CastSpecifierQualifierList *specifierQualifierList = structDeclaration->specifierQualifierList;
	const CastDeclarator *declarator = CAST_CHILD2(structDeclaration, structDeclaratorList, structDeclarator);
	const CastDirectDeclarator *directDeclarator = CAST_CHILD(declarator, directDeclarator);

	member = {};
	member.name = directDeclarator ? PushString(arena, directDeclarator->name) : "<none>";
	member.isConst = specifierQualifierList && specifierQualifierList->typeQualifier &&
		specifierQualifierList->typeQualifier->type == CAST_CONST;
	member.isArray = directDeclarator && directDeclarator->isArray;
	member.arrayDim = directDeclarator && directDeclarator->expression ? Cast_EvaluateInt(directDeclarator->expression) : 0;
	for (const CastPointer *pointer = CAST_CHILD(declarator, pointer); pointer; pointer = pointer->next) {
		member.pointerCount++;
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
	const char *identifier = NULL;

	const CastSpecifierQualifierList *specifierList = specifierQualifierList;
	while (specifierList)
	{
		const CastTypeSpecifier *typeSpecifier = specifierList->typeSpecifier;
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
				identifier = PushString(arena, typeSpecifier->identifier); break;
			} else if (typeSpecifier->type == CAST_INT) {
				isInt = true;
			} else if (typeSpecifier->type == CAST_UNSIGNED) {
				isUnsigned = true;
			} else if (typeSpecifier->type == CAST_SHORT) {
				isShort = true;
			} else if (typeSpecifier->type == CAST_LONG) {
				isLongLong = isLong; isLong = true;
			} else {
				break;
			}
		}
		specifierList = specifierList->next;
	}

	ReflexID reflexId = ReflexID_Null;
	if (isVoid) {
		reflexId = ReflexID_Void;
	} else if (isBool) {
		reflexId = ReflexID_Bool;
	} else if (isChar) {
		if (isUnsigned) reflexId = ReflexID_UnsignedChar;
		else reflexId = ReflexID_Char;
	} else if (isInt) {
		if (isUnsigned) {
			if (isLongLong) reflexId = ReflexID_UnsignedLongLongInt;
			else if (isLong) reflexId = ReflexID_UnsignedLongInt;
			else if (isShort) reflexId = ReflexID_UnsignedShortInt;
			else reflexId = ReflexID_UnsignedInt;
		} else {
			if (isLongLong) reflexId = ReflexID_LongLongInt;
			else if (isLong) reflexId = ReflexID_LongInt;
			else if (isShort) reflexId = ReflexID_ShortInt;
			else reflexId = ReflexID_Int;
		}
	} else if (isFloat) {
		reflexId = ReflexID_Float;
	} else if (isDouble) {
		reflexId = ReflexID_Double;
	}

	member.reflexId = reflexId;
	member.typeName = ReflexIsTrivial(reflexId) ? ReflexGetTrivial(reflexId)->name : identifier;

	if ( tag ) {
		member.meta = MakeReflexMeta(arena, tag->arguments);
	}

	return member.typeName != NULL;
}

static void PrintReflexID(const ReflexMember &member)
{
	if (ReflexIsTrivial(member.reflexId)) {
		printf("%s", TrivialReflexIDNames[member.reflexId]);
	} else {
		printf("ReflexID_%s", member.typeName);
	}
}

static const char *OpsTypeName(const ReflexMember &member)
{
	if (member.pointerCount > 0) return "Pointer";
	if (member.isArray) return "Array";
	if (ReflexIsTrivial(member.reflexId)) return TrivialReflexIDNames[member.reflexId] + sizeof("ReflexID_") - 1;
	if (ReflexIsStruct(member.reflexId)) return "Struct";
	if (ReflexIsEnum(member.reflexId)) return "Enum";
	return member.typeName;
}

static void MakeComponentNames(const char *structName, char *typeName, char *fieldName)
{
	StrCopy(typeName, structName);

	const u32 length = StrLen(typeName);
	const u32 suffixLength = StrLen("Component");
	if (length > suffixLength && StrEq(typeName + length - suffixLength, "Component")) {
		typeName[length - suffixLength] = 0;
	}

	StrCopy(fieldName, typeName);
	if (fieldName[0] >= 'A' && fieldName[0] <= 'Z') {
		fieldName[0] = (char)(fieldName[0] - 'A' + 'a');
	}
}

static void PrintMemberDeclaration(const ReflexMember &member)
{
	printf("	%s%s ", member.isConst ? "const " : "", member.typeName);
	for (u32 i = 0; i < member.pointerCount; ++i) {
		printf("*");
	}
	printf("%s", member.name);
	if (member.isArray) {
		printf("[%u]", (u32)member.arrayDim);
	}
	printf(";\n");
}

constexpr u32 MAX_STRUCT_MEMBERS = 128;
void GetStructMemberDeclarations(const CastStructSpecifier *cstruct, const CastStructDeclaration* structDeclarations[MAX_STRUCT_MEMBERS], u32 *structDeclarationCount)
{
	ASSERT(structDeclarationCount != nullptr);
	u32 &declarationCount = *structDeclarationCount;

	// Only members tagged with the field tag macro are reflected
	declarationCount = 0;
	const CastStructDeclarationList *structDeclarationList = cstruct->structDeclarationList;
	while (structDeclarationList) {
		const CastStructDeclaration *structDeclaration = structDeclarationList->structDeclaration;
		if (structDeclaration && structDeclaration->tag) {
			ASSERT(declarationCount < MAX_STRUCT_MEMBERS);
			structDeclarations[declarationCount++] = structDeclaration;
		}
		structDeclarationList = structDeclarationList->next;
	}
}

static bool GenerateReflex(const Cast *cast, Arena &arena)
{
	const CastStructSpecifier *structs[128];
	ReflexMeta structMetas[ARRAY_COUNT(structs)];
	u32 structCount = 0;

	const CastStructDeclaration *structDeclarations[MAX_STRUCT_MEMBERS]; // members
	u32 structDeclarationCount = 0;

	const CastEnumSpecifier *enums[128];
	u32 enumCount = 0;

	String functions[128];
	String functionScripts[128];
	u32 functionCount = 0;

	// Get all the global struct and enum specifiers from the AST.
	// Only structs and enums tagged with their tag macro are reflected.
	const CastTranslationUnit *translationUnit = cast->translationUnit;
	while (translationUnit)
	{
		if (translationUnit->externalDeclaration)
		{
			if (translationUnit->externalDeclaration->declaration &&
				translationUnit->externalDeclaration->declaration->declarationSpecifiers &&
				translationUnit->externalDeclaration->declaration->declarationSpecifiers->typeSpecifier)
			{
				const CastTypeSpecifier *typeSpecifier =
					translationUnit->externalDeclaration->declaration->declarationSpecifiers->typeSpecifier;

				if (typeSpecifier->type == CAST_STRUCT && typeSpecifier->structSpecifier && typeSpecifier->structSpecifier->tag) {
					ASSERT(structCount < ARRAY_COUNT(structs));
					structMetas[structCount] = MakeReflexMeta(arena, typeSpecifier->structSpecifier->tag->arguments);
					structs[structCount++] = typeSpecifier->structSpecifier;
				} else if (typeSpecifier->type == CAST_ENUM && typeSpecifier->enumSpecifier && typeSpecifier->enumSpecifier->tag) {
					ASSERT(enumCount < ARRAY_COUNT(enums));
					enums[enumCount++] = typeSpecifier->enumSpecifier;
				}
			}
			else if (translationUnit->externalDeclaration->functionDefinition)
			{
				const CastFunctionDefinition *func = translationUnit->externalDeclaration->functionDefinition;
				const bool isVoid = func->declarationSpecifiers &&
					func->declarationSpecifiers->typeSpecifier &&
					func->declarationSpecifiers->typeSpecifier->type == CAST_VOID;

				const CastParameterTypeList *parameterTypeList = func->declarator->directDeclarator->parameterTypeList;
				const CastParameterDeclaration *parameterDeclaration = (parameterTypeList && parameterTypeList->parameterList) ?
					parameterTypeList->parameterList->parameterDeclaration : NULL;
				const CastTypeSpecifier *paramTypeSpecifier = (parameterDeclaration && parameterDeclaration->declarationSpecifiers) ?
					parameterDeclaration->declarationSpecifiers->typeSpecifier : NULL;

				// A script parameter is a struct previously tagged with REFLEX(Script)
				bool hasScriptParameter = false;
				if (paramTypeSpecifier && paramTypeSpecifier->type == CAST_IDENTIFIER)
				{
					const String paramTypeName = paramTypeSpecifier->identifier;
					const ReflexMetaFlags scriptFlag = FindMetaFlag(MakeString("Script"));
					for (u32 i = 0; i < structCount && !hasScriptParameter; ++i) {
						hasScriptParameter = StrEq(structs[i]->name, paramTypeName) &&
							(structMetas[i].flags & scriptFlag);
					}
				}

				if (isVoid && hasScriptParameter)
				{
					ASSERT(functionCount < ARRAY_COUNT(functions));
					const u32 functionIndex = functionCount++;
					functions[functionIndex] = func->declarator->directDeclarator->name;
					functionScripts[functionIndex] = paramTypeSpecifier->identifier;
				}
			}
		}
		translationUnit = translationUnit->next;
	}

	// Reflex structs
	ReflexStruct reflexStructs[2 * ARRAY_COUNT(structs)]; // Room for a descriptor per struct
	u32 reflexStructCount = structCount;

	for (u32 index = 0; index < structCount; ++index)
	{
		const CastStructSpecifier *cstruct = structs[index];

		GetStructMemberDeclarations(cstruct, structDeclarations, &structDeclarationCount);

		ReflexMember *members = PushArray(arena, ReflexMember, structDeclarationCount);
		for (u32 memberIndex = 0; memberIndex < structDeclarationCount; ++memberIndex)
		{
			ReflexMember &member = members[memberIndex];
			if (!MakeReflexMember(arena, structDeclarations[memberIndex], member)) {
				LOG(Error, "Reflex: the type of <%.*s::%s> cannot be reflected\n", StringPrintfArgs(cstruct->name), member.name);
				return false;
			}

			// The real ID of a reflected type only exists in the consumer of the generated code
			// Anyway, we just identify here if these types are structs or enums
			if (member.reflexId == ReflexID_Null) {
				for (u32 i = 0; i < structCount; ++i) {
					if (StrEq(structs[i]->name, member.typeName)) member.reflexId = ReflexID_StructBegin;
				}
				for (u32 i = 0; i < enumCount; ++i) {
					if (StrEq(enums[i]->name, member.typeName)) member.reflexId = ReflexID_EnumBegin;
				}
			}
		}

		reflexStructs[index] = {
			.name = PushString(arena, cstruct->name),
			.members = members,
			.memberCount = (u16)structDeclarationCount,
			.meta = structMetas[index],
		};
	}

	// Reflex enums
	ReflexEnum reflexEnums[ARRAY_COUNT(enums)];

	for (u32 index = 0; index < enumCount; ++index)
	{
		const CastEnumSpecifier *cenum = enums[index];

		ReflexMeta meta = {};
		if (cenum->tag) {
			meta = MakeReflexMeta(arena, cenum->tag->arguments);
		}

		// The last enumerator of an enum tagged REFLEX(Count) counts the others, so it
		// is left out: it is no value the enum can hold
		u32 enumeratorCount = 0;
		for (const CastEnumeratorList *it = CAST_CHILD(cenum, enumeratorList); it; it = it->next) {
			if (CAST_CHILD(it, enumerator)) enumeratorCount++;
		}
		if ((meta.flags & FindMetaFlag(MakeString("Count"))) && enumeratorCount > 0) {
			enumeratorCount--;
		}

		ReflexEnumerator *enumerators = PushArray(arena, ReflexEnumerator, enumeratorCount);
		i32 enumeratorValue = 0;
		const CastEnumeratorList *enumeratorList = CAST_CHILD(cenum, enumeratorList);
		while (enumeratorList && (u32)enumeratorValue < enumeratorCount) {
			const CastEnumerator *enumerator = CAST_CHILD(enumeratorList, enumerator);
			if (enumerator) {
				enumerators[enumeratorValue] = {
					.name = PushString(arena, enumerator->name),
					.value = enumeratorValue,
				};
				enumeratorValue++;
			}
			enumeratorList = enumeratorList->next;
		}

		reflexEnums[index] = {
			.name = PushString(arena, cenum->name),
			.enumerators = enumerators,
			.enumeratorCount = (u16)enumeratorCount,
			.meta = meta,
		};
	}

	// Components, in the order their structs appear in the parsed files
	u32 componentIndices[ARRAY_COUNT(structs)];
	const char *componentTypeNames[ARRAY_COUNT(structs)];
	const char *componentFieldNames[ARRAY_COUNT(structs)];
	u32 componentCount = 0;

	const ReflexMetaFlags componentFlag = FindMetaFlag(MakeString("Component"));
	for (u32 index = 0; index < structCount; ++index)
	{
		const ReflexStruct &reflexStruct = reflexStructs[index];
		if (reflexStruct.meta.flags & componentFlag)
		{
			char typeName[128];
			char fieldName[128];
			MakeComponentNames(reflexStruct.name, typeName, fieldName);

			componentIndices[componentCount] = index;
			componentTypeNames[componentCount] = PushString(arena, typeName);
			componentFieldNames[componentCount] = PushString(arena, fieldName);
			componentCount++;
		}
	}

	// The descriptor printed for each component is reflected too. It shares the component's
	// members, since their offsets are only printed later, from the name of each struct.
	// A component with no properties has nothing to describe, so it gets no descriptor.
	for (u32 index = 0; index < componentCount; ++index)
	{
		const ReflexStruct &component = reflexStructs[componentIndices[index]];
		if (component.memberCount > 0)
		{
			char descName[128];
			SPrintf(descName, "%sDesc", component.name);

			ASSERT(reflexStructCount < ARRAY_COUNT(reflexStructs));
			ReflexStruct &desc = reflexStructs[reflexStructCount++];
			desc = component;
			desc.name = PushString(arena, descName);
		}
	}

	if (reflexStructCount > REFLEX_MAX_STRUCTS || enumCount > REFLEX_MAX_ENUMS) {
		LOG(Error, "Reflex: too many types, %u structs (max %u) and %u enums (max %u)\n", reflexStructCount, REFLEX_MAX_STRUCTS, enumCount, REFLEX_MAX_ENUMS);
		return false;
	}

	// Reflected members can point to not reflected types declared in other files.
	// These are opaque and we call them custom types
	const char *customTypeNames[128];
	bool customTypeIsValue[128];
	u32 customTypeCount = 0;

	for (u32 index = 0; index < structCount; ++index)
	{
		const ReflexStruct &reflexStruct = reflexStructs[index];

		for (u32 memberIndex = 0; memberIndex < reflexStruct.memberCount; ++memberIndex)
		{
			const ReflexMember &member = reflexStruct.members[memberIndex];
			if (member.reflexId != ReflexID_Null) {
				continue; // Trivial or reflected struct/enum
			}

			const char *typeName = member.typeName;
			const bool isValue = member.pointerCount == 0;

			u32 customIndex = 0;
			while (customIndex < customTypeCount && !StrEq(customTypeNames[customIndex], typeName)) {
				customIndex++;
			}
			if (customIndex == customTypeCount) {
				ASSERT(customTypeCount < ARRAY_COUNT(customTypeNames));
				customTypeNames[customTypeCount] = typeName;
				customTypeIsValue[customTypeCount] = false;
				customTypeCount++;
			}
			customTypeIsValue[customIndex] = customTypeIsValue[customIndex] || isValue;
		}
	}

	printf("\n");
	printf("#ifdef REFLEX_GENERATED_CUSTOM_TYPES\n");

	printf("\n");
	printf("////////////////////////////////////////////////////////////////////////\n");
	printf("// Include this file with REFLEX_GENERATED_CUSTOM_TYPES before reflex.h\n");
	printf("\n");

	if (reflexStructCount > 0)
	{
		printf("#define REFLEX_ID_STRUCT_TYPES \\\n");
		for (u32 index = 0; index < reflexStructCount; ++index)
		{
			printf("	ReflexID_%s%s, \\\n", reflexStructs[index].name, index == 0 ? " = ReflexID_StructBegin" : "");
		}
		printf("\n");
	}

	if (enumCount > 0)
	{
		printf("#define REFLEX_ID_ENUM_TYPES \\\n");
		for (u32 index = 0; index < enumCount; ++index)
		{
			printf("	ReflexID_%s%s, \\\n", reflexEnums[index].name, index == 0 ? " = ReflexID_EnumBegin" : "");
		}
		printf("\n");
	}

	if (customTypeCount > 0)
	{
		printf("#define REFLEX_ID_CUSTOM_TYPES \\\n");
		for (u32 index = 0; index < customTypeCount; ++index)
		{
			printf("	ReflexID_%s%s, \\\n", customTypeNames[index], index == 0 ? " = ReflexID_CustomBegin" : "");
		}
	}

	printf("\n");
	printf("#undef REFLEX_GENERATED_CUSTOM_TYPES\n");
	printf("#endif // REFLEX_GENERATED_CUSTOM_TYPES\n");

	printf("\n");
	printf("#ifdef REFLEX_GENERATED_DECLARATION\n");

	printf("\n");
	printf("////////////////////////////////////////////////////////////////////////\n");
	printf("// Include this file with REFLEX_GENERATED_DECLARATION once every type used by a\n");
	printf("// component's members (including ones reflected further down the same file) is declared\n");
	printf("\n");

	printf("\n");
	printf("////////////////////////////////////////////////////////////////////////\n");
	printf("// Meta flags, one per tag found within REFLEX(...)\n");
	printf("\n");

	printf("enum ReflexMetaFlagBits : u64\n");
	printf("{\n");
	printf("	ReflexMetaFlag_None = 0,\n");
	for (u32 index = 0; index < gMetaFlagCount; ++index)
	{
		printf("	ReflexMetaFlag_%.*s = (u64)1 << %u,\n", StringPrintfArgs(gMetaFlagNames[index]), index);
	}
	printf("};\n");
	printf("\n");

	if (componentCount > 0)
	{
		printf("\n");
		printf("////////////////////////////////////////////////////////////////////////\n");
		printf("// Components, in the order their structs are declared. Saved data refers to\n");
		printf("// them by these values, so a component that already exists cannot be moved.\n");
		printf("\n");

		printf("enum ComponentType\n");
		printf("{\n");
		for (u32 index = 0; index < componentCount; ++index)
		{
			printf("	ComponentType_%s,\n", componentTypeNames[index]);
		}
		printf("	ComponentType_Count,\n");
		printf("};\n");
		printf("\n");

		printf("enum ComponentBits\n");
		printf("{\n");
		for (u32 index = 0; index < componentCount; ++index)
		{
			printf("	Component_%s = 1 << ComponentType_%s,\n", componentTypeNames[index], componentTypeNames[index]);
		}
		printf("};\n");
		printf("\n");

		printf("constexpr const char *ComponentNames[] = {\n");
		for (u32 index = 0; index < componentCount; ++index)
		{
			printf("	\"%s\",\n", componentTypeNames[index]);
		}
		printf("};\n");
		printf("\n");

		printf("// As they appear in the entity blocks of the asset files, e.g. \".light = {...}\"\n");
		printf("constexpr const char *ComponentFieldNames[] = {\n");
		for (u32 index = 0; index < componentCount; ++index)
		{
			printf("	\"%s\",\n", componentFieldNames[index]);
		}
		printf("};\n");
		printf("\n");
	}

	printf("\n");
	printf("////////////////////////////////////////////////////////////////////////\n");
	printf("// Component descriptors\n");
	printf("\n");

	for (u32 index = 0; index < componentCount; ++index)
	{
		const ReflexStruct &reflexStruct = reflexStructs[componentIndices[index]];

		if (reflexStruct.memberCount > 0)
		{
			printf("struct %sDesc {\n", reflexStruct.name);
			for (u32 memberIndex = 0; memberIndex < reflexStruct.memberCount; ++memberIndex)
			{
				PrintMemberDeclaration(reflexStruct.members[memberIndex]);
			}
			printf("};\n");
			printf("\n");
		}
	}

	printf("\n");
	printf("#undef REFLEX_GENERATED_DECLARATION\n");
	printf("#endif // REFLEX_GENERATED_DECLARATION\n");

	printf("\n");
	printf("\n");
	printf("#ifdef REFLEX_GENERATED_IMPLEMENTATION\n");

	if (customTypeCount > 0)
	{
		printf("\n");
		printf("////////////////////////////////////////////////////////////////////////\n");
		printf("// Custom Types: used by reflected members but not reflected themselves\n");

		for (u32 index = 0; index < customTypeCount; ++index)
		{
			const char *typeName = customTypeNames[index];

			printf("\n");
			printf("// ReflexCustom info\n");
			printf("static const ReflexCustom reflexCustom_%s =\n", typeName);
			printf("{\n");
			printf("  .name = \"%s\",\n", typeName);
			printf("  .size = sizeof(%s),\n", typeName);
			printf("};\n");

			printf("\n");
			printf("// ReflexCustom registration\n");
			printf("static const ReflexID ReflexIDStub_%s = ReflexRegisterCustom(&reflexCustom_%s, ReflexID_%s);\n", typeName, typeName, typeName);
			printf("\n");
		}
	}

	// Enums
	printf("\n");
	for (u32 index = 0; index < enumCount; ++index)
	{
		const ReflexEnum &reflexEnum = reflexEnums[index];

		printf("\n");
		printf("////////////////////////////////////////////////////////////////////////\n");
		printf("// enum %s\n", reflexEnum.name);

		printf("\n");
		printf("// ReflexEnumerator info\n");
		printf("static const ReflexEnumerator reflexEnumerators_%s[] = {\n", reflexEnum.name);
		for (u32 enumeratorIndex = 0; enumeratorIndex < reflexEnum.enumeratorCount; ++enumeratorIndex) {
			const ReflexEnumerator &enumerator = reflexEnum.enumerators[enumeratorIndex];
			printf("  { ");
			printf(".name = \"%s\", ", enumerator.name);
			printf(".value = %d, ", enumerator.value);
			printf("},\n");
		}
		printf("};\n");

		PrintReflexMetaArgs(reflexEnum.name, reflexEnum.meta);

		printf("\n");
		printf("// ReflexEnum info\n");
		printf("static const ReflexEnum reflexEnum_%s =\n", reflexEnum.name);
		printf("{\n");
		printf("  .name = \"%s\",\n", reflexEnum.name);
		printf("  .enumerators = reflexEnumerators_%s,\n", reflexEnum.name);
		printf("  .enumeratorCount = ARRAY_COUNT(reflexEnumerators_%s),\n", reflexEnum.name);
		printf("  .meta = { ");
		PrintReflexMeta(reflexEnum.name, reflexEnum.meta, arena);
		printf(" },\n");
		printf("};\n");

		printf("\n");
		printf("// ReflexEnum registration\n");
		printf("static const ReflexID ReflexIDStub_%s = ReflexRegisterEnum(&reflexEnum_%s, ReflexID_%s);\n", reflexEnum.name, reflexEnum.name, reflexEnum.name);
		printf("\n");
	}

	// Structs
	for (u32 index = 0; index < reflexStructCount; ++index)
	{
		const ReflexStruct &reflexStruct = reflexStructs[index];
		const char *structName = reflexStruct.name;

		printf("\n");
		printf("////////////////////////////////////////////////////////////////////////\n");
		printf("// struct %s\n", structName);

		if (reflexStruct.memberCount > 0)
		{
			// The args array of a member's meta must be printed before reflexMembers_%s[],
			// since it can't be declared inline within that array's initializer
			for (u32 memberIndex = 0; memberIndex < reflexStruct.memberCount; ++memberIndex)
			{
				const ReflexMember &member = reflexStruct.members[memberIndex];
				char argsName[256];
				SPrintf(argsName, "%s_%s", structName, member.name);
				PrintReflexMetaArgs(argsName, member.meta);
			}

			printf("\n");
			printf("// ReflexMember info\n");
			printf("static const ReflexMember reflexMembers_%s[] = {\n", structName);
		}

		for ( u32 memberIndex = 0; memberIndex < reflexStruct.memberCount; ++memberIndex)
		{
			const ReflexMember &member = reflexStruct.members[memberIndex];
			char argsName[256];
			SPrintf(argsName, "%s_%s", structName, member.name);

			printf("  { ");
			printf(".name = \"%s\", ", member.name);
			printf(".typeName = \"%s\", ", member.typeName);
			printf(".ops = REFLEX_OPS(%s), ", OpsTypeName(member));
			printf(".isConst = %s, ", member.isConst ? "true" : "false");
			printf(".pointerCount = %u, ", (u32)member.pointerCount);
			printf(".isArray = %s, ", member.isArray ? "true" : "false");
			printf(".arrayDim = %u, ", (u32)member.arrayDim);
			printf(".reflexId = ");
			PrintReflexID(member);
			printf(", ");
			printf(".offset = OFFSET_OF(%s, %s), ", structName, member.name);
			printf(".meta = { ");
			PrintReflexMeta(argsName, member.meta, arena);
			printf(" } ");
			printf("},\n");
		}

		if (reflexStruct.memberCount > 0)
		{
			printf("};\n");
		}

		PrintReflexMetaArgs(structName, reflexStruct.meta);

		printf("\n");
		printf("// ReflexStruct info\n");
		printf("static const ReflexStruct reflexStruct_%s =\n", structName);
		printf("{\n");
		printf("  .name = \"%s\",\n", structName);
		if (reflexStruct.memberCount > 0) {
			printf("  .members = reflexMembers_%s,\n", structName);
			printf("  .memberCount = ARRAY_COUNT(reflexMembers_%s),\n", structName);
		} else {
			// A tagged struct with no tagged members: an empty array is not valid C++
			printf("  .members = NULL,\n");
			printf("  .memberCount = 0,\n");
		}
		printf("  .size = sizeof(%s),\n", structName);
		printf("  .meta = { ");
		PrintReflexMeta(structName, reflexStruct.meta, arena);
		printf(" },\n");
		printf("};\n");

		printf("\n");
		printf("// ReflexStruct registration\n");
		printf("static const ReflexID ReflexIDStub_%s = ReflexRegisterStruct(&reflexStruct_%s, ReflexID_%s);\n", structName, structName, structName);
		printf("\n");
	}

	// Functions
	printf("\n");
	printf("////////////////////////////////////////////////////////////////////////\n");
	printf("// Functions\n");

	for (u32 index = 0; index < functionCount; ++index)
	{
		String functionName = functions[index];
		String scriptName = functionScripts[index];
		printf("\n");
		printf("static void %.*s_%.*s(void *instance) {\n", StringPrintfArgs(scriptName), StringPrintfArgs(functionName));
		printf("	%.*s(*(%.*s*)instance);\n", StringPrintfArgs(functionName), StringPrintfArgs(scriptName));
		printf("}\n");
		printf("static const ReflexFunction reflexFunction_%.*s_%.*s = { \"%.*s\", \"%.*s\", %.*s_%.*s };\n",
				StringPrintfArgs(scriptName), StringPrintfArgs(functionName),
				StringPrintfArgs(scriptName), StringPrintfArgs(functionName),
				StringPrintfArgs(scriptName), StringPrintfArgs(functionName));
		printf("static const ReflexID ReflexIDStub_%.*s_%.*s = ReflexRegisterFunction(&reflexFunction_%.*s_%.*s);\n",
				StringPrintfArgs(scriptName), StringPrintfArgs(functionName),
				StringPrintfArgs(scriptName), StringPrintfArgs(functionName));
	}

	printf("\n");
	printf("#undef REFLEX_GENERATED_IMPLEMENTATION\n");
	printf("#endif // REFLEX_GENERATED_IMPLEMENTATION\n\n");

	return true;
}

int main(int argc, char **argv)
{
	if (argc < 2 )
	{
		LOG(Info, "Usage: %s <c file> [<c file>...]\n", argv[0]);
		return -1;
	}

	u32 globalArenaSize = MB(4);
	byte *globalArenaBase = (byte*)AllocateVirtualMemory(globalArenaSize);
	Arena globalArena = MakeArena(globalArenaBase, globalArenaSize, "globalArena");

	Cast cast = {};

	for (int argIndex = 1; argIndex < argc; ++argIndex)
	{
		const char *filename = argv[argIndex];

		u64 fileSize;
		if ( !GetFileSize(filename, fileSize) || fileSize == 0 )
		{
			LOG(Error, "GetFileSize() failed reading %s\n", filename);
			return -1;
		}

		char* bytes = PushArray(globalArena, char, fileSize + 1);
		if ( !ReadEntireFile(filename, bytes, fileSize) )
		{
			LOG(Error, "ReadEntireFile() failed reading %s\n", filename);
			return -1;
		}
		bytes[fileSize] = 0;

		if ( !Cast_Append(globalArena, cast, bytes, fileSize, castConfig) )
		{
			LOG(Error, "Cast_Append() failed:\n");
			LOG(Error, "- file: %s\n", filename);
			LOG(Error, "- message: %s\n", Cast_GetError());
			return -1;
		}
	}

	if ( !GenerateReflex(&cast, globalArena) )
	{
		return -1;
	}

	return 0;
}

