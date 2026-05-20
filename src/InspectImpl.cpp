#include "InspectImpl.h"
#include "Inspector.h"
#include <format>
#include <iostream>
#include "Debug.h"

/*
命令： IHInspect.View
参数：
1.
	-VarName	变量名
或
	-TypeName	类型名
	-Address	地址
或
	-TypeID	类型ID
	-Address	地址
2.
	-Path		路径元素列表（可选）
	-Trait		Default/Both/Full/None :
		Default : 默认的显示模式，次要的Trait不会起作用，隐藏原始成员会生效
		Both : 次要的Trait不会起作用, 总是显示原始成员
		Full : 显示所有可用的Trait信息, 总是显示原始成员
		None : 不使用Trait, 只显示原始成员
	-Depth		显示深度，默认-1（无限制）
	-ShowID     <bool> 默认false

*/
bool AnalyzeInstanceFromViewArgs(JsonObject Args, ObjectInstance& Inst);
void IHInspect_View_Impl(JsonObject Args)
{
	//1. 解析出 ObjectInstance
	ObjectInstance instance;
	if (!AnalyzeInstanceFromViewArgs(Args, instance))
		return;

	//2. 解析 Depth
	int depth = -1; //default unlimited
	auto oDepth = Args.GetObjectItem("Depth");
	if (oDepth && oDepth.IsTypeNumber())
	{
		depth = oDepth.GetInt();
	}

	//3. 解析 ShowID
	bool showID = false; //default false
	auto oShowID = Args.GetObjectItem("ShowID");
	if (oShowID && oShowID.IsTypeBool())
	{
		showID = oShowID.GetBool();
	}

	//4. View
	Debug::LogFormat("[Inspector] META info {}\n",
		(void*)instance.TypeInfo.get());
	Debug::LogFormat("[Inspector] Viewing instance of type {} at address 0x{:08X} with depth {} and ShowID {}\n",
		instance.TypeInfo->typeName, instance.Address, depth, showID);

	JsonFile result = ViewAsObject(instance, showID, depth);
	ECDebug::DoNotEcho();
	std::cout << result.GetObj().GetTextEx();
	ECDebug::ReturnString(~result.GetObj().GetText());

	//-Trait -Path 暂不支持
}

/*
	-VarName	变量名
或
	-TypeName	类型名
	-Address	地址
或
	-TypeID	类型ID
	-Address	地址
*/

bool AnalyzeInstanceFromViewArgs(JsonObject Args, ObjectInstance& Inst)
{
	auto oVarName = Args.GetObjectItem("VarName");
	if (oVarName && oVarName.IsTypeString())
	{
		ObjectInstanceVar var;
		var.VarName = oVarName.GetString();

		auto OptionalInst = GetVarInstance(var);

		if (OptionalInst)
		{
			Inst = *OptionalInst;
			return true;
		}
		else
		{
			ECDebug::ReturnError(~std::format("Variable \"{}\" not found or not an instance", var.VarName), ERROR_NOT_FOUND);
			return false;
		}
	}

	auto oAddress = Args.GetObjectItem("Address");
	auto oTypeName = Args.GetObjectItem("TypeName");

	if (oAddress && oAddress.IsTypeString() && oTypeName && oTypeName.IsTypeString())
	{
		std::string addrStr = oAddress.GetString();
		DWORD address = std::strtoul(addrStr.c_str(), nullptr, 16);

		auto pMeta = GetOrUnpackTypeMetaInfo(oTypeName.GetString());
		if (pMeta)
		{
			Inst.Address = address;
			Inst.TypeInfo = pMeta;
			return true;
		}
		else
		{
			ECDebug::ReturnError(~std::format("Type \"{}\" Not Found", oTypeName.GetString()), ERROR_NOT_FOUND);
			return false;
		}
	}


	auto oTypeID = Args.GetObjectItem("TypeID");
	if (oAddress && oAddress.IsTypeString() && oTypeID && oTypeID.IsTypeString())
	{
		std::string addrStr = oAddress.GetString();
		DWORD address = std::strtoul(addrStr.c_str(), nullptr, 16);
		//string as 0x%08X
		std::string typeIDStr = oTypeID.GetString();
		uint32_t typeID = std::strtoul(typeIDStr.c_str(), nullptr, 16);
		if (HasTypeMetaInfo(typeID))
		{
			Inst.Address = address;
			Inst.TypeInfo = GetObjectTypeMetaInfo(typeID);
			return true;
		}
		else
		{
			ECDebug::ReturnError(~std::format("Type ID 0x{:08X} not found", typeID), ERROR_NOT_FOUND);
			return false;
		}
	}

	ECDebug::ReturnStdError(ERROR_BAD_ARGUMENTS);
	return false;
}



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
		auto pMeta = GetOrUnpackTypeMetaInfo(oTypeName.GetString());
		if (pMeta)
		{
			JsonFile result = InspectType(pMeta);
			ECDebug::DoNotEcho();
			std::cout << result.GetObj().GetTextEx();
			ECDebug::ReturnString(~result.GetObj().GetText());
		}
		else
		{
			ECDebug::ReturnError(~std::format("Type \"{}\" Not Found", oTypeName.GetString()), ERROR_NOT_FOUND);
		}

		return;
	}

	auto oTypeID = Args.GetObjectItem("ID");
	if (oTypeName && oTypeName.IsTypeString())
	{
		//string as 0x%08X
		std::string typeIDStr = oTypeID.GetString();
		uint32_t typeID = std::strtoul(typeIDStr.c_str(), nullptr, 16);

		if (HasTypeMetaInfo(typeID))
		{
			JsonFile result = InspectType(GetObjectTypeMetaInfo(typeID));
			ECDebug::DoNotEcho();
			std::cout << result.GetObj().GetTextEx();
			ECDebug::ReturnString(~result.GetObj().GetText());
		}
		else
		{
			ECDebug::ReturnError(~std::format("Type ID 0x{:08X} not found", typeID), ERROR_NOT_FOUND);
		}
	}

	ECDebug::ReturnStdError(ERROR_BAD_ARGUMENTS);
}