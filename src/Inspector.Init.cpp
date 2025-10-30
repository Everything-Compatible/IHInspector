#include "Inspector.h"
#include <filesystem>
#include <EC.Misc.h>
#include <Debug.h>
#include <print>

std::unordered_map<std::string, std::unique_ptr<ObjectTypeMetaInfo>> TypeMetaInfoMap;
std::unordered_set<ObjectTypeMetaInfo*> UsedMetaInfoPtr;

std::string TrimString(const std::string& str)
{
	size_t first = str.find_first_not_of(" \t\n\r");
	if (first == std::string::npos)
		return "";
	size_t last = str.find_last_not_of(" \t\n\r");
	return str.substr(first, (last - first + 1));
}

ObjectTypeMetaInfo* GetObjectTypeMetaInfo(const std::string& _typeName)
{
	auto typeName = TrimString(_typeName);
	auto it = TypeMetaInfoMap.find(typeName);
	if (it != TypeMetaInfoMap.end())
	{
		return it->second.get();
	}
	return nullptr;
}

ObjectTypeMetaInfo* GetOrCreateTypeMetaInfo(const std::string& _typeName, size_t Size)
{
	auto typeName = TrimString(_typeName);
	auto typeInfo = GetObjectTypeMetaInfo(typeName);
	if (typeInfo)
	{
		return typeInfo;
	}
	else
	{
		ObjectTypeMetaInfo newTypeInfo;
		if (newTypeInfo.Load(typeName, Size))
		{
			AddTypeMetaInfo(newTypeInfo);
			return GetObjectTypeMetaInfo(typeName);
		}
		else
		{
			return nullptr;
		}
	}
}

bool HasTypeMetaInfo(const std::string& typeName)
{
	return TypeMetaInfoMap.find(typeName) != TypeMetaInfoMap.end();
}

bool HasTypeMetaInfo(const ObjectTypeMetaInfo* pMeta)
{
	return UsedMetaInfoPtr.find(const_cast<ObjectTypeMetaInfo*>(pMeta)) != UsedMetaInfoPtr.end();
}

void AddTypeMetaInfo(const ObjectTypeMetaInfo& typeInfo)
{
	TypeMetaInfoMap[typeInfo.typeName] = std::make_unique<ObjectTypeMetaInfo>(typeInfo);
	UsedMetaInfoPtr.insert(TypeMetaInfoMap[typeInfo.typeName].get());
}

bool ObjectTypeMetaInfo::Load(const std::string& _typeName, JsonObject Obj)
{
	typeName = TrimString(_typeName);

	auto oSize = Obj.GetObjectItem("size");
	if (oSize && oSize.IsTypeNumber())
		typeSize = static_cast<size_t>(oSize.GetInt());
	else return false;

	auto oType = Obj.GetObjectItem("type");
	if (!oType || !oType.IsTypeString())
		return false;
	std::string typeStr = oType.GetString();

	if (typeStr == "struct")
	{
		ObjectTypeMeta_Struct Data;
		if (!Data.Load(Obj)) return false;
		typeData = Data;
		return true;
	}
	else if (typeStr == "union")
	{
		ObjectTypeMeta_Union Data;
		if (!Data.Load(Obj)) return false;
		typeData = Data;
		return true;
	}
	else if (typeStr == "enum")
	{
		ObjectTypeMeta_Enum Data;
		if (!Data.Load(Obj)) return false;
		typeData = Data;
		return true;
	}
	else
	{
		return false;
	}
}

bool ObjectTypeMeta_Enum::Load(JsonObject Obj)
{
	auto oValue = Obj.GetObjectItem("values");
	if (!oValue || !oValue.IsTypeObject())
		return false;

	for (auto& [Name, Val] : oValue.GetMapInt())
	{
		auto Value = static_cast<int64_t>(Val);
		EnumValues[Name] = Value;
		EnumNames[Value] = Name;
	}

	return true;
}

bool ObjectTypeMeta_Struct::Load(JsonObject Obj)
{
	auto oBase = Obj.GetObjectItem("base_classes");
	if (oBase && oBase.IsTypeArray())
	{
		for (auto& baseClassItem : oBase.GetArrayString())
		{
			BaseClasses.push_back(baseClassItem);
		}
	}

	auto oMembers = Obj.GetObjectItem("members");
	if (!oMembers || !oMembers.IsTypeArray())
		return false;
	for (auto& memberItem : oMembers.GetArrayObject())
	{
		ObjectTypeMeta_Member member;
		if (!member.Load(memberItem))
			return false;
		Members[member.MemberName] = std::move(member);
	}

	// Build MemberOrders
	for (auto& [Name, Member] : Members)
	{
		MemberOrders.push_back(Name);
	}
	std::sort(MemberOrders.begin(), MemberOrders.end(),
		[this](const std::string& a, const std::string& b) {
			return Members[a].MemberOffset < Members[b].MemberOffset;
		});

	return true;
}

bool ObjectTypeMeta_Union::Load(JsonObject Obj)
{
	auto oMembers = Obj.GetObjectItem("members");
	if (!oMembers || !oMembers.IsTypeArray())
		return false;
	for (auto& memberItem : oMembers.GetArrayObject())
	{
		ObjectTypeMeta_Member member;
		if (!member.Load(memberItem))
			return false;
		UnionTypes.push_back(member.MemberName);
		Members[member.MemberName] = std::move(member);
	}

	return true;
}

