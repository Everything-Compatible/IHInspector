#include "DynamicPatch.h"
#include <imagehlp.h>
#include <cstring>

#pragma comment(lib, "imagehlp.lib")

// 节区扫描工具
bool LocateExecutableSection(const char* sectionName, SectionInfo* result)
{
    HMODULE moduleBase = GetModuleHandle(nullptr);
    if (!moduleBase) {
        return false;
    }

    PIMAGE_DOS_HEADER dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(moduleBase);

    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
        return false;
    }

    PIMAGE_NT_HEADERS ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(
        reinterpret_cast<BYTE*>(moduleBase) + dosHeader->e_lfanew);

    if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) {
        return false;
    }

    PIMAGE_SECTION_HEADER sectionHeader = IMAGE_FIRST_SECTION(ntHeaders);

    for (WORD i = 0; i < ntHeaders->FileHeader.NumberOfSections; ++i, ++sectionHeader)
    {
        char name[9] = { 0 };
        std::strncpy(name, reinterpret_cast<const char*>(sectionHeader->Name), 8);

        if (std::strcmp(sectionName, name) == 0)
        {
            result->virtualAddress = reinterpret_cast<void*>(
                reinterpret_cast<BYTE*>(moduleBase) + sectionHeader->VirtualAddress);
            result->virtualSize = sectionHeader->Misc.VirtualSize;
            return true;
        }
    }
    
    return false;
}

// 执行所有存储的补丁
void CodeModifier::ExecuteAllStored()
{
    SectionInfo section;
    if (!LocateExecutableSection(CODE_PATCH_SECTION, &section)) {
        return;
    }

    const BYTE* current = reinterpret_cast<const BYTE*>(section.virtualAddress);
    const BYTE* end = current + section.virtualSize;

    while (current < end)
    {
        const CodeModifier* modifier = reinterpret_cast<const CodeModifier*>(current);

        // 终止条件：空偏移量
        if (modifier->m_targetOffset == 0) {
            break;
        }

        // 验证数据有效性
        if (modifier->m_dataSize > 0 && modifier->m_patchData != nullptr) {
            modifier->Execute();
        }

        // 移动到下一个补丁
        current += sizeof(CodeModifier);
    }
}

// 跳转和调用补丁实现
void CodeModifier::InsertFarJump(DWORD offset, DWORD target)
{
    FarJumpInstruction instruction(offset, target);
    CodeModifier modifier(offset, sizeof(instruction), reinterpret_cast<const BYTE*>(&instruction));
    modifier.Execute();
}

void CodeModifier::InsertNearCall(DWORD offset, DWORD target)
{
    NearCallInstruction instruction(offset, target);
    CodeModifier modifier(offset, sizeof(instruction), reinterpret_cast<const BYTE*>(&instruction));
    modifier.Execute();
}

void CodeModifier::InsertExtendedCall(DWORD offset, DWORD target)
{
    ExtendedCallInstruction instruction(offset, target);
    CodeModifier modifier(offset, sizeof(instruction), reinterpret_cast<const BYTE*>(&instruction));
    modifier.Execute();
}

void CodeModifier::UpdateVTableEntry(DWORD vtableOffset, DWORD newFunction)
{
    ModifyWithData<DWORD>(vtableOffset, { newFunction });
}