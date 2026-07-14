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

    std::string FieldToString(const json::JsonValue& value)
    {
        if (value.IsString())
        {
            return value.AsString();
        }
        return value.Dump();
    }

    void PrintRecord(const json::JsonValue& record)
    {
        bool first = true;
        for (const auto& [key, value] : record.AsObject())
        {
            if (!first)
            {
                std::cout << ", ";
            }
            std::cout << key << ": " << FieldToString(value);
            first = false;
        }
        std::cout << "\n";
    }

    void PrintAllRecords(const json::JsonValue& database)
    {
        const auto& records = database.AsArray();
        if (records.empty())
        {
            std::cout << "데이터가 없습니다.\n";
            return;
        }
        for (const auto& record : records)
        {
            PrintRecord(record);
        }
    }

    const json::JsonValue* FindById(const json::JsonValue& database, int id)
    {
        for (const auto& record : database.AsArray())
        {
            const json::JsonValue* idValue = record.Find("id");
            if (idValue && static_cast<int>(idValue->AsNumber()) == id)
            {
                return &record;
            }
        }
        return nullptr;
    }

    bool TryParseId(const std::string& text, int& outId)
    {
        try
        {
            outId = std::stoi(text);
            return true;
        }
        catch (const std::exception&)
        {
            return false;
        }
    }

    void HandleSearchById(const json::JsonValue& database)
    {
        std::string idText = ReadLine("검색할 id: ");
        int id = 0;
        if (!TryParseId(idText, id))
        {
            std::cout << "숫자로 된 id를 입력하세요.\n";
            return;
        }
        if (const json::JsonValue* record = FindById(database, id))
        {
            PrintRecord(*record);
        }
        else
        {
            std::cout << "해당 id의 데이터를 찾을 수 없습니다.\n";
        }
    }

    void HandleSearchByField(const json::JsonValue& database)
    {
        std::string field = ReadLine("검색할 필드명: ");
        std::string value = ReadLine("검색할 값: ");

        bool found = false;
        for (const auto& record : database.AsArray())
        {
            const json::JsonValue* fieldValue = record.Find(field);
            if (fieldValue && FieldToString(*fieldValue) == value)
            {
                PrintRecord(record);
                found = true;
            }
        }
        if (!found)
        {
            std::cout << "일치하는 데이터를 찾을 수 없습니다.\n";
        }
    }

    void HandleRead(const json::JsonValue& database)
    {
        std::cout << "\n-- Read --\n"
                   << "1. 전체 목록 보기\n"
                   << "2. id로 검색\n"
                   << "3. 필드명으로 검색\n"
                   << "선택: ";
        int choice = ReadMenuChoice();
        switch (choice)
        {
        case 1:
            PrintAllRecords(database);
            break;
        case 2:
            HandleSearchById(database);
            break;
        case 3:
            HandleSearchByField(database);
            break;
        default:
            std::cout << "올바른 메뉴 번호를 입력하세요.\n";
            break;
        }
    }

    void HandleUpdate(json::JsonValue& database)
    {
        std::string idText = ReadLine("수정할 id: ");
        int id = 0;
        if (!TryParseId(idText, id))
        {
            std::cout << "숫자로 된 id를 입력하세요.\n";
            return;
        }

        for (auto& record : database.AsArray())
        {
            const json::JsonValue* idValue = record.Find("id");
            if (!idValue || static_cast<int>(idValue->AsNumber()) != id)
            {
                continue;
            }

            std::cout << "현재 데이터: ";
            PrintRecord(record);

            std::string field = ReadLine("수정할 필드명 (id는 수정 불가): ");
            if (field == "id")
            {
                std::cout << "id는 수정할 수 없습니다.\n";
                return;
            }
            if (!record.Find(field))
            {
                std::cout << "존재하지 않는 필드입니다.\n";
                return;
            }

            std::string newValue = ReadLine("새 값: ");
            record[field] = json::JsonValue(newValue);
            database.SaveToFile(kDataFilePath, 2);

            std::cout << "수정되었습니다: ";
            PrintRecord(record);
            return;
        }

        std::cout << "해당 id의 데이터를 찾을 수 없습니다.\n";
    }

    void HandleDelete(json::JsonValue& database)
    {
        std::string idText = ReadLine("삭제할 id: ");
        int id = 0;
        if (!TryParseId(idText, id))
        {
            std::cout << "숫자로 된 id를 입력하세요.\n";
            return;
        }

        auto& records = database.AsArray();
        auto it = std::find_if(records.begin(), records.end(), [id](const json::JsonValue& record)
        {
            const json::JsonValue* idValue = record.Find("id");
            return idValue && static_cast<int>(idValue->AsNumber()) == id;
        });

        if (it == records.end())
        {
            std::cout << "해당 id의 데이터를 찾을 수 없습니다.\n";
            return;
        }

        std::cout << "삭제 대상: ";
        PrintRecord(*it);

        std::string confirm = ReadLine("정말 삭제하시겠습니까? (y/n): ");
        if (confirm != "y" && confirm != "Y")
        {
            std::cout << "삭제를 취소했습니다.\n";
            return;
        }

        records.erase(it);
        database.SaveToFile(kDataFilePath, 2);
        std::cout << "삭제되었습니다.\n";
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
            HandleRead(database);
            break;
        case 3:
            HandleUpdate(database);
            break;
        case 4:
            HandleDelete(database);
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
