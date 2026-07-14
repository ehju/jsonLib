#include <algorithm>
#include <iostream>
#include <limits>
#include <string>

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#endif

#include "Json.h"

namespace
{
    const std::filesystem::path kDataFilePath = "data.json";

    json::JsonValue LoadDatabase()
    {
        if (std::filesystem::exists(kDataFilePath))
        {
            return json::JsonValue::LoadFromFile(kDataFilePath);
        }
        return json::JsonValue::MakeArray();
    }

    void PrintMenu()
    {
        std::cout << "\n===== CRUD 콘솔 애플리케이션 =====\n"
                   << "1. Create - 새 데이터 추가\n"
                   << "2. Read   - 목록 보기 / 검색\n"
                   << "3. Update - 데이터 수정\n"
                   << "4. Delete - 데이터 삭제\n"
                   << "5. 종료\n"
                   << "선택: ";
    }

    int ReadMenuChoice()
    {
        int choice = 0;
        if (!(std::cin >> choice))
        {
            std::cin.clear();
            choice = -1;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return choice;
    }

    std::string ReadLine(const std::string& prompt)
    {
        std::cout << prompt;
        std::string line;
        std::getline(std::cin, line);
        return line;
    }

    int NextId(const json::JsonValue& database)
    {
        int maxId = 0;
        for (const auto& record : database.AsArray())
        {
            if (const json::JsonValue* id = record.Find("id"))
            {
                maxId = std::max(maxId, static_cast<int>(id->AsNumber()));
            }
        }
        return maxId + 1;
    }

    void HandleCreate(json::JsonValue& database)
    {
        std::cout << "\n-- 새 데이터 추가 --\n";
        std::string name = ReadLine("name: ");
        std::string value = ReadLine("value: ");

        json::JsonValue record = json::JsonValue::MakeObject();
        record["id"] = json::JsonValue(NextId(database));
        record["name"] = json::JsonValue(name);
        record["value"] = json::JsonValue(value);

        database.Push(record);
        database.SaveToFile(kDataFilePath, 2);

        std::cout << "저장되었습니다: " << record.Dump() << "\n";
    }
}

int main()
{
#ifdef _WIN32
    // 소스가 UTF-8로 컴파일되므로 콘솔 입출력 코드페이지도 UTF-8로 맞춰 한글 깨짐을 방지
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    json::JsonValue database = LoadDatabase();

    bool running = true;
    while (running)
    {
        PrintMenu();
        int choice = ReadMenuChoice();

        switch (choice)
        {
        case 1:
            HandleCreate(database);
            break;
        case 2:
            std::cout << "[Read] 아직 구현되지 않았습니다.\n";
            break;
        case 3:
            std::cout << "[Update] 아직 구현되지 않았습니다.\n";
            break;
        case 4:
            std::cout << "[Delete] 아직 구현되지 않았습니다.\n";
            break;
        case 5:
            running = false;
            break;
        default:
            std::cout << "올바른 메뉴 번호를 입력하세요.\n";
            break;
        }
    }

    std::cout << "프로그램을 종료합니다.\n";
    return 0;
}
