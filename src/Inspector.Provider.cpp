#include "Inspector.Provider.h"
#include <format>
#include <TriggerTypeClass.h>
#include <EventClass.h>
#include "Debug.h"

bool FirstACP = true;

std::u8string ACPGetString(const AddressCommentInfo& AddrInfo);

const char* MoveToIHCore(const std::u8string& S)
{
	char* p = (char*)IH::Malloc(S.size() + 1);
	strncpy(p, (const char*)S.c_str(), S.size() + 1);
	return p;
}

const char* MoveToIHCore(const std::string& S)
{
	char* p = (char*)IH::Malloc(S.size() + 1);
	strncpy(p, (const char*)S.c_str(), S.size() + 1);
	return p;
}

std::string UnicodetoUTF8(const std::wstring& Unicode)
{
	int UTF8len = WideCharToMultiByte(CP_UTF8, 0, Unicode.c_str(), -1, 0, 0, 0, 0);// 获取UTF-8编码长度
	char* UTF8 = new CHAR[UTF8len + 4]{};
	WideCharToMultiByte(CP_UTF8, 0, Unicode.c_str(), -1, UTF8, UTF8len, 0, 0); //转换成UTF-8编码
	std::string ret = UTF8;
	delete[] UTF8;
	return ret;
}

std::u8string FirstACPString_Scenario()
{
	auto& pScenario = ScenarioClass::Instance;
	if (pScenario)
	{
		return ~std::format("ScenarioClass Max UniqueID : {}.", pScenario->UniqueID);
	}
	else
	{
		return u8"无法访问 ScenarioClass::Instance.";
	}
}

std::u8string FirstACPString_Frame()
{
	auto& CurFrame = Unsorted::CurrentFrame;
	return ~std::format("CurrentFrame : {}.", CurFrame);
}

std::u8string GetFirstACPString()
{
	std::u8string S;
	S += FirstACPString_Scenario();
	S += u8"\n    ";
	S += FirstACPString_Frame();

	//For debug
	//AddressCommentInfo as;
	//volatile void* Obj[8]{};
	//Obj[0] = (void*)0xDEADBEEF;
	//Obj[1] = (void*)HouseClass::CurrentPlayer;
	//Obj[2] = (void*)HouseClass::Observer;
	//Obj[3] = (void*)(UnitClass::Array)[0];
	//Obj[4] = (void*)(ObjectTypeClass::Array)[0];
	//Obj[5] = (void*)(AbstractClass::Array)[0];
	//Obj[6] = (void*)*((AbstractClass::Array).back());
	//Obj[7] = (void*)(TriggerClass::Array)[0];
	//as.CanRead = true; as.CanExecute = false;
	//for(auto& ObjAddr : Obj)
	//{
	//	as.Addr = (DWORD)ObjAddr;
	//	S += u8"\n    " + ~std::format("0x{:08X}", as.Addr);
	//	S += u8"\n    " + ACPGetString(as);
	//}

	return S;
}


const char* __cdecl InspectorACP(const AddressCommentInfo& AddrInfo)
{
	std::u8string S;
	if (FirstACP)
	{
		FirstACP = false;
		PushPresetInstanceFromYRPP();
		auto ACP = ACPGetString(AddrInfo);
		auto FACP = GetFirstACPString();
		if(ACP.empty())S = FACP;
		else if(FACP.empty())S = ACP;
		else S = FACP + u8"\n    " + ACP;
	}
	else
	{
		S = ACPGetString(AddrInfo);
	}
	return MoveToIHCore(S);
}

bool IsReadable(HANDLE hProc, LPCVOID Ptr)
{
	MEMORY_BASIC_INFORMATION BInfo;
	if (VirtualQueryEx(hProc, Ptr, &BInfo, sizeof(BInfo)))
	{
		if (
			BInfo.Protect == 0x20 || BInfo.Protect == 0x40 || BInfo.Protect == 0x80 ||
			BInfo.Protect == 0x2 || BInfo.Protect == 0x4 || BInfo.Protect == 0x8
			)return true;
		else return false;
	}
	else return false;
}

std::unordered_set<AbstractClass*> AccessibleAbstractClass;
std::unordered_set<AbstractTypeClass*> AccessibleAbstractTypeClass;

