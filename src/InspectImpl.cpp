#include "InspectImpl.h"
#include "Inspector.h"
#include <format>
#include <iostream>

/*
命令： IHInspect.Type
参数：
1.
	-Name	类型名
或
	-ID		类型ID (HEX, 0x%08X)
*/
void IHInspect_Type_Impl(JsonObject Args)
{
	auto oTypeName = Args.GetObjectItem("Name");

	if (oTypeName && oTypeName.IsTypeString())
	{
		auto pMeta = GetObjectTypeMetaInfo(oTypeName.GetString());
		if (pMeta)
		{
			JsonFile result = InspectType(pMeta);
			ECDebug::DoNotEcho();
			std::cout << result.GetObj().GetTextEx();
			ECDebug::ReturnString(~result.GetObj().GetText());
		}
		else
		{
			ECDebug::ReturnError(~std::format("Type \"{}\" not found.", oTypeName.GetString()), ERROR_NOT_FOUND);
		}

		return;
	}

	auto oTypeID = Args.GetObjectItem("ID");
	if (oTypeName && oTypeName.IsTypeString())
	{
		//string as 0x%08X
		std::string typeIDStr = oTypeID.GetString();
		size_t typeID = std::strtoul(typeIDStr.c_str(), nullptr, 16);

		ObjectTypeMetaInfo* pMeta = reinterpret_cast<ObjectTypeMetaInfo*>(typeID);

		if (HasTypeMetaInfo(pMeta))
		{
			JsonFile result = InspectType(pMeta);
			ECDebug::DoNotEcho();
			std::cout << result.GetObj().GetTextEx();
			ECDebug::ReturnString(~result.GetObj().GetText());
		}
		else
		{
			ECDebug::ReturnError(~std::format("TypeID 0x{:08X} not found.", typeID), ERROR_NOT_FOUND);
		}
	}

	ECDebug::ReturnStdError(ERROR_BAD_ARGUMENTS);
}