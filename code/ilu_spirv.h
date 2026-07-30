/*
# SPIRV reflection tools.

This file mainly provides a couple of functions to retrieve descriptor layouts from
binary SPIRV streams and print some information.

These are the publicly exposed functions:
```
	SpvParser SpvParserInit( spv_u8 *bytes, spv_u32 byteCount );
	SpvParser SpvParserInit( spv_u32 *words, spv_u32 wordCount );
	bool SpvParseDescriptors(SpvParser *parser, SpvDescriptorSetList *descriptorSetList, void *tempMem, spv_u32 tempMemSize);
	void SpvPrintDescriptorSetList(SpvDescriptorSetList *descriptorSetList);
	void SpvPrintDisassembly(SpvParser *parser);
```

Before including this header file, some customization macros can be defined:
```
	#define SPV_IMPLEMENTATION  // To include function implementations
	#define SPV_PRINT_FUNCTIONS // To include print functions
	#define SPV_PRINTF printf   // To provide a custom print function
	#define SPV_ASSERT assert   // To provide a custom assert macro
	#include "ilu_spirv.h"
```

Check the following link for reference on the implementation of Spirv tools:

- [SPIRV-Tools](https://github.com/KhronosGroup/SPIRV-Tools/tree/main/source)
*/

#ifndef ILU_SPIRV_H
#define ILU_SPIRV_H

// Defines ///////////////////////////////////////////////////////////////////////////////

#define SPV_INDEX_INSTRUCTION 5u
#define SPV_MAX_DESCRIPTOR_SETS 4u
#define SPV_MAX_DESCRIPTORS_PER_SET 16u

#define SPV_CT_ASSERT3(expression, line) static int ct_assert_##line[(expression) ? 1 : 0]
#define SPV_CT_ASSERT2(expression, line) SPV_CT_ASSERT3(expression, line)
#define SPV_CT_ASSERT(expression) SPV_CT_ASSERT2(expression, __LINE__)

#define SPV_ARRAY_COUNT(array) (sizeof(array)/sizeof(array[0]))


// Sized types ///////////////////////////////////////////////////////////////////////////

typedef unsigned char spv_u8;
typedef unsigned short int spv_u16;
typedef unsigned int spv_u32;
SPV_CT_ASSERT(sizeof(spv_u8) == 1);
SPV_CT_ASSERT(sizeof(spv_u16) == 2);
SPV_CT_ASSERT(sizeof(spv_u32) == 4);


// Spirv types ///////////////////////////////////////////////////////////////////////////

enum SpvOp
{
	SpvOpSource = 3,
	SpvOpSourceExtension = 4,
	SpvOpName = 5,
	SpvOpMemberName = 6,
	SpvOpExtInstImport = 11,
	SpvOpMemoryModel = 14,
	SpvOpEntryPoint = 15,
	SpvOpExecutionMode = 16,
	SpvOpCapability = 17,
	SpvOpTypeVoid = 19,
	SpvOpTypeBool = 20,
	SpvOpTypeInt = 21,
	SpvOpTypeFloat = 22,
	SpvOpTypeVector = 23,
	SpvOpTypeMatrix = 24,
	SpvOpTypeImage = 25,
	SpvOpTypeSampler = 26,
	SpvOpTypeSampledImage = 27,
	SpvOpTypeArray = 28,
	SpvOpTypeRuntimeArray = 29,
	SpvOpTypeStruct = 30,
	SpvOpTypeOpaque = 31,
	SpvOpTypePointer = 32,
	SpvOpTypeFunction = 33,
	SpvOpConstant = 43,
	SpvOpFunction = 54,
	SpvOpFunctionEnd = 56,
	SpvOpFunctionCall = 57,
	SpvOpVariable = 59,
	SpvOpLoad = 61,
	SpvOpStore = 62,
	SpvOpAccessChain = 65,
	SpvOpDecorate = 71,
	SpvOpMemberDecorate = 72,
	SpvOpVectorShuffle = 79,
	SpvOpCompositeConstruct = 80,
	SpvOpCompositeExtract = 81,
	SpvOpVectorTimesMatrix = 144,
	SpvOpMatrixTimesVector = 145,
	SpvOpMatrixTimesMatrix = 146,
	SpvOpOuterProduct = 147,
	SpvOpDot = 148,
	SpvOpLabel = 248,
	SpvOpReturn = 253,
};

