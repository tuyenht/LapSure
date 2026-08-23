# S13 — Âm thanh & Camera

## Purpose
Guide real functional verification of camera, microphone and stereo speakers. Device enumeration alone is insufficient.

## Subtests

### Camera
- device identity;
- start Media Foundation sample;
- show live/last sample preview only when a real frame was captured;
- automatic evidence: frame acquisition success;
- operator confirmation: image quality/obstruction if required.

PASS requires actual sample evidence, not PnP presence.

### Microphone
- actual PCM capture;
- live level/waveform derived from captured data;
- duration/peak/RMS or existing meaningful signal evidence;
- playback optional if safely implemented;
- operator confirmation for audible quality/noise.

### Speakers
- separate Left / Right stimulus;
- clear buttons: `Phát loa trái`, `Phát loa phải`;
- operator chooses `Nghe rõ`, `Rè/yếu`, `Không nghe`;
- record manual evidence/confidence.

## Screen layout
Top cards: Camera / Microphone / Loa stereo with independent statuses.
Main task panel focuses on one current subtest.
Right rail: progress, instructions, next action.

## States
`CHƯA KIỂM TRA`, `ĐANG KIỂM TRA`, `CẦN XÁC NHẬN`, `ĐẠT`, `CẦN LƯU Ý`, `KHÔNG ĐẠT`, `KHÔNG HỖ TRỢ`.

If OS/WinPE capability lacks Media Foundation or capture support, show unsupported—not hardware failure.

## Actions
- `Bắt đầu`
- `Thử lại`
- `Xác nhận kết quả`
- `Tiếp tục`
- `Bỏ qua` with clear final-coverage consequence.
