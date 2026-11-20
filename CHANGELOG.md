# Changelog

This log records actual project changes from the adoption of the local Git workflow.
Earlier implementation reports remain in `docs/`; they are not retroactive Git
releases. No historical commits or version tags are inferred from those reports.

## Unreleased

- Redesign the project readme in English and Spanish, with original cover art and reviewed captures from the 1.4.0 RC2 host renderer.
- Document build requirements, architecture, testing, contribution and security reporting for the 1.1.0 RC Legacy default branch.
- Add issue forms, a pull request template and minimal host checks. Document the private remote, repository metadata and asset rights.

### Documentation

- Define persistent project rules for Conventional Commits, descriptive branches,
  relevant checks, release tags and task reports in `AGENTS.md`.
- Update the local Git guide to use those conventions and maintain this changelog
  with each coherent improvement.

### Compatibility and limitations

- No game code, save format, assets, dependencies or canonical version changed.
- The current game remains `1.1.0 RC Legacy`, pending physical validation.
- The original workflow was local. The repository now uses an authorized private remote. See the GitHub settings guide.

## Existing baseline

The first Git commit, `6edbf58`, captured the project as it existed when local Git
was initialized. Its original message is preserved. It is not a claim that earlier
versions had Git history, nor a declaration of physical stability.
