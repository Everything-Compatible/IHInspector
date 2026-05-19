#include "Misc.h"
#include <regex>

bool RegexFull_Nothrow(const std::string& Str, const std::string& Regex) throw()
{
    try { return std::regex_match(Str, std::regex(Regex)); }
    catch (std::exception& e) { (void)e; return false; }
}
bool RegexFull_Throw(const std::string& Str, const std::string& Regex)
{
    try { return std::regex_match(Str, std::regex(Regex)); }
    catch (std::exception& e) { throw(e); }
}
bool RegexNone_Nothrow(const std::string& Str, const std::string& Regex) throw()
{
    try { return !std::regex_search(Str, std::regex(Regex)); }
    catch (std::exception& e) { (void)e; return false; }
}
bool RegexNone_Throw(const std::string& Str, const std::string& Regex)
{
    try { return !std::regex_search(Str, std::regex(Regex)); }
    catch (std::exception& e) { throw(e); }
}
bool RegexNotFull_Nothrow(const std::string& Str, const std::string& Regex) throw()
{
    try { return !std::regex_match(Str, std::regex(Regex)); }
    catch (std::exception& e) { (void)e; return false; }
}
bool RegexNotFull_Throw(const std::string& Str, const std::string& Regex)
{
    try { return !std::regex_match(Str, std::regex(Regex)); }
    catch (std::exception& e) { throw(e); }
}
bool RegexNotNone_Nothrow(const std::string& Str, const std::string& Regex) throw()
{
    try { return std::regex_search(Str, std::regex(Regex)); }
    catch (std::exception& e) { (void)e; return false; }
}
bool RegexNotNone_Throw(const std::string& Str, const std::string& Regex)
{
    try { return std::regex_search(Str, std::regex(Regex)); }
    catch (std::exception& e) { throw(e); }
}