// DatabaseExtractor.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <fstream>
#include <cstring>
#include <map>

#include "utils/CPPython/cppython.h"
#include <cassert>

#define ParseArgs std::ifstream& fin, std::map<uint64_t, string> IDMap
#define ReadFin(data) ReadData(fin, data)

std::ofstream fout("out.json", std::ios::binary);

using namespace CPPython;

struct ElementTypeStruct
{
    uint64_t Name;
    uint64_t TypeName;
};

struct ElementInfoStruct
{
    string Name = "";
    string Type = "";
};

#pragma pack(2)
struct GUID
{
    uint32_t GUID1;
    uint16_t GUID2;
    uint16_t GUID3;
    uint8_t  GUID4[8];
};

bool ParseMapArray(ParseArgs);
bool ParseValArray(ParseArgs, uint8_t);
bool ParseStr(ParseArgs);
bool ParseData(ParseArgs, ElementInfoStruct);
ElementInfoStruct GetData(ParseArgs);

template<class T>
void ReadData(std::ifstream& fin, T& data)
{
    fin.read((char*)&data, sizeof(data));
}

char* GUIDtoString(GUID);

const char* Files[3] = {
    "Database",
    "ItemData",
    "UnlockData"
};

void print_map(std::string_view comment, const std::map<uint64_t, string>& m)
{
    std::cout << comment;
    for (const auto& [key, value] : m) {
        std::cout << key << " = " << value << "; ";
    }
    std::cout << "\n";
}

std::map<uint64_t, string> ParseMap(const char*);

void PrintInfo(ElementInfoStruct& e)
{
    printf("%s %s\n", e.Type.c_str(), e.Name.c_str());
}

bool ParseValArray(ParseArgs, uint8_t ValueSize = 4)
{
    uint64_t Size;
    ReadFin(Size);
    printf("Data Size = %llX\n", Size);

    uint32_t Count;
    ReadFin(Count);
    printf("Elements Count = %lX\n", Count);

    fout << "[\n";
    for (uint32_t i = 0; i < Count; i++)
    {
        uint64_t Value = 0;
        fin.read((char*)&Value, ValueSize);
        printf("%llX [%s]\n", Value, IDMap[Value].c_str());
        fout << "\""<<IDMap[Value]<<"\"";
        if (i + 1 < Count)
            fout << ",";
        fout << "\n";
    }
    fout << "]\n";

    return true;
}

bool ParseMapArray(ParseArgs)
{
    uint64_t Size;
    ReadFin(Size);
    printf("Data Size = %llX\n", Size);

    uint32_t Count;
    ReadFin(Count);

    printf("Elements Count: %lX\n[\n", Count);
    fout << "[\n";
    if (Count)
        fout << "{\n";
    uint32_t CurrentCount = 0;
    while (CurrentCount < Count)
    {
        auto sData = GetData(fin, IDMap);
        printf("Element %lX: (\n", CurrentCount);
        if (sData.Name == "None")
        {
            printf("null,)\n");
            fout << "}";
            CurrentCount++;
            if (CurrentCount < Count)
                fout << ",\n{\n";
            else
                fout << "\n";
            continue;
        }
        if (!ParseData(fin, IDMap, sData))
            return false;
           fout << ",\n";
        printf(")\n");
    }
    printf("]\n");
    fout << "]\n";

    return true;
}

bool ParseStr(ParseArgs)
{
    uint64_t Size;
    ReadFin(Size);
    printf("Field Size = %llX\n", Size);

    uint32_t NameLength;
    char* Name;
    fin.read((char*)&NameLength, 4);
    Name = new char[NameLength];
    fin.read(Name, NameLength);

    printf("Value: %s", Name);
    printf(",\n");
    fout << "\"" << Name << "\"";
    return true;
}

