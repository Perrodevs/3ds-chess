#include "localization/localization.hpp"
#include <cassert>
#include <set>
#include <iostream>
int main(){std::set<std::string>keys;
 for(auto e:localization::catalog()){assert(*e->es&&*e->en&&keys.insert(e->es).second);localization::setLanguage(localization::Language::English);assert(localization::translate(e->es)==e->en);localization::setLanguage(localization::Language::Spanish);assert(localization::translate(e->es)==e->es);}
 assert(keys.size()>1300);localization::setLanguage(localization::Language::English);
 assert(localization::translate("Material final: blancas 5 / negras 3")=="Final material: white 5 / black 3");
 assert(localization::translate("ACADEMIA < 2/10 >")=="ACADEMY < 2/10 >");
 std::string chars="áéíóúüñÁÉÍÓÚÜÑ¿¡";assert(localization::glyphCount(chars)==16);size_t n=0;while(n<chars.size())assert(localization::fontIndex(localization::nextCodepoint(chars,n))>=95);
 std::cout<<keys.size()<<" unique translation keys: both languages, formatted numbers and Spanish glyph mapping passed\n";
}
