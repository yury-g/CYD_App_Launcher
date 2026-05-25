# Code Audits

External and internal review notes for the PulseSensor CyberDeck with CYD ESP-322432S028.

Audits are kept as historical snapshots. Do not silently rewrite an old audit
after code changes; add a follow-up response or a new audit instead.

## Index

| File | Date | Type | Summary |
| --- | --- | --- | --- |
| `2026-05-25_audit-01_initial-scrutiny.md` | 2026-05-25 | External audit | Initial review of the older public `main` state; strongest themes were identity mismatch, onboarding gaps, and small firmware reliability fixes. |
| `2026-05-25_audit-02_re-review.md` | 2026-05-25 | External re-review | Re-review of the same older public state; many claims became stale after the current source cleanup reached `main`. |
| `2026-05-25_codex-response.md` | 2026-05-25 | Current response | Current recommendation after comparing the audits to current `main`. |

## Convention

- Reference audit findings by their original number where useful, for example
  `Audit 01 C3` or `Audit 02 N2`.
- Treat audit language as evidence, not authority. Re-check against current
  source before acting.
- Prefer small follow-up commits: public story cleanup, firmware reliability,
  then optional classroom/onboarding polish.
