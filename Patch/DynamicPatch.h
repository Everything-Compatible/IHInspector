#pragma once
#include <windows.h>
#include <initializer_list>
#include <cstring>
#include <memory>

// 使用更独特的节区名称，避免冲突
#define CODE_PATCH_SECTION ".codepatch"
#pragma section(CODE_PATCH_SECTION, read, execute)

// 指令结构体
#pragma pack(push, 1)
struct FarJumpInstruction
{
    BYTE opcode;        // 0xE9 for near jump, 0xEA for far jump
    DWORD offset;       // 跳转偏移量

    FarJumpInstruction(DWORD from, DWORD to)
    {
        opcode = 0xE9;
        offset = to - from - 5;
    }
};

struct NearCallInstruction
{
    BYTE opcode;        // 0xE8
    DWORD offset;       // 调用偏移量

    NearCallInstruction(DWORD from, DWORD to)
    {
        opcode = 0xE8;
        offset = to - from - 5;
    }
};

struct ExtendedCallInstruction
{
    WORD prefix;        // 特殊调用前缀
    DWORD offset;       // 调用偏移量

    ExtendedCallInstruction(DWORD from, DWORD to)
    {
        prefix = 0x15FF; // FF15 操作码
        offset = to;
    }
};
#pragma pack(pop)

// 内存保护封装类
class MemoryGuard
{
private:
    void* m_address;
    size_t m_size;
    DWORD m_oldProtect;
    bool m_locked;

public:
    MemoryGuard(void* address, size_t size, DWORD newProtect = PAGE_EXECUTE_READWRITE)
        : m_address(address), m_size(size), m_locked(false)
    {
        if (VirtualProtect(m_address, m_size, newProtect, &m_oldProtect)) {
            m_locked = true;
        }
    }

    ~MemoryGuard()
    {
        if (m_locked) {
            DWORD temp;
            VirtualProtect(m_address, m_size, m_oldProtect, &temp);
        }
    }

    // 禁用复制
    MemoryGuard(const MemoryGuard&) = delete;
    MemoryGuard& operator=(const MemoryGuard&) = delete;
};

// 节区信息结构
struct SectionInfo
{
    void* virtualAddress;
    size_t virtualSize;
};

// 主补丁类
#pragma pack(push, 1)
class CodeModifier
{
private:
    DWORD m_targetOffset;
    DWORD m_dataSize;
    const BYTE* m_patchData;

public:
    // 默认构造函数
    CodeModifier() : m_targetOffset(0), m_dataSize(0), m_patchData(nullptr) {}

    // 参数化构造函数
    CodeModifier(DWORD offset, DWORD size, const BYTE* data)
        : m_targetOffset(offset), m_dataSize(size), m_patchData(data) {}

    // 应用补丁到内存
    void Execute() const
    {
        if (m_targetOffset == 0 || m_dataSize == 0 || m_patchData == nullptr) {
            return;
        }

        void* targetAddress = reinterpret_cast<void*>(m_targetOffset);

        // 使用RAII模式管理内存保护
        MemoryGuard guard(targetAddress, m_dataSize);

        // 安全地复制数据
        std::memcpy(targetAddress, m_patchData, m_dataSize);
    }

    // 静态方法集
    static void ExecuteAllStored();

    // 类型化数据补丁
    template <typename DataType>
    static void ModifyWithData(DWORD offset, std::initializer_list<DataType> dataPattern)
    {
        CodeModifier modifier(
            offset,
            static_cast<DWORD>(dataPattern.size() * sizeof(DataType)),
            reinterpret_cast<const BYTE*>(dataPattern.begin())
        );
        modifier.Execute();
    }

    // 原始字节补丁（数组版本）
    template <size_t ArraySize>
    static void ModifyRawBytes(DWORD offset, const BYTE(&byteArray)[ArraySize])
    {
        CodeModifier modifier(offset, static_cast<DWORD>(ArraySize), byteArray);
        modifier.Execute();
    }

    // 原始字节补丁（初始化列表版本）
    static void ModifyRawBytes(DWORD offset, std::initializer_list<BYTE> byteData)
    {
        ModifyWithData<BYTE>(offset, byteData);
    }

    // 原始字节补丁（指针+长度版本）
    static void ModifyRawBytes(DWORD offset, const BYTE* dataPtr, size_t dataLength)
    {
        CodeModifier modifier(offset, static_cast<DWORD>(dataLength), dataPtr);
        modifier.Execute();
    }

    // 远跳转补丁
    static void InsertFarJump(DWORD offset, DWORD jumpTarget);
    static void InsertFarJump(DWORD offset, void* jumpTarget)
    {
        InsertFarJump(offset, reinterpret_cast<DWORD>(jumpTarget));
    }

    // 近调用补丁
    static void InsertNearCall(DWORD offset, DWORD callTarget);
    static void InsertNearCall(DWORD offset, void* callTarget)
    {
        InsertNearCall(offset, reinterpret_cast<DWORD>(callTarget));
    }

    // 6字节调用补丁（特殊格式）
    static void InsertExtendedCall(DWORD offset, DWORD callTarget);
    static void InsertExtendedCall(DWORD offset, void* callTarget)
    {
        InsertExtendedCall(offset, reinterpret_cast<DWORD>(callTarget));
    }

    // 虚函数表修补
    static void UpdateVTableEntry(DWORD vtableOffset, DWORD newFunction);
    static void UpdateVTableEntry(DWORD vtableOffset, void* newFunction)
    {
        UpdateVTableEntry(vtableOffset, reinterpret_cast<DWORD>(newFunction));
    }

    template <typename T>
    static DWORD GetMemberFunctionAddress(T func)
    {
        union {
            T memberFunc;
            DWORD address;
        } converter;

        converter.memberFunc = func;
        return converter.address;
    }

    template <typename T>
    static void UpdateVTableEntry(DWORD vtableOffset, T memberFunc)
    {
        UpdateVTableEntry(vtableOffset, GetMemberFunctionAddress(memberFunc));
    }

    // 偏移量修补（别名）
    static void ModifyPointer(DWORD offset, DWORD newValue)
    {
        UpdateVTableEntry(offset, newValue);
    }
    static void ModifyPointer(DWORD offset, void* newValue)
    {
        UpdateVTableEntry(offset, newValue);
    }
};
#pragma pack(pop)

// 节区扫描函数声明
bool LocateExecutableSection(const char* sectionName, SectionInfo* result);