#include "Inspector.h"
#include "Misc.h"
#include <Debug.h>

bool ObjectTypeMetaInfo::TryAttach(ObjectTagPtr Tag, SkipTryAttachCheck)
{
	if (!Tag)return false;

	Tags.insert({ Tag->Name, Tag });

	return true;
}

bool ObjectTypeMetaInfo::TryAttach(ObjectTagPtr Tag)
{
	if (!Tag)return false;

	bool CanAttach = false;

	switch (Tag->TypeNameMatchMode)	
	{
	case ObjectMetaTag::Equal:
		CanAttach = (typeName == Tag->ApplyTo);
		break;
	case ObjectMetaTag::NotEqual:
		CanAttach = (typeName != Tag->ApplyTo);
		break;
	case ObjectMetaTag::Contains:
		CanAttach = (typeName.find(Tag->ApplyTo) != std::string::npos);
		break;
	case ObjectMetaTag::NotContains:
		CanAttach = (typeName.find(Tag->ApplyTo) == std::string::npos);
		break;
	case ObjectMetaTag::Match:
		CanAttach = RegexNotNone_Nothrow(typeName, Tag->ApplyTo);
		break;
	case ObjectMetaTag::MatchFull:
		CanAttach = RegexFull_Nothrow(typeName, Tag->ApplyTo);
		break;
	case ObjectMetaTag::MatchNone:
		CanAttach = RegexNone_Nothrow(typeName, Tag->ApplyTo);
		break;
	case ObjectMetaTag::CanAccessPath:
		CanAttach = AccessibleViaPath(Tag->TargetPath);
		break;
	case ObjectMetaTag::CannotAccessPath:
		CanAttach = !AccessibleViaPath(Tag->TargetPath);
		break;
	case ObjectMetaTag::AccessType:
		CanAttach = CanAccessTypeViaPath(Tag->TargetPath, Tag->ApplyTo);
		break;
	case ObjectMetaTag::NotAccessType:
		CanAttach = !CanAccessTypeViaPath(Tag->TargetPath, Tag->ApplyTo);
		break;
	}

	if (!CanAttach) return false;

	Tags.insert({ Tag->Name, Tag });

	return true;
}

bool ObjectTypeMetaInfo::HasTag(const std::string& TagName) const
{
	return Tags.contains(TagName);
}

ObjectTypeMetaInfo::_TagMapRange ObjectTypeMetaInfo::GetTag(const std::string& TagName)
{
	if (Tags.contains(TagName))
		return Tags.equal_range(TagName);
	return {Tags.end(), Tags.end()};
}

bool ObjectTypeMetaInfo::AccessibleViaPath(const ObjectAccessPath& Path)
{
	return AccessibleViaPath(Path, 0);
}

bool ObjectTypeMetaInfo::CanAccessTypeViaPath(const ObjectAccessPath& Path, ObjectTypeMetaPtr TargetType)
{
	auto Types = AccessTypeViaPath(Path);
	for (auto& T : Types)
	{
		if (T->ID == TargetType->ID)
			return true;
	}
	return false;
}

bool ObjectTypeMetaInfo::CanAccessTypeViaPath(const ObjectAccessPath& Path, const std::string& TargetTypeName)
{
	if (TargetTypeName == "#struct")
		return CanAccessMetaTypeViaPath<ObjectTypeMeta_Struct>(Path);
	if (TargetTypeName == "#union")
		return CanAccessMetaTypeViaPath<ObjectTypeMeta_Union>(Path);
	if (TargetTypeName == "#array")
		return CanAccessMetaTypeViaPath<ObjectTypeMeta_FixedArray>(Path);
	if (TargetTypeName == "#basic")
		return CanAccessMetaTypeViaPath<ObjectTypeMeta_Basic>(Path);
	if (TargetTypeName == "#pointer")
		return CanAccessMetaTypeViaPath<ObjectTypeMeta_Pointer>(Path);
	if (TargetTypeName == "#enum")
		return CanAccessMetaTypeViaPath<ObjectTypeMeta_Enum>(Path);
	if (TargetTypeName == "#pending")
		return CanAccessMetaTypeViaPath<ObjectTypeMeta_Pending>(Path);
	if (TargetTypeName == "#void")
		return CanAccessMetaTypeViaPath<ObjectTypeMeta_Void>(Path);




	auto TargetType = GetOrUnpackTypeMetaInfo(TargetTypeName);
	if (!TargetType) return false;
	return CanAccessTypeViaPath(Path, TargetType);
}