enum SpvType
{
	SpvTypeNone,
	SpvTypeImage,
	SpvTypeSampler,
	SpvTypeSampledImage,
	SpvTypeUniformBuffer,
	SpvTypeStorageBuffer,
	SpvTypeStorageTexelBuffer,
	SpvTypeCount
};

static const char *SpvTypeStrings[] =
{
	"SpvTypeNone",
	"SpvTypeImage",
	"SpvTypeSampler",
	"SpvTypeSampledImage",
	"SpvTypeUniformBuffer",
	"SpvTypeStorageBuffer",
	"SpvTypeStorageTexelBuffer",
};

SPV_CT_ASSERT(SPV_ARRAY_COUNT(SpvTypeStrings) == SpvTypeCount);

enum SpvDim
{
	SpvDim1D,
	SpvDim2D,
	SpvDim3D,
	SpvDimCube,
	SpvDimRect,
	SpvDimBuffer,
	SpvDimSubpassData,
};

static const char *SpvDimStrings[] =
{
	"SpvDim1D",
	"SpvDim2D",
	"SpvDim3D",
	"SpvDimCube",
	"SpvDimRect",
	"SpvDimBuffer",
	"SpvDimSubpassData",
};

enum SpvExecutionModel
{
	SpvExecutionModelVertex = 0,
	SpvExecutionModelFragment = 4,
	SpvExecutionModelCompute = 5,
};

enum SpvDecoration
{
	SpvDecorationBinding = 33,
	SpvDecorationDescriptorSet = 34,
};

enum SpvStorageClass
{
	SpvStorageClassUniformConstant = 0,
	SpvStorageClassUniform = 2,
	SpvStorageClassPushConstant = 9,
};

enum SpvStageFlagBits
{
	SpvStageFlagsVertexBit = (1<<0),
	SpvStageFlagsFragmentBit = (1<<1),
	SpvStageFlagsComputeBit = (1<<2),
};

typedef spv_u8 SpvStageFlags;

struct SpvHeader
{
	spv_u32 magic;
	spv_u32 version;
	spv_u32 generator;
	spv_u32 bound;
	spv_u32 schema;
};

struct SpvParser
{
	// Stream
	spv_u32 *words;
	spv_u32 wordCount;

	// Header
	SpvHeader *header;

	// State
	spv_u32 *instructionWords;
	spv_u16 instructionOpCode;
	spv_u16 instructionWordCount;
};

struct SpvDescriptor
{
	spv_u8 binding;
	spv_u8 set;
	spv_u8 type;
	SpvStageFlags stageFlags;
	const char *name;
};

struct SpvDescriptorSet
{
	SpvDescriptor bindings[SPV_MAX_DESCRIPTORS_PER_SET];
};

struct SpvDescriptorSetList
{
	SpvDescriptorSet sets[SPV_MAX_DESCRIPTOR_SETS];
};


// Prototypes ////////////////////////////////////////////////////////////////////////////

SpvParser SpvParserInit( spv_u8 *bytes, spv_u32 byteCount );
SpvParser SpvParserInit( spv_u32 *words, spv_u32 wordCount );
bool SpvParseDescriptors(SpvParser *parser, SpvDescriptorSetList *descriptorSetList, void *tempMem, spv_u32 tempMemSize);
#if defined(SPV_PRINT_FUNCTIONS)
void SpvPrintDescriptorSetList(SpvDescriptorSetList *descriptorSetList);
void SpvPrintDisassembly(SpvParser *parser);
#endif // #if defined(SPV_PRINT_FUNCTIONS)

