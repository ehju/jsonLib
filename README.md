# JsonPoC

C++20 표준 라이브러리만으로 작성된 가벼운 JSON 파싱/직렬화 라이브러리입니다.
외부 의존성 없이 JSON 문자열을 파싱하고, 값을 조작하고, 파일로 저장/로드할 수 있습니다.

## 구성

| 파일 | 설명 |
| --- | --- |
| `JsonPoC/Json.h` | `json::JsonValue`, `json::JsonParseException` 등 공개 인터페이스 |
| `JsonPoC/Json.cpp` | 재귀 하강 파서와 직렬화(Dump) 구현 |
| `JsonPoC/JsonPoC.cpp` | 라이브러리 사용 예제 (콘솔 데모) |

## 주요 기능

- **파싱**: 객체, 배열, 문자열(`\uXXXX` 서로게이트 페어 포함), 숫자(지수 표기 포함), `true`/`false`/`null` 지원
- **오류 처리**: 파싱 실패 시 `JsonParseException`으로 줄/열 번호와 메시지 제공
- **직렬화**: `Dump(prettyIndent)` — `0`이면 압축 출력, 양수면 들여쓰기 출력. 숫자는 왕복 변환 가능한 최단 표현으로 출력
- **순서 보존**: 객체 키는 입력된 순서를 그대로 유지 (`std::vector<std::pair<...>>` 기반)
- **값 조작**: `operator[]`, `Find()`, `Push()`로 객체/배열 값 조회 및 수정
- **파일 입출력**: `SaveToFile()` / `LoadFromFile()`로 JSON 파일 저장 및 읽기

## 사용 예제

```cpp
#include "Json.h"

using namespace json;

// 파싱
JsonValue value = JsonValue::Parse(R"({"name": "JsonPoC", "version": 1})");

// 값 조회
if (const JsonValue* name = value.Find("name"))
{
    std::cout << name->AsString() << "\n";
}

// 값 수정/추가
value["version"] = JsonValue(2);
value["newField"] = JsonValue("added");

// 직렬화 (들여쓰기 2칸)
std::cout << value.Dump(2) << "\n";

// 파일로 저장 및 다시 로드
value.SaveToFile("output.json");
JsonValue loaded = JsonValue::LoadFromFile("output.json");
```

## 빌드

Visual Studio 2022(v145 도구 세트) 이상, C++20 표준을 사용합니다.

1. `JsonPoC.slnx`를 Visual Studio에서 엽니다.
2. `Debug`/`Release` × `x64`/`Win32` 구성 중 하나를 선택해 빌드합니다.
3. 소스 파일이 UTF-8로 작성되어 있으므로 컴파일러 옵션에 `/utf-8`이 적용되어 있습니다.

콘솔에서 한글이 깨지는 경우, 실행 파일이 시작 시 `SetConsoleOutputCP(CP_UTF8)`을 호출해 콘솔 코드페이지를 UTF-8로 전환하도록 처리되어 있습니다. 그래도 깨진다면 콘솔 글꼴을 트루타입 글꼴(예: 맑은 고딕, Consolas)로 변경하세요.
