#pragma once
#include <EC.h>
#include <variant>
#include <optional>
#include <memory>
#include <map>

//All chars are UTF-8

std::string TrimString(const std::string& str);
std::string TypeStrTrimString(const std::string& str);

struct ObjectTypeMetaInfo;
using ObjectTypeMetaPtr = std::shared_ptr<ObjectTypeMetaInfo>;
struct ObjectMetaTag;
using ObjectTagPtr = std::shared_ptr<ObjectMetaTag>;
struct ObjectAccessDirection;
struct ObjectInstance;
struct ObjectAccessPath;

struct ObjectAccessDir_Member
{
	bool AllMembers = false;
	std::vector<std::string> MemberName;
};

struct ObjectAccessDir_Index
{
	size_t Index;
};

struct ObjectAccessDir_GetPtr
{

};

struct ObjectAccessDir_Deref
{

};

struct ObjectAccessDir_Cast
{
	ObjectTypeMetaPtr TargetType;
};

struct ObjectAccessDir_ExplicitCast
{
	ObjectTypeMetaPtr TargetType;
};

struct ObjectAccessDir_Enum
{
	std::string EnumName;
};

struct ObjectTypeMeta_Member
{
	std::string MemberName;
	size_t MemberOffset;
	ObjectTypeMetaPtr MemberType;

	bool Load(JsonObject Obj);
	void Inspect(JsonObject Obj);
};

struct ObjectTypeMeta_BaseClass
{
	size_t BaseOffset;
	ObjectTypeMetaPtr MemberType;

	bool Load(JsonObject Obj);
	void Inspect(JsonObject Obj); 
};

struct ObjectTypeMeta_Struct
{
	std::vector<ObjectTypeMeta_BaseClass> BaseClasses;
	std::unordered_map<std::string, ObjectTypeMeta_Member> Members;
	std::vector<std::string> MemberOrders;

	bool Load(JsonObject Obj);
	void Inspect(JsonObject Obj);

	bool IsOnInheritTree(ObjectTypeMetaInfo& info, ObjectTypeMetaPtr TargetType) const;

	std::vector<ObjectTypeMetaPtr> AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Member&) const;
	nullptr_t AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Index&) const;
	ObjectTypeMetaPtr AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_GetPtr&) const;
	nullptr_t AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Deref&) const;
	ObjectTypeMetaPtr AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Cast&) const;
	ObjectTypeMetaPtr AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_ExplicitCast&) const;
	nullptr_t AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Enum&) const;

	void View(ObjectInstance& Instance, JsonObject Obj, int Depth, bool ShowID) const;
};

struct ObjectTypeMeta_Enum
{
	std::unordered_map<std::string, int64_t> EnumValues;
	std::map<int64_t, std::string> EnumNames;

	bool Load(JsonObject Obj);
	void Inspect(JsonObject Obj);

	ObjectTypeMetaPtr AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Member&) const;
	nullptr_t AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Index&) const;
	ObjectTypeMetaPtr AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_GetPtr&) const;
	nullptr_t AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Deref&) const;
	ObjectTypeMetaPtr AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Cast&) const;
	ObjectTypeMetaPtr AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_ExplicitCast&) const;
	ObjectTypeMetaPtr AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Enum&) const;

	void View(ObjectInstance& Instance, JsonObject Obj, int Depth, bool ShowID) const;
};

struct ObjectTypeMeta_Union
{
	std::unordered_map<std::string, ObjectTypeMeta_Member> Members;
	std::vector<std::string> UnionNames;

	bool Load(JsonObject Obj);
	void Inspect(JsonObject Obj);

	std::vector<ObjectTypeMetaPtr> AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Member&) const;
	nullptr_t AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Index&) const;
	ObjectTypeMetaPtr AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_GetPtr&) const;
	nullptr_t AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Deref&) const;
	ObjectTypeMetaPtr AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Cast&) const;
	ObjectTypeMetaPtr AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_ExplicitCast&) const;
	nullptr_t AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Enum&) const;

	void View(ObjectInstance& Instance, JsonObject Obj, int Depth, bool ShowID) const;
};

