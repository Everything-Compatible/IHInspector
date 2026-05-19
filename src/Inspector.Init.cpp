#include "Inspector.h"
#include <filesystem>
#include <EC.Misc.h>
#include <Debug.h>
#include <print>

std::unordered_map<std::string, ObjectTypeMetaPtr> TypeMetaInfoMap;
std::unordered_map<uint32_t, ObjectTypeMetaPtr> TypeMetaIDMap;
std::unordered_set<ObjectTagPtr> MetaTagSet;
bool TraitPreprocessed = false;

std::string TrimString(const std::string& str)
{
	size_t first = str.find_first_not_of(" \t\n\r");
	if (first == std::string::npos)
		return "";
	size_t last = str.find_last_not_of(" \t\n\r");
	return str.substr(first, (last - first + 1));
}

std::string TypeStrTrimString(const std::string& str)
{
	/*
	中间>=1 个空白字符 -> 1个空格
	'*'前后均有空白字符（除非在首尾）
	首尾无空白字符
	*/

	std::string Final;
	Final.reserve(str.size());

	enum ThisChar {
		NotSpace,
		Asterisk,
		Space,
	} Last = Space, This = Space;

	enum PushType {
		No,
		Current,
		Blank,
		Spaced,
	};

	PushType PushTable[3][3] = {
		{Current, Spaced, Blank},
		{Spaced, Current, Blank},
		{Current, Current, No},
	};

	for (auto c : str)
	{
		if (isspace(c))This = Space;
		else if (c == '*')This = Asterisk;
		else This = NotSpace;

		switch (PushTable[Last][This])
		{
		case Spaced:
			Final.push_back(' ');
		case Current: 
			Final.push_back(c);
			break;
		case Blank:
			Final.push_back(' ');
		case No:
			break;
		}

		Last = This;
	}

	while (!Final.empty() && isspace(Final.back()))
		Final.pop_back();

	return Final;
}

std::string TypeStrTrimString_TypeIdx(const std::string& str)
{
	auto typeName = TypeStrTrimString(str);
	if (typeName.starts_with("struct "))
		typeName = typeName.substr(7);
	else if (typeName.starts_with("union "))
		typeName = typeName.substr(6);
	else if (typeName.starts_with("enum "))
		typeName = typeName.substr(5);
	return typeName;
}

ObjectTypeMetaPtr GetObjectTypeMetaInfo(const std::string& _typeName)
{
	auto typeName = TypeStrTrimString_TypeIdx(_typeName);
	
	

	auto it = TypeMetaInfoMap.find(typeName);
	if (it != TypeMetaInfoMap.end())
	{
		return it->second;
	}
	return nullptr;
}

ObjectTypeMetaPtr GetObjectTypeMetaInfo(uint32_t ID)
{
	auto it = TypeMetaIDMap.find(ID);
	if (it != TypeMetaIDMap.end())
	{
		return it->second;
	}
	return nullptr;
}

ObjectTypeMetaPtr GetOrCreateTypeMetaInfoEx(const std::string& _typeName, size_t Size, bool UnpackOnly)
{
	auto typeName = TypeStrTrimString_TypeIdx(_typeName);
	auto typeInfo = GetObjectTypeMetaInfo(typeName);
	if (typeInfo)
	{
		return typeInfo;
	}

	auto newTypeInfo = std::make_shared<ObjectTypeMetaInfo>();
	if (newTypeInfo->Load(typeName, Size, UnpackOnly))
	{
		AddTypeMetaInfo(newTypeInfo);
		return GetObjectTypeMetaInfo(typeName);
	}
	else
	{
		return nullptr;
	}
}

ObjectTypeMetaPtr GetOrUnpackTypeMetaInfo(const std::string& _typeName)
{
	return GetOrCreateTypeMetaInfoEx(_typeName, 0, true);
}

ObjectTypeMetaPtr GetOrCreateTypeMetaInfo(const std::string& _typeName, size_t Size)
{
	return GetOrCreateTypeMetaInfoEx(_typeName, Size, false);
}

bool HasTypeMetaInfo(const std::string& _typeName)
{
	auto typeName = TypeStrTrimString(_typeName);
	return TypeMetaInfoMap.find(typeName) != TypeMetaInfoMap.end();
}

