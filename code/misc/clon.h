/*
 * clon.h
 * Author: Jesus Diaz Garcia
 * CLON stands for C-like object notation.
 * It is a text format designed to store data in a descriptive and similar way to the C language.
 */

#ifndef CLON_H
#define CLON_H

#include "../ilu_core.h"

#define CAST_IMPLEMENTATION
#include "cast.h"

#include "reflex.h"

#define CLON_GLOBALS_LIST_SIZE 64

struct ClonGlobal
{
	const char *typeName;
	const char *name;
	void *data;
	u32 elemCount; // Number of elements in the array
};

struct ClonGlobalsList
{
	ClonGlobal globals[CLON_GLOBALS_LIST_SIZE];
	u32 globalsCount;
	ClonGlobalsList *next;
};

struct Clon
{
	ClonGlobalsList *globalsList;
};

void ClonAddGlobal(Clon *clon, Arena *arena, const char *typeName, const char *name, void *data, u32 elemCount)
{
	ClonGlobalsList *globalsList = clon->globalsList;
	if (!globalsList || globalsList->globalsCount == CLON_GLOBALS_LIST_SIZE)
	{
		ClonGlobalsList *prevGlobalsList = globalsList;
		globalsList = PushZeroStruct(*arena, ClonGlobalsList);
		globalsList->next = prevGlobalsList;
		clon->globalsList = globalsList;
	}

	ClonGlobal *global = &globalsList->globals[globalsList->globalsCount++];
	global->typeName = typeName;
	global->name = name;
	global->data = data;
	global->elemCount = elemCount;
}

const ClonGlobal *ClonGetGlobal(const Clon *clon, const char *type_name, const char *global_name)
{
	const ClonGlobalsList *list = clon->globalsList;

	while (list)
	{
		for (u32 i = 0; i < list->globalsCount; ++i)
		{
			const ClonGlobal *global = &list->globals[i];

			if (StrEq(global->typeName, type_name) && StrEq(global->name, global_name))
			{
				return global;
			}
		}

		list = list->next;
	}

	return NULL;
}

const ClonGlobal *ClonGetGlobal(const Clon *clon, const char *global_name)
{
	const ClonGlobalsList *list = clon->globalsList;

	while (list)
	{
		for (u32 i = 0; i < list->globalsCount; ++i)
		{
			const ClonGlobal *global = &list->globals[i];

			if (StrEq(global->name, global_name))
			{
				return global;
			}
		}

		list = list->next;
	}

	return NULL;
}

void ClonFillTrivial(const Clon *clon, Arena *arena, void *dataPtr, ReflexID reflexId, const CastInitializer *initializer)
{
	const CastExpression *expression = CAST_CHILD(initializer, expression);
	if (expression)
	{
		if (expression->type == CAST_EXPR_NUMBER)
		{
			if (reflexId == ReflexID_Bool) {
				*(bool*)dataPtr = Cast_EvaluateBool(expression);
			} else if (reflexId == ReflexID_Char) {
				*(char*)dataPtr = Cast_EvaluateChar(expression);
			} else if (reflexId == ReflexID_UnsignedChar) {
				*(unsigned char*)dataPtr = Cast_EvaluateUnsignedChar(expression);
			} else if (reflexId == ReflexID_Int) {
				*(int*)dataPtr = Cast_EvaluateInt(expression);
			} else if (reflexId == ReflexID_ShortInt) {
				*(short int*)dataPtr = Cast_EvaluateShortInt(expression);
			} else if (reflexId == ReflexID_LongInt) {
				*(long int*)dataPtr = Cast_EvaluateLongInt(expression);
			} else if (reflexId == ReflexID_LongLongInt) {
				*(long long int*)dataPtr = Cast_EvaluateLongLongInt(expression);
			} else if (reflexId == ReflexID_UnsignedInt) {
				*(unsigned int*)dataPtr = Cast_EvaluateUnsignedInt(expression);
			} else if (reflexId == ReflexID_UnsignedShortInt) {
				*(unsigned short int*)dataPtr = Cast_EvaluateUnsignedShortInt(expression);
			} else if (reflexId == ReflexID_UnsignedLongInt) {
				*(unsigned long int*)dataPtr = Cast_EvaluateUnsignedLongInt(expression);
			} else if (reflexId == ReflexID_UnsignedLongLongInt) {
				*(unsigned long long int*)dataPtr = Cast_EvaluateUnsignedLongLongInt(expression);
			} else if (reflexId == ReflexID_Float) {
				*(float*)dataPtr = Cast_EvaluateFloat(expression);
			} else if (reflexId == ReflexID_Double) {
				*(double*)dataPtr = Cast_EvaluateDouble(expression);
			} else {
				LOG(Error, "Void member does not expect any number.\n");
			}
		}
		else if (expression->type == CAST_EXPR_STRING)
		{
			char *str = PushArray(*arena, char, expression->constant.size+1);
			StrCopyN(str, expression->constant.str, expression->constant.size);
			*(char **)dataPtr = str;
		}
		else if (expression->type == CAST_EXPR_ARRAY_COUNT)
		{
			char globalName[128];
			StrCopyN(globalName, expression->constant.str, expression->constant.size);
			const ClonGlobal *clonGlobal = ClonGetGlobal(clon, globalName);
			if (clonGlobal)
			{
				*(unsigned int*)dataPtr = clonGlobal->elemCount;
			}
			else
			{
				*(unsigned int*)dataPtr = 1;
				LOG(Error,"Could not find global %s for ARRAY_COUNT.\n", globalName);
			}
		}
		else if (expression->type == CAST_EXPR_IDENTIFIER)
		{
			LOG(Debug, "Identifier?\n");
		}
		else
		{
			LOG(Error, "Invalid expression type (%u) to initialize trivial type.\n", expression->type);
		}
	}
	else
	{
		LOG(Error, "Invalid code path: No expression provided for trivial type... better handle this error in the calling code.\n");
	}
}

