# S23 — Khôi phục phiên bị gián đoạn

## Purpose
Use the existing stress/session journal to prevent crash/reboot/power-loss from erasing or misrepresenting evidence.

## Trigger
On startup or session open, if `previousInterruptedSessionDetected` / journal evidence indicates an unfinished session, show a recovery surface before presenting a clean completed state.

## Required information
- device/session identity if recoverable
- prior mode
- start/last-update time if stored
- completed stages
- interrupted stage
- evidence already saved
- journal path
- reason may be `Không xác định` unless explicitly known

## Primary choices
1. **Khôi phục và tiếp tục**
   - restore safe session context;
   - do not pretend the interrupted stress stage passed;
   - rerun or continue only where existing journal semantics safely support it.
2. **Đóng phiên ở trạng thái chưa hoàn tất**
   - preserve evidence;
   - final verdict remains incomplete.
3. **Bỏ dữ liệu phiên**
   - destructive confirmation;
   - clearly state what journal/session evidence will be removed.

## Safety semantics
- Crash/reboot itself is evidence, not automatically proof of a particular component failure.
- An interrupted stage is never PASS.
- Existing pre-interruption evidence may remain usable if independently valid.
- New post-recovery evidence must be timestamped and distinguishable.

## UI
Use a focused recovery dialog/page with warning styling, not red “hardware failure” unless actual failure evidence exists.

## Acceptance
A prior interrupted journal can never lead to a clean BUY/PASS merely because the app restarted successfully.
