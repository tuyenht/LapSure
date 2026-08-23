# S13 — Âm thanh & Camera

`components: [C01,C02,C03,C04,C08,C10,C11,C12]`

## User outcome
Verify camera sample, microphone capture and stereo L/R through actual stimulus.

## Objects
Camera preview/sample status; mic capture/waveform; stereo left/right stimulus; operator quality confirmation; source/confidence.

## Data
`FunctionalItemResult`, Media Foundation evidence, WaveIn/PCM evidence, operator confirmation.

## Invariant
Camera/mic device presence is not PASS. Camera requires actual usable frame/sample evidence; mic requires actual capture evidence; stereo L/R requires stimulus + operator confirmation.

## Acceptance
No false PASS from enumeration; clear privacy/permission failure states; manual quality judgment labeled.