void PushPresetInstanceFromYRPP()
{
	auto hProc = GetCurrentProcess();
	for (auto& Obj : AbstractClass::Array)
	{
		if (IsReadable(hProc, Obj))
		{
			AccessibleAbstractClass.insert(Obj);
		}
	}
	for (auto& Obj : TriggerClass::Array)
	{
		if (IsReadable(hProc, Obj))
		{
			AccessibleAbstractClass.insert(Obj);
		}
	}
	for (auto& Obj : AbstractTypeClass::Array)
	{
		if (IsReadable(hProc, Obj))
		{
			AccessibleAbstractTypeClass.insert(Obj);
		}
	}

	Debug::LogFormat("[Inspector] Pushed {} accessible AbstractClass instances.\n", AccessibleAbstractClass.size());
	Debug::LogFormat("[Inspector] Pushed {} accessible AbstractTypeClass instances.\n", AccessibleAbstractTypeClass.size());

	AccessibleAbstractClass.insert(AccessibleAbstractTypeClass.begin(), AccessibleAbstractTypeClass.end());
}

std::u8string AbsObj_AbsType(AbstractTypeClass* pAbsType);

std::u8string AbsObj_HouseType(HouseTypeClass* pHouse)
{
	if (pHouse)
	{
		auto pParent = pHouse->ParentCountry.operator const CHAR*();
		return AbsObj_AbsType(pHouse) + ~std::format(", ParentCountry = {}", pParent ? pParent : "null");
	}
	else
	{
		return u8"空的作战方类型指针";
	}
}

std::u8string AbsObj_House(HouseClass* pHouse)
{
	if (pHouse)
	{
		auto Type = pHouse->Type;
		return ~std::format("Name = {}, ArrayIndex = {}, {}, {}, Type : {}",
			pHouse->PlainName, pHouse->ArrayIndex,
			pHouse->IsCurrentPlayer() ? "Player" : "NOT Player",
			pHouse->IsObserver() ? "Observer" : "NOT Observer",
			~AbsObj_HouseType(Type));
	}
	else
	{
		return u8"空的作战方指针";
	}
}

std::u8string AbsObj_AbsType(AbstractTypeClass* pAbsType)
{
	if (pAbsType)
	{
		auto pUIName = pAbsType->UIName;
		std::string UINameU8;
		if (pUIName && IsReadable(GetCurrentProcess(), pUIName))UINameU8 = UnicodetoUTF8(pUIName);
		else UINameU8 = "???";
		return ~std::format("ID = {}, Name = {}, UIName = {}", pAbsType->get_ID(), pAbsType->Name, UINameU8);
	}
	else
	{
		return u8"空的类型指针";
	}
}

std::u8string GetTypeClassInfo(AbstractClass* pAbs, AbstractType AbsType)
{
	switch (AbsType)
	{
	case AbstractType::Aircraft:
		return AbsObj_AbsType(((AircraftClass*)pAbs)->Type);
	//case AbstractType::AITrigger:
	case AbstractType::Anim:
		return AbsObj_AbsType(((AnimClass*)pAbs)->Type);
	case AbstractType::Building:
		return AbsObj_AbsType(((BuildingClass*)pAbs)->Type);
	case AbstractType::Infantry:
		return AbsObj_AbsType(((InfantryClass*)pAbs)->Type);
	case AbstractType::Overlay:
		return AbsObj_AbsType(((OverlayClass*)pAbs)->Type);
	case AbstractType::ParticleSystem:
		return AbsObj_AbsType(((ParticleSystemClass*)pAbs)->Type);
	case AbstractType::Particle:
		return AbsObj_AbsType(((ParticleClass*)pAbs)->Type);
	case AbstractType::Script:
		return AbsObj_AbsType(((ScriptClass*)pAbs)->Type);
	case AbstractType::Smudge:
		return AbsObj_AbsType(((SmudgeClass*)pAbs)->Type);
	case AbstractType::Super:
		return AbsObj_AbsType(((SuperClass*)pAbs)->Type);
	case AbstractType::Team:
		return AbsObj_AbsType(((TeamClass*)pAbs)->Type);
	case AbstractType::Terrain:
		return AbsObj_AbsType(((TerrainClass*)pAbs)->Type);
	case AbstractType::Trigger:
		return AbsObj_AbsType(((TriggerClass*)pAbs)->Type);
	case AbstractType::Unit:
		return AbsObj_AbsType(((UnitClass*)pAbs)->Type);
	case AbstractType::VoxelAnim:
		return AbsObj_AbsType(((VoxelAnimClass*)pAbs)->Type);
	case AbstractType::Bullet:
	{
		auto pBullet = ((BulletClass*)pAbs);
		auto pBulletType = pBullet->Type;
		auto pWHType = pBullet->WH;
		auto pWeaponType = pBullet->WeaponType;
		return ~std::format("BulletType : {}, WarheadType : {}, WeaponType : {}",
			~AbsObj_AbsType(pBulletType), ~AbsObj_AbsType(pWHType), ~AbsObj_AbsType(pWeaponType));
	}
	default : 
		return u8"";
	}
}

