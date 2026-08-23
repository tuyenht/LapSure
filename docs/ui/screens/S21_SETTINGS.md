# S21 — Cài đặt

`components: [C01,C02,C03,C09,C10,C11]`  
`visual: ../references/approved/S21_SETTINGS.jpg`

## User outcome
Configure safe user-facing behavior without casually exposing dangerous engineering controls.

## Objects
Default mode; UI preferences; report path; evidence/provider readiness; privacy; gated advanced section.

## Data
Application settings and actual capabilities/provider state.

## Invariant
Settings cannot disable evidence gates in a way that still allows false PASS/BUY. Trust/hash policy remains enforced.

## Acceptance
Defaults safe; destructive/reset actions confirmed; unsupported provider shown as readiness status rather than hardware failure.
