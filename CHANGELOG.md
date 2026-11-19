# Changelog

This log records actual project changes from the adoption of the local Git workflow.
Earlier implementation reports remain in `docs/`; they are not retroactive Git
releases. No historical commits or version tags are inferred from those reports.

## Unreleased

### Added

- Add optional fictional Elon/Byte and Elon/Jensen dialogue to actual AI-AI
  tournament pairings, without new entrants, popups or result/reward changes.
- Match the requested bilingual puzzle greeting and technological guest category.

## 1.2.0 RC1 — Space & Language (physical validation pending)

### Build and compatibility

- Preserve the selected build language during target verification, preventing an
  English package from being silently rebuilt with Spanish defaults.
- Generate ES/EN alternative SD packages from one source, with matching SMDH,
  source ZIP and SHA-256. Explicit language choices override package defaults.
- Preserve previous saves on load; archive writes now use v2. Older executables
  require their corresponding backup when downgrading. No hardware validation.

### Added — profile UI integration

- Add first-run language selection and independent per-profile language settings;
  update the interface without changing notation or game state.
- Integrate secret-opponent selection, one-shot unlock banner and persisted result
  scenes; keep empty selector slots inert.
- Preserve literal user names and FEN; translate before wrapping and centering.
- Retry failed secret-progress writes before another game, without duplicate payouts.

### Added — secret rival modules

- Register Elon Musk at stable ID40 with nine fictional dialogue events, an
  original code-drawn portrait with four expressions and an original music theme.
- Add verified Byte/Jensen unlock progress and persistent cinematic alternation
  in a separate profile sidecar; reject practice, duplicates and early abandonments.
- Add lightweight Roadster, rocket and failed-launch result scenes with skip.
- Expand opponent statistics to41 entries and read legacy archive version1;
  new archive writes use version2. Existing opponent IDs remain unchanged.

### Added — localization foundation

- Add central ES/EN translation catalogs, bounded composition lookup, numeric
  formatting and UTF-8 Spanish glyph mapping.
- Add catalog coverage, uniqueness, format and text-capacity checks; preserve
  rule notation and machine-readable formats independently of language.


### Earlier workflow documentation (1.1.0 snapshot)

- Define persistent project rules for Conventional Commits, descriptive branches,
  relevant checks, release tags and task reports in `AGENTS.md`.
- Update the local Git guide to use those conventions and maintain this changelog
  with each coherent improvement.

### Earlier workflow scope (before this update)

- That workflow-only change did not alter game code, saves, assets or version.
- At workflow adoption, the game was `1.1.0 RC Legacy`, pending physical validation.
- Git was kept local; no remotes, publication or release tags were added then.

## Existing baseline

The first Git commit, `6edbf58`, captured the project as it existed when local Git
was initialized. Its original message is preserved. It is not a claim that earlier
versions had Git history, nor a declaration of physical stability.