void ClonFillEnum(const Clon *clon, void *enumData, const ReflexEnum *reflexEnum, const CastInitializer *initializer)
{
	const CastExpression *expression = CAST_CHILD(initializer, expression);
	if (expression)
	{
		if (expression->type == CAST_EXPR_IDENTIFIER)
		{
			char enumeratorName[128];
			StrCopyN(enumeratorName, expression->constant.str, expression->constant.size);
			const i32 enumeratorValue = ReflexGetEnumValue(reflexEnum, enumeratorName);
			*(i32*)enumData = enumeratorValue;
		}
		else
		{
			LOG(Error, "Invalid expression type (%u) to initialize struct type.\n", expression->type);
		}
	}
	else
	{
		LOG(Error, "Invalid code path: No expression provided for enum type... better handle this error in the calling code.\n");
	}
}

void ClonFillStruct(const Clon *clon, Arena *arena, void *structData, const ReflexStruct *rstruct, const CastInitializer *initializer)
{
	const CastInitializerList *baseMemberInitializerList = CAST_CHILD(initializer, initializerList);
	const CastInitializerList *memberInitializerList = baseMemberInitializerList;
	bool canAssumeNextInitializer = true;

	// TODO: It might be better to iterate over the initializers and try to match the struct members instead
	for (u32 i = 0; i < rstruct->memberCount && memberInitializerList; ++i)
	{
		const ReflexMember *member = rstruct->members + i;

		const CastDesignator *designator = CAST_CHILD3(memberInitializerList, designation, designatorList, designator);

		const CastInitializer *memberInitializer = NULL;
		if (designator)
		{
			if (StrEqN(member->name, designator->identifier.str, designator->identifier.size)) {
				// Current initializer designator matches current member name
				memberInitializer = CAST_CHILD(memberInitializerList, initializer);
			} else {
				// Search for another designator that matches the current member name
				memberInitializerList = baseMemberInitializerList;
				while (memberInitializerList) {
					designator = CAST_CHILD3(memberInitializerList, designation, designatorList, designator);
					if (designator && StrEqN(member->name, designator->identifier.str, designator->identifier.size)) {
						memberInitializer = CAST_CHILD(memberInitializerList, initializer);
						break;
					}
					memberInitializerList = memberInitializerList->next;
				}
			}
		}
		else if (canAssumeNextInitializer)
		{
			memberInitializer = CAST_CHILD(memberInitializerList, initializer);
		}

		// Could not find a valid initializer for the current member
		if (!memberInitializer) {
			LOG(Warning, "Couldn't find matching initializer for member %s in struct %s.\n", member->name, rstruct->name);
			memberInitializerList = baseMemberInitializerList;
			canAssumeNextInitializer = false;
			continue;
		} else {
			canAssumeNextInitializer = true;
		}

		byte *memberPtr = (byte*)ReflexGetMemberPtr(structData, member);
		const u32 elemSize = member->pointerCount > 0 ? sizeof(void*) : ReflexGetTypeSize(member->reflexId);
		const u32 numElems = member->isArray ? member->arrayDim : 1;
		ASSERT(member->pointerCount < 2);

		// NOTE: Used for arrayed or struct elements (initializations within braces { ... })
		const CastInitializerList *elementInitializerList = CAST_CHILD(memberInitializer, initializerList);

		for (u32 elemIndex = 0; elemIndex < numElems; ++elemIndex)
		{
			const CastInitializer *elementInitializer = memberInitializer;
			if (member->isArray) {
				elementInitializer = CAST_CHILD(elementInitializerList, initializer);
				if (!elementInitializer) {
					break;
				}
				elementInitializerList = elementInitializerList->next;
			}

			const CastExpression *expression = CAST_CHILD(elementInitializer, expression);

			byte *elemPtr = memberPtr + elemIndex * elemSize;

			if (member->pointerCount == 0)
			{
				if (ReflexIsTrivial(member->reflexId))
				{
					ClonFillTrivial(clon, arena, elemPtr, member->reflexId, elementInitializer);
				}
				else if (ReflexIsStruct(member->reflexId))
				{
					const ReflexStruct *rstruct2 = ReflexGetStruct(member->reflexId);
					ClonFillStruct(clon, arena, elemPtr, rstruct2, elementInitializer);
				}
				else if (ReflexIsEnum(member->reflexId))
				{
					const ReflexEnum *renum = ReflexGetEnum(member->reflexId);
					ClonFillEnum(clon, elemPtr, renum, elementInitializer);
				}
				else
				{
					LOG(Error, "Invalid code path: Unhandled type for member %s.\n", member->name);
				}
			}
			else if (member->pointerCount == 1)
			{
				if (expression->type == CAST_EXPR_IDENTIFIER)
				{
					char *globalName = PushArray(*arena, char, expression->constant.size+1);
					StrCopyN(globalName, expression->constant.str, expression->constant.size);
					const ClonGlobal *clonGlobal = ClonGetGlobal(clon, globalName);
					*(void**)elemPtr = clonGlobal->data;
				}
				else
				{
					LOG(Error, "Unsupported expression type for member %s of type %s*.\n", member->name, rstruct->name);
				}
			}
			else
			{
				LOG(Error, "Unsupported more than one level of pointer indirections.\n");
			}
		}

		memberInitializerList = memberInitializerList->next;
	}
}

