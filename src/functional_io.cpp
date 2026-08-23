#include "lap/functional_io.h"
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <wlanapi.h>
#include <bluetoothapis.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

#pragma comment(lib,"mfplat.lib")
#pragma comment(lib,"mfreadwrite.lib")
#pragma comment(lib,"mfuuid.lib")
#pragma comment(lib,"wlanapi.lib")
#pragma comment(lib,"bthprops.lib")
#pragma comment(lib,"winmm.lib")
#pragma comment(lib,"ole32.lib")

namespace lap {
namespace {
FunctionalItemResult R(const wchar_t*id,const wchar_t*name,FunctionalStatus st,const std::wstring&detail,const std::wstring&evidence,Confidence c,bool automated=true){FunctionalItemResult r{};r.id=id;r.name=name;r.status=st;r.detail=detail;r.evidence=evidence;r.confidence=c;r.automated=automated;return r;}
template<class T> void Rel(T*&p){if(p){p->Release();p=nullptr;}}

FunctionalItemResult CameraFrameProbe(){
 HRESULT co=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);bool uninit=SUCCEEDED(co);HRESULT hr=MFStartup(MF_VERSION);
 if(FAILED(hr)){if(uninit)CoUninitialize();return R(L"camera_function",L"Camera functional capture",FunctionalStatus::NotTested,L"Media Foundation unavailable",L"MFStartup failed",Confidence::Low);}
 IMFAttributes* a=nullptr;IMFActivate** devs=nullptr;UINT32 count=0;hr=MFCreateAttributes(&a,1);if(SUCCEEDED(hr))hr=a->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);if(SUCCEEDED(hr))hr=MFEnumDeviceSources(a,&devs,&count);
 if(FAILED(hr)||count==0){Rel(a);MFShutdown();if(uninit)CoUninitialize();return R(L"camera_function",L"Camera functional capture",FunctionalStatus::Unsupported,L"No Media Foundation camera source",L"MFEnumDeviceSources returned no video source",Confidence::Medium);}
 IMFMediaSource* source=nullptr;IMFSourceReader* reader=nullptr;hr=devs[0]->ActivateObject(IID_PPV_ARGS(&source));if(SUCCEEDED(hr))hr=MFCreateSourceReaderFromMediaSource(source,nullptr,&reader);
 DWORD stream=0,flags=0;LONGLONG ts=0;IMFSample* sample=nullptr;bool got=false;
 if(SUCCEEDED(hr)){for(int i=0;i<30&&!got;i++){hr=reader->ReadSample(static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM),0,&stream,&flags,&ts,&sample);if(SUCCEEDED(hr)&&sample){DWORD n=0;if(SUCCEEDED(sample->GetBufferCount(&n))&&n>0)got=true;Rel(sample);}if(flags&MF_SOURCE_READERF_ENDOFSTREAM)break;}}
 Rel(reader);if(source)source->Shutdown();Rel(source);for(UINT32 i=0;i<count;i++)Rel(devs[i]);CoTaskMemFree(devs);Rel(a);MFShutdown();if(uninit)CoUninitialize();
 return R(L"camera_function",L"Camera functional capture",got?FunctionalStatus::Pass:FunctionalStatus::Fail,got?L"Camera produced a Media Foundation video sample":L"Camera enumerated but no video sample was captured",L"Native Media Foundation source activation + ReadSample",got?Confidence::High:Confidence::Medium);
}

