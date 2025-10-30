#include "Inspector.h"
#include <format>
#include <print>

JsonFile InspectType(ObjectTypeMetaInfo* Meta)
{
	JsonFile f;
	if (!Meta) 	return f;
	auto r = f.GetObj();

	r.AddString("type_name", Meta->typeName);
	//r.AddString("TypeID", std::format("0x{:08X}", reinterpret_cast<uint32_t>(Meta)));
	r.AddInt("size", static_cast<int>(Meta->typeSize));

	//Type Data
	//Meta->typeData
	std::visit(
		[&r](auto&& arg) {
			arg.Inspect(r);
		}
		, Meta->typeData
	);

	return f;

	//Tags
	//todo
}

void ObjectTypeMeta_Member::Inspect(JsonObject Obj)
{
	Obj.AddInt("offset", static_cast<int>(MemberOffset));
	Obj.AddString("type_name", MemberType->typeName);
	//Obj.AddString("TypeID", std::format("0x{:08X}", reinterpret_cast<uint32_t>(MemberType)));
}

void ObjectTypeMeta_Struct::Inspect(JsonObject Obj)
{
	Obj.AddString("type", "Struct");
	Obj.AddArrayString("base_classes", BaseClasses);

	JsonFile jf;
	for (auto Name : MemberOrders)
	{
		JsonFile um;
		auto& Mem = Members[Name];
		Mem.Inspect(um);
		jf.GetObj().AddObjectItem(Mem.MemberName, std::move(um));
	}

	Obj.AddObjectItem("members", std::move(jf));
}

void ObjectTypeMeta_Enum::Inspect(JsonObject Obj)
{
	Obj.AddString("type", "Enum");
	JsonFile jf;
	for (auto& [Name, Value] : EnumValues)
	{
		jf.GetObj().AddInt(Name, static_cast<int>(Value));
	}
	Obj.AddObjectItem("values", std::move(jf));
}

void ObjectTypeMeta_Union::Inspect(JsonObject Obj)
{
	Obj.AddString("type", "Union");
	JsonFile jf;
	for (auto& [Name, Member] : Members)
	{
		JsonFile um;
		auto& Mem = Members[Name];
		Mem.Inspect(um);
		jf.GetObj().AddObjectItem(Name, std::move(um));
	}

	Obj.AddObjectItem("members", std::move(jf));
}

const char* ObjectTypeMeta_Basic::GetBasicTypeName() const
{
	/*
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
	*/
	static const char* const Names[] = {
		"bool",
		"int8_t",
		"uint8_t",
		"int16_t",
		"uint16_t",
		"int32_t",
		"uint32_t",
		"int64_t",
		"uint64_t",
		"float",
		"double",
		"char",
		"wchar_t",
	};
	return Names[BasicType];
}

void ObjectTypeMeta_Basic::Inspect(JsonObject Obj)
{
	Obj.AddString("type", GetBasicTypeName());
}

void ObjectTypeMeta_FixedArray::Inspect(JsonObject Obj) {
	Obj.AddString("type", "Array");
	Obj.AddInt("count", static_cast<int>(ElementCount));
	Obj.AddString("elem_type_name", ElementType->typeName);
	//Obj.AddString("ElemTypeID", std::format("0x{:08X}", reinterpret_cast<uint32_t>(ElementType)));
}

void ObjectTypeMeta_Pointer::Inspect(JsonObject Obj) {
	Obj.AddString("type", "Pointer");
	Obj.AddString("pointed_type_name", PointedType->typeName);
	//Obj.AddString("PointedTypeID", std::format("0x{:08X}", reinterpret_cast<uint32_t>(PointedType)));
}

void ObjectTypeMeta_Pending::Inspect(JsonObject Obj) {
	Obj.AddString("type", "Pending");
}