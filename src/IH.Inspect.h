#pragma once
#include <EC.h>

#define IHInspectorLibName "IHInspector"
#define IHInspectorVersion 1

using ObjectMetaHandle = void*;
using ObjectPathHandle = void*;

struct ObjectReference
{
	DWORD Address;
	ObjectMetaHandle Type;

	bool IsValid() const
	{
		return Address != 0 && Type != nullptr;
	}
};

ObjectMetaHandle __cdecl GetObjectMetaInfo(UTF8_CString TypeName);

UTF8_CString __cdecl GetTypeName(ObjectMetaHandle Meta);

void __cdecl SetObjectVariable(UTF8_CString VarName, ObjectReference Ref);

//return invalid ref if not exist
ObjectReference __cdecl GetObjectVariable(UTF8_CString VarName);

JsonFile __cdecl InspectObject(ObjectReference ObjRef);

JsonFile __cdecl InspectType(ObjectMetaHandle Meta);

//call DestroyPathHandle by caller afterwards
ObjectPathHandle __cdecl CreateObjectPath(PArray<UTF8_CString> PathElements);

void __cdecl DestroyPathHandle(ObjectPathHandle Path);

ObjectReference __cdecl AccessPathFromObject(ObjectReference ObjRef, ObjectPathHandle Path);

JsonFile __cdecl GetTraitInfo(ObjectReference ObjRef, UTF8_CString TraitName);

//call arr->Delete by caller afterwards
PArray<ObjectReference> __cdecl GetTraitObjects(ObjectReference ObjRef, UTF8_CString TraitName);




/*
-------------------------------------------------------------------------------------------
*/

//Console Command:

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
	-Trait		特性名列表（可选）

*/
void IHInspect_View(JsonObject Args);

/*
命令： IHInspect.Type
参数：
1.
	-Name	类型名
或
	-ID		类型ID (HEX, 0x%08X)
*/
void IHInspect_Type(JsonObject Args);