#pragma once
#include <string>

bool RegexFull_Nothrow(const std::string& Str, const std::string& Regex) throw();
bool RegexFull_Throw(const std::string& Str, const std::string& Regex);
bool RegexNone_Nothrow(const std::string& Str, const std::string& Regex) throw();
bool RegexNone_Throw(const std::string& Str, const std::string& Regex);
bool RegexNotFull_Nothrow(const std::string& Str, const std::string& Regex) throw();
bool RegexNotFull_Throw(const std::string& Str, const std::string& Regex);
bool RegexNotNone_Nothrow(const std::string& Str, const std::string& Regex) throw();
bool RegexNotNone_Throw(const std::string& Str, const std::string& Regex);