#endif // #if ILU_SPIRV_H


#if defined(SPV_IMPLEMENTATION)

// External macros ///////////////////////////////////////////////////////////////////////

#ifndef SPV_ASSERT
#include <assert.h>
#define SPV_ASSERT assert
#endif

#ifndef SPV_PRINTF
#include <stdio.h>
#define SPV_PRINTF printf
#endif


// Parsing ///////////////////////////////////////////////////////////////////////////////

static spv_u8 SpvVersionMajor(spv_u32 version)
{
	const spv_u8 major = ( version & 0x00ff0000 ) >> 16;
	return major;
}

static spv_u8 SpvVersionMinor(spv_u32 version)
{
	const spv_u8 minor = ( version & 0x0000ff00 ) >> 8;
	return minor;
}

static spv_u16 SpvGetWordCount(spv_u32 firstInstructionWord)
{
	const spv_u16 wordCount = ( firstInstructionWord >> 16 ) & 0xffff;
	return wordCount;
}

static spv_u16 SpvGetOpCode(spv_u32 firstInstructionWord)
{
	const spv_u16 opCode = ( firstInstructionWord >> 0 ) & 0xffff;
	return opCode;
}

static void SpvParserUpdateInstruction(SpvParser *parser)
{
	const spv_u32 firstInstructionWord = *parser->instructionWords;
	parser->instructionOpCode = SpvGetOpCode(firstInstructionWord);
	parser->instructionWordCount = SpvGetWordCount(firstInstructionWord);
}

static void SpvParserRewind(SpvParser *parser)
{
	parser->instructionWords = parser->words + SPV_INDEX_INSTRUCTION;
	SpvParserUpdateInstruction(parser);
}

static spv_u32 SpvParserInstructionOpCode(SpvParser *parser)
{
	return parser->instructionOpCode;
}

static spv_u32 SpvParserInstructionWordCount(SpvParser *parser)
{
	return parser->instructionWordCount;
}

static void SpvParserAdvance(SpvParser *parser)
{
	parser->instructionWords += parser->instructionWordCount;
	SpvParserUpdateInstruction(parser);
}

static bool SpvParserFinished(SpvParser *parser)
{
	return parser->instructionWords - parser->words >= parser->wordCount;
}

static void SpvTryParseEntryPoint(SpvParser *parser, spv_u32 *executionModel)
{
	const spv_u32 *words = parser->instructionWords;
	const spv_u32 opCode = SpvParserInstructionOpCode(parser);

	if ( opCode == SpvOpEntryPoint )
	{
		*executionModel = words[1];
	}
}

enum SpvIdFlags
{
	SpvIdFlagDescriptor = (1<<0),
	SpvIdFlagPushConstant = (1<<1),
};

struct SpvId
{
	spv_u8 binding;
	spv_u8 set;
	spv_u8 type;
	spv_u8 flags;
	const char *name;
};

static void SpvTryParseType(SpvParser *parser, SpvId *ids)
{
	const spv_u32 *words = parser->instructionWords;
	const spv_u32 opCode = SpvParserInstructionOpCode(parser);

	spv_u32 resultId = 0;
	spv_u32 typeId = 0;
	spv_u32 dim = 0;
	spv_u32 sampledType = 0;
	spv_u32 firstFieldTypeId = 0;

	switch ( opCode ) {
		case SpvOpTypeImage:
			resultId = words[1];
			sampledType = words[2];
			dim = words[3];
			if (dim == SpvDimBuffer) {
				ids[resultId].type = SpvTypeStorageTexelBuffer;
			} else {
				ids[resultId].type = SpvTypeImage;
			}
			break;
		case SpvOpTypeSampler:
			resultId = words[1];
			ids[resultId].type = SpvTypeSampler;
			break;
		case SpvOpTypeSampledImage:
			resultId = words[1];
			ids[resultId].type = SpvTypeSampledImage;
			break;
		case SpvOpTypeStruct:
			resultId = words[1];
			firstFieldTypeId = words[2];
			if ( ids[firstFieldTypeId].type == SpvTypeStorageBuffer ) {
				ids[resultId].type = SpvTypeStorageBuffer;
			} else {
				ids[resultId].type = SpvTypeUniformBuffer;
			}
			break;
		case SpvOpTypePointer:
			resultId = words[1];
			//storageClass = words[2];
			typeId = words[3];
			ids[resultId].type = ids[typeId].type;
			break;
		case SpvOpTypeRuntimeArray:
			resultId = words[1];
			ids[resultId].type = SpvTypeStorageBuffer;
			break;
	};
}