FunctionalItemResult MicrophoneCaptureProbe(){
 WAVEFORMATEX fmt{};fmt.wFormatTag=WAVE_FORMAT_PCM;fmt.nChannels=1;fmt.nSamplesPerSec=16000;fmt.wBitsPerSample=16;fmt.nBlockAlign=2;fmt.nAvgBytesPerSec=32000;
 HWAVEIN h=nullptr;MMRESULT mm=waveInOpen(&h,WAVE_MAPPER,&fmt,0,0,CALLBACK_NULL);if(mm!=MMSYSERR_NOERROR)return R(L"mic_function",L"Microphone capture",FunctionalStatus::Unsupported,L"Cannot open default microphone",L"waveInOpen failed",Confidence::Medium);
 std::vector<short> data(16000*2);WAVEHDR wh{};wh.lpData=(LPSTR)data.data();wh.dwBufferLength=(DWORD)(data.size()*sizeof(short));
 bool ok=waveInPrepareHeader(h,&wh,sizeof(wh))==MMSYSERR_NOERROR&&waveInAddBuffer(h,&wh,sizeof(wh))==MMSYSERR_NOERROR&&waveInStart(h)==MMSYSERR_NOERROR;if(ok)Sleep(2000);waveInStop(h);waveInReset(h);waveInUnprepareHeader(h,&wh,sizeof(wh));waveInClose(h);
 long long sum=0;int peak=0;size_t n=wh.dwBytesRecorded/sizeof(short);for(size_t i=0;i<n;i++){int v=std::abs((int)data[i]);sum+=v;peak=std::max(peak,v);}double avg=n?double(sum)/n:0;bool signal=n>1000&&peak>150&&avg>8;
 return R(L"mic_function",L"Microphone capture",signal?FunctionalStatus::Pass:FunctionalStatus::Warning,L"Recorded "+std::to_wstring(n)+L" samples; peak="+std::to_wstring(peak)+L", avgAbs="+std::to_wstring((int)avg),L"Native waveIn 16 kHz mono PCM, 2-second capture; low level is WARNING rather than FAIL",signal?Confidence::High:Confidence::Medium);
}

bool PlayTone(bool left){
 WAVEFORMATEX f{};f.wFormatTag=WAVE_FORMAT_PCM;f.nChannels=2;f.nSamplesPerSec=44100;f.wBitsPerSample=16;f.nBlockAlign=4;f.nAvgBytesPerSec=176400;const int frames=44100/2;std::vector<short>d(frames*2);
 for(int i=0;i<frames;i++){short s=(short)(std::sin(2.0*3.141592653589793*660.0*i/44100.0)*9000);d[i*2]=left?s:0;d[i*2+1]=left?0:s;}
 HWAVEOUT h=nullptr;if(waveOutOpen(&h,WAVE_MAPPER,&f,0,0,CALLBACK_NULL)!=MMSYSERR_NOERROR)return false;WAVEHDR wh{};wh.lpData=(LPSTR)d.data();wh.dwBufferLength=(DWORD)(d.size()*sizeof(short));bool ok=waveOutPrepareHeader(h,&wh,sizeof(wh))==MMSYSERR_NOERROR&&waveOutWrite(h,&wh,sizeof(wh))==MMSYSERR_NOERROR;if(ok)while(!(wh.dwFlags&WHDR_DONE))Sleep(20);waveOutUnprepareHeader(h,&wh,sizeof(wh));waveOutClose(h);return ok;
}
FunctionalItemResult StereoProbe(HWND owner){
 if(!PlayTone(true))return R(L"speaker_function",L"Stereo speaker L/R",FunctionalStatus::Unsupported,L"Cannot open default playback device",L"waveOutOpen/write failed",Confidence::Medium,false);
 int l=MessageBoxW(owner,L"Bạn vừa nghe tín hiệu CHỈ KÊNH TRÁI?\nYES = đúng / NO = sai hoặc không nghe thấy",L"Speaker LEFT",MB_YESNO|MB_ICONQUESTION);
 if(!PlayTone(false))return R(L"speaker_function",L"Stereo speaker L/R",FunctionalStatus::Fail,L"Right-channel playback failed",L"Left played; right waveOut failed",Confidence::Medium,false);
 int rr=MessageBoxW(owner,L"Bạn vừa nghe tín hiệu CHỈ KÊNH PHẢI?\nYES = đúng / NO = sai hoặc không nghe thấy",L"Speaker RIGHT",MB_YESNO|MB_ICONQUESTION);bool ok=l==IDYES&&rr==IDYES;
 return R(L"speaker_function",L"Stereo speaker L/R",ok?FunctionalStatus::Pass:FunctionalStatus::Fail,ok?L"User confirmed correct isolated left/right output":L"Left/right channel confirmation failed",L"Generated 660 Hz stereo PCM: isolated left then isolated right",Confidence::High,false);
}

