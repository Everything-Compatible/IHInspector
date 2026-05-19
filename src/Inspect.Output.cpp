#include "Inspector.h"
#include <format>
#include <print>

JsonFile InspectType(ObjectTypeMetaPtr Meta)
{
	if (!Meta)return JsonFile{};	
	return Meta->Inspect();
}

JsonFile ObjectTypeMetaInfo::Inspect()
{
	JsonFile f;
	auto r = f.GetObj();
	r.AddString("type_name", typeName);

	if(IsType<ObjectTypeMeta_Pending>())
	{
		r.AddString("type", "Pending");
		return f;
	}

	r.AddString("type_id", std::format("0x{:08X}", ID));
	r.AddInt("size", static_cast<int>(typeSize));

	//Type Data
	//Meta->typeData
	std::visit(
		[&r](auto&& arg) {
			arg.Inspect(r);
		}
		, typeData
	);

	std::vector<std::string> s;
	s.reserve(Tags.size());
	for (auto& [Name, Tag] : Tags)
		s.push_back(Name);

	r.AddArrayString("tags", s);

	return f;
}

void ObjectTypeMeta_Member::Inspect(JsonObject Obj)
{
	Obj.AddString("type_name", MemberType->typeName);
	Obj.AddInt("offset", static_cast<int>(MemberOffset));
}

void ObjectTypeMeta_BaseClass::Inspect(JsonObject Obj)
{
	Obj.AddString("type_name", MemberType->typeName);
	Obj.AddInt("offset", static_cast<int>(BaseOffset));
}

void ObjectTypeMeta_Struct::Inspect(JsonObject Obj)
{
	Obj.AddString("type", "Struct");

	std::vector<JsonFile> F;
	for (auto& Base : BaseClasses)
	{
		JsonFile um;
		Base.Inspect(um);
		F.push_back(std::move(um));
	}
	Obj.AddArrayObject("base_classes", std::move(F));

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

const char* ObjectTypeMeta_Basic::GetBasicTypeName(_Type Type)
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
	return Names[Type];
}

const char* ObjectTypeMeta_Basic::GetBasicTypeName() const
{
	return GetBasicTypeName(BasicType);
}

size_t ObjectTypeMeta_Basic::GetSize() const
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
	static size_t const Sizes[] = {
		1,
		1,
		1,
		2,
		2,
		4,
		4,
		8,
		8,
		4,
		8,
		1,
		2,
	};
	return Sizes[BasicType];
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

void ObjectTypeMeta_Void::Inspect(JsonObject Obj) {
	Obj.AddString("type", "void");
}


/*
----------------------------------------------------------------------
---------------------------INSTANCE VIEW------------------------------
----------------------------------------------------------------------
*/

JsonFile ViewAsObject(ObjectInstance Instance, bool ShowID, int Depth)
{
	return Instance.View(ShowID, Depth);
}

void ObjectInstance::ViewBasic(JsonObject r, bool ShowID)
{
	r.AddString("address", std::format("0x{:08X}", Address));
	r.AddString("type_name", TypeInfo->typeName);
	if(ShowID)r.AddString("type_id", std::format("0x{:08X}", TypeInfo->ID));
	r.AddInt("size", static_cast<int>(TypeInfo->typeSize));
}

JsonFile ObjectInstance::View(bool ShowID, int Depth)
{
	JsonFile f;
	auto r = f.GetObj();
	
	ViewBasic(r, ShowID);

	if (Depth == 0)
	{
		//Custom Briefing Tag
		if (CanApplyTrait("GetCustomBriefing", ""))
		{
			auto BriefingInfos = GetTraitInfo("GetCustomBriefing", "");
			r.AddObjectItem("briefing", std::move(BriefingInfos));
		}
	}

	std::visit(
		[&, this](auto&& arg) {
			arg.View(*this, r, Depth, ShowID);
		}
		, TypeInfo->typeData
	);

	return f;
}


