from pathlib import Path
from app_source_view import read_app_source

R=Path(__file__).resolve().parents[1]
io=(R/"src/functional_io.cpp").read_text(encoding="utf-8")
main=read_app_source(R)
fn=(R/"src/functional.cpp").read_text(encoding="utf-8")
cm=(R/"CMakeLists.txt").read_text(encoding="utf-8")
checks=[
("Camera Media Foundation enumeration","MFEnumDeviceSources" in io),
("Camera actual sample required","ReadSample" in io and "GetBufferCount" in io),
("Microphone native waveIn","waveInOpen" in io and "waveInStart" in io),
("Microphone records level evidence","peak=" in io and "avgAbs=" in io),
("Low mic signal not hard FAIL","FunctionalStatus::Warning" in io[io.find("MicrophoneCaptureProbe"):io.find("bool PlayTone")]),
("Stereo generated PCM","PlayTone(true)" in io and "PlayTone(false)" in io),
("Stereo requires human L/R confirmation","Speaker LEFT" in io and "Speaker RIGHT" in io),
("WiFi native WLAN API","WlanOpenHandle" in io and "WlanQueryInterface" in io),
("WiFi signal quality captured","wlanSignalQuality" in io),
("Bluetooth native radio API","BluetoothFindFirstRadio" in io and "BluetoothGetRadioInfo" in io),
("Bluetooth radio presence is not PASS","CHƯA đủ để PASS chức năng" in io and "FunctionalStatus::ManualRequired" in io),
("Bluetooth known-good interaction required","known-good Bluetooth device" in io and "operator-confirmed pairing/interaction succeeded" in io),
("Focused audio-camera wizard","RunAudioCameraWizard" in io),
("Focused network wizard","RunNetworkConnectivityWizard" in io),
("Batch results committed","CommitManualResults" in main and "RunFunctionalIoWizard" in main),
("I/O button disabled during audit","gFuncIo" in main and "SetFunctionalButtonsEnabled" in main),
("WiFi/Bluetooth start MANUAL","wifi_function" in fn and "bluetooth_function" in fn),
("Functional I/O compiled","src/functional_io.cpp" in cm),
("Required Windows libs linked","mfplat" in cm and "wlanapi" in cm and "bthprops" in cm),
]
bad=[]
for n,ok in checks:
 print(("PASS" if ok else "FAIL"),n)
 if not ok: bad.append(n)
print(f"{len(checks)-len(bad)}/{len(checks)} PASS")
raise SystemExit(bool(bad))