static void SpvTryParseDescriptor(SpvParser *parser, SpvId *ids)
{
	const spv_u32 *words = parser->instructionWords;
	const spv_u32 opCode = SpvParserInstructionOpCode(parser);

	if ( opCode == SpvOpDecorate )
	{
		const spv_u32 targetId = words[1];
		const spv_u32 decoration = words[2];

		if ( decoration == SpvDecorationBinding )
		{
			ids[targetId].binding = (spv_u8)words[3];
		}
		else if ( decoration == SpvDecorationDescriptorSet )
		{
			ids[targetId].set = (spv_u8)words[3];
		}
	}
}

static void SpvTryParseDescriptorId(SpvParser *parser, SpvId *ids)
{
	const spv_u32 *words = parser->instructionWords;
	const spv_u32 opCode = SpvParserInstructionOpCode(parser);

	if ( opCode == SpvOpVariable )
	{
		const spv_u32 resultTypeId = words[1];
		const spv_u32 resultId = words[2];
		const spv_u32 storageClass = words[3];

		if ( storageClass == SpvStorageClassUniform || storageClass == SpvStorageClassUniformConstant )
		{
			ids[resultId].type = ids[resultTypeId].type;
			ids[resultId].flags |= SpvIdFlagDescriptor;
		}
		else if ( storageClass == SpvStorageClassPushConstant )
		{
			ids[resultId].type = ids[resultTypeId].type;
			ids[resultId].flags |= SpvIdFlagPushConstant;
		}
	}
}

static void SpvTryParseName(SpvParser *parser, SpvId *ids)
{
	const spv_u32 *words = parser->instructionWords;
	const spv_u32 opCode = SpvParserInstructionOpCode(parser);

	if ( opCode == SpvOpName )
	{
		const spv_u32 targetId = words[1];
		ids[targetId].name = (const char*)&words[2];
	}
}

bool SpvParseDescriptors(SpvParser *parser, SpvDescriptorSetList *descriptorSetList, void *tempMem, spv_u32 tempMemSize)
{
	SPV_ASSERT( parser );
	SPV_ASSERT( parser->header );

	const spv_u32 idCount = parser->header->bound;
	SPV_ASSERT( sizeof(SpvId) * idCount < tempMemSize );

	SpvId *ids = (SpvId*)tempMem;
	for ( spv_u32 id = 0; id < idCount; ++id ) {
		ids[id] = {};
	}

	spv_u32 executionModel = 0;

	SpvParserRewind(parser);
	while ( !SpvParserFinished(parser) ) {
		SpvTryParseEntryPoint(parser, &executionModel);
		SpvTryParseType(parser, ids);
		SpvTryParseDescriptor(parser, ids);
		SpvTryParseDescriptorId(parser, ids);
		SpvTryParseName(parser, ids);
		SpvParserAdvance(parser);
	}

	for (spv_u32 id = 0; id < idCount; ++id)
	{
		if (ids[id].flags & SpvIdFlagDescriptor)
		{
			const spv_u8 binding = ids[id].binding;
			const spv_u8 set = ids[id].set;
			const spv_u8 type = ids[id].type;

			SpvDescriptor *descriptor = &descriptorSetList->sets[set].bindings[binding];

			const bool firstTimeAccessed = (descriptor->stageFlags == 0);

			if ( firstTimeAccessed || descriptor->type == type )
			{
				descriptor->binding = ids[id].binding;
				descriptor->set = ids[id].set;
				descriptor->type = ids[id].type;
				descriptor->stageFlags |= executionModel == SpvExecutionModelVertex ? SpvStageFlagsVertexBit : 0;
				descriptor->stageFlags |= executionModel == SpvExecutionModelFragment ? SpvStageFlagsFragmentBit : 0;
				descriptor->stageFlags |= executionModel == SpvExecutionModelCompute ? SpvStageFlagsComputeBit : 0;
				descriptor->name = ids[id].name;
			}
			else
			{
				SPV_PRINTF( "Warning: SpvParseDescriptors - descriptor(set:%u, binding:%u) - Type mismatch (%u / %u)\n", set, binding, descriptor->type, type );
				SPV_ASSERT( firstTimeAccessed || descriptor->type == type );
			}
		}
	}

	return true;
}