std::vector<ObjectTypeMetaPtr> ObjectTypeMetaInfo::AccessTypeViaPath(const ObjectAccessPath& Path)
{
	return AccessTypeViaPath(Path, 0);
}

bool ObjectTypeMetaInfo::AccessibleViaPath(const ObjectAccessPath& Path, size_t CurrentIndex)
{
	if (CurrentIndex >= Path.Directions.size())
		return true;

	auto& Dir = Path.Directions[CurrentIndex];
	auto NextType = AccessType(Dir);
	if (NextType.empty())
		return false;
	for (auto& Next : NextType)
	{
		if (Next->AccessibleViaPath(Path, CurrentIndex + 1))
			return true;
	}
	return false;
}

std::vector<ObjectTypeMetaPtr> ObjectTypeMetaInfo::AccessTypeViaPath(const ObjectAccessPath& Path, size_t CurrentIndex)
{
	if (CurrentIndex >= Path.Directions.size())
		return { this->shared_from_this() };

	auto& Dir = Path.Directions[CurrentIndex];
	auto NextType = AccessType(Dir);
	if (NextType.empty())
		return {};

	std::vector<ObjectTypeMetaPtr> Result;
	for (auto& Next : NextType)
	{
		if (CurrentIndex == Path.Directions.size() - 1)
			Result.push_back(Next);
		else
		{
			auto NextResults = Next->AccessTypeViaPath(Path, CurrentIndex + 1);
			Result.append_range(NextResults);
		}
	}

	return Result;
}

std::vector<ObjectTypeMetaPtr> ObjectTypeMetaInfo::AccessType(ObjectAccessDirection Dir)
{
	std::vector<ObjectTypeMetaPtr> Result;

	std::visit([this, &Dir, &Result](const auto& v) {
		std::visit([v, this, &Result](const auto& dir) {
			auto P = v.AccessType(*this, dir);
			if constexpr (std::is_same_v<decltype(P), ObjectTypeMetaPtr>)
			{
				if (P)Result.push_back(P);
			}
			else if constexpr (std::is_same_v<decltype(P), std::vector<ObjectTypeMetaPtr>>)
			{
				Result.append_range(P);
			}
			//P can also be nullptr_t (meaning no accessible)
		}, Dir.Direction);
	}, typeData);

	std::visit([this, &Result](const auto& dir) {
		Result.append_range(AccessTypeByTag(dir));
	}, Dir.Direction);

	return Result;
}

ObjectTypeMetaPtr ObjectTypeMetaInfo::GetPointerType() const
{
	return GetOrUnpackTypeMetaInfo(typeName + "*");
}

bool ObjectTypeMeta_Struct::IsOnInheritTree(ObjectTypeMetaInfo& info, ObjectTypeMetaPtr TargetType) const
{
	auto ID = info.ID;
	if (ID == TargetType->ID)
		return true;
	for (auto& Base : BaseClasses)
	{
		if (Base.MemberType->ID == TargetType->ID)
			return true;

		if (Base.MemberType->IsType<ObjectTypeMeta_Struct>())
		{
			auto& BaseStruct = std::get<ObjectTypeMeta_Struct>(Base.MemberType->typeData);
			if (BaseStruct.IsOnInheritTree(*Base.MemberType, TargetType))
				return true;
		}
	}

	return false;
}

