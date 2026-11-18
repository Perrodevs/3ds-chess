#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
namespace localization {
enum class Language { Spanish, English };
#ifdef DEFAULT_LANGUAGE_EN
inline constexpr Language packageDefault=Language::English;
#else
inline constexpr Language packageDefault=Language::Spanish;
#endif
struct Entry { const char *es; const char *en; };
void setLanguage(Language);
Language language();
std::string translate(const std::string &);
const std::vector<const Entry*> &catalog();
size_t glyphCount(const std::string &);
uint32_t nextCodepoint(const std::string &,size_t &);
int fontIndex(uint32_t); // ASCII95 glyphs plus Spanish accents/inverted punctuation.
bool loadLanguage(const std::string &,Language &);
bool saveLanguage(const std::string &,Language);
}
