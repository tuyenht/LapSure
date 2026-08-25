# Round 5.1 — Review Baseline

Logical Round 5.1 review baseline:
`df3ab209c4afba21ac42ed7bbbb2dfcb615419b6`

Run #592 runtime/security/package baseline:
`d7944104b90f6290f8f444de572f06a90d16a676`

Review policy:
- do not re-review all historical PR #2 commits for every Round 5.1 change;
- use bounded compare ranges from this logical baseline and per-tranche checkpoints;
- keep PR #2 Draft throughout Round 5.1;
- documentation-only commits do not invalidate Run #592 production binary evidence but do move the PR head;
- any production/build/test/workflow change requires new proof at the invalidated level and a new final package before physical acceptance.
