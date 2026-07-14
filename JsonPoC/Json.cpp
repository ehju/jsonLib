#include "Json.h"

#include <fstream>
#include <sstream>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <algorithm>

namespace json
{
    namespace
    {
        // 파싱 위치와 원본 텍스트를 함께 들고 다니는 커서
        class Cursor
        {
        public:
            explicit Cursor(const std::string& text) : m_text(text), m_pos(0) {}

            bool AtEnd() const { return m_pos >= m_text.size(); }

            char Peek() const
            {
                if (AtEnd())
                {
                    Fail("예상치 못하게 입력이 끝났습니다.");
                }
                return m_text[m_pos];
            }

            char Advance()
            {
                char c = Peek();
                ++m_pos;
                return c;
            }

            bool Match(char expected)
            {
                if (!AtEnd() && m_text[m_pos] == expected)
                {
                    ++m_pos;
                    return true;
                }
                return false;
            }

            void Expect(char expected, const std::string& context)
            {
                if (!Match(expected))
                {
                    Fail("'" + std::string(1, expected) + "' 문자가 필요합니다 (" + context + ")");
                }
            }

            void SkipWhitespace()
            {
                while (!AtEnd())
                {
                    char c = m_text[m_pos];
                    if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
                    {
                        ++m_pos;
                    }
                    else
                    {
                        break;
                    }
                }
            }

            [[noreturn]] void Fail(const std::string& message) const
            {
                size_t line = 1;
                size_t column = 1;
                for (size_t i = 0; i < m_pos && i < m_text.size(); ++i)
                {
                    if (m_text[i] == '\n')
                    {
                        ++line;
                        column = 1;
                    }
                    else
                    {
                        ++column;
                    }
                }
                throw JsonParseException(message, m_pos, line, column);
            }

            size_t Pos() const { return m_pos; }

        private:
            const std::string& m_text;
            size_t m_pos;
        };

        JsonValue ParseValue(Cursor& cur);

        void ParseHex4(Cursor& cur, unsigned int& outCodepoint)
        {
            unsigned int value = 0;
            for (int i = 0; i < 4; ++i)
            {
                char c = cur.Advance();
                value <<= 4;
                if (c >= '0' && c <= '9') value |= static_cast<unsigned int>(c - '0');
                else if (c >= 'a' && c <= 'f') value |= static_cast<unsigned int>(c - 'a' + 10);
                else if (c >= 'A' && c <= 'F') value |= static_cast<unsigned int>(c - 'A' + 10);
                else cur.Fail("잘못된 \\u 이스케이프 시퀀스입니다.");
            }
            outCodepoint = value;
        }

        void AppendUtf8(std::string& out, unsigned int codepoint)
        {
            if (codepoint <= 0x7F)
            {
                out.push_back(static_cast<char>(codepoint));
            }
            else if (codepoint <= 0x7FF)
            {
                out.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
                out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
            }
            else if (codepoint <= 0xFFFF)
            {
                out.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
                out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
            }
            else
            {
                out.push_back(static_cast<char>(0xF0 | (codepoint >> 18)));
                out.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
            }
        }

        std::string ParseStringLiteral(Cursor& cur)
        {
            cur.Expect('"', "문자열 시작");
            std::string result;
            for (;;)
            {
                char c = cur.Advance();
                if (c == '"')
                {
                    break;
                }
                if (c == '\\')
                {
                    char esc = cur.Advance();
                    switch (esc)
                    {
                    case '"': result.push_back('"'); break;
                    case '\\': result.push_back('\\'); break;
                    case '/': result.push_back('/'); break;
                    case 'b': result.push_back('\b'); break;
                    case 'f': result.push_back('\f'); break;
                    case 'n': result.push_back('\n'); break;
                    case 'r': result.push_back('\r'); break;
                    case 't': result.push_back('\t'); break;
                    case 'u':
                    {
                        unsigned int codepoint = 0;
                        ParseHex4(cur, codepoint);
                        if (codepoint >= 0xD800 && codepoint <= 0xDBFF)
                        {
                            // 서로게이트 페어 처리
                            if (cur.Advance() != '\\' || cur.Advance() != 'u')
                            {
                                cur.Fail("서로게이트 페어 뒤에 \\u 이스케이프가 필요합니다.");
                            }
                            unsigned int low = 0;
                            ParseHex4(cur, low);
                            if (low < 0xDC00 || low > 0xDFFF)
                            {
                                cur.Fail("잘못된 서로게이트 페어입니다.");
                            }
                            unsigned int combined = 0x10000 + ((codepoint - 0xD800) << 10) + (low - 0xDC00);
                            AppendUtf8(result, combined);
                        }
                        else
                        {
                            AppendUtf8(result, codepoint);
                        }
                        break;
                    }
                    default:
                        cur.Fail("알 수 없는 이스케이프 시퀀스입니다.");
                    }
                }
                else
                {
                    result.push_back(c);
                }
            }
            return result;
        }

