#include "Inspector.Provider.h"

const char* __cdecl InspectorACP(const AddressCommentInfo& AddrInfo)
{
	std::u8string S = AddrInfo.CanRead ? u8"我可以读" : u8"我不可以读";

	char* p = (char*)IH::Malloc(S.size() + 1);
	strncpy(p, (const char*)S.c_str(), S.size() + 1);
	return p;
}