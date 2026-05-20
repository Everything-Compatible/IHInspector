#include "Inspector.h"
#include <EC.h>
#include "Debug.h"

std::unordered_map<std::string, ObjectInstance> PresetInstances;

void AddPresetObjectInstance(const std::string& VarName, ObjectInstance Instance)
{
	PresetInstances[VarName] = Instance;
	Debug::LogFormat("[Inspector] Meta Addr {}\n", (void*)Instance.TypeInfo.get());
	Debug::LogFormat("[Inspector] Added preset variable \"{}\" with type \"{}\" at address 0x{:08X}\n",
		VarName, Instance.TypeInfo->typeName, Instance.Address);
}

void AddObjectInstance(const std::string& VarName, ObjectInstance Instance)
{
	char Value[64]{};
	sprintf_s(Value, 64, "Inst::%08X@%08X", Instance.TypeInfo->ID, Instance.Address);
	ECDebug::SetGlobalVar(~VarName, conv Value);
}

std::optional<ObjectInstance> GetVarInstance(ObjectInstanceVar Variable)
{
	auto Str = Variable.ValueString();
	if (!Str.empty())
	{
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

	auto it = PresetInstances.find(Variable.VarName);
	if (it != PresetInstances.end())
	{
		Debug::LogFormat("[Inspector] Meta Addr {}\n", (void*)it->second.TypeInfo.get());
		Debug::LogFormat("[Inspector] Added preset variable \"{}\" with type \"{}\" at address 0x{:08X}\n",
			Variable.VarName, it->second.TypeInfo->typeName, it->second.Address);

		return it->second;
	}
	else
	{
		return std::nullopt;
	}
}

UTF8_String ObjectInstanceVar::ValueString() const
{
	return ECDebug::GetGlobalVar(~VarName);
}