#pragma once
#include <EC.h>

const char* __cdecl InspectorACP(const AddressCommentInfo& AddrInfo);

const char* MoveToIHCore(const std::u8string& S);
const char* MoveToIHCore(const std::string& S);

//Call on crash
void PushPresetInstanceFromYRPP();

std::u8string GetFirstACPString();