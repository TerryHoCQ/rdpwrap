

#include "pch.h"
#include <Windows.h>
#include <stdlib.h>
#include "IniFile.h"

INI_FILE::INI_FILE(const wchar_t* FilePath)
{
	DWORD Status = 0;
	DWORD NumberOfBytesRead = 0;

	HANDLE hFile = CreateFile(FilePath, GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		return;
	}

	FileSize = GetFileSize(hFile, NULL);
	if (FileSize == INVALID_FILE_SIZE)
	{
		CloseHandle(hFile); // 添加关闭文件句柄
		return;
	}

	FileRaw.reset(new char[FileSize]); // 使用std::unique_ptr的reset分配数组
	Status = ReadFile(hFile, FileRaw.get(), FileSize, &NumberOfBytesRead, NULL); // 使用get()获取原始指针
	if (!Status)
	{
		CloseHandle(hFile); // 添加关闭文件句柄
		return;
	}

	CreateStringsMap();
	Parse();

	CloseHandle(hFile); // 确保关闭文件句柄
}



INI_FILE::~INI_FILE()
{
	// 这里不需要显式删除Section和Variables的内存，vector会自动管理
	// 唯一指针FileRaw和FileStringsMap也会在析构函数中自动释放
}



bool INI_FILE::CreateStringsMap()
{
	DWORD StringsCount = 1;

	// 遍历计算字符串数量
	for (DWORD i = 0; i < FileSize; i++)
	{
		if (FileRaw[i] == '\r' && FileRaw[i + 1] == '\n') StringsCount++;
	}

	FileStringsCount = StringsCount;

	// 使用 std::make_unique 分配内存
	FileStringsMap = std::make_unique<DWORD[]>(StringsCount);
	FileStringsMap[0] = 0;

	StringsCount = 1;

	// 生成文件字符串映射
	for (DWORD i = 0; i < FileSize; i++)
	{
		if (FileRaw[i] == '\r' && FileRaw[i + 1] == '\n')
		{
			FileStringsMap[StringsCount] = i + 2;
			StringsCount++;
		}
	}

	return true;
}


int INI_FILE::StrTrim(char* Str)
{
	int i = 0, j;
	while ((Str[i] == ' ') || (Str[i] == '\t'))
	{
		i++;
	}
	if (i > 0)
	{
		for (j = 0; j < strlen(Str); j++)
		{
			Str[j] = Str[j + i];
		}
		Str[j] = '\0';
	}

	i = strlen(Str) - 1;
	while ((Str[i] == ' ') || (Str[i] == '\t'))
	{
		i--;
	}
	if (i < (strlen(Str) - 1))
	{
		Str[i + 1] = '\0';
	}
	return 0;
}

DWORD INI_FILE::GetFileStringFromNum(DWORD StringNumber, char* RetString, DWORD Size)
{
	DWORD CurrentStringNum = 0;
	DWORD EndStringPos = 0;
	DWORD StringSize = 0;

	if (StringNumber > FileStringsCount) return 0;

	for (DWORD i = FileStringsMap[StringNumber]; i < FileSize; i++)
	{
		if (i == (FileSize - 1))
		{
			EndStringPos = FileSize;
			break;
		}
		if (FileRaw[i] == '\r' && FileRaw[i + 1] == '\n')
		{
			EndStringPos = i;
			break;
		}
	}

	StringSize = EndStringPos - FileStringsMap[StringNumber];

	if (Size < StringSize) return 0;

	memset(RetString, 0x00, Size);
	memcpy(RetString, &(FileRaw[FileStringsMap[StringNumber]]), StringSize);
	return StringSize;
}

bool INI_FILE::IsVariable(const char* Str, DWORD StrSize)
{
	bool Quotes = false;

	for (DWORD i = 0; i < StrSize; i++)
	{
		if (Str[i] == '"' || Str[i] == '\'') Quotes = !Quotes;
		if (Str[i] == '=' && !Quotes) return true;
	}
	return false;
}