bool ClonParse(Clon *clon, Arena *arena, const char *data, u32 dataSize)
{
	const Cast *cast = Cast_Create(*arena, data, dataSize);
	if (cast)
	{
		const CastTranslationUnit *translationUnit = CAST_CHILD(cast, translationUnit);

		while (translationUnit)
		{
			const CastDeclaration *declaration = CAST_CHILD2(translationUnit, externalDeclaration, declaration);
			const CastInitDeclarator *initDeclarator = CAST_CHILD2(declaration, initDeclaratorList, initDeclarator);
			const CastDeclarator *declarator = CAST_CHILD(initDeclarator, declarator);
			const CastDirectDeclarator *directDeclarator = CAST_CHILD(declarator, directDeclarator);
			const CastInitializer *initializer = CAST_CHILD(initDeclarator, initializer);

			if (directDeclarator && initializer)
			{
				const CastDeclarationSpecifiers *specifiers = CAST_CHILD(declaration, declarationSpecifiers);
				CastTypeSpecifier *typeSpecifier = 0;
				while (specifiers && !typeSpecifier)
				{
					typeSpecifier = specifiers->typeSpecifier;
					specifiers = specifiers->next;
				}

				// TODO: Consider that global vars can also be pointers
				u32 pointerCount = 0;
				const CastPointer *pointer = declarator->pointer;
				while (pointer)
				{
					pointerCount++;
					pointer = pointer->next;
				}

				if (typeSpecifier && typeSpecifier->type == CAST_IDENTIFIER)
				{
					char *globalTypeName = PushArray(*arena, char, typeSpecifier->identifier.size+1);
					StrCopy(globalTypeName, typeSpecifier->identifier);

					const ReflexStruct *rstruct = ReflexGetStructFromName(globalTypeName);

					if (rstruct)
					{
						const bool isArray = directDeclarator->isArray;
						const u32 typeSize = rstruct->size;
						const u32 elemCount = !isArray ? 1 :
							directDeclarator->expression ? Cast_EvaluateInt(directDeclarator->expression):
							initializer->initializerCount;
						const u32 globalSize = typeSize * elemCount;

						char *globalName = PushArray(*arena, char, directDeclarator->name.size+1);
						StrCopy(globalName, directDeclarator->name);

						void *globalData = PushSize(*arena, globalSize);
						ClonAddGlobal(clon, arena, globalTypeName, globalName, globalData, elemCount);

						if ( isArray )
						{
							const CastInitializerList *initializerList = CAST_CHILD(initializer, initializerList);
							u32 elemIndex = 0;

							while (initializerList)
							{
								const CastInitializer *structInitializer = CAST_CHILD(initializerList, initializer);
								ClonFillStruct(clon, arena, (byte*)globalData + elemIndex * typeSize, rstruct, structInitializer);
								initializerList = initializerList->next;
								elemIndex++;
							}
						}
						else
						{
							const CastInitializer *structInitializer = initializer;
							ClonFillStruct(clon, arena, globalData, rstruct, initializer);
						}
					}
				}
			}

			translationUnit = translationUnit->next;
		}
	}

	return cast != NULL;
}

#endif // #ifndef CLON_H

