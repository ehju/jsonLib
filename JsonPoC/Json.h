#pragma once

#include <string>
#include <vector>
#include <utility>
#include <memory>
#include <variant>
#include <stdexcept>
#include <cstdint>
#include <filesystem>

namespace json
{
    class JsonValue;

    using JsonArray = std::vector<JsonValue>;
    // 순서 보존을 위해 map 대신 vector<pair> 사용
    using JsonObject = std::vector<std::pair<std::string, JsonValue>>;

    enum class JsonType
    {
        Null,
        Boolean,
        Number,
        String,
        Array,
        Object
    };

    class JsonParseException : public std::runtime_error
    {
    public:
        JsonParseException(const std::string& message, size_t offset, size_t line, size_t column)
            : std::runtime_error(message)
            , m_offset(offset)
            , m_line(line)
            , m_column(column)
        {
        }

        size_t Offset() const noexcept { return m_offset; }
        size_t Line() const noexcept { return m_line; }
        size_t Column() const noexcept { return m_column; }

    private:
        size_t m_offset;
        size_t m_line;
        size_t m_column;
    };

    class JsonValue
    {
    public:
        JsonValue() : m_data(nullptr) {}
        JsonValue(std::nullptr_t) : m_data(nullptr) {}
        JsonValue(bool value) : m_data(value) {}
        JsonValue(int value) : m_data(static_cast<double>(value)) {}
        JsonValue(double value) : m_data(value) {}
        JsonValue(const char* value) : m_data(std::string(value)) {}
        JsonValue(std::string value) : m_data(std::move(value)) {}
        JsonValue(JsonArray value) : m_data(std::move(value)) {}
        JsonValue(JsonObject value) : m_data(std::move(value)) {}

        static JsonValue MakeArray() { return JsonValue(JsonArray{}); }
        static JsonValue MakeObject() { return JsonValue(JsonObject{}); }

        JsonType Type() const noexcept
        {
            switch (m_data.index())
            {
            case 0: return JsonType::Null;
            case 1: return JsonType::Boolean;
            case 2: return JsonType::Number;
            case 3: return JsonType::String;
            case 4: return JsonType::Array;
            case 5: return JsonType::Object;
            }
            return JsonType::Null;
        }

        bool IsNull() const noexcept { return Type() == JsonType::Null; }
        bool IsBool() const noexcept { return Type() == JsonType::Boolean; }
        bool IsNumber() const noexcept { return Type() == JsonType::Number; }
        bool IsString() const noexcept { return Type() == JsonType::String; }
        bool IsArray() const noexcept { return Type() == JsonType::Array; }
        bool IsObject() const noexcept { return Type() == JsonType::Object; }

        bool AsBool() const { return std::get<bool>(m_data); }
        double AsNumber() const { return std::get<double>(m_data); }
        const std::string& AsString() const { return std::get<std::string>(m_data); }
        const JsonArray& AsArray() const { return std::get<JsonArray>(m_data); }
        JsonArray& AsArray() { return std::get<JsonArray>(m_data); }
        const JsonObject& AsObject() const { return std::get<JsonObject>(m_data); }
        JsonObject& AsObject() { return std::get<JsonObject>(m_data); }

        // 객체 전용: 키로 값 찾기. 없으면 nullptr 반환
        const JsonValue* Find(const std::string& key) const;
        JsonValue* Find(const std::string& key);

        // 객체 전용: 키에 대응하는 값 참조. 없으면 새로 추가
        JsonValue& operator[](const std::string& key);

        // 배열 전용
        JsonValue& operator[](size_t index);
        const JsonValue& operator[](size_t index) const;

        void Push(JsonValue value);

        // 직렬화. prettyIndent가 0보다 크면 들여쓰기 형태로 출력
        std::string Dump(int prettyIndent = 0) const;

        // 파싱
        static JsonValue Parse(const std::string& text);

        // 파일 입출력
        static JsonValue LoadFromFile(const std::filesystem::path& path);
        void SaveToFile(const std::filesystem::path& path, int prettyIndent = 2) const;

    private:
        void DumpTo(std::string& out, int prettyIndent, int depth) const;

        std::variant<std::nullptr_t, bool, double, std::string, JsonArray, JsonObject> m_data;
    };

} // namespace json
