from pathlib import Path

path = Path("src/functional_io.cpp")
text = path.read_text(encoding="utf-8")

old = r'''FunctionalItemResult BluetoothProbe(){
 BLUETOOTH_FIND_RADIO_PARAMS p{};p.dwSize=sizeof(p);HANDLE radio=nullptr;HBLUETOOTH_RADIO_FIND fr=BluetoothFindFirstRadio(&p,&radio);if(!fr)return R(L"bluetooth_function",L"Bluetooth functional state",FunctionalStatus::Unsupported,L"No Bluetooth radio enumerated",L"BluetoothFindFirstRadio",Confidence::High);
 BLUETOOTH_RADIO_INFO ri{};ri.dwSize=sizeof(ri);DWORD er=BluetoothGetRadioInfo(radio,&ri);BLUETOOTH_DEVICE_SEARCH_PARAMS sp{};sp.dwSize=sizeof(sp);sp.fReturnAuthenticated=TRUE;sp.fReturnRemembered=TRUE;sp.fReturnUnknown=TRUE;sp.fReturnConnected=TRUE;sp.fIssueInquiry=FALSE;sp.hRadio=radio;BLUETOOTH_DEVICE_INFO di{};di.dwSize=sizeof(di);HBLUETOOTH_DEVICE_FIND fd=BluetoothFindFirstDevice(&sp,&di);bool device=fd!=nullptr;if(fd)BluetoothFindDeviceClose(fd);CloseHandle(radio);BluetoothFindRadioClose(fr);
 return R(L"bluetooth_function",L"Bluetooth functional state",er==ERROR_SUCCESS?FunctionalStatus::Pass:FunctionalStatus::Warning,er==ERROR_SUCCESS?(device?L"Radio accessible; known/visible device enumerated":L"Radio accessible; no device currently enumerated"):L"Radio handle exists but radio info query failed",L"Native Bluetooth radio API; this proves stack/radio access, not RF throughput or pairing loopback",er==ERROR_SUCCESS?Confidence::High:Confidence::Medium);
}
'''

new = r'''FunctionalItemResult BluetoothProbe(HWND owner){
 BLUETOOTH_FIND_RADIO_PARAMS p{};p.dwSize=sizeof(p);HANDLE radio=nullptr;HBLUETOOTH_RADIO_FIND fr=BluetoothFindFirstRadio(&p,&radio);if(!fr)return R(L"bluetooth_function",L"Bluetooth functional state",FunctionalStatus::Unsupported,L"No Bluetooth radio enumerated",L"BluetoothFindFirstRadio returned no radio",Confidence::High,false);
 BLUETOOTH_RADIO_INFO ri{};ri.dwSize=sizeof(ri);DWORD er=BluetoothGetRadioInfo(radio,&ri);BLUETOOTH_DEVICE_SEARCH_PARAMS sp{};sp.dwSize=sizeof(sp);sp.fReturnAuthenticated=TRUE;sp.fReturnRemembered=TRUE;sp.fReturnUnknown=TRUE;sp.fReturnConnected=TRUE;sp.fIssueInquiry=FALSE;sp.hRadio=radio;BLUETOOTH_DEVICE_INFO di{};di.dwSize=sizeof(di);HBLUETOOTH_DEVICE_FIND fd=BluetoothFindFirstDevice(&sp,&di);bool device=fd!=nullptr;if(fd)BluetoothFindDeviceClose(fd);CloseHandle(radio);BluetoothFindRadioClose(fr);
 if(er!=ERROR_SUCCESS)return R(L"bluetooth_function",L"Bluetooth functional state",FunctionalStatus::Warning,L"Bluetooth radio handle exists but radio information could not be queried",L"BluetoothFindFirstRadio succeeded; BluetoothGetRadioInfo failed",Confidence::Medium,false);
 std::wstring prompt=L"LapSure đã truy cập được Bluetooth radio. Đây mới là bằng chứng presence/stack, CHƯA đủ để PASS chức năng.\n\nHãy dùng một thiết bị Bluetooth đã biết hoạt động (chuột, bàn phím, tai nghe hoặc điện thoại), ghép nối/kết nối và thực hiện một tương tác thực tế.\n\nYES = tương tác thành công\nNO = đã thử với thiết bị known-good nhưng không hoạt động\nCANCEL = chưa thực hiện / chưa có thiết bị mẫu";
 int answer=MessageBoxW(owner,prompt.c_str(),L"Bluetooth functional verification",MB_YESNOCANCEL|MB_ICONQUESTION);
 std::wstring radioEvidence=L"Native Bluetooth radio API accessible"+(device?L"; known/visible device enumerated":L"; no remembered/visible device enumerated");
 if(answer==IDYES)return R(L"bluetooth_function",L"Bluetooth functional state",FunctionalStatus::Pass,L"Operator confirmed successful interaction with a known-good Bluetooth device",radioEvidence+L"; operator-confirmed pairing/interaction succeeded",Confidence::High,false);
 if(answer==IDNO)return R(L"bluetooth_function",L"Bluetooth functional state",FunctionalStatus::Fail,L"Operator attempted a known-good Bluetooth device but interaction failed",radioEvidence+L"; operator-confirmed functional attempt failed",Confidence::High,false);
 return R(L"bluetooth_function",L"Bluetooth functional state",FunctionalStatus::ManualRequired,L"Bluetooth radio is accessible, but a known-good device interaction has not been completed",radioEvidence+L"; RF/pairing/functionality remains unverified",Confidence::Medium,false);
}
'''

if text.count(old) != 1:
    raise SystemExit(f"BluetoothProbe old implementation count={text.count(old)}")
text = text.replace(old, new, 1)
old_call = "out.push_back(BluetoothProbe());"
new_call = "out.push_back(BluetoothProbe(owner));"
if text.count(old_call) != 1:
    raise SystemExit(f"BluetoothProbe call count={text.count(old_call)}")
text = text.replace(old_call, new_call, 1)
path.write_text(text, encoding="utf-8")
print("Applied Bluetooth known-good interaction evidence gate")