void ObjectTypeMeta_Struct::View(ObjectInstance& Instance, JsonObject Obj, int Depth, bool ShowID) const
{
	Obj.AddString("type", "Struct");

	if (Depth == 0)
	{
		Obj.AddString("info", std::format("Struct {} at 0x{:08X}",
			Instance.TypeInfo->typeName,
			Instance.Address));
	}
	else
	{
		bool FlattenLayout = Instance.TypeInfo->HasTag("FlattenLayout");
		JsonFile jf;
		std::unordered_set<std::string> usedNames;
		
		if (!FlattenLayout)
		{
			JsonFile bs;
			for (auto& base : BaseClasses)
			{
				auto& BaseType = base.MemberType;
				DWORD baseAddress = Instance.Address + static_cast<DWORD>(base.BaseOffset);

				ObjectInstance BaseInstance;
				BaseInstance.Address = baseAddress;
				BaseInstance.TypeInfo = BaseType;

				JsonFile baseObj = BaseInstance.View(ShowID, Depth - 1);
				bs.GetObj().AddObjectItem(BaseType->typeName, std::move(baseObj));
			}
			Obj.AddObjectItem("base_classes", std::move(bs));
		}
		else
		{
			JsonFile bs;
			for (auto& base : BaseClasses)
			{
				JsonFile bt;

				auto& BaseType = base.MemberType;
				DWORD baseAddress = Instance.Address + static_cast<DWORD>(base.BaseOffset);

				ObjectInstance BaseInstance;
				BaseInstance.Address = baseAddress;
				BaseInstance.TypeInfo = BaseType;

				BaseInstance.ViewBasic(bt.GetObj(), ShowID);
				
				bs.GetObj().AddObjectItem(base.MemberType->typeName, std::move(bt));
			}
			Obj.AddObjectItem("base_classes", std::move(bs));

			//base class can be struct or union
			//flatten base members
			for (auto& base : BaseClasses)
			{
				auto& BaseType = base.MemberType;
				DWORD baseAddress = Instance.Address + static_cast<DWORD>(base.BaseOffset);

				ObjectInstance BaseInstance;
				BaseInstance.Address = baseAddress;
				BaseInstance.TypeInfo = BaseType;

				if (BaseType->IsType<ObjectTypeMeta_Struct>())
				{
					auto& BaseStruct = std::get<ObjectTypeMeta_Struct>(BaseType->typeData);

					for (auto& Name : BaseStruct.MemberOrders)
					{
						auto& Mem = BaseStruct.Members.at(Name);
						bool NeedsRename = usedNames.find(Mem.MemberName) != usedNames.end();
						DWORD memberAddress = baseAddress + static_cast<DWORD>(Mem.MemberOffset);

						ObjectInstance MemInstance;
						MemInstance.Address = memberAddress;
						MemInstance.TypeInfo = Mem.MemberType;

						auto RegName = NeedsRename ? BaseType->typeName + "::" + Mem.MemberName : Mem.MemberName;
						jf.GetObj().AddObjectItem(RegName, MemInstance.View(ShowID, Depth - 1));
						usedNames.insert(RegName);
					}
				}
				else if (BaseType->IsType<ObjectTypeMeta_Union>())
				{
					//Key Name = Union Type
					auto& BaseUnion = std::get<ObjectTypeMeta_Union>(BaseType->typeData);

					jf.GetObj().AddObjectItem("Base::" + BaseType->typeName, BaseInstance.View(ShowID, Depth - 1));
				}
			}
		}
		
		for (auto& Name : MemberOrders)
		{
			auto& Mem = Members.at(Name);
			DWORD memberAddress = Instance.Address + static_cast<DWORD>(Mem.MemberOffset);
			bool NeedsRename = usedNames.find(Mem.MemberName) != usedNames.end();

			ObjectInstance MemInstance;
			MemInstance.Address = memberAddress;
			MemInstance.TypeInfo = Mem.MemberType;

			auto RegName = NeedsRename ? Instance.TypeInfo->typeName + "::" + Mem.MemberName : Mem.MemberName;
			jf.GetObj().AddObjectItem(RegName, MemInstance.View(ShowID, Depth - 1));
		}

		Obj.AddObjectItem("members", std::move(jf));
	}
}