ObjectTypeMetaPtr ObjectTypeMeta_Basic::PickIntBySize(size_t Size)
{
	if (Size == 1)
		return GetOrCreateTypeMetaInfo(GetBasicTypeName(_Type::Int8), 1);
	else if (Size == 2)
		return GetOrCreateTypeMetaInfo(GetBasicTypeName(_Type::Int16), 2);
	else if (Size == 4)
		return GetOrCreateTypeMetaInfo(GetBasicTypeName(_Type::Int32), 4);
	else if (Size == 8)
		return GetOrCreateTypeMetaInfo(GetBasicTypeName(_Type::Int64), 8);
	else return nullptr;
}
ObjectTypeMetaPtr ObjectTypeMeta_Basic::PickUnsignedIntBySize(size_t Size)
{
	if(Size == 1)
		return GetOrCreateTypeMetaInfo(GetBasicTypeName(_Type::UInt8), 1);
	else if (Size == 2)
		return GetOrCreateTypeMetaInfo(GetBasicTypeName(_Type::UInt16), 2);
	else if (Size == 4)
		return GetOrCreateTypeMetaInfo(GetBasicTypeName(_Type::UInt32), 4);
	else if (Size == 8)
		return GetOrCreateTypeMetaInfo(GetBasicTypeName(_Type::UInt64), 8);
	else return nullptr;
}
ObjectTypeMetaPtr ObjectTypeMeta_Basic::PickRealBySize(size_t Size)
{
	if(Size == 4)
		return GetOrCreateTypeMetaInfo(GetBasicTypeName(_Type::Float), 4);
	else if (Size == 8)
		return GetOrCreateTypeMetaInfo(GetBasicTypeName(_Type::Double), 8);
	else return nullptr;
}










/*
----------------------------------------------------------------------
-----------------------------ACCESS TYPE------------------------------
----------------------------------------------------------------------
*/


std::vector<ObjectTypeMetaPtr> ObjectTypeMetaInfo::AccessTypeByTag(const ObjectAccessDir_Member& mem)
{
	
	if (mem.AllMembers)
	{
		if (CanApplyTrait("AccessMember", ""))
			return (ApplyTrait("AccessMember", ""));
	}
	else
	{
		std::vector<ObjectTypeMetaPtr> Result;
		for(auto& str : mem.MemberName)
			if (CanApplyTrait("AccessMember", str))
				Result.append_range(ApplyTrait("AccessMember", str));
		return Result;
	}
	return {};
}
std::vector<ObjectTypeMetaPtr> ObjectTypeMetaInfo::AccessTypeByTag(const ObjectAccessDir_Index& idx)
{
	auto str = std::to_string(idx.Index);
	if (CanApplyTrait("AccessIndex", str))
		return ApplyTrait("AccessIndex", str);
	return {};
}
std::vector<ObjectTypeMetaPtr> ObjectTypeMetaInfo::AccessTypeByTag(const ObjectAccessDir_GetPtr&)
{
	if (CanApplyTrait("AccessGetPtr", ""))
		return ApplyTrait("AccessGetPtr", "");
	return {};
}
std::vector<ObjectTypeMetaPtr> ObjectTypeMetaInfo::AccessTypeByTag(const ObjectAccessDir_Deref&)
{
	if (CanApplyTrait("AccessDeref", ""))
		return ApplyTrait("AccessDeref", "");
	return {};
}
std::vector<ObjectTypeMetaPtr> ObjectTypeMetaInfo::AccessTypeByTag(const ObjectAccessDir_Cast& CastTo)
{
	if (CanApplyTrait("AccessCast", CastTo.TargetType->typeName))
		return ApplyTrait("AccessCast", CastTo.TargetType->typeName);
	return {};
}
std::vector<ObjectTypeMetaPtr> ObjectTypeMetaInfo::AccessTypeByTag(const ObjectAccessDir_ExplicitCast& CastTo)
{
	if (CanApplyTrait("AccessExplicitCast", CastTo.TargetType->typeName))
		return ApplyTrait("AccessExplicitCast", CastTo.TargetType->typeName);
	return {};
}
std::vector<ObjectTypeMetaPtr> ObjectTypeMetaInfo::AccessTypeByTag(const ObjectAccessDir_Enum& E)
{
	if (CanApplyTrait("AccessEnum", E.EnumName))
		return ApplyTrait("AccessEnum", E.EnumName);
	return {};
}

