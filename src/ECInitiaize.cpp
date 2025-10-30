#include <EC.h>
#include "Inspector.h"

UTF8_CString __cdecl ObjectVarRouter(const char* Key)
{
	return u8"Not Implemented";
}

void Initialize()
{
	IH::SetTextDrawRouter("GameObj", ObjectVarRouter);
	InitTypeMetaInfo();
}