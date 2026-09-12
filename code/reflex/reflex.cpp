#define CAST_IMPLEMENTATION
#include "cast.h"
#include "../ilu_core.h"
#include "reflex.h"

#define StringPrintfArgs(string) string.size, string.str

// Macros used in the parsed files to tag reflected structs, properties and enums
static const CastConfig castConfig = {
	.structTag = "ILU_STRUCT",
	.fieldTag = "ILU_PROPERTY",
	.enumTag = "ILU_ENUM",
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

// Fills the member the way the generated code will. A type named by an identifier keeps
// ReflexID_Null: its ReflexID only exists in the compiled code, so it is printed by name.
// Returns false when the type cannot be named at all.
static bool MakeReflexMember(Arena &arena, const CastStructDeclaration *structDeclaration, ReflexMember &member)
{
	const CastTag *tag = structDeclaration->tag;
	const CastSpecifierQualifierList *specifierQualifierList = structDeclaration->specifierQualifierList;
	const CastDeclarator *declarator = CAST_CHILD2(structDeclaration, structDeclaratorList, structDeclarator);
	const CastDirectDeclarator *directDeclarator = CAST_CHILD(declarator, directDeclarator);

	member = {};
	member.name = directDeclarator ? PushString(arena, directDeclarator->name) : "<none>";
	member.hint = tag && tag->arguments.size > 0 ? PushString(arena, tag->arguments) : NULL;
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

// A component's names come from its struct name: LightComponent gives ComponentType_Light,
// the name "Light" and the ".light" field the asset files use
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

// Prints the member back as a C declaration, e.g. "const char *name;"
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

				// A script parameter is a struct previously tagged with ILU_STRUCT(Script)
				bool hasScriptParameter = false;
				if (paramTypeSpecifier && paramTypeSpecifier->type == CAST_IDENTIFIER)
				{
					const String paramTypeName = paramTypeSpecifier->identifier;
					for (u32 i = 0; i < structCount && !hasScriptParameter; ++i) {
						hasScriptParameter = StrEq(structs[i]->name, paramTypeName) &&
							structs[i]->tag && StrEq(structs[i]->tag->arguments, "Script");
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
			.hint = cstruct->tag->arguments.size > 0 ? PushString(arena, cstruct->tag->arguments) : NULL,
			.members = members,
			.memberCount = (u16)structDeclarationCount,
		};
	}

	// Components, in the order their structs appear in the parsed files
	u32 componentIndices[ARRAY_COUNT(structs)];
	const char *componentTypeNames[ARRAY_COUNT(structs)];
	const char *componentFieldNames[ARRAY_COUNT(structs)];
	u32 componentCount = 0;

	for (u32 index = 0; index < structCount; ++index)
	{
		const ReflexStruct &reflexStruct = reflexStructs[index];
		if (reflexStruct.hint && StrEq(reflexStruct.hint, "Component"))
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
			desc.hint = NULL;
		}
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
	printf("#ifdef REFLEX_GENERATED_DECLARATION\n");

	printf("\n");
	printf("////////////////////////////////////////////////////////////////////////\n");
	printf("// Include this file with REFLEX_GENERATED_DECLARATION before reflex.h\n");
	printf("\n");

	if (customTypeCount > 0)
	{
		printf("#define REFLEX_ID_CUSTOM_TYPES \\\n");
		for (u32 index = 0; index < customTypeCount; ++index)
		{
			printf("	ReflexID_%s, \\\n", customTypeNames[index]);
		}
	}

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
		const CastEnumSpecifier *cenum = enums[index];

		printf("\n");
		printf("////////////////////////////////////////////////////////////////////////\n");
		printf("// enum %.*s\n", StringPrintfArgs(cenum->name));

		// The last enumerator of an enum tagged ILU_ENUM(Count) counts the others, so it
		// is left out: it is no value the enum can hold
		u32 enumeratorCount = 0;
		for (const CastEnumeratorList *it = CAST_CHILD(cenum, enumeratorList); it; it = it->next) {
			if (CAST_CHILD(it, enumerator)) enumeratorCount++;
		}
		if (cenum->tag && StrEq(cenum->tag->arguments, "Count") && enumeratorCount > 0) {
			enumeratorCount--;
		}

		printf("\n");
		printf("// ReflexEnumerator info\n");
		printf("static const ReflexEnumerator reflexEnumerators_%.*s[] = {\n", StringPrintfArgs(cenum->name));
		i32 enumeratorValue = 0;
		const CastEnumeratorList *enumeratorList = CAST_CHILD(cenum, enumeratorList);
		while (enumeratorList && (u32)enumeratorValue < enumeratorCount) {
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
			printf("\n");
			printf("// ReflexMember info\n");
			printf("static const ReflexMember reflexMembers_%s[] = {\n", structName);
		}

		for ( u32 memberIndex = 0; memberIndex < reflexStruct.memberCount; ++memberIndex)
		{
			const ReflexMember &member = reflexStruct.members[memberIndex];

			printf("  { ");
			printf(".name = \"%s\", ", member.name);
			if (member.hint) {
				printf(".hint = \"%s\", ", member.hint);
			} else {
				printf(".hint = NULL, ");
			}
			printf(".typeName = \"%s\", ", member.typeName);
			printf(".ops = REFLEX_OPS(%s), ", OpsTypeName(member));
			printf(".isConst = %s, ", member.isConst ? "true" : "false");
			printf(".pointerCount = %u, ", (u32)member.pointerCount);
			printf(".isArray = %s, ", member.isArray ? "true" : "false");
			printf(".arrayDim = %u, ", (u32)member.arrayDim);
			printf(".reflexId = ");
			PrintReflexID(member);
			printf(", ");
			printf(".offset = OFFSET_OF(%s, %s) ", structName, member.name);
			printf("},\n");
		}

		if (reflexStruct.memberCount > 0)
		{
			printf("};\n");
		}

		printf("\n");
		printf("// ReflexStruct info\n");
		printf("static const ReflexStruct reflexStruct_%s =\n", structName);
		printf("{\n");
		printf("  .name = \"%s\",\n", structName);
		if (reflexStruct.hint) {
			printf("  .hint = \"%s\",\n", reflexStruct.hint);
		} else {
			printf("  .hint = NULL,\n");
		}
		if (reflexStruct.memberCount > 0) {
			printf("  .members = reflexMembers_%s,\n", structName);
			printf("  .memberCount = ARRAY_COUNT(reflexMembers_%s),\n", structName);
		} else {
			// A tagged struct with no tagged members: an empty array is not valid C++
			printf("  .members = NULL,\n");
			printf("  .memberCount = 0,\n");
		}
		printf("  .size = sizeof(%s),\n", structName);
		printf("};\n");

		printf("\n");
		printf("// ReflexStruct registration\n");
		printf("static const ReflexID ReflexID_%s = ReflexRegisterStruct(&reflexStruct_%s);\n", structName, structName);
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

