# S18 — Đánh giá cuối cùng & Báo cáo

`components: [C01,C02,C03,C04,C05,C06,C08,C10,C12]`  
`visual: ../references/approved/S18_FINAL_REPORT.jpg`

## User outcome
Explain the evidence-gated purchase recommendation and remaining uncertainty.

## Objects
Recommendation; mandatory coverage; confidence; critical failures; warnings; seller/factory mismatch; unchecked/unsupported required items; causal reasons; negotiation notes after technical truth.

## Data
`AuditDecision`, `CoverageDomain`, findings, seller/factory comparison.

## Invariant
Required incomplete coverage blocks clean BUY. Recommendation, coverage and confidence are separate. Do not implement generic “Điểm sức khỏe 88/100”. Optional overall coverage must not hide incomplete mandatory coverage.

## Acceptance
Every conclusion drills down to evidence; no price/cosmetic factor overrides safety/critical reject; user understands why the verdict exists.