union SpvEndianness
{
	spv_u8 values[4];
	struct {
		spv_u32 value;
	};
};

static const SpvEndianness SPIRV_ENDIANNESS_LITTLE_ENDIAN = { 4, 3, 2, 1 };
static const SpvEndianness SPIRV_ENDIANNESS_BIG_ENDIAN = { 1, 2, 3, 4 };
static const SpvEndianness SPIRV_MAGIC_LITTLE_ENDIAN = { 0x03, 0x02, 0x23, 0x07 }; // Magic number: 0x07230203
static const SpvEndianness SPIRV_MAGIC_BIG_ENDIAN = { 0x07, 0x23, 0x02, 0x03 }; // Magic number: 0x07230203
static const spv_u32 SPIRV_HOST_ENDIANNESS = 0x01020304;

static spv_u32 SpvSwapWord(spv_u32 word)
{
	const spv_u32 swappedWord = (word & 0x000000ff) << 24 | (word & 0x0000ff00) << 8 | (word & 0x00ff0000) >> 8 | (word & 0xff000000) >> 24;
	return swappedWord;
}

SpvParser SpvParserInit( spv_u8 *bytes, spv_u32 byteCount )
{
	SPV_ASSERT( byteCount % 4 == 0 );
	const spv_u32 wordCount = byteCount / 4;
	spv_u32 *words = (spv_u32*)bytes;
	return SpvParserInit( words, wordCount );
}

SpvParser SpvParserInit( spv_u32 *words, spv_u32 wordCount )
{
	SpvHeader *header = (SpvHeader*)words;

	// Check module endianness based on the magic number
	SpvEndianness endianness;
	if ( header->magic == SPIRV_MAGIC_LITTLE_ENDIAN.value ) {
		endianness = SPIRV_ENDIANNESS_LITTLE_ENDIAN;
	} else if ( header->magic == SPIRV_MAGIC_BIG_ENDIAN.value ) {
		endianness = SPIRV_ENDIANNESS_BIG_ENDIAN;
	} else {
		SPV_ASSERT(0 && "Invalid SPIRV word stream.");
	}

	// Fix endianness
	// FIXME: This is probably not correct... I think decoration strings use
	// 1-byte characters, so this swapping could be messing their order.
	// Likely 4-byte words should be swapped when accessed.
	if ( endianness.value != SPIRV_HOST_ENDIANNESS ) {
		for (spv_u32 i = 0; i < wordCount; ++i) {
			words[i] = SpvSwapWord( words[i] );
		}
	}

	SpvParser parser = { words, wordCount, header };
	SpvParserRewind( &parser );
	return parser;
}

static const char *SpvTypeToString(SpvType type)
{
	const char *typeString = type < SpvTypeCount ? SpvTypeStrings[type] : "SpvTypeUnknown";
	return typeString;
}