struct ObjectTypeMeta_Basic
{
	enum _Type{
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
		COUNT
	} BasicType;

	const char* GetBasicTypeName() const;
	size_t GetSize() const;

	bool Load(const std::string& typeStr);
	void Inspect(JsonObject Obj);

	nullptr_t AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Member&) const;
	nullptr_t AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Index&) const;
	ObjectTypeMetaPtr AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_GetPtr&) const;
	nullptr_t AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Deref&) const;
	ObjectTypeMetaPtr AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Cast&) const;
	ObjectTypeMetaPtr AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_ExplicitCast&) const;
	nullptr_t AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Enum&) const;

	void View(ObjectInstance& Instance, JsonObject Obj, int Depth, bool ShowID) const;

	static const char* GetBasicTypeName(_Type Type);
	static ObjectTypeMetaPtr PickIntBySize(size_t Size);
	static ObjectTypeMetaPtr PickUnsignedIntBySize(size_t Size);
	static ObjectTypeMetaPtr PickRealBySize(size_t Size);
	static ObjectTypeMetaPtr AddType(const std::string& typeName, _Type type);

};

struct ObjectTypeMeta_FixedArray
{
	ObjectTypeMetaPtr ElementType;
	size_t ElementCount;
	void Inspect(JsonObject Obj);

	nullptr_t AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Member&) const;
	ObjectTypeMetaPtr AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Index&) const;
	ObjectTypeMetaPtr AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_GetPtr&) const;
	ObjectTypeMetaPtr AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Deref&) const;
	ObjectTypeMetaPtr AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Cast&) const;
	ObjectTypeMetaPtr AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_ExplicitCast&) const;
	nullptr_t AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Enum&) const;

	void View(ObjectInstance& Instance, JsonObject Obj, int Depth, bool ShowID) const;
};

//NOTE : char* & wchar_t* 特别认为是Basic而非Pointer
struct ObjectTypeMeta_Pointer
{
	ObjectTypeMetaPtr PointedType;
	void Inspect(JsonObject Obj);

	nullptr_t AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Member&) const;
	ObjectTypeMetaPtr AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Index&) const;
	ObjectTypeMetaPtr AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_GetPtr&) const;
	ObjectTypeMetaPtr AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Deref&) const;
	ObjectTypeMetaPtr AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Cast&) const;
	ObjectTypeMetaPtr AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_ExplicitCast&) const;
	nullptr_t AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Enum&) const;

	void View(ObjectInstance& Instance, JsonObject Obj, int Depth, bool ShowID) const;
};

struct ObjectTypeMeta_Pending
{
	void Inspect(JsonObject Obj);

	nullptr_t AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Member&) const;
	nullptr_t AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Index&) const;
	ObjectTypeMetaPtr AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_GetPtr&) const;
	nullptr_t AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Deref&) const;
	ObjectTypeMetaPtr AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Cast&) const;
	ObjectTypeMetaPtr AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_ExplicitCast&) const;
	nullptr_t AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Enum&) const;

	void View(ObjectInstance& Instance, JsonObject Obj, int Depth, bool ShowID) const;

	static ObjectTypeMetaPtr AddType();
};

struct ObjectTypeMeta_Void
{
	void Inspect(JsonObject Obj);

	nullptr_t AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Member&) const;
	nullptr_t AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Index&) const;
	ObjectTypeMetaPtr AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_GetPtr&) const;
	nullptr_t AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Deref&) const;
	ObjectTypeMetaPtr AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Cast&) const;
	ObjectTypeMetaPtr AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_ExplicitCast&) const;
	nullptr_t AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Enum&) const;

	void View(ObjectInstance& Instance, JsonObject Obj, int Depth, bool ShowID) const;

	static ObjectTypeMetaPtr AddType();
};

