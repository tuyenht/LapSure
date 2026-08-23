#include "lap/baseline.h"
#include <windows.h>
#include <chrono>
#include <thread>
#include <vector>
#include <atomic>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <cwctype>
namespace lap { namespace {
std::wstring Lower(std::wstring s){std::transform(s.begin(),s.end(),s.begin(),towlower);return s;}
struct Baseline{std::wstring cpuContains;double low{-1},high{-1};std::wstring source;};
std::vector<Baseline> Load(const std::wstring&appDir){
 std::vector<Baseline> out;std::wifstream f(std::filesystem::path(appDir)/L"baselines"/L"cpu_microbench.tsv");std::wstring line;
 while(std::getline(f,line)){if(line.empty()||line[0]==L'#')continue;std::wstringstream ss(line);std::wstring a,b,c,d;
  if(!std::getline(ss,a,L'\t')||!std::getline(ss,b,L'\t')||!std::getline(ss,c,L'\t')||!std::getline(ss,d))continue;
  try{out.push_back({a,std::stod(b),std::stod(c),d});}catch(...){}
 }return out;
}
}
CpuBenchmarkResult RunCpuMicroBenchmark(const std::wstring&cpuName,const std::wstring&appDir,const std::atomic_bool*cancel){
 CpuBenchmarkResult r{};SYSTEM_INFO si{};GetSystemInfo(&si);unsigned n=std::max<DWORD>(1,si.dwNumberOfProcessors);
 std::atomic_bool stop{false};std::atomic<unsigned long long> ops{0};std::vector<std::thread> ts;
 for(unsigned i=0;i<n;i++)ts.emplace_back([&]{double x=1.001;unsigned long long local=0;while(!stop){for(int k=0;k<10000;k++){x=x*1.0000001+0.0000003;x=x/(1.00000001+x*0.0000000001);local+=3;}if(cancel&&cancel->load())break;}ops.fetch_add(local);});
 auto start=std::chrono::steady_clock::now();for(int i=0;i<50;i++){if(cancel&&cancel->load())break;Sleep(100);}stop=true;for(auto&t:ts)t.join();
 double sec=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();if(sec<=0)return r;r.score=(double)ops.load()/sec/1000000.0;
 for(auto&b:Load(appDir)){if(Lower(cpuName).find(Lower(b.cpuContains))!=std::wstring::npos){r.expectedLow=b.low;r.expectedHigh=b.high;r.baselineSource=b.source;r.confidence=Confidence::Medium;
   double mid=(b.low+b.high)/2.0;if(mid>0){r.percentOfBaseline=100.0*r.score/mid;r.verdict=r.score<b.low?L"BELOW BASELINE":r.score>b.high?L"ABOVE BASELINE":L"WITHIN BASELINE";}break;}}
 if(r.expectedLow<0){r.verdict=L"NOT SCORED";r.baselineSource=L"No validated local baseline for this CPU/benchmark version.";r.confidence=Confidence::Low;}
 return r;
}
}