bool ObjectTypeMeta_Basic::Load(const std::string& typeStr)
{
	return false;
}

bool ObjectTypeMeta_Member::Load(JsonObject Obj)
{
	/*
	{
        "name": "xxx",
        "type": "xxx",
        "size": n,
        "offset": n
    }
	*/
	auto oName = Obj.GetObjectItem("name");
	if (oName && oName.IsTypeString())
		MemberName = TrimString(oName.GetString());
	else return false;

	auto oOffset = Obj.GetObjectItem("offset");
	if (oOffset && oOffset.IsTypeNumber())
		MemberOffset = static_cast<size_t>(oOffset.GetInt());
	else return false;

	auto oSize = Obj.GetObjectItem("size");
	size_t ObjSize = 0;
	if (oSize && oSize.IsTypeNumber())
		ObjSize = static_cast<size_t>(oSize.GetInt());
	else return false;

	auto oType = Obj.GetObjectItem("type");
	if (oType && oType.IsTypeString())
	{
		std::string typeName = oType.GetString();
		auto MemberTypeInfo = GetOrCreateTypeMetaInfo(typeName, ObjSize);
		if (!MemberTypeInfo) return false;
		MemberType = MemberTypeInfo;
		return true;
	}
	else return false;
}

bool ObjectTypeMetaInfo::Load(const std::string& _typeName, size_t Size)
{
	//Resolve :
	//	ObjectTypeData_Basic,
	//	ObjectTypeData_FixedArray,
	//	ObjectTypeData_Pointer,
	//	ObjectTypeData_Pending
	typeName = TrimString(_typeName);
	typeSize = Size;

	//	ObjectTypeData_Basic
	ObjectTypeMeta_Basic ba;
	if (ba.Load(typeName))
	{
		typeData = ba;
		return true;
	}

	//	ObjectTypeData_FixedArray
	// if end with [xxx]
	size_t pos1 = typeName.find_last_of('[');
	size_t pos2 = typeName.find_last_of(']');
	if (pos1 != std::string::npos && pos2 == typeName.length() - 1 && pos2 > pos1 + 1)
	{
		std::string elemTypeName = typeName.substr(0, pos1);
		std::string countStr = typeName.substr(pos1 + 1, pos2 - pos1 - 1);
		size_t elemCount = static_cast<size_t>(std::strtoull(countStr.c_str(), nullptr, 10));
		
		ObjectTypeMetaInfo* elemType = GetOrCreateTypeMetaInfo(elemTypeName, typeSize / elemCount);

		ObjectTypeMeta_FixedArray fa;
		fa.ElementType = elemType;
		fa.ElementCount = elemCount;

		typeData = fa;
		return true;
	}

	//	ObjectTypeData_Pointer
	// if end with '*'
	if (typeName.back() == '*' && typeSize == sizeof(void*))
	{
		std::string pointedTypeName = typeName.substr(0, typeName.length() - 1);

		ObjectTypeMetaInfo* pointedType = GetOrCreateTypeMetaInfo(pointedTypeName, sizeof(void*));

		ObjectTypeMeta_Pointer pa;
		pa.PointedType = pointedType;
		typeData = pa;
		return true;
	}

	//	ObjectTypeData_Pending
	ObjectTypeMeta_Pending pd;
	typeData = pd;
	return true;
}

void InitTypeMetaInfo()
{
	/*
	1. Init Basic Types
	2. Init User Specified Types
	3. Output Debug Info
	4. Init Tags
	*/

	// Init Basic Types
	// TODO: Add initialization logic for basic types here.

	// Init User Specified Types
	// from ./Inspector/Types/*.json
	std::filesystem::path typeDefDir = std::filesystem::current_path() / "Inspector" / "Types";
	if (std::filesystem::exists(typeDefDir) && std::filesystem::is_directory(typeDefDir))
	{
		for (const auto& entry : std::filesystem::directory_iterator(typeDefDir))
		{
			if (entry.is_regular_file() && entry.path().extension() == ".json")
			{
				JsonFile jsonFile;
				auto ErrorStr = jsonFile.ParseChecked(~ReadFileToString(entry.path().string().c_str()), "<SYNTAX ERROR>");

				if (!ErrorStr.empty())
				{
					Debug::LogFormat("[Inspector] Failed to read file \"{}\", Error:\n{}\n", entry.path().string(), ErrorStr);
				}

				if (jsonFile.Available())
				{
					for (auto& [Name, Obj] : jsonFile.GetObj().GetMapObject())
					{
						ObjectTypeMetaInfo typeInfo;
						if (typeInfo.Load(Name, Obj))
						{
							AddTypeMetaInfo(typeInfo);
						}
						else
						{
							Debug::LogFormat("[Inspector] Failed to load Type Meta Info of \"{}\"\n", Name);
						}
					}
				}
			}
		}
	}

	// Output Debug Info
	// TODO: Add debug output logic here.
	for (const auto& [TypeName, TypeInfo] : TypeMetaInfoMap)
	{
		if (std::holds_alternative<ObjectTypeMeta_Pending>(TypeInfo->typeData))
		{
			Debug::LogFormat("[Inspector] Type \"{}\" is pending definition.\n", TypeName);
			//std::println("\033[37m[Inspector]\033[1;33m Type \"{}\" is pending definition.\033[0m", TypeName);
		}
	}

	// Init Tags
	// TODO: Add tag initialization logic here.

}