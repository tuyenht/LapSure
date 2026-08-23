#include "lap/process.h"
#include <windows.h>
#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

namespace lap {
namespace {
std::wstring DecodeBytes(const std::string& bytes) {
    if (bytes.empty()) return L"";
    auto decode=[&](UINT cp, DWORD flags)->std::wstring {
        int n=MultiByteToWideChar(cp,flags,bytes.data(),static_cast<int>(bytes.size()),nullptr,0);
        if(n<=0) return L"";
        std::wstring w(static_cast<size_t>(n),L'\0');
        if(MultiByteToWideChar(cp,flags,bytes.data(),static_cast<int>(bytes.size()),w.data(),n)<=0) return L"";
        return w;
    };
    auto utf8=decode(CP_UTF8,MB_ERR_INVALID_CHARS);
    return utf8.empty()?decode(CP_ACP,0):utf8;
}

void DrainPipe(HANDLE pipe, std::string& dst) {
    constexpr size_t kMaxCapture=16u*1024u*1024u;
    char buffer[8192]; DWORD got=0;
    while(ReadFile(pipe,buffer,sizeof(buffer),&got,nullptr) && got>0) {
        if(dst.size()<kMaxCapture) {
            const size_t room=kMaxCapture-dst.size();
            dst.append(buffer,buffer+(got<room?got:room));
        }
    }
}
}

ProcessResult RunProcessCapture(const std::wstring& commandLine, unsigned timeoutMs, const std::atomic_bool* cancel) {
    ProcessResult r{};
    const auto started=std::chrono::steady_clock::now();
    SECURITY_ATTRIBUTES sa{sizeof(sa),nullptr,TRUE};
    HANDLE readPipe=nullptr,writePipe=nullptr;
    if(!CreatePipe(&readPipe,&writePipe,&sa,0)) { r.error=L"CreatePipe failed"; return r; }
    if(!SetHandleInformation(readPipe,HANDLE_FLAG_INHERIT,0)){CloseHandle(writePipe);CloseHandle(readPipe);r.error=L"SetHandleInformation failed: "+std::to_wstring(GetLastError());return r;}

    STARTUPINFOW si{}; si.cb=sizeof(si); si.dwFlags=STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;
    si.hStdOutput=writePipe; si.hStdError=writePipe; si.hStdInput=GetStdHandle(STD_INPUT_HANDLE); si.wShowWindow=SW_HIDE;
    PROCESS_INFORMATION pi{};
    std::vector<wchar_t> cmd(commandLine.begin(),commandLine.end()); cmd.push_back(L'\0');

    HANDLE job=CreateJobObjectW(nullptr,nullptr);
    if(job) {
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION ji{};
        ji.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        if(!SetInformationJobObject(job,JobObjectExtendedLimitInformation,&ji,sizeof(ji))){CloseHandle(job);job=nullptr;}
    }

    BOOL ok=CreateProcessW(nullptr,cmd.data(),nullptr,nullptr,TRUE,CREATE_NO_WINDOW|CREATE_SUSPENDED,nullptr,nullptr,&si,&pi);
    CloseHandle(writePipe); writePipe=nullptr;
    if(!ok) {
        if(job) CloseHandle(job);
        CloseHandle(readPipe);
        r.error=L"CreateProcess failed: "+std::to_wstring(GetLastError());
        return r;
    }
    r.launched=true;
    bool assignedJob=false;
    if(job) assignedJob=AssignProcessToJobObject(job,pi.hProcess)!=FALSE;
    if(ResumeThread(pi.hThread)==static_cast<DWORD>(-1)){r.error=L"ResumeThread failed: "+std::to_wstring(GetLastError());TerminateProcess(pi.hProcess,0xDEAD);}

    std::string bytes;
    std::thread reader([&]{DrainPipe(readPipe,bytes);});

    DWORD wait=WAIT_TIMEOUT;
    for(;;) {
        wait=WaitForSingleObject(pi.hProcess,100);
        if(wait==WAIT_OBJECT_0) break;
        if(wait==WAIT_FAILED){r.error=L"WaitForSingleObject failed: "+std::to_wstring(GetLastError());r.timedOut=true;break;}
        if(cancel && cancel->load()) { r.cancelled=true; break; }
        auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-started).count();
        if(timeoutMs && elapsed>=timeoutMs) { r.timedOut=true; break; }
    }

    if(r.cancelled || r.timedOut) {
        if(job && assignedJob) TerminateJobObject(job,0xDEAD);
        else TerminateProcess(pi.hProcess,0xDEAD);
        WaitForSingleObject(pi.hProcess,3000);
    }
    DWORD code=0; if(GetExitCodeProcess(pi.hProcess,&code)) r.exitCode=code;

    CloseHandle(pi.hThread); CloseHandle(pi.hProcess);
    if(job) CloseHandle(job); // If assigned, KILL_ON_JOB_CLOSE also closes lingering descendants holding pipe handles.
    if(reader.joinable()) {
        if(WaitForSingleObject(static_cast<HANDLE>(reader.native_handle()),1000)==WAIT_TIMEOUT)CancelSynchronousIo(static_cast<HANDLE>(reader.native_handle()));
        reader.join();
    }
    CloseHandle(readPipe);

    r.output=DecodeBytes(bytes);
    r.elapsedMs=static_cast<unsigned long>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-started).count());
    return r;
}
}