FunctionalItemResult WifiProbe(){
 HANDLE h=nullptr;DWORD negotiated=0;DWORD e=WlanOpenHandle(2,nullptr,&negotiated,&h);if(e!=ERROR_SUCCESS)return R(L"wifi_function",L"Wi-Fi functional state",FunctionalStatus::Unsupported,L"WLAN API unavailable",L"WlanOpenHandle failed",Confidence::Medium);
 PWLAN_INTERFACE_INFO_LIST list=nullptr;e=WlanEnumInterfaces(h,nullptr,&list);bool any=false,connected=false;LONG quality=-1;
 if(e==ERROR_SUCCESS&&list){any=list->dwNumberOfItems>0;for(DWORD i=0;i<list->dwNumberOfItems;i++){auto&it=list->InterfaceInfo[i];if(it.isState==wlan_interface_state_connected){connected=true;DWORD sz=0;WLAN_OPCODE_VALUE_TYPE op{};PWLAN_CONNECTION_ATTRIBUTES ca=nullptr;if(WlanQueryInterface(h,&it.InterfaceGuid,wlan_intf_opcode_current_connection,nullptr,&sz,(PVOID*)&ca,&op)==ERROR_SUCCESS&&ca){quality=(LONG)ca->wlanAssociationAttributes.wlanSignalQuality;WlanFreeMemory(ca);}break;}}}
 if(list)WlanFreeMemory(list);WlanCloseHandle(h,nullptr);if(!any)return R(L"wifi_function",L"Wi-Fi functional state",FunctionalStatus::Unsupported,L"No WLAN interface",L"Native WLAN API enumeration",Confidence::High);if(!connected)return R(L"wifi_function",L"Wi-Fi functional state",FunctionalStatus::Warning,L"Wi-Fi adapter exists but is not connected",L"WLAN interface enumeration succeeded; no association",Confidence::High);return R(L"wifi_function",L"Wi-Fi functional state",FunctionalStatus::Pass,L"Associated; signal quality="+std::to_wstring(quality)+L"%",L"WlanEnumInterfaces + current_connection query",Confidence::High);
}

FunctionalItemResult BluetoothProbe(){
 BLUETOOTH_FIND_RADIO_PARAMS p{};p.dwSize=sizeof(p);HANDLE radio=nullptr;HBLUETOOTH_RADIO_FIND fr=BluetoothFindFirstRadio(&p,&radio);if(!fr)return R(L"bluetooth_function",L"Bluetooth functional state",FunctionalStatus::Unsupported,L"No Bluetooth radio enumerated",L"BluetoothFindFirstRadio",Confidence::High);
 BLUETOOTH_RADIO_INFO ri{};ri.dwSize=sizeof(ri);DWORD er=BluetoothGetRadioInfo(radio,&ri);BLUETOOTH_DEVICE_SEARCH_PARAMS sp{};sp.dwSize=sizeof(sp);sp.fReturnAuthenticated=TRUE;sp.fReturnRemembered=TRUE;sp.fReturnUnknown=TRUE;sp.fReturnConnected=TRUE;sp.fIssueInquiry=FALSE;sp.hRadio=radio;BLUETOOTH_DEVICE_INFO di{};di.dwSize=sizeof(di);HBLUETOOTH_DEVICE_FIND fd=BluetoothFindFirstDevice(&sp,&di);bool device=fd!=nullptr;if(fd)BluetoothFindDeviceClose(fd);CloseHandle(radio);BluetoothFindRadioClose(fr);
 return R(L"bluetooth_function",L"Bluetooth functional state",er==ERROR_SUCCESS?FunctionalStatus::Pass:FunctionalStatus::Warning,er==ERROR_SUCCESS?(device?L"Radio accessible; known/visible device enumerated":L"Radio accessible; no device currently enumerated"):L"Radio handle exists but radio info query failed",L"Native Bluetooth radio API; this proves stack/radio access, not RF throughput or pairing loopback",er==ERROR_SUCCESS?Confidence::High:Confidence::Medium);
}
}
std::vector<FunctionalItemResult> RunFunctionalIoWizard(HWND owner){
 std::vector<FunctionalItemResult> out;MessageBoxW(owner,L"Functional I/O sẽ kiểm tra camera, microphone, loa stereo, Wi-Fi và Bluetooth.\n\nHãy đóng ứng dụng đang dùng camera/microphone trước khi tiếp tục.",L"Functional I/O",MB_OK|MB_ICONINFORMATION);out.push_back(CameraFrameProbe());MessageBoxW(owner,L"Hãy nói bình thường vào microphone trong khoảng 2 giây sau khi bấm OK.",L"Microphone test",MB_OK|MB_ICONINFORMATION);out.push_back(MicrophoneCaptureProbe());out.push_back(StereoProbe(owner));out.push_back(WifiProbe());out.push_back(BluetoothProbe());return out;
}
}