bool INI_FILE::FillVariable(INI_SECTION_VARIABLE* Variable, const char* Str, DWORD StrSize)
{
	bool Quotes = false;

	for (DWORD i = 0; i < StrSize; i++)
	{
		if (Str[i] == '"' || Str[i] == '\'') Quotes = !Quotes;
		if (Str[i] == '=' && !Quotes)
		{
			memset(Variable->VariableName, 0, MAX_STRING_LEN);
			memset(Variable->VariableValue, 0, MAX_STRING_LEN);
			memcpy(Variable->VariableName, Str, i);
			memcpy(Variable->VariableValue, &(Str[i + 1]), StrSize - (i - 1));
			StrTrim(Variable->VariableName);
			StrTrim(Variable->VariableValue);
			break;
		}
	}
	return true;
}

bool INI_FILE::Parse()
{
	DWORD CurrentStringNum = 0;
	char CurrentString[512];
	DWORD CurrentStringSize = 0;

	DWORD SectionsCount = 0;
	DWORD VariablesCount = 0;

	DWORD CurrentSectionNum = -1;
	DWORD CurrentVariableNum = -1;

	// Calculate sections count
	for (DWORD CurrentStringNum = 0; CurrentStringNum < FileStringsCount; CurrentStringNum++)
	{
		CurrentStringSize = GetFileStringFromNum(CurrentStringNum, CurrentString, 512);

		if (CurrentString[0] == ';') continue; // It's a comment

		if (CurrentString[0] == '[' && CurrentString[CurrentStringSize - 1] == ']') // It's section declaration
		{
			SectionsCount++;
			continue;
		}
	}

	std::vector<DWORD> SectionVariableCount(SectionsCount, 0);

	for (DWORD CurrentStringNum = 0; CurrentStringNum < FileStringsCount; CurrentStringNum++)
	{
		CurrentStringSize = GetFileStringFromNum(CurrentStringNum, CurrentString, 512);

		if (CurrentString[0] == ';') continue; // It's a comment

		if (CurrentString[0] == '[' && CurrentString[CurrentStringSize - 1] == ']') // It's section declaration
		{
			CurrentSectionNum++;
			continue;
		}
		if (IsVariable(CurrentString, CurrentStringSize))
		{
			VariablesCount++;
			SectionVariableCount[CurrentSectionNum]++;
			continue;
		}
	}

	IniData.SectionCount = SectionsCount;
	IniData.Sections.resize(SectionsCount);

	for (DWORD i = 0; i < SectionsCount; i++)
	{
		IniData.Sections[i].VariablesCount = SectionVariableCount[i];
		IniData.Sections[i].Variables.resize(SectionVariableCount[i]);
	}

	CurrentSectionNum = -1;
	CurrentVariableNum = -1;

	for (DWORD CurrentStringNum = 0; CurrentStringNum < FileStringsCount; CurrentStringNum++)
	{
		CurrentStringSize = GetFileStringFromNum(CurrentStringNum, CurrentString, 512);

		if (CurrentString[0] == ';') // It's a comment
		{
			continue;
		}

		if (CurrentString[0] == '[' && CurrentString[CurrentStringSize - 1] == ']')
		{
			CurrentSectionNum++;
			CurrentVariableNum = 0;
			memset(IniData.Sections[CurrentSectionNum].SectionName, 0, MAX_STRING_LEN);
			memcpy(IniData.Sections[CurrentSectionNum].SectionName, &(CurrentString[1]), (CurrentStringSize - 2));
			continue;
		}

		if (IsVariable(CurrentString, CurrentStringSize))
		{
			FillVariable(&(IniData.Sections[CurrentSectionNum].Variables[CurrentVariableNum]), CurrentString, CurrentStringSize);
			CurrentVariableNum++;
			continue;
		}
	}

	return true;
}