bool HasTypeMetaInfo(uint32_t ID)
{
	return  TypeMetaIDMap.find(ID) != TypeMetaIDMap.end();
}

void AddTypeMetaInfo(ObjectTypeMetaPtr typeInfo)
{
	if (!typeInfo)return;
	if (HasTypeMetaInfo(typeInfo->typeName))
	{
		//resolve existing
		//probably pending
		auto& Obj = *TypeMetaInfoMap[typeInfo->typeName];
		if (Obj.ID != typeInfo->ID)
		{
			typeInfo->ID = ECUniqueID();
			for (auto& Tag : MetaTagSet)
				typeInfo->TryAttach(Tag);

			Obj = *typeInfo;
		}
	}
	else
	{
		typeInfo->ID = ECUniqueID();
		for (auto& Tag : MetaTagSet)
			typeInfo->TryAttach(Tag);


		TypeMetaInfoMap[typeInfo->typeName] = typeInfo;
		TypeMetaIDMap[typeInfo->ID] = typeInfo;
	}

	if (TraitPreprocessed)
	{
		for (auto& [_, trait] : TypeMetaTraitMap)
		{
			if (
				trait->GetTraitType() == ObjectTypeMetaTrait::Preprocessor &&
				trait->CanApplyToType(typeInfo, "")
			)
			{
				trait->ApplyToType(typeInfo, "");
			}
		}
	}
}

void AddMetaTag(ObjectTagPtr Tag)
{
	if (!Tag)return;
	if (MetaTagSet.contains(Tag))return;

	MetaTagSet.insert(Tag);

	for (auto& [_, typeInfo] : TypeMetaInfoMap)
		typeInfo->TryAttach(Tag);
}

bool ObjectTypeMetaInfo::Load(const std::string& _typeName, JsonObject Obj)
{
	typeName = TypeStrTrimString(_typeName);

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
		for (auto& baseClassItem : oBase.GetArrayObject())
		{
			ObjectTypeMeta_BaseClass bc;
			if (bc.Load(baseClassItem))
			{
				BaseClasses.push_back(std::move(bc));
			}
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
		UnionNames.push_back(member.MemberName);
		Members[member.MemberName] = std::move(member);
	}

	return true;
}

ObjectTypeMetaPtr ObjectTypeMeta_Basic::AddType(const std::string& typeName, _Type type)
{
	if (HasTypeMetaInfo(typeName))
	{
		return GetObjectTypeMetaInfo(typeName);
	}

	ObjectTypeMetaPtr typeInfo = std::make_shared<ObjectTypeMetaInfo>();
	typeInfo->typeName = TypeStrTrimString(typeName);
	ObjectTypeMeta_Basic ba;
	ba.BasicType = type;
	typeInfo->typeSize = ba.GetSize();
	typeInfo->typeData = ba;
	AddTypeMetaInfo(typeInfo);
	return typeInfo;
}

ObjectTypeMetaPtr ObjectTypeMeta_Void::AddType()
{
	ObjectTypeMetaPtr typeInfo = std::make_shared<ObjectTypeMetaInfo>();
	ObjectTypeMeta_Void v;
	typeInfo->typeName = "void";
	typeInfo->typeSize = 1;
	typeInfo->typeData = v;
	AddTypeMetaInfo(typeInfo);
	return typeInfo;
}