using TypeMetaVar = std::variant <
	ObjectTypeMeta_Struct,
	ObjectTypeMeta_Enum,
	ObjectTypeMeta_Union,
	ObjectTypeMeta_Basic,
	ObjectTypeMeta_FixedArray,
	ObjectTypeMeta_Pointer,
	ObjectTypeMeta_Void,
	ObjectTypeMeta_Pending
>;
//改变这个的同时改变ObjectTypeMetaInfo::CanAccessTypeViaPath

struct ObjectInstance
{
	DWORD Address;
	ObjectTypeMetaPtr TypeInfo;

	bool CanApplyTrait(const std::string& TraitName, const std::string& Arg);
	std::vector<ObjectInstance> ApplyTrait(const std::string& TraitName, const std::string& Arg);
	JsonFile GetTraitInfo(const std::string& TraitName, const std::string& Arg);

	std::vector<ObjectInstance> AccessObjectViaPath(const ObjectAccessPath& Path);
	JsonFile View(bool ShowID, int Depth);

	void ViewBasic(JsonObject obj, bool ShowID);
};

struct ObjectInstanceVar
{
	std::string VarName;

	UTF8_String ValueString() const;
};

struct ObjectAccessDirection
{
	std::variant<
		ObjectAccessDir_Member,
		ObjectAccessDir_Index,
		ObjectAccessDir_Enum,
		ObjectAccessDir_GetPtr,
		ObjectAccessDir_Deref,
		ObjectAccessDir_Cast,
		ObjectAccessDir_ExplicitCast
	> Direction;

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
	bool Load(JsonObject Obj);
};

struct ObjectAccessPath
{
	std::vector<ObjectAccessDirection> Directions;

	bool Load(JsonObject Obj);
};

struct ObjectMetaTag : public std::enable_shared_from_this<ObjectMetaTag>
{
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

	bool Load(JsonObject Obj);
};

struct SkipTryAttachCheck final {};

struct ObjectTypeMetaInfo : public std::enable_shared_from_this<ObjectTypeMetaInfo>
{
	std::string typeName;
	size_t typeSize;
	uint32_t ID;

	TypeMetaVar typeData;
	std::unordered_multimap<std::string, ObjectTagPtr> Tags;

	bool Load(const std::string& _typeName, JsonObject Obj);

	//assume always return true
	bool Load(const std::string& _typeName, size_t Size, bool UnpackOnly);

	JsonFile Inspect();

	bool TryAttach(ObjectTagPtr Tag);
	bool TryAttach(ObjectTagPtr Tag, SkipTryAttachCheck);

	bool AccessibleViaPath(const ObjectAccessPath& Path);
	bool CanAccessTypeViaPath(const ObjectAccessPath& Path, ObjectTypeMetaPtr TargetType);
	bool CanAccessTypeViaPath(const ObjectAccessPath& Path, const std::string& TargetTypeName);

	template<typename T>
	bool CanAccessMetaTypeViaPath(const ObjectAccessPath& Path)
	{
		for (auto& Ty : AccessTypeViaPath(Path))if (Ty->IsType<T>())return true;
		return false;
	}

	bool HasTag(const std::string& TagName) const;

	using _TagMapIter = std::unordered_multimap<std::string, ObjectTagPtr>::iterator;
	using _TagMapRange = std::pair<_TagMapIter, _TagMapIter>;
	_TagMapRange GetTag(const std::string& TagName);

	//return empty if not accessible
	std::vector<ObjectTypeMetaPtr> AccessType(ObjectAccessDirection Dir);

	std::vector<ObjectTypeMetaPtr> AccessTypeViaPath(const ObjectAccessPath& Path);

	ObjectTypeMetaPtr GetPointerType() const;

	template<typename T>
	bool IsType() const
	{
		return std::holds_alternative<T>(typeData);
	}

	bool CanApplyTrait(const std::string& TraitName, const std::string& Arg);
	std::vector<ObjectTypeMetaPtr> ApplyTrait(const std::string& TraitName, const std::string& Arg);

private :
	bool AccessibleViaPath(const ObjectAccessPath& Path, size_t CurrentIndex);
	std::vector<ObjectTypeMetaPtr> AccessTypeViaPath(const ObjectAccessPath& Path, size_t CurrentIndex);