std::vector<ObjectTypeMetaPtr> ObjectTypeMeta_Struct::AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Member& mem) const
{
	std::vector<ObjectTypeMetaPtr> Result;
	if (mem.AllMembers)
		for (auto& [_, Member] : Members)
			Result.push_back(Member.MemberType);
	else for (auto& Name : mem.MemberName)
	{
		auto it = Members.find(Name);
		if (it != Members.end())
			Result.push_back(it->second.MemberType);
	}
	
	for (auto& Base : BaseClasses)
	{
		if (Base.MemberType->IsType<ObjectTypeMeta_Struct>())
		{
			auto& BaseStruct = std::get<ObjectTypeMeta_Struct>(Base.MemberType->typeData);
			auto res = BaseStruct.AccessType(*Base.MemberType, mem);
			Result.append_range(res);
		}
	}

	return Result;
}
nullptr_t ObjectTypeMeta_Struct::AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Index&) const
{
	return nullptr;
}
ObjectTypeMetaPtr ObjectTypeMeta_Struct::AccessType(ObjectTypeMetaInfo& info, const ObjectAccessDir_GetPtr&) const
{
	return info.GetPointerType();
}
nullptr_t ObjectTypeMeta_Struct::AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Deref&) const
{
	return nullptr;
}
ObjectTypeMetaPtr ObjectTypeMeta_Struct::AccessType(ObjectTypeMetaInfo& info, const ObjectAccessDir_Cast& CastTo) const
{
	if(IsOnInheritTree(info, CastTo.TargetType))
		return CastTo.TargetType;
	return nullptr;
}
ObjectTypeMetaPtr ObjectTypeMeta_Struct::AccessType(ObjectTypeMetaInfo& info, const ObjectAccessDir_ExplicitCast& CastTo) const
{
	if (IsOnInheritTree(info, CastTo.TargetType))
		return CastTo.TargetType;
	return nullptr;
}
nullptr_t ObjectTypeMeta_Struct::AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Enum&) const
{
	return nullptr;
}


ObjectTypeMetaPtr ObjectTypeMeta_Enum::AccessType(ObjectTypeMetaInfo& info, const ObjectAccessDir_Member& mem) const
{
	if(mem.AllMembers)
		return ObjectTypeMeta_Basic::PickIntBySize(info.typeSize);
	else for(auto& Name : mem.MemberName)
		if (EnumValues.contains(Name))
			return ObjectTypeMeta_Basic::PickIntBySize(info.typeSize);
	return nullptr;
}
nullptr_t ObjectTypeMeta_Enum::AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Index&) const
{
	return nullptr;
}
ObjectTypeMetaPtr ObjectTypeMeta_Enum::AccessType(ObjectTypeMetaInfo& info, const ObjectAccessDir_GetPtr&) const
{
	return info.GetPointerType();
}
nullptr_t ObjectTypeMeta_Enum::AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Deref&) const
{
	return nullptr;
}
ObjectTypeMetaPtr ObjectTypeMeta_Enum::AccessType(ObjectTypeMetaInfo& info, const ObjectAccessDir_Cast& CastTo) const
{
	if (CastTo.TargetType->IsType<ObjectTypeMeta_Basic>())
		return CastTo.TargetType;
	else if (CastTo.TargetType->IsType<ObjectTypeMeta_Enum>())
		return CastTo.TargetType;
	return nullptr;
}
ObjectTypeMetaPtr ObjectTypeMeta_Enum::AccessType(ObjectTypeMetaInfo& info, const ObjectAccessDir_ExplicitCast& CastTo) const
{
	if (CastTo.TargetType->ID == info.ID)
		return CastTo.TargetType;
	return nullptr;
}
ObjectTypeMetaPtr ObjectTypeMeta_Enum::AccessType(ObjectTypeMetaInfo& info, const ObjectAccessDir_Enum& E) const
{
	if (EnumValues.contains(E.EnumName))
		return ObjectTypeMeta_Basic::PickIntBySize(info.typeSize);
	return nullptr;
}


