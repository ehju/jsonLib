# CRUD 콘솔 애플리케이션 구현 계획

`JsonPoC` 라이브러리(`json::JsonValue`)를 그대로 재사용해 JSON 파일 기반 CRUD 콘솔 애플리케이션을 만든다. 구현 순서는 **기본 골격 → Create → Read → Update → Delete** 순으로 진행한다. 각 단계는 실제로 실행해 눈으로 동작을 확인한 뒤 다음 단계로 넘어간다.

## 데이터 모델

- 저장 파일: `data.json` (최상위는 JSON 배열, 각 원소는 객체 하나 = 레코드 하나)
- 각 레코드는 최소한 고유 식별자 필드 `id`(정수, 자동 증가)를 가지며, 그 외 필드(예: `name`, `value` 등)는 자유롭게 확장 가능한 구조로 설계
- 예시:
  ```json
  [
    { "id": 1, "name": "example", "value": "hello" }
  ]
  ```

## Phase 0 — 기본 애플리케이션 골격

- `Project1`(현재 빈 스캐폴드)을 `CRUD` 프로젝트로 정리하거나, 동일한 설정으로 새 `CRUD` 프로젝트를 만들어 `JsonPoC.slnx`에 추가
- `JsonPoC/Json.h`, `JsonPoC/Json.cpp`를 `CRUD` 프로젝트에 기존 항목으로 추가해 라이브러리를 그대로 재사용 (코드 중복 금지)
- `CLAUDE.md`에 명시된 대로 4개 구성 모두 `<AdditionalOptions>/utf-8 %(AdditionalOptions)</AdditionalOptions>` 적용
- `main()`에서 `SetConsoleOutputCP(CP_UTF8)` / `SetConsoleCP(CP_UTF8)` 호출 (한글 콘솔 깨짐 방지)
- 프로그램 시작 시 `data.json`이 있으면 `JsonValue::LoadFromFile`로 불러오고, 없으면 빈 배열로 시작
- 메뉴 루프 구현: 실행 시 `1) Create  2) Read  3) Update  4) Delete  5) 종료` 형태의 텍스트 메뉴를 반복 출력하고 입력을 받아 분기 (이 시점엔 각 메뉴가 "미구현" 메시지만 출력해도 됨)
- 검증: 빌드 후 실행해 메뉴가 뜨고 종료가 정상 동작하는지 확인

## Phase 1 — Create

- 사용자로부터 필드 값을 입력받아 새 레코드(`JsonValue::MakeObject()`)를 만든다
- `id`는 기존 레코드 중 최댓값 + 1로 자동 부여 (배열이 비어있으면 1부터 시작)
- 배열에 `Push()`로 추가 후 `SaveToFile()`로 즉시 저장 (메모리 상태와 파일 상태를 항상 동기화)
- 검증: 실행 → 데이터 입력 → 종료 후 재실행 시 `data.json`에서 정상적으로 로드되는지 확인

## Phase 2 — Read

- 전체 목록 보기: 배열을 순회하며 각 레코드를 보기 좋게 출력 (표 형태 또는 필드별 나열)
- 특정 ID/키 값으로 검색: `id` 입력 → 배열에서 일치하는 레코드 탐색 후 출력, 없으면 "찾을 수 없음" 메시지
- 필요하면 키(필드명)+값으로 검색하는 옵션도 추가 (예: `name`으로 검색)
- 검증: Create로 여러 건 입력 후 전체 목록/ID 검색/미존재 ID 검색 각각 확인

## Phase 3 — Update

- ID를 입력받아 해당 레코드를 찾는다 (없으면 오류 메시지 후 메뉴로 복귀)
- 찾은 레코드의 어떤 필드를 수정할지 선택 → 새 값 입력 → `record[field] = JsonValue(newValue)`로 수정
- 수정 후 `SaveToFile()`로 즉시 저장
- 검증: 특정 레코드의 필드 하나를 수정 → 재실행 후 Read로 변경 사항이 유지되는지 확인

## Phase 4 — Delete

- ID를 입력받아 해당 레코드를 찾는다 (없으면 오류 메시지)
- 삭제 전 대상 레코드 내용을 보여주고 **확인(y/n) 절차**를 거친 뒤 삭제 (안전한 삭제)
- 배열에서 해당 원소 제거 후 `SaveToFile()`로 즉시 저장
- 검증: 삭제 후 Read로 목록에서 사라졌는지, 재실행 후에도 삭제 상태가 유지되는지 확인

## 공통 원칙

- 모든 저장/로드는 `json::JsonValue`의 `SaveToFile`/`LoadFromFile`, `Dump`/`Parse`만 사용하고 파일을 직접 열어 파싱하지 않는다
- 객체 키 순서 보존, 왕복 안전 숫자 직렬화 등 `JsonPoC` 라이브러리의 기존 동작을 변경하지 않는다
- 각 Phase 완료 시점마다 실행 파일을 직접 실행해 해당 기능이 실제로 동작하는지 확인한 뒤 다음 Phase로 진행한다