	std::vector<ObjectTypeMetaPtr> AccessTypeByTag(const ObjectAccessDir_Member&);
	std::vector<ObjectTypeMetaPtr> AccessTypeByTag(const ObjectAccessDir_Index&);
	std::vector<ObjectTypeMetaPtr> AccessTypeByTag(const ObjectAccessDir_GetPtr&);
	std::vector<ObjectTypeMetaPtr> AccessTypeByTag(const ObjectAccessDir_Deref&);
	std::vector<ObjectTypeMetaPtr> AccessTypeByTag(const ObjectAccessDir_Cast&);
	std::vector<ObjectTypeMetaPtr> AccessTypeByTag(const ObjectAccessDir_ExplicitCast&);
	std::vector<ObjectTypeMetaPtr> AccessTypeByTag(const ObjectAccessDir_Enum&);
};

struct NOVTABLE ObjectTypeMetaTrait
{
	enum TraitType {
		Preprocessor,
		Primary,
		Secondary
	};

	virtual TraitType GetTraitType() const = 0;
	virtual std::vector<ObjectInstance> GetTraitObjects(ObjectInstance Instance, const std::string& Arg) = 0;
	virtual JsonFile GetTraitInfo(ObjectInstance Instance, const std::string& Arg) = 0;
	virtual std::vector<ObjectTypeMetaPtr> ApplyToType(ObjectTypeMetaPtr TypeInfo, const std::string& Arg) = 0;
	virtual bool CanApplyToType(ObjectTypeMetaPtr TypeInfo, const std::string& Arg) = 0;
	virtual bool CanApplyToObject(ObjectInstance Instance, const std::string& Arg) = 0;
};

extern std::unordered_map<std::string, ObjectTypeMetaPtr> TypeMetaInfoMap;
extern std::unordered_map<uint32_t, ObjectTypeMetaPtr> TypeMetaIDMap;
extern std::unordered_set<ObjectTagPtr> MetaTagSet;
extern std::unordered_map<std::string, std::unique_ptr<ObjectTypeMetaTrait>> TypeMetaTraitMap;

//MUST CHECK - maybe nullptr
ObjectTypeMetaPtr GetObjectTypeMetaInfo(const std::string& typeName);
ObjectTypeMetaPtr GetObjectTypeMetaInfo(uint32_t ID);

//try to unpack : array/pointer/basic
ObjectTypeMetaPtr GetOrUnpackTypeMetaInfo(const std::string& typeName);

//always assume return not null
ObjectTypeMetaPtr GetOrCreateTypeMetaInfo(const std::string& typeName, size_t Size);

bool HasTypeMetaInfo(const std::string& typeName);

bool HasTypeMetaInfo(uint32_t ID);

void AddTypeMetaInfo(ObjectTypeMetaPtr typeInfo);

void AddMetaTag(ObjectTagPtr Tag);

void InitTraitObject();

void InitTypeMetaInfo();

void AddObjectInstance(const std::string& VarName, ObjectInstance Instance);

void AddPresetObjectInstance(const std::string& VarName, ObjectInstance Instance);

ObjectTypeMetaTrait* GetTypeMetaTrait(const std::string& TraitName);




//EXPORT BASE:

JsonFile ViewAsObject(ObjectInstance Instance, bool ShowID, int Depth);

std::optional<ObjectInstance> GetVarInstance(ObjectInstanceVar Variable);

ObjectInstance AccessPath(ObjectInstance Instance, const ObjectAccessPath& Path);

JsonFile GetTraitInfo(ObjectInstance Instance, const std::string& TraitName);

std::vector<ObjectInstance> GetTraitObjects(ObjectInstance Instance, const std::string& TraitName);

JsonFile InspectType(ObjectTypeMetaPtr Meta);