std::vector<ObjectTypeMetaPtr> ObjectTypeMeta_Union::AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Member& mem) const
{
	std::vector<ObjectTypeMetaPtr> Result;
	if (mem.AllMembers) 
		for (auto& [_, Member] : Members)
			Result.push_back(Member.MemberType);
	else for (auto& Name : mem.MemberName)
	{
		auto it = Members.find(Name);
		if (it != Members.end())
			Result.push_back(it->second.MemberType);
	}
	return Result;
}
nullptr_t ObjectTypeMeta_Union::AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Index&) const
{
	return nullptr;
}
ObjectTypeMetaPtr ObjectTypeMeta_Union::AccessType(ObjectTypeMetaInfo& info, const ObjectAccessDir_GetPtr&) const
{
	return info.GetPointerType();
}
nullptr_t ObjectTypeMeta_Union::AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Deref&) const
{
	return nullptr;
}
ObjectTypeMetaPtr ObjectTypeMeta_Union::AccessType(ObjectTypeMetaInfo& info, const ObjectAccessDir_Cast& CastTo) const
{
	if(CastTo.TargetType->ID == info.ID)
		return CastTo.TargetType;
	for (auto& [_, Member] : Members)
	{
		if (CastTo.TargetType->ID == Member.MemberType->ID)
			return CastTo.TargetType;
	}
	return nullptr;
}
ObjectTypeMetaPtr ObjectTypeMeta_Union::AccessType(ObjectTypeMetaInfo& info, const ObjectAccessDir_ExplicitCast& CastTo) const
{
	if (CastTo.TargetType->ID == info.ID)
		return CastTo.TargetType;
	return nullptr;
}
nullptr_t ObjectTypeMeta_Union::AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Enum&) const
{
	return nullptr;
}


nullptr_t ObjectTypeMeta_Basic::AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Member&) const
{
	return nullptr;
}
nullptr_t ObjectTypeMeta_Basic::AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Index&) const
{
	return nullptr;
}
ObjectTypeMetaPtr ObjectTypeMeta_Basic::AccessType(ObjectTypeMetaInfo& info, const ObjectAccessDir_GetPtr&) const
{
	return info.GetPointerType();
}
nullptr_t ObjectTypeMeta_Basic::AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Deref&) const
{
	return nullptr;
}
ObjectTypeMetaPtr ObjectTypeMeta_Basic::AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Cast& CastTo) const
{
	if (CastTo.TargetType->IsType<ObjectTypeMeta_Basic>())
		return CastTo.TargetType;
	else if (CastTo.TargetType->typeSize == GetSize())
		return CastTo.TargetType;
	return nullptr;
}
ObjectTypeMetaPtr ObjectTypeMeta_Basic::AccessType(ObjectTypeMetaInfo& info, const ObjectAccessDir_ExplicitCast& CastTo) const
{
	if (CastTo.TargetType->IsType<ObjectTypeMeta_Basic>())
	{
		ObjectTypeMeta_Basic& TargetBasic = std::get<ObjectTypeMeta_Basic>(CastTo.TargetType->typeData);
		if (TargetBasic.BasicType == BasicType)
			return CastTo.TargetType;
	}
	return nullptr;
}
nullptr_t ObjectTypeMeta_Basic::AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Enum&) const
{
	return nullptr;
}


nullptr_t ObjectTypeMeta_FixedArray::AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Member&) const
{
	return nullptr;
}
ObjectTypeMetaPtr ObjectTypeMeta_FixedArray::AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Index&) const
{
	return ElementType;
}
ObjectTypeMetaPtr ObjectTypeMeta_FixedArray::AccessType(ObjectTypeMetaInfo& info, const ObjectAccessDir_GetPtr&) const
{
	return info.GetPointerType();
}
ObjectTypeMetaPtr ObjectTypeMeta_FixedArray::AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Deref&) const
{
	return ElementType;
}
ObjectTypeMetaPtr ObjectTypeMeta_FixedArray::AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Cast& CastTo) const
{
	if (CastTo.TargetType->IsType<ObjectTypeMeta_Pointer>())
		return CastTo.TargetType;
	else if (CastTo.TargetType->IsType<ObjectTypeMeta_FixedArray>())
		return CastTo.TargetType;
	return nullptr;
}
ObjectTypeMetaPtr ObjectTypeMeta_FixedArray::AccessType(ObjectTypeMetaInfo& info, const ObjectAccessDir_ExplicitCast& CastTo) const
{
	if (CastTo.TargetType->ID == info.ID)
		return CastTo.TargetType;
	return nullptr;
}
nullptr_t ObjectTypeMeta_FixedArray::AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Enum&) const
{
	return nullptr;
}