        JsonValue ParseNumber(Cursor& cur)
        {
            size_t start = cur.Pos();
            std::string buffer;
            if (!cur.AtEnd() && cur.Peek() == '-')
            {
                buffer.push_back(cur.Advance());
            }
            if (cur.AtEnd() || !isdigit(static_cast<unsigned char>(cur.Peek())))
            {
                cur.Fail("숫자 형식이 올바르지 않습니다.");
            }
            if (cur.Peek() == '0')
            {
                buffer.push_back(cur.Advance());
            }
            else
            {
                while (!cur.AtEnd() && isdigit(static_cast<unsigned char>(cur.Peek())))
                {
                    buffer.push_back(cur.Advance());
                }
            }
            if (!cur.AtEnd() && cur.Peek() == '.')
            {
                buffer.push_back(cur.Advance());
                if (cur.AtEnd() || !isdigit(static_cast<unsigned char>(cur.Peek())))
                {
                    cur.Fail("소수점 뒤에 숫자가 필요합니다.");
                }
                while (!cur.AtEnd() && isdigit(static_cast<unsigned char>(cur.Peek())))
                {
                    buffer.push_back(cur.Advance());
                }
            }
            if (!cur.AtEnd() && (cur.Peek() == 'e' || cur.Peek() == 'E'))
            {
                buffer.push_back(cur.Advance());
                if (!cur.AtEnd() && (cur.Peek() == '+' || cur.Peek() == '-'))
                {
                    buffer.push_back(cur.Advance());
                }
                if (cur.AtEnd() || !isdigit(static_cast<unsigned char>(cur.Peek())))
                {
                    cur.Fail("지수 표기 뒤에 숫자가 필요합니다.");
                }
                while (!cur.AtEnd() && isdigit(static_cast<unsigned char>(cur.Peek())))
                {
                    buffer.push_back(cur.Advance());
                }
            }
            (void)start;
            return JsonValue(std::stod(buffer));
        }

        void ParseLiteral(Cursor& cur, const char* literal)
        {
            for (const char* p = literal; *p; ++p)
            {
                if (cur.Advance() != *p)
                {
                    cur.Fail(std::string("'") + literal + "' 리터럴이 올바르지 않습니다.");
                }
            }
        }

        JsonValue ParseArray(Cursor& cur)
        {
            cur.Expect('[', "배열 시작");
            JsonArray arr;
            cur.SkipWhitespace();
            if (cur.Match(']'))
            {
                return JsonValue(std::move(arr));
            }
            for (;;)
            {
                cur.SkipWhitespace();
                arr.push_back(ParseValue(cur));
                cur.SkipWhitespace();
                if (cur.Match(','))
                {
                    continue;
                }
                cur.Expect(']', "배열 종료");
                break;
            }
            return JsonValue(std::move(arr));
        }

        JsonValue ParseObject(Cursor& cur)
        {
            cur.Expect('{', "객체 시작");
            JsonObject obj;
            cur.SkipWhitespace();
            if (cur.Match('}'))
            {
                return JsonValue(std::move(obj));
            }
            for (;;)
            {
                cur.SkipWhitespace();
                std::string key = ParseStringLiteral(cur);
                cur.SkipWhitespace();
                cur.Expect(':', "키-값 구분자");
                cur.SkipWhitespace();
                JsonValue value = ParseValue(cur);
                obj.emplace_back(std::move(key), std::move(value));
                cur.SkipWhitespace();
                if (cur.Match(','))
                {
                    continue;
                }
                cur.Expect('}', "객체 종료");
                break;
            }
            return JsonValue(std::move(obj));
        }

        JsonValue ParseValue(Cursor& cur)
        {
            cur.SkipWhitespace();
            if (cur.AtEnd())
            {
                cur.Fail("값이 필요합니다.");
            }
            char c = cur.Peek();
            switch (c)
            {
            case '{': return ParseObject(cur);
            case '[': return ParseArray(cur);
            case '"': return JsonValue(ParseStringLiteral(cur));
            case 't': ParseLiteral(cur, "true"); return JsonValue(true);
            case 'f': ParseLiteral(cur, "false"); return JsonValue(false);
            case 'n': ParseLiteral(cur, "null"); return JsonValue(nullptr);
            default:
                if (c == '-' || isdigit(static_cast<unsigned char>(c)))
                {
                    return ParseNumber(cur);
                }
                cur.Fail("알 수 없는 값 형식입니다.");
            }
            return JsonValue();
        }

        void AppendEscapedString(std::string& out, const std::string& s)
        {
            out.push_back('"');
            for (unsigned char c : s)
            {
                switch (c)
                {
                case '"': out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\b': out += "\\b"; break;
                case '\f': out += "\\f"; break;
                case '\n': out += "\\n"; break;
                case '\r': out += "\\r"; break;
                case '\t': out += "\\t"; break;
                default:
                    if (c < 0x20)
                    {
                        char buf[8];
                        std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                        out += buf;
                    }
                    else
                    {
                        out.push_back(static_cast<char>(c));
                    }
                }
            }
            out.push_back('"');
        }

