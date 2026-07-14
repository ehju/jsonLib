# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 프로젝트 목적

이 저장소의 목표는 **CRUD**라는 이름의 콘솔 애플리케이션을 개발하는 것입니다. `JsonPoC` 프로젝트에서 만든 자체 C++20 JSON 파싱/직렬화 라이브러리(`json::JsonValue`, `JsonPoC/Json.h` / `JsonPoC/Json.cpp`)를 그대로 활용하며, JSON 파일을 데이터 저장소로 삼아 다음 CRUD 기능을 제공합니다.

- **Create**: 새로운 데이터를 입력받아 JSON 파일에 저장
- **Read**: 전체 목록 조회 및 특정 ID/키 값으로 검색
- **Update**: 기존 데이터를 선택하여 특정 필드 수정
- **Delete**: 특정 데이터를 안전하게 삭제

`CRUD` 프로젝트는 `JsonPoC` 프로젝트의 코드 구조와 관례(아래 "따라야 할 코드 구조" 참고)를 유지하면서 구현되어야 합니다. JSON 파싱/직렬화 로직을 새로 만들지 말고, 반드시 `json::JsonValue` API를 통해 데이터를 다뤄야 합니다.

## 빌드

Visual Studio 솔루션(`JsonPoC.slnx`)과 MSBuild, C++20, 툴셋 v145를 사용합니다. CMake나 Makefile은 없습니다.

MSBuild가 기본 PATH에 없으므로 `vswhere`로 먼저 위치를 찾은 뒤 빌드합니다 (PowerShell 예시):

```powershell
$msbuild = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe
& $msbuild "JsonPoC.slnx" /p:Configuration=Debug /p:Platform=x64 /nologo /v:minimal
```

구성: `Debug`/`Release` x `x64`/`Win32`. 결과물 경로는 구성/플랫폼에 따라 달라지며, 예를 들어 `x64/Debug/JsonPoC.exe` 형태입니다.

빌드한 실행 파일은 직접 실행해 확인합니다 (테스트 프레임워크나 lint 설정은 없음):

```powershell
.\x64\Debug\JsonPoC.exe
```

현재 자동화된 테스트는 없으며, `JsonPoC.cpp`의 콘솔 데모를 실행하고 출력/`output.json` 내용을 확인하는 방식으로 동작을 검증합니다. `CRUD` 프로젝트를 추가할 때도 같은 방식(직접 실행해서 Create/Read/Update/Delete 흐름을 눈으로 확인)으로 검증하면 됩니다.

## 중요한 빌드 설정: `/utf-8`

소스 파일에 한국어 주석/문자열이 포함되어 있고 UTF-8로 인코딩되어 있습니다. `JsonPoC/JsonPoC.vcxproj`의 4개 `ClCompile` 구성 모두 `<AdditionalOptions>/utf-8 %(AdditionalOptions)</AdditionalOptions>`를 유지해야 합니다. 이 옵션이 없으면 MSVC가 소스를 시스템 코드페이지(예: CP949)로 잘못 해석해 한글이 포함된 부분에서 C2065/C2001류의 컴파일 오류가 발생합니다. `CRUD` 프로젝트를 새로 만들 때도 동일하게 `/utf-8` 옵션을 추가하고, 소스 파일은 UTF-8 인코딩을 유지해야 합니다.

또한 `JsonPoC.cpp`의 `main()`은 Windows에서 `SetConsoleOutputCP(CP_UTF8)` / `SetConsoleCP(CP_UTF8)`를 호출해 콘솔이 한글/유니코드 출력을 올바르게 표시하도록 합니다(그렇지 않으면 Visual Studio 콘솔이 시스템 로케일 코드페이지를 기본값으로 사용해 한글이 깨집니다). `CRUD` 콘솔 애플리케이션도 동일한 처리를 `main()` 시작 부분에 넣어야 합니다.

## JsonPoC 라이브러리 아키텍처 (CRUD가 그대로 재사용할 부분)

- **`json::JsonValue`** (`Json.h`)는 모든 JSON 타입을 하나로 표현하는 variant 기반 값 타입입니다 (`std::variant<nullptr_t, bool, double, std::string, JsonArray, JsonObject>`). 파서/값-트리 클래스 계층을 따로 두지 않고, 파싱·저장·직렬화가 모두 이 하나의 타입 위에서 이루어집니다.
- **`JsonObject`**는 `map`이 아니라 `std::vector<std::pair<std::string, JsonValue>>`입니다. 파싱 → 수정 → 직렬화 과정에서 키의 입력 순서를 보존하기 위한 의도적인 설계입니다. 객체 키 조회(`Find`, `operator[]`)는 선형 탐색이며, 이는 순서 보존이라는 설계 목표에 따른 것이므로 "최적화가 덜 된 것"으로 보고 임의로 `map`/`unordered_map`으로 바꾸면 안 됩니다.
- **파싱** (`Json.cpp`)은 내부 `Cursor` 클래스를 중심으로 한 수작업 재귀 하강 파서입니다. `Cursor`는 바이트 오프셋을 추적하고, 실패 시 `JsonParseException`으로 줄/열 번호를 함께 보고합니다. 모든 파싱 함수(`ParseValue`, `ParseObject`, `ParseArray`, `ParseStringLiteral`, `ParseNumber`)는 `Cursor&`를 받아 `JsonValue`를 반환하거나 예외를 던집니다.
- **문자열 이스케이프**는 `\uXXXX` 전체와 UTF-16 서로게이트 페어를 지원하며, `AppendUtf8`을 통해 UTF-8로 변환됩니다.
- **직렬화** (`JsonValue::Dump` / `DumpTo`)는 압축 출력(`prettyIndent = 0`)과 들여쓰기 출력을 모두 지원합니다. 숫자는 고정 정밀도가 아니라, `%.*g`로 정밀도를 늘려가며 `strtod`로 왕복 검증해 가장 짧은 표현을 찾아 출력합니다 (`3.14`가 `3.1400000000000001`처럼 깨지지 않도록 하기 위함).
- **파일 입출력**: `JsonValue::SaveToFile` / `LoadFromFile`이 `Dump`/`Parse`를 `std::filesystem::path` 기반 파일 스트림으로 감싸 제공합니다. `CRUD` 애플리케이션의 Create/Read/Update/Delete는 파일을 직접 열지 말고 이 두 함수를 통해 JSON 파일을 읽고 써야 합니다.

## 저장소 구성 관련 참고

- `.gitignore`는 VS 빌드/캐시 산출물(`.vs/`, `x64/`, `*.vcxproj.user`), 런타임에 생성되는 `output.json`, 로컬/비밀 설정(`.mcp.json`, `.claude/settings.local.json`)을 제외합니다. 이 파일들이 로컬에 다시 생기더라도 커밋하지 않습니다.