std::u8string AbsObjExt_Trigger(TriggerClass* pTrigger)
{
	if (pTrigger)
	{
		auto Next = pTrigger->NextTrigger;
		auto Enabled = pTrigger->Enabled;
		auto House = pTrigger->House;
		return ~std::format("Trigger:  Next = {} Enabled = {}\n    House : {}",
			(void*)Next, Enabled ? "true" : "false", ~AbsObj_House(House));
	}
	else
	{
		return u8"空的触发器指针";
	}
}

std::u8string AbsObjExt_Event(EventClass* pEvent)
{
	if (pEvent)
	{
		auto Type = pEvent->Type;
		auto IsExecuted = pEvent->IsExecuted;
		auto Frame = pEvent->Frame;
		auto HouseIndex = (int)pEvent->HouseIndex;
		auto pHouse = (HouseIndex > 0 && HouseIndex < HouseClass::Array.Count) ? HouseClass::Array[HouseIndex] : nullptr;
		return ~std::format("Event:  Type = {}, IsExecuted = {}, Frame = {}\n    House : {}",
			(int)Type, IsExecuted ? "true" : "false", Frame, ~AbsObj_House(pHouse));
	}
	else
	{
		return u8"空的事件指针";
	}
}

std::u8string AbsObjExt_Techno(TechnoClass* pTechno)
{
	if (pTechno)
	{
		auto Health = pTechno->Health;
		auto InLimbo = pTechno->InLimbo;
		auto pType = pTechno->GetTechnoType();
		if (pType)
		{
			auto Strength = pType->Strength;
			return ~std::format("Techno:  HP = {}/{}, InLimbo = {}",
				Health, Strength, InLimbo ? "true" : "false");
		}
		else
		{
			return ~std::format("Techno:  HP = {}/??, InLimbo = {}",
				Health, InLimbo ? "true" : "false");
		}
	}
	else
	{
		return u8"空的科技指针";
	}
}

std::u8string ACPGetString(const AddressCommentInfo& AddrInfo)
{
	if (AddrInfo.Addr && AccessibleAbstractClass.contains((AbstractClass*)AddrInfo.Addr))
	{
		auto pAbs = (AbstractClass*)AddrInfo.Addr;
		auto pTypeName = pAbs->GetRTTIName();
		auto UniqueID = pAbs->UniqueID;
		auto Coord = pAbs->GetCoords();

		auto BaseStr = ~std::format("Type = {}, UniqueID = {}, Coord = ({}, {}, {})"
			, pTypeName, UniqueID, Coord.X, Coord.Y, Coord.Z);

		auto AbsType = pAbs->WhatAmI();
		switch (AbsType)
		{
		case AbstractType::House:
			return BaseStr + u8"\n    " + AbsObj_House((HouseClass*)pAbs);
		case AbstractType::HouseType:
			return BaseStr + u8"\n    " + AbsObj_HouseType((HouseTypeClass*)pAbs);
		default :
			auto pHouse = pAbs->GetOwningHouse();
			if (pHouse)
			{
				BaseStr = BaseStr + u8"\n    所属作战方: " + AbsObj_House(pHouse);
			}
			break;
		}

		if (AccessibleAbstractTypeClass.contains((AbstractTypeClass*)pAbs))
		{
			auto pAbsType = (AbstractTypeClass*)pAbs;
			if (pAbsType)
			{
				BaseStr = BaseStr + u8"\n    " + AbsObj_AbsType(pAbsType);
			}
		}

		auto TCInfo = GetTypeClassInfo(pAbs, AbsType);
		if(!TCInfo.empty())
		{
			BaseStr = BaseStr + u8"\n    " + TCInfo;
		}
		
		switch (AbsType)
		{
		case AbstractType::Trigger:
			return BaseStr + u8"\n    " + AbsObjExt_Trigger((TriggerClass*)pAbs);
		case AbstractType::Event:
			return BaseStr + u8"\n    " + AbsObjExt_Event((EventClass*)pAbs);
		case AbstractType::Infantry:
		case AbstractType::Aircraft:
		case AbstractType::Building:
		case AbstractType::Unit:
			return BaseStr + u8"\n    " + AbsObjExt_Techno((TechnoClass*)pAbs);
		default:
			return BaseStr;
		}
	}
	
	//std::u8string S = AddrInfo.CanRead ? u8"我可以读" : u8"我不可以读";
	//return S;
	return u8"";
}