void ObjectTypeMeta_Enum::View(ObjectInstance& Instance, JsonObject Obj, int Depth, bool ShowID) const
{
	Obj.AddString("type", "Enum");

	// read enum value
	auto Address = reinterpret_cast<LPVOID>(Instance.Address);
	SIZE_T readSize = 0;
	auto Size = Instance.TypeInfo->typeSize;

	int64_t value = 0;
	ReadProcessMemory(GetCurrentProcess(), Address, &value, Size, &readSize);

	if (readSize == Size)
	{
		Obj.AddString("enum_value", std::format("{}", value));
		auto it = EnumNames.find(value);
		if (it != EnumNames.end())
		{
			Obj.AddString("enum_name", it->second);
		}
		else
		{
			//try bitmask
			//turn to unsigned and check bits from the largest enum value
			uint64_t uvalue = static_cast<uint64_t>(value);
			std::vector<std::string> matchedNames;

			for (auto rit = EnumNames.rbegin(); rit != EnumNames.rend(); ++rit)
			{
				uint64_t enumVal = static_cast<uint64_t>(rit->first);
				if (enumVal != 0 && (uvalue & enumVal) == enumVal)
				{
					matchedNames.push_back(rit->second);
					uvalue &= ~enumVal;
				}
			}

			if (matchedNames.size() > 0)
			{
				if (uvalue != 0)
					matchedNames.push_back(std::format("0x{:X}", uvalue));
				Obj.AddArrayString("enum_name", matchedNames);
			}
			else {
				Obj.AddString("enum_name", "??");
			}
		}
	}
	else
	{
		Obj.AddString("enum_value", "??");
		return;
	}
}

void ObjectTypeMeta_Union::View(ObjectInstance& Instance, JsonObject Obj, int Depth, bool ShowID) const
{
	Obj.AddString("type", "Union");

	if (Depth == 0)
	{
		Obj.AddString("info", std::format("Union {} at 0x{:08X}",
			Instance.TypeInfo->typeName,
			Instance.Address));
	}
	else
	{
		if (Instance.CanApplyTrait("SelectUnionMember", ""))
		{
			auto Chosen = Instance.ApplyTrait("SelectUnionMember", "");
			
			auto& MemInstance = Chosen[0];
			JsonFile chosenObj = MemInstance.View(ShowID, Depth - 1);
			Obj.AddObjectItem("selected_member", std::move(chosenObj));
		}
		else
		{
			JsonFile jf;
			for (auto& [Name, Mem] : Members)
			{
				DWORD memberAddress = Instance.Address + static_cast<DWORD>(Mem.MemberOffset);
				ObjectInstance MemInstance;
				MemInstance.Address = memberAddress;
				MemInstance.TypeInfo = Mem.MemberType;
				jf.GetObj().AddObjectItem(Name, MemInstance.View(ShowID, Depth - 1));
			}
			Obj.AddObjectItem("members", std::move(jf));
		}
	}
}

void ObjectTypeMeta_Basic::View(ObjectInstance& Instance, JsonObject Obj, int Depth, bool ShowID) const
{
	// type & value
	Obj.AddString("type", GetBasicTypeName());

	// read value
	LPVOID Address = reinterpret_cast<LPVOID>(Instance.Address);
	SIZE_T readSize = 0;

	auto ReadVal = [&](auto& value) -> auto {
		ReadProcessMemory(GetCurrentProcess(), Address, &value, sizeof(value), &readSize);
		return (readSize == sizeof(value));
	};

	auto Resolve = [&](auto& value) {
		if(ReadVal(value))Obj.AddString("value", std::format("{}", value));
		else Obj.AddString("value", "??");
	};

	auto ResolveT = [&]<typename T>() {
		T value;
		Resolve(value);
	};

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
	switch (BasicType)
	{
	case Bool:ResolveT.template operator() < bool > (); break;
	case Int8:ResolveT.template operator() < int8_t > (); break;
	case UInt8:ResolveT.template operator() < uint8_t > (); break;
	case Int16:ResolveT.template operator() < int16_t > (); break;
	case UInt16:ResolveT.template operator() < uint16_t > (); break;
	case Int32:ResolveT.template operator() < int32_t > (); break;
	case UInt32:ResolveT.template operator() < uint32_t > (); break;
	case Int64:ResolveT.template operator() < int64_t > (); break;
	case UInt64:ResolveT.template operator() < uint64_t > (); break;
	case Float:ResolveT.template operator() < float > (); break;
	case Double:ResolveT.template operator() < double > (); break;
	case Char:
	{
		char value;
		if (ReadVal(value))Obj.AddString("value", EscapeString(std::string{ value }));
		else Obj.AddString("value", "??");
	}break;
	case WChar:
	{
		wchar_t value;
		if (ReadVal(value))Obj.AddString("value", EscapeString(~UTF16ToUTF8(std::wstring{ value })));
		else Obj.AddString("value", "??");
	}break;
	}
}

