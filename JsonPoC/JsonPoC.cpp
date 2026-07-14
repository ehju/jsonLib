#include <iostream>

#ifdef _WIN32
#include <Windows.h>
#endif

#include "Json.h"

int main()
{
#ifdef _WIN32
    // 소스가 UTF-8로 컴파일되므로 콘솔 입출력 코드페이지도 UTF-8로 맞춰 한글 깨짐을 방지
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    using namespace json;

    // 파싱 예제
    const std::string text = R"({
        "name": "JsonPoC",
        "version": 1,
        "enabled": true,
        "tags": ["json", "cpp", "poc"],
        "meta": { "author": "example", "score": 3.14 }
    })";

    try
    {
        JsonValue value = JsonValue::Parse(text);

        std::cout << "파싱 결과 (pretty):\n" << value.Dump(2) << "\n\n";

        if (const JsonValue* name = value.Find("name"))
        {
            std::cout << "name = " << name->AsString() << "\n";
        }

        // 값 수정 및 추가
        value["version"] = JsonValue(2);
        value["newField"] = JsonValue("added");

        // 파일로 저장
        const std::filesystem::path outPath = "output.json";
        value.SaveToFile(outPath, 2);
        std::cout << "\n저장 완료: " << outPath << "\n";

        // 파일에서 다시 로드
        JsonValue loaded = JsonValue::LoadFromFile(outPath);
        std::cout << "다시 읽은 결과:\n" << loaded.Dump(2) << "\n";

        // 유니코드 이스케이프 및 서로게이트 페어 테스트
        JsonValue unicodeValue = JsonValue::Parse(R"({"emoji":"😀","kr":"가나다"})");
        std::cout << "\n유니코드 테스트: " << unicodeValue.Dump() << "\n";
    }
    catch (const JsonParseException& ex)
    {
        std::cerr << "JSON 파싱 오류 (" << ex.Line() << "행 " << ex.Column() << "열): " << ex.what() << "\n";
        return 1;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "오류: " << ex.what() << "\n";
        return 1;
    }

    // 잘못된 JSON 오류 처리 테스트
    try
    {
        JsonValue::Parse("{ invalid }");
    }
    catch (const JsonParseException& ex)
    {
        std::cout << "예상된 오류: " << ex.what() << " (line " << ex.Line() << ", col " << ex.Column() << ")\n";
    }

    return 0;
}