ObjectTypeMetaPtr ObjectTypeMeta_Pending::AddType()
{
	ObjectTypeMetaPtr typeInfo = std::make_shared<ObjectTypeMetaInfo>();
	ObjectTypeMeta_Pending p;
	typeInfo->typeName = "#raw";
	typeInfo->typeSize = 1;
	typeInfo->typeData = p;
	AddTypeMetaInfo(typeInfo);
	return typeInfo;
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
		MemberName = TypeStrTrimString(oName.GetString());
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

bool ObjectTypeMeta_BaseClass::Load(JsonObject Obj)
{
	/*
	{
		"type": "xxx",
		"size": n,
		"offset": n
	}
	*/
	auto oOffset = Obj.GetObjectItem("offset");
	if (oOffset && oOffset.IsTypeNumber())
		BaseOffset = static_cast<size_t>(oOffset.GetInt());
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

bool ObjectTypeMetaInfo::Load(const std::string& _typeName, size_t Size, bool UnpackOnly)
{
	//Resolve :
	//	ObjectTypeData_Basic,
	//	ObjectTypeData_FixedArray,
	//	ObjectTypeData_Pointer,
	//	ObjectTypeData_Pending
	typeName = TypeStrTrimString(_typeName);
	typeSize = Size;

	//	ObjectTypeData_Basic
	ObjectTypeMeta_Basic ba;
	if (ba.Load(typeName))
	{
		typeSize = ba.GetSize();
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

		if (UnpackOnly)
		{
			auto elemType = GetOrUnpackTypeMetaInfo(elemTypeName);
			if (!elemType)return false;

			ObjectTypeMeta_FixedArray fa;
			fa.ElementType = elemType;
			fa.ElementCount = elemCount;

			typeData = fa;
			typeSize = elemType->typeSize * elemCount;
			return true;
		}
		else
		{
			auto elemType = GetOrCreateTypeMetaInfo(elemTypeName, typeSize / elemCount);

			ObjectTypeMeta_FixedArray fa;
			fa.ElementType = elemType;
			fa.ElementCount = elemCount;

			typeData = fa;
			return true;
		}
	}

	//	ObjectTypeData_Pointer
	// if end with '*'
	if (typeName.back() == '*' && typeSize == sizeof(void*))
	{
		std::string pointedTypeName = typeName.substr(0, typeName.length() - 1);

		if (UnpackOnly)
		{
			//允许创建指向未知类型的指针
			auto pointedType = GetOrCreateTypeMetaInfo(pointedTypeName, 0);
			if (!pointedType) return false;

			ObjectTypeMeta_Pointer pa;
			pa.PointedType = pointedType;
			typeData = pa;
			typeSize = sizeof(void*);
			return true;
		}
		else
		{
			auto pointedType = GetOrCreateTypeMetaInfo(pointedTypeName, sizeof(void*));

			ObjectTypeMeta_Pointer pa;
			pa.PointedType = pointedType;
			typeData = pa;
			return true;
		}
	}

	//if (UnpackOnly) return false;

	//	ObjectTypeData_Pending
	ObjectTypeMeta_Pending pd;
	typeData = pd;
	return true;
}







bool ObjectAccessDirection::Load(JsonObject Obj)
{
	/*
	Format :
	{"Member": <str>}
	{"Member": [<str>, ...]}
	{"Index": <int>}
	{"Enum": <str>}
	{"Cast": <str>}
	{"ExplicitCast": <str>}
	"GetPtr"
	"Deref"
	"AllMembers"
	*/

	if (Obj.IsTypeString())
	{
		auto dirStr = Obj.GetString();
		if (dirStr == "GetPtr")
		{
			Direction = ObjectAccessDir_GetPtr{};
			return true;
		}
		else if (dirStr == "Deref")
		{
			Direction = ObjectAccessDir_Deref{};
			return true;
		}
		else if (dirStr == "AllMembers")
		{
			ObjectAccessDir_Member mem;
			mem.AllMembers = true;
			Direction = mem;
			return true;
		}
		else
		{
			return false;
		}
	}
	else if (Obj.IsTypeObject())
	{
		auto oMember = Obj.GetObjectItem("Member");
		if (oMember && oMember.IsTypeString())
		{
			ObjectAccessDir_Member mem;
			mem.MemberName.push_back(TypeStrTrimString(oMember.GetString()));
			mem.AllMembers = false;
			Direction = mem;
			return true;
		}
		else if (oMember && oMember.IsTypeArray())
		{
			ObjectAccessDir_Member mem;
			for (auto& memberItem : oMember.GetArrayObject())
			{
				if (memberItem.IsTypeString())mem.MemberName.push_back(TypeStrTrimString(memberItem.GetString()));
				else return false;
			}
			mem.AllMembers = false;
			Direction = mem;
			return true;
		}

		auto oIndex = Obj.GetObjectItem("Index");
		if (oIndex && oIndex.IsTypeNumber())
		{
			ObjectAccessDir_Index idx;
			idx.Index = static_cast<size_t>(oIndex.GetInt());
			Direction = idx;
			return true;
		}

		auto oEnum = Obj.GetObjectItem("Enum");
		if (oEnum && oEnum.IsTypeString())
		{
			ObjectAccessDir_Enum E;
			E.EnumName = TypeStrTrimString(oEnum.GetString());
			Direction = E;
			return true;
		}

		auto oCast = Obj.GetObjectItem("Cast");
		if (oCast && oCast.IsTypeString())
		{
			std::string typeName = TypeStrTrimString(oCast.GetString());
			auto TargetType = GetOrCreateTypeMetaInfo(typeName, 0);
			if (!TargetType) return false;
			ObjectAccessDir_Cast C;
			C.TargetType = TargetType;
			Direction = C;
			return true;
		}

		auto oExplicitCast = Obj.GetObjectItem("ExplicitCast");
		if (oExplicitCast && oExplicitCast.IsTypeString())
		{
			std::string typeName = TypeStrTrimString(oExplicitCast.GetString());
			auto TargetType = GetOrCreateTypeMetaInfo(typeName, 0);
			if (!TargetType) return false;
			ObjectAccessDir_ExplicitCast C;
			C.TargetType = TargetType;
			Direction = C;
			return true;
		}

		return false;
	}
	else return false;
}

bool ObjectAccessPath::Load(JsonObject Obj)
{
	if (!Obj.IsTypeArray())
		return false;
	for (auto& dirItem : Obj.GetArrayObject())
	{
		ObjectAccessDirection dir;
		if (!dir.Load(dirItem))
			return false;
		Directions.push_back(std::move(dir));
	}
	return true;
}

bool ObjectMetaTag::Load(JsonObject Obj)
{
	/*
	enum 
	{
		Equal,
		NotEqual,
		Contains,
		NotContains,
		Match,
		MatchFull,
		MatchNone,
		CanAccessPath,
		CannotAccessPath,
		AccessType,
		NotAccessType
	}
	TypeNameMatchMode;
	std::string ApplyTo;
	std::string Name;
	ObjectAccessPath TargetPath;
	JsonFile Arguments;
	*/
	if (!Obj) return false;

	//REQUIRED
	auto oName = Obj.GetObjectItem("name");
	if (oName && oName.IsTypeString())
		Name = oName.GetString();
	else return false;

	//REQUIRED
	auto oApplyTo = Obj.GetObjectItem("apply_to");
	if (oApplyTo && oApplyTo.IsTypeString())
		ApplyTo = oApplyTo.GetString();
	else return false;

	//REQUIRED
	auto oMatchMode = Obj.GetObjectItem("match_mode");
	if (oMatchMode && oMatchMode.IsTypeString())
	{
		std::string modeStr = oMatchMode.GetString();
		if (modeStr == "equal") TypeNameMatchMode = Equal;
		else if (modeStr == "not_equal") TypeNameMatchMode = NotEqual;
		else if (modeStr == "contains") TypeNameMatchMode = Contains;
		else if (modeStr == "not_contains") TypeNameMatchMode = NotContains;
		else if (modeStr == "match") TypeNameMatchMode = Match;
		else if (modeStr == "match_full") TypeNameMatchMode = MatchFull;
		else if (modeStr == "match_none") TypeNameMatchMode = MatchNone;
		else if (modeStr == "can_access_path") TypeNameMatchMode = CanAccessPath;
		else if (modeStr == "cannot_access_path") TypeNameMatchMode = CannotAccessPath;
		else if (modeStr == "access_type") TypeNameMatchMode = AccessType;
		else if (modeStr == "not_access_type") TypeNameMatchMode = NotAccessType;
		else {
			Debug::LogFormat("[Inspector] Unknown Tag Match Mode: \"{}\"\n", modeStr);
			return false;
		}
	}
	else return false;

	//OPTIONAL
	auto oTargetPath = Obj.GetObjectItem("target_path");
	if (oTargetPath)
	{
		if (!TargetPath.Load(oTargetPath))
			return false;
	}
	else TargetPath.Directions.clear();

	//OPTIONAL
	if (Obj.HasItem("arguments"))
	{
		Arguments = Obj.DetachObjectItem("arguments");
	}

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
	for (int i = 0; i < ObjectTypeMeta_Basic::COUNT; i++)
	{
		auto type = static_cast<ObjectTypeMeta_Basic::_Type>(i);
		auto typeName = ObjectTypeMeta_Basic::GetBasicTypeName(type);
		ObjectTypeMeta_Basic::AddType(typeName, type);
	}
	ObjectTypeMeta_Basic::AddType("unsigned char", ObjectTypeMeta_Basic::_Type::UInt8);
	ObjectTypeMeta_Basic::AddType("unsigned wchar_t", ObjectTypeMeta_Basic::_Type::UInt16);
	ObjectTypeMeta_Basic::AddType("unsigned short", ObjectTypeMeta_Basic::_Type::UInt16);
	ObjectTypeMeta_Basic::AddType("unsigned", ObjectTypeMeta_Basic::_Type::UInt32);
	ObjectTypeMeta_Basic::AddType("unsigned int", ObjectTypeMeta_Basic::_Type::UInt32);
	ObjectTypeMeta_Basic::AddType("unsigned long", ObjectTypeMeta_Basic::_Type::UInt32);
	ObjectTypeMeta_Basic::AddType("unsigned long long", ObjectTypeMeta_Basic::_Type::UInt64);
	ObjectTypeMeta_Basic::AddType("short", ObjectTypeMeta_Basic::_Type::Int16);
	ObjectTypeMeta_Basic::AddType("int", ObjectTypeMeta_Basic::_Type::Int32);
	ObjectTypeMeta_Basic::AddType("long", ObjectTypeMeta_Basic::_Type::Int32);
	ObjectTypeMeta_Basic::AddType("long long", ObjectTypeMeta_Basic::_Type::Int64);
	ObjectTypeMeta_Basic::AddType("signed char", ObjectTypeMeta_Basic::_Type::Char);
	ObjectTypeMeta_Basic::AddType("signed wchar_t", ObjectTypeMeta_Basic::_Type::WChar);
	ObjectTypeMeta_Basic::AddType("signed short", ObjectTypeMeta_Basic::_Type::Int16);
	ObjectTypeMeta_Basic::AddType("signed int", ObjectTypeMeta_Basic::_Type::Int32);
	ObjectTypeMeta_Basic::AddType("signed", ObjectTypeMeta_Basic::_Type::Int32);
	ObjectTypeMeta_Basic::AddType("signed long", ObjectTypeMeta_Basic::_Type::Int32);
	ObjectTypeMeta_Basic::AddType("signed long long", ObjectTypeMeta_Basic::_Type::Int64);

	ObjectTypeMeta_Void::AddType();


	// Init Traits
	InitTraitObject();

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
						auto typeInfo = std::make_shared<ObjectTypeMetaInfo>();
						if (typeInfo->Load(Name, Obj))
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

	// Init Tags
	// TODO: Add tag initialization logic here.
	typeDefDir = std::filesystem::current_path() / "Inspector" / "Tags";
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
					for (auto& Obj : jsonFile.GetObj().GetArrayObject())
					{
						auto Tag = std::make_shared<ObjectMetaTag>();
						if (Tag->Load(Obj))
						{
							AddMetaTag(Tag);
						}
						else
						{
							Debug::LogFormat("[Inspector] Failed to load Meta Tag : \"{}\"\n", Obj.GetText());
						}
					}
				}
			}
		}
	}

	for (const auto& [TypeName, TypeInfo] : TypeMetaInfoMap)
	{
		for (auto& [_, Trait] : TypeMetaTraitMap)
		{
			if (
				Trait->GetTraitType() == ObjectTypeMetaTrait::Preprocessor &&
				Trait->CanApplyToType(TypeInfo, "")
			)
				Trait->ApplyToType(TypeInfo, "");
		}
	}
	TraitPreprocessed = true;


	// Output Debug Info
	// TODO: Add debug output logic here.
	for (const auto& [TypeName, TypeInfo] : TypeMetaInfoMap)
	{
		if (TypeInfo->IsType<ObjectTypeMeta_Pending>())
		{
			Debug::LogFormat("[Inspector] Type \"{}\" is pending definition.\n", TypeName);
			//std::println("\033[37m[Inspector]\033[1;33m Type \"{}\" is pending definition.\033[0m", TypeName);
		}
	}
}