PINI_SECTION INI_FILE::GetSection(const char* SectionName)
{
	for (DWORD i = 0; i < IniData.SectionCount; i++)
	{
		if (
			(strlen(IniData.Sections[i].SectionName) == strlen(SectionName)) &&
			(memcmp(IniData.Sections[i].SectionName, SectionName, strlen(SectionName)) == 0)
			)
		{
			return &IniData.Sections[i];
		}
	}
	return NULL;
}


bool INI_FILE::SectionExists(const char* SectionName)
{
	if (GetSection(SectionName) == NULL)	return false;
	return true;
}

bool INI_FILE::VariableExists(const char* SectionName, const char* VariableName)
{
	INI_SECTION_VARIABLE Variable = { 0 };
	return GetVariableInSectionPrivate(SectionName, VariableName, &Variable);
}

bool INI_FILE::GetVariableInSectionPrivate(const char* SectionName, const char* VariableName, INI_SECTION_VARIABLE* RetVariable)
{
	INI_SECTION* Section = NULL;
	INI_SECTION_VARIABLE* Variable = NULL;

	// Find section
	Section = GetSection(SectionName);
	if (Section == NULL)
	{
		SetLastError(318); // This region is not found
		return false;
	}

	// Find variable
	for (DWORD i = 0; i < Section->VariablesCount; i++)
	{
		if (
			(strlen(Section->Variables[i].VariableName) == strlen(VariableName)) &&
			(memcmp(Section->Variables[i].VariableName, VariableName, strlen(VariableName)) == 0)
			)
		{
			Variable = &(Section->Variables[i]);
			break;
		}
	}
	if (Variable == NULL)
	{
		SetLastError(1898); // Member of the group is not found
		return false;
	}

	memset(RetVariable, 0x00, sizeof(*RetVariable));
	memcpy(RetVariable, Variable, sizeof(*Variable));

	return true;
}

bool INI_FILE::GetVariableInSection(const char* SectionName, const char* VariableName, INI_VAR_STRING* RetVariable)
{
	bool Status = false;
	INI_SECTION_VARIABLE Variable = {};

	Status = GetVariableInSectionPrivate(SectionName, VariableName, &Variable);
	if (!Status)	return Status;

	memset(RetVariable, 0x00, sizeof(*RetVariable));
	memcpy(RetVariable->Name, Variable.VariableName, strlen(Variable.VariableName));
	memcpy(RetVariable->Value, Variable.VariableValue, strlen(Variable.VariableValue));

	return true;
}

bool INI_FILE::GetVariableInSection(const char* SectionName, const char* VariableName, INI_VAR_DWORD* RetVariable)
{
	bool Status = false;
	INI_SECTION_VARIABLE Variable = {};

	Status = GetVariableInSectionPrivate(SectionName, VariableName, &Variable);
	if (!Status)	return Status;

	memset(RetVariable, 0x00, sizeof(*RetVariable));
	memcpy(RetVariable->Name, Variable.VariableName, strlen(Variable.VariableName));

#ifndef _WIN64
	RetVariable->ValueDec = strtol(Variable.VariableValue, NULL, 10);
	RetVariable->ValueHex = strtol(Variable.VariableValue, NULL, 16);
#else
	RetVariable->ValueDec = _strtoi64(Variable.VariableValue, NULL, 10);
	RetVariable->ValueHex = _strtoi64(Variable.VariableValue, NULL, 16);
#endif
	return true;
}

