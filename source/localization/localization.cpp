#include "localization/localization.hpp"
#include <algorithm>
#include <cstring>
#include <array>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <unistd.h>
namespace localization {
namespace {
Language active=Language::Spanish;
const Entry entries[]={
#include "localization/app_catalog.inc"
#include "localization/content_catalog.inc"
#include "localization/tools_catalog.inc"
};
bool boundary(unsigned char c){return !((c>='a'&&c<='z')||(c>='A'&&c<='Z')||c>=128);}
struct Cached{Language lang=Language::Spanish;std::string source,result;};
std::array<Cached,64> cache;size_t cacheNext=0;
const Entry *exact(const std::string&s){const auto&all=catalog();auto it=std::lower_bound(all.begin(),all.end(),s,[](const Entry*e,const std::string&key){return std::strcmp(e->es,key.c_str())<0;});return it!=all.end()&&s==(*it)->es?*it:nullptr;}
bool formatted(const Entry&e,const std::string&text,std::string&out){
 std::vector<std::string> args;size_t p=0,i=0;std::string pattern=e.es;
 while(p<pattern.size()){
  if(pattern[p]=='%'&&(pattern.compare(p,2,"%d")==0||pattern.compare(p,4,"%02d")==0)){
   size_t begin=i;if(i<text.size()&&(text[i]=='-'||text[i]=='+'))++i;size_t digits=i;
   while(i<text.size()&&text[i]>='0'&&text[i]<='9')++i;
   if(i==digits)return false;
   args.push_back(text.substr(begin,i-begin));p+=pattern.compare(p,2,"%d")==0?2:4;
  }else{if(i>=text.size()||pattern[p++]!=text[i++])return false;}
 }
 if(i!=text.size()||args.empty())return false;
 std::string target=e.en;size_t a=0;out.clear();
 for(size_t n=0;n<target.size();){if(target[n]=='%'&&(target.compare(n,2,"%d")==0||target.compare(n,4,"%02d")==0)){if(a>=args.size())return false;out+=args[a++];n+=target.compare(n,2,"%d")==0?2:4;}else out+=target[n++];}
 return a==args.size();
}
bool read(const std::string&path,Language&out){std::ifstream f(path,std::ios::binary);char b[9]{};f.read(b,9);if(f.bad()||f.gcount()!=8)return false;std::string s(b,8);if(s.substr(0,5)!="LANG1"||(s[5]!='S'&&s[5]!='E')||s[6]!=char(s[5]^0x55)||s[7]!='\n')return false;out=s[5]=='E'?Language::English:Language::Spanish;return true;}
}
void setLanguage(Language l){active=l;}
Language language(){return active;}
const std::vector<const Entry*>&catalog(){static const auto sorted=[](){std::vector<const Entry*>v;for(const auto&e:entries)v.push_back(&e);std::sort(v.begin(),v.end(),[](const Entry*a,const Entry*b){return std::strcmp(a->es,b->es)<0;});return v;}();return sorted;}
std::string translate(const std::string&s){
 if(active==Language::Spanish||s.empty())return s;
 if(const auto*e=exact(s))return e->en;
 for(const auto&c:cache)if(c.lang==active&&c.source==s)return c.result;
 static const auto buckets=[](){std::array<std::vector<const Entry*>,256>b;for(auto e:catalog())if(*e->es)b[(unsigned char)*e->es].push_back(e);for(auto&v:b)std::sort(v.begin(),v.end(),[](const Entry*a,const Entry*b){return std::char_traits<char>::length(a->es)>std::char_traits<char>::length(b->es);});return b;}();
 std::string out;
 static const auto formats=[](){std::vector<const Entry*>v;for(auto*e:catalog())if(std::strstr(e->es,"%d")||std::strstr(e->es,"%02d"))v.push_back(e);return v;}();
 for(const auto *e:formats)if(formatted(*e,s,out))return out;
 out.reserve(s.size()+16);
 for(size_t i=0;i<s.size();){const Entry*found=nullptr;size_t n=0;
  for(const auto*e:buckets[(unsigned char)s[i]]){n=std::char_traits<char>::length(e->es);if(n>s.size()-i||s.compare(i,n,e->es)!=0)continue;
   if(i&&!boundary((unsigned char)s[i-1])&&!boundary((unsigned char)e->es[0]))continue;
   if(i+n<s.size()&&!boundary((unsigned char)s[i+n])&&!boundary((unsigned char)e->es[n-1]))continue;
   found=e;break;}
  if(found){out+=found->en;i+=n;}else out+=s[i++];
 }
 if(s.size()<=512){cache[cacheNext]={active,s,out};cacheNext=(cacheNext+1)%cache.size();}
 return out;
}
uint32_t nextCodepoint(const std::string&s,size_t&i){if(i>=s.size())return 0;auto c=(unsigned char)s[i++];if(c<128)return c;int n=c>=0xf0?3:c>=0xe0?2:c>=0xc2?1:0;if(!n)return '?';uint32_t value=c&((1u<<(6-n))-1);for(int j=0;j<n;++j){if(i>=s.size()||((unsigned char)s[i]&0xc0)!=0x80)return '?';value=(value<<6)|((unsigned char)s[i++]&63);}return value;}
size_t glyphCount(const std::string&s){size_t i=0,n=0;while(i<s.size()){nextCodepoint(s,i);++n;}return n;}
int fontIndex(uint32_t c){if(c>=32&&c<=126)return int(c-32);static constexpr uint32_t extra[]={0xe1,0xe9,0xed,0xf3,0xfa,0xfc,0xf1,0xc1,0xc9,0xcd,0xd3,0xda,0xdc,0xd1,0xbf,0xa1};for(int i=0;i<16;++i)if(c==extra[i])return 95+i;return '?'-32;}
bool loadLanguage(const std::string&p,Language&l){Language c;if(!read(p,c)&&!read(p+".bak",c))return false;l=c;return true;}
bool saveLanguage(const std::string&p,Language l){if(l!=Language::Spanish&&l!=Language::English)return false;std::string s="LANG1";s+=l==Language::English?'E':'S';s+=char(s[5]^0x55);s+='\n';auto tmp=p+".tmp";FILE*f=fopen(tmp.c_str(),"wb");if(!f)return false;bool ok=fwrite(s.data(),1,s.size(),f)==s.size();if(fflush(f)||fsync(fileno(f)))ok=false;if(fclose(f))ok=false;Language prior;if(ok&&read(p,prior)){std::ifstream from(p,std::ios::binary);std::string old((std::istreambuf_iterator<char>(from)),{});FILE*b=fopen((p+".bak.tmp").c_str(),"wb");if(!b)ok=false;else{bool good=fwrite(old.data(),1,old.size(),b)==old.size();if(fflush(b)||fsync(fileno(b)))good=false;if(fclose(b))good=false;ok=good&&rename((p+".bak.tmp").c_str(),(p+".bak").c_str())==0;}}
 if(ok&&rename(tmp.c_str(),p.c_str())==0)return true;
 remove(tmp.c_str());return false;}
}
