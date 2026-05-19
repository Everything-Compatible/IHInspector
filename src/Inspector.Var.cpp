#include "Inspector.h"

void AddObjectInstance(const std::string& VarName, ObjectInstance Instance)
{
	char Value[64]{};
	sprintf_s(Value, 64, "Inst::%08X@%08X", Instance.TypeInfo->ID, Instance.Address);
	ECDebug::SetGlobalVar(~VarName, conv Value);
}

std::optional<ObjectInstance> GetVarInstance(ObjectInstanceVar Variable)
{
	auto Str = Variable.ValueString();
	uint32_t ID;
	DWORD Addr;
	auto Ret = sscanf_s((~Str).c_str(), "Inst::%08X@%08X", &ID, &Addr);

	if (Ret != 2)return std::nullopt;

	if (!HasTypeMetaInfo(ID))return std::nullopt;

	ObjectInstance Inst;
	Inst.Address = Addr;
	Inst.TypeInfo = GetObjectTypeMetaInfo(ID);

	return Inst;
}

UTF8_String ObjectInstanceVar::ValueString() const
{
	return ECDebug::GetGlobalVar(~VarName);
}