#if defined(SPV_PRINT_FUNCTIONS)
static void SpvPrintInstructionDisassembly(SpvParser *parser)
{
	const spv_u32 *words = parser->instructionWords;
	const spv_u16 opCode = SpvParserInstructionOpCode(parser);
	const spv_u16 wordCount = SpvParserInstructionWordCount(parser);

	switch (opCode)
	{
		case SpvOpSource:
			{
				const spv_u32 sourceLanguage = words[1];
				const spv_u32 version = words[2];
				SPV_PRINTF("      SpvOpSource - sourceLanguage: %u - version: %u", sourceLanguage, version);
				if (wordCount > 3) {
					const spv_u32 fileId = words[3];
					SPV_PRINTF(" - fileId: %u", fileId);
				}
				if (wordCount > 4) {
					const char *source = (const char *)&words[4];
					SPV_PRINTF(" - source: %s", source);
				}
				SPV_PRINTF("\n");
			}
			break;
		case SpvOpSourceExtension: SPV_PRINTF("SpvOpSourceExtension - wordCount: %u\n", wordCount); break;
		case SpvOpName:
			{
				const spv_u32 targetId = words[1];
				const char *name = (const char *)&words[2];
				SPV_PRINTF("      SpvOpName - targetId: %u - name: %s\n", targetId, name);
			}
			break;
		case SpvOpMemberName:
			{
				const spv_u32 typeId = words[1];
				const spv_u32 member = words[2];
				const char *name = (const char *)&words[3];
				SPV_PRINTF("      SpvOpMemberName - typeId: %u - member: %u - name: %s\n", typeId, member, name);
			}
			break;
		case SpvOpExtInstImport: SPV_PRINTF("SpvOpExtInstImport - wordCount: %u\n", wordCount); break;
		case SpvOpMemoryModel:
			{
				const spv_u32 addressingModel = words[1];
				const spv_u32 memoryModel = words[2];
				SPV_PRINTF("      SpvOpMemoryModel - addressingModel: %u - memoryModel: %u\n", addressingModel, memoryModel);
			}
			break;
		case SpvOpEntryPoint:
			{
				const spv_u32 executionModel = words[1];
				const spv_u32 entryPointId = words[2];
				const char *name = (const char *)&words[3];
				SPV_PRINTF("      SpvOpEntryPoint - executionModel: %u - entryPointId: %u - name: %s - ...\n", executionModel, entryPointId, name);
			}
			break;
		case SpvOpExecutionMode:
			{
				const spv_u32 entryPointId = words[1];
				const spv_u32 executionModel = words[2];
				SPV_PRINTF("      SpvOpExecutionMode - entryPointId: %u - executionModel: %u - ...\n", entryPointId, executionModel);
			}
			break;
		case SpvOpCapability:
			{
				const spv_u32 capability = words[1];
				SPV_PRINTF("      SpvOpCapability - capability: %u\n", capability);
			}
			break;
		case SpvOpTypeVoid:
			{
				const spv_u32 resultId = words[1];
				SPV_PRINTF("%%%u = SpvOpTypeVoid\n", resultId);
			}
			break;
		case SpvOpTypeBool:
			{
				const spv_u32 resultId = words[1];
				SPV_PRINTF("%%%u = SpvOpTypeBool\n", resultId);
			}
			break;
		case SpvOpTypeInt:
			{
				const spv_u32 resultId = words[1];
				const spv_u32 width = words[2];
				const spv_u32 sign = words[3];
				SPV_PRINTF("%%%u = SpvOpTypeInt - width: %u - sign: %u\n", resultId, width, sign);
			}
			break;
		case SpvOpTypeFloat:
			{
				const spv_u32 resultId = words[1];
				const spv_u32 width = words[2];
				SPV_PRINTF("%%%u = SpvOpTypeFloat - width: %u\n", resultId, width);
			}
			break;
		case SpvOpTypeVector:
			{
				const spv_u32 resultId = words[1];
				const spv_u32 componentTypeId = words[2];
				const spv_u32 componentCount = words[3];
				SPV_PRINTF("%%%u = SpvOpTypeVector - componentTypeId: %u - componentCount: %u\n", resultId, componentTypeId, componentCount);
			}
			break;
		case SpvOpTypeMatrix:
			{
				const spv_u32 resultId = words[1];
				const spv_u32 columnTypeId = words[2];
				const spv_u32 columnCount = words[3];
				SPV_PRINTF("%%%u = SpvOpTypeMatrix - columnTypeId: %u - columnCount: %u\n", resultId, columnTypeId, columnCount);
			}
			break;
		case SpvOpTypeImage: SPV_PRINTF("SpvOpTypeImage - wordCount: %u\n", wordCount); break;
		case SpvOpTypeSampler: SPV_PRINTF("SpvOpTypeSampler - wordCount: %u\n", wordCount); break;
		case SpvOpTypeSampledImage: SPV_PRINTF("SpvOpTypeSampledImage - wordCount: %u\n", wordCount); break;
		case SpvOpTypeArray: SPV_PRINTF("SpvOpTypeArray - wordCount: %u\n", wordCount); break;
		case SpvOpTypeRuntimeArray: SPV_PRINTF("SpvOpTypeRuntimeArray - wordCount: %u\n", wordCount); break;
		case SpvOpTypeStruct:
			{
				const spv_u32 resultId = words[1];
				SPV_PRINTF("%%%u = SpvOpTypeStruct", resultId);
				for (spv_u32 i = 2; i < wordCount; ++i)
				{
					const spv_u32 typeId = words[i];
					SPV_PRINTF(" - typeId: %u", typeId);
				}
				if (wordCount > 2) SPV_PRINTF("\n");
			}
			break;
		case SpvOpTypeOpaque: SPV_PRINTF("SpvOpTypeOpaque - wordCount: %u\n", wordCount); break;
		case SpvOpTypePointer:
			{
				const spv_u32 resultId = words[1];
				const spv_u32 storageClass = words[2];
				const spv_u32 typeId = words[3];
				SPV_PRINTF("%%%u = SpvOpTypePointer - storageClass: %u - typeId: %u\n", resultId, storageClass, typeId);
				if (storageClass == SpvStorageClassUniform)
				{
					// TODO: Save this id
				}
			}
			break;
		case SpvOpTypeFunction:
			{
				const spv_u32 resultId = words[1];
				const spv_u32 returnTypeId = words[2];
				SPV_PRINTF("%%%u = SpvOpTypeFunction - returnTypeId: %u\n", resultId, returnTypeId);
				for (spv_u32 i = 3; i < wordCount; ++i)
				{
					const spv_u32 typeId = words[i];
					SPV_PRINTF(" - typeId: %u", typeId);
				}
				if (wordCount > 3) SPV_PRINTF("\n");
			}
			break;
		case SpvOpConstant:
			{
				const spv_u32 resultTypeId = words[1];
				const spv_u32 resultId = words[2];
				const spv_u32 value = words[3];
				SPV_PRINTF("%%%u = SpvOpConstant - returnTypeId: %u - value: %u\n", resultId, resultTypeId, value);
			}
			break;
		case SpvOpFunction: SPV_PRINTF("SpvOpFunction - wordCount: %u\n", wordCount); break;
		case SpvOpFunctionEnd: SPV_PRINTF("SpvOpFunctionEnd - wordCount: %u\n", wordCount); break;
		case SpvOpFunctionCall: SPV_PRINTF("SpvOpFunctionCall - wordCount: %u\n", wordCount); break;
		case SpvOpVariable:
			{
				const spv_u32 resultTypeId = words[1];
				const spv_u32 resultId = words[2];
				const spv_u32 storageClass = words[3];
				SPV_PRINTF("%%%u = SpvOpVariable - resultTypeId: %u - storageClass: %u - ...\n", resultId, resultTypeId, storageClass);
			}
			break;
		case SpvOpLoad: SPV_PRINTF("SpvOpLoad - wordCount: %u\n", wordCount); break;
		case SpvOpStore: SPV_PRINTF("SpvOpStore - wordCount: %u\n", wordCount); break;
		case SpvOpAccessChain: SPV_PRINTF("SpvOpAccessChain - wordCount: %u\n", wordCount); break;
		case SpvOpDecorate:
			{
				const spv_u32 targetId = words[1];
				const spv_u32 decoration = words[2];
				SPV_PRINTF("      SpvOpDecorate - targetId: %u - decoration: %u...\n", targetId, decoration);
			}
			break;
		case SpvOpMemberDecorate:
			{
				const spv_u32 structyreTypeId = words[1];
				const spv_u32 member = words[2];
				const spv_u32 decoration = words[3];
				SPV_PRINTF("      SpvOpMemberDecorate - structyreTypeId: %u - member: %u - decoration: %u...\n", structyreTypeId, member, decoration);
			}
			break;
		case SpvOpVectorShuffle: SPV_PRINTF("SpvOpVectorShuffle - wordCount: %u\n", wordCount); break;
		case SpvOpCompositeConstruct: SPV_PRINTF("SpvOpCompositeConstruct - wordCount: %u\n", wordCount); break;
		case SpvOpCompositeExtract: SPV_PRINTF("SpvOpCompositeExtract - wordCount: %u\n", wordCount); break;
		case SpvOpVectorTimesMatrix: SPV_PRINTF("SpvOpVectorTimesMatrix - wordCount: %u\n", wordCount); break;
		case SpvOpMatrixTimesVector: SPV_PRINTF("SpvOpMatrixTimesVector - wordCount: %u\n", wordCount); break;
		case SpvOpMatrixTimesMatrix: SPV_PRINTF("SpvOpMatrixTimesMatrix - wordCount: %u\n", wordCount); break;
		case SpvOpOuterProduct: SPV_PRINTF("SpvOpOuterProduct - wordCount: %u\n", wordCount); break;
		case SpvOpDot: SPV_PRINTF("SpvOpDot - wordCount: %u\n", wordCount); break;
		case SpvOpLabel: SPV_PRINTF("SpvOpLabel - wordCount: %u\n", wordCount); break;
		case SpvOpReturn: SPV_PRINTF("SpvOpReturn - wordCount: %u\n", wordCount); break;
		default:
		{
			SPV_PRINTF("Unhandled instruction - opCode: %u - wordCount: %u\n", opCode, wordCount);
		}
	};
}