bool INI_FILE::GetVariableInSection(const char* SectionName, const char* VariableName, INI_VAR_BYTEARRAY* RetVariable)
{
	bool Status = false;
	INI_SECTION_VARIABLE Variable = {};

	Status = GetVariableInSectionPrivate(SectionName, VariableName, &Variable);
	if (!Status)	return Status;

	DWORD ValueLen = strlen(Variable.VariableValue);
	if ((ValueLen % 2) != 0) return false;

	// for security reasons not more than 16 bytes
	if (ValueLen > 32) ValueLen = 32;  // 32 hex digits

	memset(RetVariable, 0x00, sizeof(*RetVariable));
	memcpy(RetVariable->Name, Variable.VariableName, strlen(Variable.VariableName));

	for (DWORD i = 0; i <= ValueLen; i++)
	{
		if ((i % 2) != 0) continue;

		switch (Variable.VariableValue[i])
		{
		case '0': break;
		case '1': RetVariable->Value[(i / 2)] += (1 << 4); break;
		case '2': RetVariable->Value[(i / 2)] += (2 << 4); break;
		case '3': RetVariable->Value[(i / 2)] += (3 << 4); break;
		case '4': RetVariable->Value[(i / 2)] += (4 << 4); break;
		case '5': RetVariable->Value[(i / 2)] += (5 << 4); break;
		case '6': RetVariable->Value[(i / 2)] += (6 << 4); break;
		case '7': RetVariable->Value[(i / 2)] += (7 << 4); break;
		case '8': RetVariable->Value[(i / 2)] += (8 << 4); break;
		case '9': RetVariable->Value[(i / 2)] += (9 << 4); break;
		case 'A': RetVariable->Value[(i / 2)] += (10 << 4); break;
		case 'B': RetVariable->Value[(i / 2)] += (11 << 4); break;
		case 'C': RetVariable->Value[(i / 2)] += (12 << 4); break;
		case 'D': RetVariable->Value[(i / 2)] += (13 << 4); break;
		case 'E': RetVariable->Value[(i / 2)] += (14 << 4); break;
		case 'F': RetVariable->Value[(i / 2)] += (15 << 4); break;
		}

		switch (Variable.VariableValue[i + 1])
		{
		case '0': break;
		case '1': RetVariable->Value[(i / 2)] += 1; break;
		case '2': RetVariable->Value[(i / 2)] += 2; break;
		case '3': RetVariable->Value[(i / 2)] += 3; break;
		case '4': RetVariable->Value[(i / 2)] += 4; break;
		case '5': RetVariable->Value[(i / 2)] += 5; break;
		case '6': RetVariable->Value[(i / 2)] += 6; break;
		case '7': RetVariable->Value[(i / 2)] += 7; break;
		case '8': RetVariable->Value[(i / 2)] += 8; break;
		case '9': RetVariable->Value[(i / 2)] += 9; break;
		case 'A': RetVariable->Value[(i / 2)] += 10; break;
		case 'B': RetVariable->Value[(i / 2)] += 11; break;
		case 'C': RetVariable->Value[(i / 2)] += 12; break;
		case 'D': RetVariable->Value[(i / 2)] += 13; break;
		case 'E': RetVariable->Value[(i / 2)] += 14; break;
		case 'F': RetVariable->Value[(i / 2)] += 15; break;
		}
	}
	RetVariable->ArraySize = ValueLen / 2;
	return true;
}

bool INI_FILE::GetVariableInSection(const char* SectionName, const char* VariableName, bool* RetVariable)
{
	bool Status = false;
	INI_SECTION_VARIABLE Variable = {};

	Status = GetVariableInSectionPrivate(SectionName, VariableName, &Variable);
	if (!Status)	return Status;

	*RetVariable = (bool)strtol(Variable.VariableValue, NULL, 10);
	return true;
}

bool INI_FILE::GetSectionVariablesList(const char* SectionName, INI_SECTION_VARLIST* VariablesList)
{
	INI_SECTION* Section = GetSection(SectionName);
	if (Section == NULL)
	{
		SetLastError(318); // This section is not found
		return false;
	}

	VariablesList->EntriesCount = Section->VariablesCount;

	// 使用resize调整vector大小
	VariablesList->NamesEntries.resize(VariablesList->EntriesCount);
	VariablesList->ValuesEntries.resize(VariablesList->EntriesCount);

	for (DWORD i = 0; i < Section->VariablesCount; i++)
	{
		// 复制变量名到NamesEntries
		strncpy(VariablesList->NamesEntries[i].String, Section->Variables[i].VariableName, MAX_STRING_LEN - 1);
		VariablesList->NamesEntries[i].String[MAX_STRING_LEN - 1] = '\0'; // 确保字符串以空字符结束

		// 复制变量值到ValuesEntries
		strncpy(VariablesList->ValuesEntries[i].String, Section->Variables[i].VariableValue, MAX_STRING_LEN - 1);
		VariablesList->ValuesEntries[i].String[MAX_STRING_LEN - 1] = '\0'; // 确保字符串以空字符结束
	}

	return true;
}



