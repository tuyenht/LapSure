#include "lap/trust.h"
#include <windows.h>
#include <bcrypt.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <algorithm>
#pragma comment(lib,"bcrypt.lib")
namespace lap {
namespace {
std::wstring Lower(std::wstring s){std::transform(s.begin(),s.end(),s.begin(),towlower);return s;}
std::wstring HashFile(const std::wstring& path){
    BCRYPT_ALG_HANDLE alg=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;
    if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)!=0)return L"";
    DWORD objLen=0,cb=0,hashLen=0;BCryptGetProperty(alg,BCRYPT_OBJECT_LENGTH,(PUCHAR)&objLen,sizeof(objLen),&cb,0);
    BCryptGetProperty(alg,BCRYPT_HASH_LENGTH,(PUCHAR)&hashLen,sizeof(hashLen),&cb,0);
    std::vector<UCHAR> obj(objLen),digest(hashLen);
    if(BCryptCreateHash(alg,&hash,obj.data(),objLen,nullptr,0,0)!=0){BCryptCloseAlgorithmProvider(alg,0);return L"";}
    std::ifstream f(std::filesystem::path(path),std::ios::binary);char buf[1<<16];
    while(f){f.read(buf,sizeof(buf));auto n=f.gcount();if(n>0)BCryptHashData(hash,(PUCHAR)buf,(ULONG)n,0);}
    BCryptFinishHash(hash,digest.data(),hashLen,0);BCryptDestroyHash(hash);BCryptCloseAlgorithmProvider(alg,0);
    std::wstringstream ss;ss<<std::hex<<std::setfill(L'0');for(auto b:digest)ss<<std::setw(2)<<(unsigned)b;return Lower(ss.str());
}
}
EngineTrust VerifyEngine(const std::wstring& appDir,const std::wstring& relativePath,const std::wstring& logicalName){
    EngineTrust t{};auto full=(std::filesystem::path(appDir)/relativePath).wstring();t.fileExists=std::filesystem::exists(full);
    if(!t.fileExists){t.reason=L"Engine file not found.";return t;}
    t.sha256=HashFile(full);if(t.sha256.empty()){t.reason=L"SHA-256 calculation failed.";return t;}
    auto manifest=std::filesystem::path(appDir)/L"tools"/L"engine_manifest.txt";std::wifstream f(manifest);
    if(!f){t.reason=L"Trust manifest missing.";return t;}
    std::wstring line;while(std::getline(f,line)){
        if(line.empty()||line[0]==L'#')continue;
        auto p=line.find(L'=');if(p==std::wstring::npos)continue;
        auto name=line.substr(0,p),hash=Lower(line.substr(p+1));
        name.erase(name.find_last_not_of(L" \t\r\n")+1);hash.erase(0,hash.find_first_not_of(L" \t"));
        if(Lower(name)==Lower(logicalName)){t.manifestEntry=true;t.expectedSha256=hash;break;}
    }
    if(!t.manifestEntry){t.reason=L"No allowlist entry for engine.";return t;}
    t.hashMatches=(Lower(t.sha256)==Lower(t.expectedSha256));
    t.reason=t.hashMatches?L"Trusted engine hash matched.":L"Engine hash mismatch.";
    return t;
}
}