bool ParseData(ParseArgs, ElementInfoStruct sData)
{
    fout << "\"" << sData.Name << "\": ";
    if (sData.Type == "ArrayProperty")
    {
        if (sData.Name == "mUnlockPagesSentForOnline")
            return ParseValArray(fin, IDMap, 4);
        else if (sData.Name == "mUnlocks")
            return ParseMapArray(fin, IDMap);
        else if (sData.Name == "mUnlockPages")
            return ParseMapArray(fin, IDMap);
        else if (sData.Name == "mItems")
            return ParseMapArray(fin, IDMap);
        else if (sData.Name == "mAudioMapping")
            return ParseMapArray(fin, IDMap);
        else if (sData.Name == "Characters")
            return ParseMapArray(fin, IDMap);
        else if (sData.Name == "Sockets")
            return ParseMapArray(fin, IDMap);
        else if (sData.Name == "DefaultItems")
            return ParseMapArray(fin, IDMap);
        else if (sData.Name == "DefaultCharacterLoadouts")
            return ParseMapArray(fin, IDMap);
        else
        {
            printf("%s is not supported! Assuming Map", sData.Name.c_str());
            return ParseMapArray(fin, IDMap);
            return false;
        }
            
        //return ParseValArray(fin, IDMap, 8);
    }
    else if (sData.Type == "MapProperty")
    {
        uint64_t Size;
        ReadFin(Size);
        printf("Data Size = %llX\n", Size);

        uint32_t Count;
        ReadFin(Count);
        printf("Elements Count = %lX\n", Count);

        fout << "{\n";
        if (sData.Name== "mUnlockNameMap")
        {
            for (uint32_t i = 0; i < Count; i++)
            {
                uint64_t MapName;
                ReadFin(MapName);
                string MapKey = IDMap[MapName];
                uint32_t Val[2] = {};
                fin.read((char*)Val, 8);
                printf("%s = {%lX: %lX}\n", MapKey.c_str(), Val[0], Val[1]);
                fout << "\"" << MapKey << "\": {" << Val[0] << ": " << Val[1] << "},\n";
            }
        }
        else
        {
            for (uint32_t i = 0; i < Count; i++)
            {
                uint8_t Key;
                ReadFin(Key);

                uint64_t MapName;
                ReadFin(MapName);

                string MapKey = IDMap[MapName];

                printf("{%hhX: %s}\n", Key, MapKey.c_str());
                fout << (uint32_t)Key << ": \"" << MapKey << "\",\n";
            }
        }
        fout << "}\n";

       
    }
    else if (sData.Type == "StrProperty")
    {
        auto ret = ParseStr(fin, IDMap);
        return ret;
    }
    else if (sData.Type == "NameProperty")
    {
        uint64_t Size;
        ReadFin(Size);
        printf("Data Size = %llX\n", Size);

        uint64_t Name;
        fin.read((char*)&Name, Size);
        string FieldName = IDMap[Name];
        printf("%llX [%s]\n", Name, FieldName.c_str());
        fout << "\"" << FieldName << "\"";
    }
    else if (sData.Type == "StructProperty")
    {
        uint64_t Size;
        ReadFin(Size);
        printf("Field Size = %llX\n", Size);

        uint64_t Type;
        ReadFin(Type);
        string szType = IDMap[Type];
        printf("Field Type = %s\n", szType.c_str());


        if (szType == "FGuid")
        {
            GUID FGuid;
            ReadFin(FGuid);
            auto Guid = GUIDtoString(FGuid);
            printf("Field Value: %s", Guid);
            uint32_t WGuid[4] = {};
            memcpy(WGuid, (char*)&FGuid, 16);
            char szGUID[256];
            sprintf_s(szGUID, "\"%04lX%04lX%04lX%04lX\"", WGuid[0], WGuid[1], WGuid[2], WGuid[3]);
            fout << szGUID;
        }
        else if (szType.endswith("ListInstance") || szType.endswith("DefinitionHandle"))
        {
            fout << "{\n";
            auto ssData = GetData(fin, IDMap);
            if (!ParseData(fin, IDMap, ssData))
                return false;
            fout << "}\n";
        }
        else
        {
            printf("Field Value: %s", "Not Yet Handled");
            return false;
        }

        printf(",\n");
    }
    else if (sData.Type == "Guid")
    {
        
    }
    else if (sData.Type == "DWordProperty")
    {

        uint64_t DataSize;
        ReadFin(DataSize);
        printf("Data Size: %llX\n", DataSize);

        int64_t datas=0;
        fin.read((char*)&datas, DataSize);
        printf("Value: %llX\n", datas);
        fout << datas;
    }
    else if (sData.Type == "EnumProperty")
    {
        uint64_t DataSize;
        ReadFin(DataSize);
        printf("Data Size: %llX\n", DataSize);

        uint64_t datas=0;
        fin.read((char*)&datas, DataSize);
        printf("Value: %llX\n", datas);
        fout << datas;
    }
    else if (sData.Type == "BoolProperty")
    {
        uint64_t DataSize;
        ReadFin(DataSize);
        printf("Data Size: %llX\n", DataSize);

        uint32_t datas=0;
        ReadFin(datas);
        printf("Value: %lX\n", datas);
        fout << (datas ? "true" : "false");
    }
    else
    {
        printf("Unsupported Type %s\n", sData.Type);
        return false;
    }
    return true;
}

ElementInfoStruct GetData(ParseArgs)
{
    uint64_t ItemName;
    uint64_t ItemType;
    uint64_t ItemLength;
    
    ElementInfoStruct sData;
    ReadFin(ItemName);
    sData.Name = IDMap[ItemName];
    printf("%llX", ItemName);
    if (sData.Name == "None")
    {
        printf("\n");
        return sData;
    }
        
    ReadFin(ItemType);
    sData.Type = IDMap[ItemType];
    printf(" %llX\n", ItemType);
    PrintInfo(sData);
    return sData;
}

int main()
{
    for (int i = 0; i < 1; i++)
    {
        string FileName("Data\\");
        FileName += Files[i];
        string MapFileName = FileName + ".txt";
        auto IDMap = ParseMap(MapFileName.c_str());

        std::ifstream fin(FileName.c_str(), std::ios::binary);
        fin >> std::noskipws;

        fout << "{\n";
        while (!fin.eof())
        {
            auto sData = GetData(fin, IDMap);

            if (sData.Name == "None")
                continue;
                
            if (!ParseData(fin, IDMap, sData))
                break;
            fout << ",";
        }
        fout << "\n}";



        fin.close();
    }
}

std::map<uint64_t, string> ParseMap(const char* File)
{
    std::map<uint64_t, string> TypeMap = { {} };
    std::ifstream fin(File);
    string Line;

    while (getline(fin, Line))
    {
        auto ToMap = Line.split(':', 1);
        char* c;
        uint64_t ID = strtoull(ToMap[0], &c, 16);
        string Value = ToMap[1].strip();
        TypeMap[ID] = Value;
    }
    fin.close();

    return TypeMap;
}

char* GUIDtoString(GUID guid)
{
    char guid_string[50] = {};
    sprintf_s(guid_string, "{%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}",
        guid.GUID1, guid.GUID2, guid.GUID3,
        guid.GUID4[0], guid.GUID4[1], guid.GUID4[2], guid.GUID4[3],
        guid.GUID4[4], guid.GUID4[5], guid.GUID4[6], guid.GUID4[7]);
    return guid_string;
}