        void AppendNumber(std::string& out, double value)
        {
            if (std::isnan(value) || std::isinf(value))
            {
                out += "null";
                return;
            }
            // 정수 값은 소수점 없이 출력
            if (value == std::floor(value) &&
                std::abs(value) < 1e15)
            {
                out += std::to_string(static_cast<long long>(value));
                return;
            }
            // 왕복 변환이 가능한 가장 짧은 자릿수를 탐색
            char buf[64];
            for (int precision = 1; precision <= 17; ++precision)
            {
                std::snprintf(buf, sizeof(buf), "%.*g", precision, value);
                if (std::strtod(buf, nullptr) == value)
                {
                    break;
                }
            }
            out += buf;
        }

        void AppendIndent(std::string& out, int prettyIndent, int depth)
        {
            if (prettyIndent > 0)
            {
                out.push_back('\n');
                out.append(static_cast<size_t>(prettyIndent) * static_cast<size_t>(depth), ' ');
            }
        }
    } // namespace

    const JsonValue* JsonValue::Find(const std::string& key) const
    {
        const auto& obj = AsObject();
        auto it = std::find_if(obj.begin(), obj.end(), [&key](const auto& kv) { return kv.first == key; });
        return it == obj.end() ? nullptr : &it->second;
    }

    JsonValue* JsonValue::Find(const std::string& key)
    {
        auto& obj = AsObject();
        auto it = std::find_if(obj.begin(), obj.end(), [&key](const auto& kv) { return kv.first == key; });
        return it == obj.end() ? nullptr : &it->second;
    }

    JsonValue& JsonValue::operator[](const std::string& key)
    {
        if (IsNull())
        {
            m_data = JsonObject{};
        }
        auto& obj = AsObject();
        auto it = std::find_if(obj.begin(), obj.end(), [&key](const auto& kv) { return kv.first == key; });
        if (it != obj.end())
        {
            return it->second;
        }
        obj.emplace_back(key, JsonValue());
        return obj.back().second;
    }

    JsonValue& JsonValue::operator[](size_t index)
    {
        return AsArray().at(index);
    }

    const JsonValue& JsonValue::operator[](size_t index) const
    {
        return AsArray().at(index);
    }

    void JsonValue::Push(JsonValue value)
    {
        if (IsNull())
        {
            m_data = JsonArray{};
        }
        AsArray().push_back(std::move(value));
    }

    void JsonValue::DumpTo(std::string& out, int prettyIndent, int depth) const
    {
        switch (Type())
        {
        case JsonType::Null:
            out += "null";
            break;
        case JsonType::Boolean:
            out += AsBool() ? "true" : "false";
            break;
        case JsonType::Number:
            AppendNumber(out, AsNumber());
            break;
        case JsonType::String:
            AppendEscapedString(out, AsString());
            break;
        case JsonType::Array:
        {
            const auto& arr = AsArray();
            if (arr.empty())
            {
                out += "[]";
                break;
            }
            out.push_back('[');
            for (size_t i = 0; i < arr.size(); ++i)
            {
                if (i > 0) out.push_back(',');
                AppendIndent(out, prettyIndent, depth + 1);
                arr[i].DumpTo(out, prettyIndent, depth + 1);
            }
            AppendIndent(out, prettyIndent, depth);
            out.push_back(']');
            break;
        }
        case JsonType::Object:
        {
            const auto& obj = AsObject();
            if (obj.empty())
            {
                out += "{}";
                break;
            }
            out.push_back('{');
            for (size_t i = 0; i < obj.size(); ++i)
            {
                if (i > 0) out.push_back(',');
                AppendIndent(out, prettyIndent, depth + 1);
                AppendEscapedString(out, obj[i].first);
                out.push_back(':');
                if (prettyIndent > 0) out.push_back(' ');
                obj[i].second.DumpTo(out, prettyIndent, depth + 1);
            }
            AppendIndent(out, prettyIndent, depth);
            out.push_back('}');
            break;
        }
        }
    }

    std::string JsonValue::Dump(int prettyIndent) const
    {
        std::string out;
        DumpTo(out, prettyIndent, 0);
        return out;
    }

    JsonValue JsonValue::Parse(const std::string& text)
    {
        Cursor cur(text);
        JsonValue value = ParseValue(cur);
        cur.SkipWhitespace();
        if (!cur.AtEnd())
        {
            cur.Fail("최상위 값 이후에 불필요한 데이터가 있습니다.");
        }
        return value;
    }

    JsonValue JsonValue::LoadFromFile(const std::filesystem::path& path)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file)
        {
            throw std::runtime_error("파일을 열 수 없습니다: " + path.string());
        }
        std::ostringstream ss;
        ss << file.rdbuf();
        return Parse(ss.str());
    }

    void JsonValue::SaveToFile(const std::filesystem::path& path, int prettyIndent) const
    {
        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        if (!file)
        {
            throw std::runtime_error("파일을 쓸 수 없습니다: " + path.string());
        }
        file << Dump(prettyIndent);
    }

} // namespace json
