#include "input_validator.h"

#include <algorithm>
#include <cctype>
#include <regex>

namespace quarisma
{
namespace security
{

bool input_validator::validate_string_length(
    std::string_view str, size_t min_length, size_t max_length)
{
    const size_t len = str.length();
    return len >= min_length && len <= max_length;
}

bool input_validator::is_alphanumeric(std::string_view str)
{
    if (str.empty())
    {
        return false;
    }

    return std::all_of(str.begin(), str.end(), [](unsigned char c) { return std::isalnum(c); });
}

bool input_validator::is_printable_ascii(std::string_view str)
{
    return std::all_of(
        str.begin(),
        str.end(),
        [](unsigned char c)
        {
            return c >= 32 && c <= 126;  // Printable ASCII range
        });
}

bool input_validator::matches_pattern(std::string_view str, const std::regex& pattern)
{
    return std::regex_match(str.begin(), str.end(), pattern);
}

bool input_validator::has_no_null_bytes(std::string_view str)
{
    return std::find(str.begin(), str.end(), '\0') == str.end();
}

bool input_validator::is_safe_path(std::string_view path)
{
    // Check for path traversal sequences
    if (path.find("..") != std::string_view::npos)
    {
        return false;
    }

    // Check for absolute paths (Unix and Windows)
    if (!path.empty() && (path[0] == '/' || path[0] == '\\'))
    {
        return false;
    }

    // Check for Windows drive letters (e.g., C:)
    if (path.length() >= 2 && path[1] == ':')
    {
        return false;
    }

    // Check for null bytes
    if (!has_no_null_bytes(path))
    {
        return false;
    }

    return true;
}

bool input_validator::has_allowed_extension(
    std::string_view filename, const std::vector<std::string>& allowed_extensions)
{
    if (filename.empty() || allowed_extensions.empty())
    {
        return false;
    }

    // Find the last dot in the filename
    const size_t dot_pos = filename.rfind('.');
    if (dot_pos == std::string_view::npos)
    {
        return false;  // No extension found
    }

    std::string_view const extension = filename.substr(dot_pos);

    // Check if extension is in allowed list
    return std::any_of(
        allowed_extensions.begin(),
        allowed_extensions.end(),
        [extension](const std::string& allowed) { return extension == allowed; });
}

}  // namespace security
}  // namespace quarisma