nullptr_t ObjectTypeMeta_Pointer::AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Member&) const
{
	return nullptr;
}
ObjectTypeMetaPtr ObjectTypeMeta_Pointer::AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Index&) const
{
	return PointedType;
}
ObjectTypeMetaPtr ObjectTypeMeta_Pointer::AccessType(ObjectTypeMetaInfo& info, const ObjectAccessDir_GetPtr&) const
{
	return info.GetPointerType();
}
ObjectTypeMetaPtr ObjectTypeMeta_Pointer::AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Deref&) const
{
	return PointedType;
}
ObjectTypeMetaPtr ObjectTypeMeta_Pointer::AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Cast& CastTo) const
{
	if(CastTo.TargetType->IsType<ObjectTypeMeta_Pointer>())
		return CastTo.TargetType;
	else if(CastTo.TargetType->IsType<ObjectTypeMeta_FixedArray>())
		return CastTo.TargetType;
	return nullptr;
}
ObjectTypeMetaPtr ObjectTypeMeta_Pointer::AccessType(ObjectTypeMetaInfo& info, const ObjectAccessDir_ExplicitCast& CastTo) const
{
	if (CastTo.TargetType->ID == info.ID)
		return CastTo.TargetType;
	return nullptr;
}
nullptr_t ObjectTypeMeta_Pointer::AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Enum&) const
{
	return nullptr;
}


nullptr_t ObjectTypeMeta_Pending::AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Member&) const
{
	return nullptr;
}
nullptr_t ObjectTypeMeta_Pending::AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Index&) const
{
	return nullptr;
}
ObjectTypeMetaPtr ObjectTypeMeta_Pending::AccessType(ObjectTypeMetaInfo& info, const ObjectAccessDir_GetPtr&) const
{
	return info.GetPointerType();
}
nullptr_t ObjectTypeMeta_Pending::AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Deref&) const
{
	return nullptr;
}
ObjectTypeMetaPtr ObjectTypeMeta_Pending::AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Cast& CastTo) const
{
	return CastTo.TargetType;
}
ObjectTypeMetaPtr ObjectTypeMeta_Pending::AccessType(ObjectTypeMetaInfo& info, const ObjectAccessDir_ExplicitCast& CastTo) const
{
	if (CastTo.TargetType->ID == info.ID)
		return CastTo.TargetType;
	return nullptr;
}
nullptr_t ObjectTypeMeta_Pending::AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Enum&) const
{
	return nullptr;
}


nullptr_t ObjectTypeMeta_Void::AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Member&) const
{
	return nullptr;
}
nullptr_t ObjectTypeMeta_Void::AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Index&) const
{
	return nullptr;
}
ObjectTypeMetaPtr ObjectTypeMeta_Void::AccessType(ObjectTypeMetaInfo& info, const ObjectAccessDir_GetPtr&) const
{
	return info.GetPointerType();
}
nullptr_t ObjectTypeMeta_Void::AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Deref&) const
{
	return nullptr;
}
ObjectTypeMetaPtr ObjectTypeMeta_Void::AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Cast& CastTo) const
{
	if (CastTo.TargetType->IsType<ObjectTypeMeta_Void>())
		return CastTo.TargetType;
	return nullptr;
}
ObjectTypeMetaPtr ObjectTypeMeta_Void::AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_ExplicitCast& CastTo) const
{
	if (CastTo.TargetType->IsType<ObjectTypeMeta_Void>())
		return CastTo.TargetType;
	return nullptr;
}
nullptr_t ObjectTypeMeta_Void::AccessType(ObjectTypeMetaInfo&, const ObjectAccessDir_Enum&) const
{
	return nullptr;
}
