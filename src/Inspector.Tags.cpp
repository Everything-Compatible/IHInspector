#include "Inspector.h"

/*

Tags :
name apply_to match_mode : REQUIRED
target_path arguments : OPTIONAL

1. ResolveType
Name = "ResolveType"
TargetPath
Arguments = {
	"target_type": "<type_name>"
}
按照 TargetPath 指定的路径，将类型信息解析为 Arguments 中指定的 target_type 类型的信息。

2. AddTag
Name = "AddTag"
TargetPath
Arguments = {
	"tag_name": "<tag_name>"
	"tag_path": <target_path>?
	"tag_args": <object>?
}
在 TargetPath 指定的路径，添加一个新的 Tag。Tag 名称由 Arguments 中的 tag_name 指定
tag_path 和 tag_args 可选，分别指定 Tag 的 TargetPath 和 Arguments。

3. FlattenLayout
Name = "FlattenLayout"
No TargetPath Arguments
针对 Struct 类型，在输出时展平所有基类成员。
针对 Array 类型，在输出时展平所有元素成员。

4.CustomBriefing //TODO
Name = "CustomBriefing"
No TargetPath
Arguments = {
	...
}
为类型或实例添加自定义简要信息，具体内容由 Arguments 指定。
仅在Depth = 0时生效并触发输出。

5.SelectUnionMember //TODO
Name = "SelectUnionMember"
TargetPath
Arguments = {
	...
}
为 Union 类型的实例选择一个成员进行查看，成员选择逻辑由 Arguments 指定。

6.AsCString //TODO
Name = "AsCString"
TargetPath
Arguments = {
	"encoding": "<encoding>" // "utf-8" (default), "utf-16", "gbk"
}
将字符串类型的成员以 C 风格字符串进行查看。
TargetPath 必须指向一个指针。

7.

*/

std::unordered_map<std::string, std::unique_ptr<ObjectTypeMetaTrait>> TypeMetaTraitMap;

struct ObjectTypeMetaTrait_ResolveType : public ObjectTypeMetaTrait
{
	void TryResolveType(ObjectTypeMetaPtr TypeInfo)
	{
		if (TypeInfo->HasTag("ResolveType"))
		{
			auto Tags = TypeInfo->GetTag("ResolveType");
			for (auto it = Tags.first; it != Tags.second; ++it)
			{
				auto& Tag = it->second;

				auto oTargetName = Tag->Arguments.GetObj().GetObjectItem("target_type");
				if (oTargetName && oTargetName.IsTypeString())
				{
					std::string TargetTypeName = oTargetName.GetString();
					auto TargetTypeInfo = GetOrUnpackTypeMetaInfo(TargetTypeName);

					if (TargetTypeInfo)
					{
						auto TypeToResolve = TypeInfo->AccessTypeViaPath(Tag->TargetPath);
						for (auto& T : TypeToResolve)
						{
							T->Tags = TargetTypeInfo->Tags;
							T->typeSize = TargetTypeInfo->typeSize;
							T->typeData = TargetTypeInfo->typeData;
						}
					}

				}
			}

			
		}
	}

	virtual TraitType GetTraitType() const
	{
		return Preprocessor;
	}
	virtual std::vector<ObjectInstance> GetTraitObjects(ObjectInstance Instance, const std::string&)
	{
		return {};
	}
	virtual JsonFile GetTraitInfo(ObjectInstance Instance, const std::string&)
	{
		return JsonFile{};
	}
	virtual std::vector<ObjectTypeMetaPtr> ApplyToType(ObjectTypeMetaPtr TypeInfo, const std::string&)
	{
		TryResolveType(TypeInfo);
		return {};
	}
	virtual bool CanApplyToType(ObjectTypeMetaPtr TypeInfo, const std::string&)
	{
		return TypeInfo->HasTag("ResolveType");
	}
	virtual bool CanApplyToObject(ObjectInstance Instance, const std::string&)
	{
		return Instance.TypeInfo->HasTag("ResolveType");
	}
};