void ObjectTypeMeta_FixedArray::View(ObjectInstance& Instance, JsonObject Obj, int Depth, bool ShowID) const
{
	Obj.AddString("type", "Array");

	if (Depth == 0)
	{
		Obj.AddString("elements", std::format("Array {}[{}] at 0x{:08X}",
			ElementType->typeName,
			ElementCount,
			Instance.Address));
	}
	else
	{
		// read array elements
		LPVOID Address = reinterpret_cast<LPVOID>(Instance.Address);
		size_t elemSize = ElementType->typeSize;
		std::vector<JsonFile> Elements;
		for (size_t i = 0; i < ElementCount; i++)
		{
			DWORD elemAddress = Instance.Address + static_cast<DWORD>(i * elemSize);
			ObjectInstance ElemInstance;
			ElemInstance.Address = elemAddress;
			ElemInstance.TypeInfo = ElementType;
			JsonFile elemObj = ElemInstance.View(ShowID, Depth - 1);
			Elements.push_back(std::move(elemObj));
		}
		Obj.AddString("element_type", ElementType->typeName);
		Obj.AddInt("element_count", static_cast<int>(ElementCount));
		Obj.AddArrayObject("elements", std::move(Elements));
	}
}

void ObjectTypeMeta_Pointer::View(ObjectInstance& Instance, JsonObject Obj, int Depth, bool ShowID) const
{
	Obj.AddString("type", "Pointer");

	// read pointer value
	LPVOID Address = reinterpret_cast<LPVOID>(Instance.Address);

	DWORD value;
	SIZE_T readSize = 0;
	ReadProcessMemory(GetCurrentProcess(), Address, &value, sizeof(DWORD), &readSize);

	if (readSize == sizeof(DWORD))
	{
		Obj.AddString("pointer_value", std::format("0x{:08X}", value));
	}
	else
	{
		Obj.AddString("pointer_value", "??");
		return;
	}

	if (Depth != 0)
	{
		ObjectInstance Pointed;
		Pointed.Address = value;
		Pointed.TypeInfo = PointedType;

		JsonFile pointedObj = Pointed.View(ShowID, Depth - 1);
		Obj.AddObjectItem("pointed", std::move(pointedObj));
	}
}


void ObjectTypeMeta_Pending::View(ObjectInstance& Instance, JsonObject Obj, int Depth, bool ShowID) const
{
	Obj.AddString("type", "Pending");
	if (Depth == 0)
	{
		Obj.AddString("raw_data", std::format("byte[{}] at 0x{:08X}",
			Instance.TypeInfo->typeSize,
			Instance.Address));
	}
	else
	{
		// view as raw
		auto Size = Instance.TypeInfo->typeSize;
		LPVOID Address = reinterpret_cast<LPVOID>(Instance.Address);

		std::vector<uint8_t> Data;
		SIZE_T readSize = 0;
		Data.resize(Size);
		ReadProcessMemory(GetCurrentProcess(), Address, Data.data(), Size, &readSize);

		//as hex & string
		std::string HexStr;
		for (size_t i = 0; i < readSize; i++)
			HexStr += std::format("{:02X} ", Data[i]);
		for (size_t i = readSize; i < Size; i++)
			HexStr += "?? ";
		Obj.AddString("raw_data_hex", HexStr);

		std::string AsciiStr;
		for (size_t i = 0; i < readSize; i++)
		{
			if (Data[i] >= 32 && Data[i] <= 126)
				AsciiStr += static_cast<char>(Data[i]);
			else
				AsciiStr += '.';
		}
		for (size_t i = readSize; i < Size; i++)
			AsciiStr += '?';
		Obj.AddString("raw_data_ascii", AsciiStr);
	}
}


void ObjectTypeMeta_Void::View(ObjectInstance& Instance, JsonObject Obj, int Depth, bool ShowID) const
{
	// nothing to view
	//Obj.AddString("type", "void");
	return;
}