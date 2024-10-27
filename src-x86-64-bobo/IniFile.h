#ifndef INI_FILE_H
#define INI_FILE_H

#include <string>
#include <vector>
#include <memory>
#include <Windows.h>

#define MAX_STRING_LEN 255

// 字符串类型变量结构体
typedef struct _INI_VAR_STRING {
    char Name[MAX_STRING_LEN];   // 变量名
    char Value[MAX_STRING_LEN];  // 变量值
} INI_VAR_STRING, * PINI_VAR_STRING;

// DWORD类型变量结构体，兼容32位和64位
typedef struct _INI_VAR_DWORD {
    char Name[MAX_STRING_LEN];
#ifndef _WIN64
    DWORD ValueDec;  // 十进制值
    DWORD ValueHex;  // 十六进制值
#else
    DWORD64 ValueDec;
    DWORD64 ValueHex;
#endif
} INI_VAR_DWORD, * PINI_VAR_DWORD;

// 字节数组变量结构体
typedef struct _INI_VAR_BYTEARRAY {
    char Name[MAX_STRING_LEN];
    BYTE ArraySize;             // 数组大小
    char Value[MAX_STRING_LEN]; // 数组值
} INI_VAR_BYTEARRAY, * PINI_VAR_BYTEARRAY;

// 变量列表项结构体
typedef struct _INI_SECTION_VARLIST_ENTRY {
    char String[MAX_STRING_LEN];  // 变量项的字符串表示
} INI_SECTION_VARLIST_ENTRY, * PINI_SECTION_VARLIST_ENTRY;

// 变量列表结构体
typedef struct _INI_SECTION_VARLIST {
    DWORD EntriesCount;                                  // 条目数量
    std::vector<INI_SECTION_VARLIST_ENTRY> NamesEntries; // 名称项列表
    std::vector<INI_SECTION_VARLIST_ENTRY> ValuesEntries;// 值项列表
} INI_SECTION_VARLIST, * PINI_SECTION_VARLIST;

// INI文件中的单个变量结构体
typedef struct _INI_SECTION_VARIABLE {
    char VariableName[MAX_STRING_LEN];   // 变量名
    char VariableValue[MAX_STRING_LEN];  // 变量值
} INI_SECTION_VARIABLE, * PINI_SECTION_VARIABLE;

// INI文件中的单个段结构体
typedef struct _INI_SECTION {
    char SectionName[MAX_STRING_LEN];                     // 段名
    DWORD VariablesCount;                                 // 变量数量
    std::vector<INI_SECTION_VARIABLE> Variables;          // 变量列表
} INI_SECTION, * PINI_SECTION;

// INI文件数据结构体
typedef struct _INI_DATA {
    DWORD SectionCount;                      // 段数量
    std::vector<INI_SECTION> Sections;       // 段列表
} INI_DATA, * PINI_DATA;

// INI文件处理类
class INI_FILE {
public:
    INI_FILE(const wchar_t* filePath);       // 构造函数，初始化文件路径
    ~INI_FILE();                             // 析构函数

    // 字符串操作接口
    bool SectionExists(const char* SectionName);
    bool VariableExists(const char* SectionName, const char* VariableName);
    bool GetVariableInSection(const char* SectionName, const char* VariableName, INI_VAR_STRING* Variable);
    bool GetVariableInSection(const char* SectionName, const char* VariableName, INI_VAR_DWORD* Variable);
    bool GetVariableInSection(const char* SectionName, const char* VariableName, bool* Variable);
    bool GetVariableInSection(const char* SectionName, const char* VariableName, INI_VAR_BYTEARRAY* Variable);
    bool GetSectionVariablesList(const char* SectionName, INI_SECTION_VARLIST* VariablesList);

    // Unicode支持接口
    bool SectionExists(const wchar_t* SectionName);
    bool VariableExists(const wchar_t* SectionName, const wchar_t* VariableName);
    bool GetVariableInSection(const wchar_t* SectionName, const wchar_t* VariableName, INI_VAR_STRING* Variable);
    bool GetVariableInSection(const wchar_t* SectionName, const wchar_t* VariableName, INI_VAR_DWORD* Variable);
    bool GetVariableInSection(const wchar_t* SectionName, const wchar_t* VariableName, bool* Variable);
    bool GetVariableInSection(const wchar_t* SectionName, const wchar_t* VariableName, INI_VAR_BYTEARRAY* Variable);
    bool GetSectionVariablesList(const wchar_t* SectionName, INI_SECTION_VARLIST* VariablesList);

private:
    DWORD FileSize;                              // INI文件大小
    std::unique_ptr<char[]> FileRaw;             // 文件的原始数据缓冲区
    DWORD FileStringsCount;                      // 文件字符串数量
    std::unique_ptr<DWORD[]> FileStringsMap;     // 字符串映射，用于索引文件内容
    INI_DATA IniData;                            // 解析后的INI数据

    // 辅助函数
    int StrTrim(char* Str);                      // 字符串修整

    // 类的服务函数
    bool CreateStringsMap();                     // 创建文件的字符串映射
    bool Parse();                                // 解析文件为结构数据
    DWORD GetFileStringFromNum(DWORD StringNumber, char* RetString, DWORD Size); // 从字符串映射中获取特定行
    bool IsVariable(const char* Str, DWORD StrSize); // 检查字符串是否是变量
    bool FillVariable(INI_SECTION_VARIABLE* Variable, const char* Str, DWORD StrSize); // 填充变量结构
    PINI_SECTION GetSection(const char* SectionName); // 获取指定段
    bool GetVariableInSectionPrivate(const char* SectionName, const char* VariableName, INI_SECTION_VARIABLE* RetVariable); // 获取特定段内的变量
};

#endif // INI_FILE_H
