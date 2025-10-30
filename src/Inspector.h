#pragma once
#include <EC.h>
#include <variant>
#include <optional>

//All chars are UTF-8

std::string TrimString(const std::string& str);

struct ObjectTypeMetaInfo;

struct ObjectTypeMeta_Member
{
	std::string MemberName;
	size_t MemberOffset;
	ObjectTypeMetaInfo* MemberType;

	bool Load(JsonObject Obj);
	void Inspect(JsonObject Obj);
};

struct ObjectTypeMeta_Struct
{
	std::vector<std::string> BaseClasses;
	std::unordered_map<std::string, ObjectTypeMeta_Member> Members;
	std::vector<std::string> MemberOrders;

	bool Load(JsonObject Obj);
	void Inspect(JsonObject Obj);
};

struct ObjectTypeMeta_Enum
{
	std::unordered_map<std::string, int64_t> EnumValues;
	std::unordered_map<int64_t, std::string> EnumNames;

	bool Load(JsonObject Obj);
	void Inspect(JsonObject Obj);
};

struct ObjectTypeMeta_Union
{
	std::unordered_map<std::string, ObjectTypeMeta_Member> Members;
	std::vector<std::string> UnionTypes;

	bool Load(JsonObject Obj);
	void Inspect(JsonObject Obj);
};

struct ObjectTypeMeta_Basic
{
	enum {
		Bool,
		Int8,
		UInt8,
		Int16,
		UInt16,
		Int32,
		UInt32,
		Int64,
		UInt64,
		Float,
		Double,
		Char,
		WChar,
	} BasicType;

	const char* GetBasicTypeName() const;

	bool Load(const std::string& typeStr);
	void Inspect(JsonObject Obj);
};

struct ObjectTypeMeta_FixedArray
{
	ObjectTypeMetaInfo* ElementType;
	size_t ElementCount;
	void Inspect(JsonObject Obj);
};

//NOTE : char* & wchar_t* 特别认为是Basic而非Pointer
struct ObjectTypeMeta_Pointer
{
	ObjectTypeMetaInfo* PointedType;
	void Inspect(JsonObject Obj);
};

struct ObjectTypeMeta_Pending
{
	void Inspect(JsonObject Obj);
};

using TypeMetaVar = std::variant <
	ObjectTypeMeta_Struct,
	ObjectTypeMeta_Enum,
	ObjectTypeMeta_Union,
	ObjectTypeMeta_Basic,
	ObjectTypeMeta_FixedArray,
	ObjectTypeMeta_Pointer,
	ObjectTypeMeta_Pending
>;

struct ObjectInstance
{
	DWORD Address;
	ObjectTypeMetaInfo* TypeInfo;
};

struct ObjectInstanceVar
{
	std::string VarName;
};

struct ObjectMetaTrait
{
	std::string TraitName;
	std::function<std::optional<JsonFile>(ObjectInstance)> GetTraitInfo;
	std::function<std::vector<ObjectInstance>(ObjectInstance)> GetTraitObjects;
};

struct ObjectAccessDir_Member
{
	std::string MemberName;
};

struct ObjectAccessDir_Index
{
	size_t Index;
};

struct ObjectAccessDir_AddrOf
{

};

struct ObjectAccessDir_Deref
{

};

struct ObjectAccessDir_Reinterpret
{
	ObjectTypeMetaInfo* TargetType;
};

struct ObjectAccessDir_Enum
{
	std::string EnumName;
};

struct ObjectAccessDirection
{
	std::variant<
		ObjectAccessDir_Member,
		ObjectAccessDir_Index,
		ObjectAccessDir_Enum,
		ObjectAccessDir_AddrOf,
		ObjectAccessDir_Deref,
		ObjectAccessDir_Reinterpret
	> Direction;
};

struct ObjectAccessPath
{
	std::vector<ObjectAccessDirection> Directions;
};

struct ObjectMetaTag
{
	std::string Name;
	ObjectAccessPath TargetPath;
	JsonFile Arguments;
	bool Load(JsonObject Obj);
};

struct ObjectTypeMetaInfo
{
	std::string typeName;
	size_t typeSize;

	TypeMetaVar typeData;
	std::unordered_map<std::string, ObjectMetaTag> Tags;

	bool Load(const std::string& _typeName, JsonObject Obj);

	//assume always return true
	bool Load(const std::string& _typeName, size_t Size);
};

extern std::unordered_map<std::string, std::unique_ptr<ObjectTypeMetaInfo>> TypeMetaInfoMap;
extern std::unordered_set<ObjectTypeMetaInfo*> UsedMetaInfoPtr;

ObjectTypeMetaInfo* GetObjectTypeMetaInfo(const std::string& typeName);

//always assume return not null
ObjectTypeMetaInfo* GetOrCreateTypeMetaInfo(const std::string& typeName, size_t Size);

bool HasTypeMetaInfo(const std::string& typeName);

bool HasTypeMetaInfo(const ObjectTypeMetaInfo* pMeta);

void AddTypeMetaInfo(const ObjectTypeMetaInfo& typeInfo);

void InitTypeMetaInfo();

void AddObjectInstance(const std::string& VarName, ObjectInstance Instance);


//EXPORT BASE:

JsonFile ViewAsObject(ObjectInstance Instance);

std::optional<ObjectInstance> GetVarInstance(ObjectInstanceVar Variable);

ObjectInstance AccessPath(ObjectInstance Instance, const ObjectAccessPath& Path);

JsonFile GetTraitInfo(ObjectInstance Instance, const std::string& TraitName);

std::vector<ObjectInstance> GetTraitObjects(ObjectInstance Instance, const std::string& TraitName);

JsonFile InspectType(ObjectTypeMetaInfo* Meta);