struct ObjectTypeMetaTrait_AddTag : public ObjectTypeMetaTrait
{
	void TryAddTag(ObjectTypeMetaPtr TypeInfo)
	{
		if (TypeInfo->HasTag("AddTag"))
		{
			auto Tags = TypeInfo->GetTag("AddTag");
			for (auto it = Tags.first; it != Tags.second; ++it)
			{
				auto& Tag = it->second;
				auto oTagName = Tag->Arguments.GetObj().GetObjectItem("tag_name");
				if (oTagName && oTagName.IsTypeString())
				{
					std::string NewTagName = oTagName.GetString();
					ObjectTagPtr NewTag = std::make_shared<ObjectMetaTag>();
					NewTag->Name = NewTagName;

					auto oTagPath = Tag->Arguments.GetObj().GetObjectItem("tag_path");
					if (oTagPath && oTagPath.IsTypeArray())
					{
						if (!NewTag->TargetPath.Load(oTagPath))
							return;
					}
					else
					{
						NewTag->TargetPath.Directions.clear();
					}


					auto oTagArgs = Tag->Arguments.GetObj().GetObjectItem("tag_args");
					if (oTagArgs)
					{
						NewTag->Arguments.DuplicateFromObject(oTagArgs, true);
					}

					auto TypeToAddTag = TypeInfo->AccessTypeViaPath(Tag->TargetPath);

					for (auto& T : TypeToAddTag)
					{
						T->TryAttach(NewTag, SkipTryAttachCheck{});
					}
				}
			}
		}
	}

	virtual TraitType GetTraitType() const
	{
		return Preprocessor;
	}
	virtual std::vector<ObjectInstance> GetTraitObjects(ObjectInstance Instance, const std::string&)
	{
		return {};
	}
	virtual JsonFile GetTraitInfo(ObjectInstance Instance, const std::string&)
	{
		return JsonFile{};
	}
	virtual std::vector<ObjectTypeMetaPtr> ApplyToType(ObjectTypeMetaPtr TypeInfo, const std::string&)
	{
		TryAddTag(TypeInfo);
		return {};
	}
	virtual bool CanApplyToType(ObjectTypeMetaPtr TypeInfo, const std::string&)
	{
		return TypeInfo->HasTag("AddTag");
	}
	virtual bool CanApplyToObject(ObjectInstance Instance, const std::string&)
	{
		return Instance.TypeInfo->HasTag("AddTag");
	}
};


bool ObjectInstance::CanApplyTrait(const std::string& TraitName, const std::string& Arg)
{
	auto pTrait = GetTypeMetaTrait(TraitName);
	if (!pTrait) return false;
	return pTrait->CanApplyToObject(*this, Arg);
}
std::vector<ObjectInstance> ObjectInstance::ApplyTrait(const std::string& TraitName, const std::string& Arg)
{
	auto pTrait = GetTypeMetaTrait(TraitName);
	if (!pTrait) return {};
	return pTrait->GetTraitObjects(*this, Arg);
}

JsonFile ObjectInstance::GetTraitInfo(const std::string& TraitName, const std::string& Arg)
{
	auto pTrait = GetTypeMetaTrait(TraitName);
	if (!pTrait) return {};
	return pTrait->GetTraitInfo(*this, Arg);
}

bool ObjectTypeMetaInfo::CanApplyTrait(const std::string& TraitName, const std::string& Arg)
{
	auto pTrait = GetTypeMetaTrait(TraitName);
	if (!pTrait) return false;
	return pTrait->CanApplyToType(this->shared_from_this(), Arg);
}
std::vector<ObjectTypeMetaPtr> ObjectTypeMetaInfo::ApplyTrait(const std::string& TraitName, const std::string& Arg)
{
	auto pTrait = GetTypeMetaTrait(TraitName);
	if (!pTrait) return {};
	return pTrait->ApplyToType(this->shared_from_this(), Arg);
}

/*
----------------------------------------------------------------------
-----------------------------INIT TRAIT-------------------------------
----------------------------------------------------------------------
*/

void InitTraitObject()
{
	TypeMetaTraitMap["ResolveType"].reset(new ObjectTypeMetaTrait_ResolveType);
	TypeMetaTraitMap["AddTag"].reset(new ObjectTypeMetaTrait_AddTag);
}

ObjectTypeMetaTrait* GetTypeMetaTrait(const std::string& TraitName)
{
	if (TypeMetaTraitMap.contains(TraitName))
		return TypeMetaTraitMap[TraitName].get();
	return nullptr;
}