// ---------------------------- WCHAR_T BLOCK ----------------------------------------------

bool INI_FILE::SectionExists(const wchar_t* SectionName)
{
	char cSectionName[MAX_STRING_LEN] = { 0x00 };

	wcstombs(cSectionName, SectionName, MAX_STRING_LEN);

	return GetSection(cSectionName);
}

bool INI_FILE::VariableExists(const wchar_t* SectionName, const wchar_t* VariableName)
{
	INI_SECTION_VARIABLE Variable = { 0 };

	char cSectionName[MAX_STRING_LEN] = { 0x00 };
	char cVariableName[MAX_STRING_LEN] = { 0x00 };

	wcstombs(cSectionName, SectionName, MAX_STRING_LEN);
	wcstombs(cVariableName, VariableName, MAX_STRING_LEN);

	return GetVariableInSectionPrivate(cSectionName, cVariableName, &Variable);
}

bool INI_FILE::GetVariableInSection(const wchar_t* SectionName,const wchar_t* VariableName, INI_VAR_STRING* RetVariable)
{
	char cSectionName[MAX_STRING_LEN] = { 0x00 };
	char cVariableName[MAX_STRING_LEN] = { 0x00 };

	wcstombs(cSectionName, SectionName, MAX_STRING_LEN);
	wcstombs(cVariableName, VariableName, MAX_STRING_LEN);

	return GetVariableInSection(cSectionName, cVariableName, RetVariable);
}

bool INI_FILE::GetVariableInSection(const wchar_t* SectionName,const wchar_t* VariableName, INI_VAR_DWORD* RetVariable)
{
	char cSectionName[MAX_STRING_LEN] = { 0x00 };
	char cVariableName[MAX_STRING_LEN] = { 0x00 };

	wcstombs(cSectionName, SectionName, MAX_STRING_LEN);
	wcstombs(cVariableName, VariableName, MAX_STRING_LEN);

	return GetVariableInSection(cSectionName, cVariableName, RetVariable);
}

bool INI_FILE::GetVariableInSection(const wchar_t* SectionName, const wchar_t* VariableName, INI_VAR_BYTEARRAY* RetVariable)
{
	char cSectionName[MAX_STRING_LEN] = { 0x00 };
	char cVariableName[MAX_STRING_LEN] = { 0x00 };

	wcstombs(cSectionName, SectionName, MAX_STRING_LEN);
	wcstombs(cVariableName, VariableName, MAX_STRING_LEN);

	return GetVariableInSection(cSectionName, cVariableName, RetVariable);
}

bool INI_FILE::GetVariableInSection(const wchar_t* SectionName, const wchar_t* VariableName, bool* RetVariable)
{
	char cSectionName[MAX_STRING_LEN] = { 0x00 };
	char cVariableName[MAX_STRING_LEN] = { 0x00 };

	wcstombs(cSectionName, SectionName, MAX_STRING_LEN);
	wcstombs(cVariableName, VariableName, MAX_STRING_LEN);

	return GetVariableInSection(cSectionName, cVariableName, RetVariable);
}

bool INI_FILE::GetSectionVariablesList(const wchar_t* SectionName, INI_SECTION_VARLIST* VariablesList)
{
	char cSectionName[MAX_STRING_LEN] = { 0x00 };

	wcstombs(cSectionName, SectionName, MAX_STRING_LEN);

	return GetSectionVariablesList(cSectionName, VariablesList);
}