void SpvPrintDisassembly(SpvParser *parser)
{
	while ( !SpvParserFinished(parser) )
	{
		SpvPrintInstructionDisassembly(parser);
		SpvParserAdvance(parser);
	}
}

void SpvPrintDescriptorSetList(SpvDescriptorSetList *descriptorSetList)
{
	spv_u32 currentSet = SPV_MAX_DESCRIPTOR_SETS;
	for (spv_u32 set = 0; set < SPV_MAX_DESCRIPTOR_SETS; ++set)
	{
		for (spv_u32 binding = 0; binding < SPV_MAX_DESCRIPTORS_PER_SET; ++binding)
		{
			SpvDescriptor *desc = &descriptorSetList->sets[set].bindings[binding];
			if ( desc->type != SpvTypeNone )
			{
				if ( set != currentSet )
				{
					SPV_PRINTF("descriptor_set[%u]\n", set);
					currentSet = set;
				}

				SPV_PRINTF("  binding[%u]\n", binding);
				SPV_PRINTF("    type = %s\n", SpvTypeToString((SpvType)desc->type));
				SPV_PRINTF("    stages = ");
				if ( desc->stageFlags & SpvStageFlagsVertexBit ) SPV_PRINTF("Vertex ");
				if ( desc->stageFlags & SpvStageFlagsFragmentBit ) SPV_PRINTF("Fragment ");
				if ( desc->stageFlags & SpvStageFlagsComputeBit ) SPV_PRINTF("Compute ");
				SPV_PRINTF("\n");
			}
		}
	}
}

#endif // #if defined(SPV_PRINT_FUNCTIONS)

#endif // #if defined(SPV_IMPLEMENTATION)

