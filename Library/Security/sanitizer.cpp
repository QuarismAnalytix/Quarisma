#include "sanitizer.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>

namespace xsigma
{
namespace security
{

std::string sanitizer::remove_null_bytes(std::string_view str)
{
    std::string result;
    result.reserve(str.length());

    for (char const c : str)
    {
        if (c != '\0')
        {
            result += c;
        }
    }

    return result;
}

std::string sanitizer::remove_non_printable(std::string_view str)
{
    std::string result;
    result.reserve(str.length());

    for (unsigned char const c : str)
    {
        // Keep only printable ASCII (32-126)
        if (c >= 32 && c <= 126)
        {
            result += static_cast<char>(c);
        }
    }

    return result;
}

std::string sanitizer::trim(std::string_view str)
{
    if (str.empty())
    {
        return {};
    }

    // Find first non-whitespace character
    const auto start_it =
        std::find_if(str.begin(), str.end(), [](unsigned char c) { return !std::isspace(c); });

    // Find last non-whitespace character
    const auto end_it =
        std::find_if(str.rbegin(), str.rend(), [](unsigned char c) { return !std::isspace(c); })
            .base();

    if (start_it >= end_it)
    {
        return {};
    }

    return {start_it, end_it};
}

std::string sanitizer::truncate(std::string_view str, size_t max_length)
{
    if (str.length() <= max_length)
    {
        return std::string(str);
    }

    return std::string(str.substr(0, max_length));
}

std::string sanitizer::escape_html(std::string_view str)
{
    std::string result;
    result.reserve(str.length() * 2);  // Reserve extra space for escapes

    for (char const c : str)
    {
        switch (c)
        {
        case '<':
            result += "&lt;";
            break;
        case '>':
            result += "&gt;";
            break;
        case '&':
            result += "&amp;";
            break;
        case '"':
            result += "&quot;";
            break;
        case '\'':
            result += "&#39;";
            break;
        default:
            result += c;
            break;
        }
    }

    return result;
}

std::string sanitizer::escape_sql(std::string_view str)
{
    std::string result;
    result.reserve(str.length() * 2);

    for (char const c : str)
    {
        if (c == '\'')
        {
            result += "''";  // SQL escape for single quote
        }
        else if (c == '\\')
        {
            result += "\\\\";  // Escape backslash
        }
        else if (c == '\0')
        {
            // Skip null bytes
            continue;
        }
        else
        {
            result += c;
        }
    }

    return result;
}

std::string sanitizer::escape_shell(std::string_view str)
{
    std::string result;
    result.reserve(str.length() * 2);

    for (char const c : str)
    {
        // Escape shell special characters
        if (c == '$' || c == '`' || c == '\\' || c == '"' || c == '\'' || c == '!' || c == '&' ||
            c == '|' || c == ';' || c == '<' || c == '>' || c == '(' || c == ')' || c == '{' ||
            c == '}' || c == '[' || c == ']' || c == '*' || c == '?' || c == '~' || c == '#' ||
            c == ' ' || c == '\t' || c == '\n')
        {
            result += '\\';
        }
        result += c;
    }

    return result;
}

std::string sanitizer::escape_json(std::string_view str)
{
    std::string result;
    result.reserve(str.length() * 2);

    for (char const c : str)
    {
        switch (c)
        {
        case '"':
            result += "\\\"";
            break;
        case '\\':
            result += "\\\\";
            break;
        case '\b':
            result += "\\b";
            break;
        case '\f':
            result += "\\f";
            break;
        case '\n':
            result += "\\n";
            break;
        case '\r':
            result += "\\r";
            break;
        case '\t':
            result += "\\t";
            break;
        default:
            if (static_cast<unsigned char>(c) < 32)
            {
                // Escape control characters
                std::ostringstream oss;
                oss << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                    << static_cast<int>(static_cast<unsigned char>(c));
                result += oss.str();
            }
            else
            {
                result += c;
            }
            break;
        }
    }

    return result;
}

std::string sanitizer::escape_url(std::string_view str)
{
    std::ostringstream escaped;
    (void)escaped.fill('0');  // Explicitly ignore return value
    escaped << std::hex;

    for (unsigned char const c : str)
    {
        // Keep alphanumeric and safe characters
        if ((std::isalnum(c) != 0) || c == '-' || c == '_' || c == '.' || c == '~')
        {
            escaped << c;
        }
        else
        {
            // Percent-encode everything else
            escaped << '%' << std::setw(2) << static_cast<int>(c);
        }
    }

    return escaped.str();
}

std::string sanitizer::sanitize_path(std::string_view path)
{
    std::string result(path);

    // Remove null bytes
    result = remove_null_bytes(result);

    // Replace backslashes with forward slashes
    std::replace(result.begin(), result.end(), '\\', '/');

    // Remove drive letters (Windows) - do this before removing leading slashes
    if (result.length() >= 2 && result[1] == ':')
    {
        result.erase(0, 2);
    }

    // Remove leading slashes
    while (!result.empty() && result[0] == '/')
    {
        result.erase(0, 1);
    }

    // Remove .. sequences
    size_t pos = 0;
    while ((pos = result.find("..")) != std::string::npos)
    {
        result.erase(pos, 2);
    }

    // Remove leading slashes again (in case .. removal created them)
    while (!result.empty() && result[0] == '/')
    {
        result.erase(0, 1);
    }

    // Remove multiple consecutive slashes
    pos = 0;
    while ((pos = result.find("//", pos)) != std::string::npos)
    {
        result.erase(pos, 1);
    }

    return result;
}

std::string sanitizer::sanitize_filename(std::string_view filename)
{
    std::string result;
    result.reserve(filename.length());

    for (unsigned char const c : filename)
    {
        // Keep only safe characters: alphanumeric, underscore, hyphen, dot
        if ((std::isalnum(c) != 0) || c == '_' || c == '-' || c == '.')
        {
            result += static_cast<char>(c);
        }
        else
        {
            result += '_';  // Replace unsafe characters with underscore
        }
    }

    // Ensure filename doesn't start with a dot (hidden file)
    if (!result.empty() && result[0] == '.')
    {
        result[0] = '_';
    }

    return result;
}

}  // namespace security
}  // namespace xsigma
