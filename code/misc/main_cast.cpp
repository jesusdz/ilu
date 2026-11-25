#include "../ilu_core.h"

#define CAST_IMPLEMENTATION
#define CAST_PRINT
#include "../reflex/cast.h"

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

			const Cast *cast = Cast_Create(globalArena, bytes, fileSize);
			if (cast)
			{
				Cast_Print(cast);
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

