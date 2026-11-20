# 3DS Chess — project workflow

These instructions apply to all future work in this repository.

## Local Git and the private remote

- The authorized remote is the private `origin`, `Perrodevs/3ds-chess`. Keep
  all other work local. Push only when the owner authorizes the scope.
- Before pushing, verify the target and its visibility, compare its branch tip,
  review the commits and assets being transferred, and verify the remote hash
  afterward. Use explicit branch refspecs, without force or automatic tags.
- Use independent worktrees for concurrent work. Send gameplay changes through
  a reviewed pull request and keep `main` for accepted work. Updating its
  presentation does not approve merging a gameplay candidate.
- Do not change visibility, enable Pages, publish releases or distribute
  packages without authorization.
- Respect the owner's existing changes. Inspect `git status` and the relevant diff
  before editing or staging; stage only files or hunks belonging to the task.
- Do not delete branches, rewrite history, amend existing commits or use destructive
  recovery commands such as `git reset --hard` or `git clean -fd` without explicit
  authorization. Preserve ignored private resources, saves and recovery copies.
- Preserve the initial commit `6edbf58`. It is a snapshot of 1.1.0 RC Legacy, not
  a reconstructed history of earlier releases. Do not rename it retroactively.

## Conventional Commits

Use `type(scope): short description`, in English, with a clear, specific verb.
Allowed types: `feat`, `fix`, `perf`, `refactor`, `test`, `docs`, `style`, `build`,
`chore`. Choose a scope matching the affected system, such as `ai`, `save`,
`profiles`, `engine`, `localization`, `characters` or `workflow`.

Examples:

- `feat(ai): add configurable opponent personalities`
- `fix(save): prevent duplicate tournament rewards`
- `perf(engine): reduce repeated position evaluation`
- `test(profiles): verify save isolation`

Do not use generic descriptions such as `update`, `changes`, `fix stuff` or `final`.
Create one commit for a coherent completed improvement and its relevant tests.
Keep unrelated improvements separate; do not create a commit for every tiny edit.

Before each commit:

1. Review modified files and the complete staged diff.
2. Run the tests relevant to the change. Documentation-only changes need document
   and diff checks, not an unnecessary full game rebuild.
3. Check that saves, credentials, private resources, generated binaries, caches and
   excluded packages are absent from the index. Never bypass ignores casually.
4. Verify that the message accurately describes the implementation and that only
   the current task is staged.
5. Report failed or unavailable checks. Never present a failed test as validation.

## Branches

Use descriptive names based on the actual task, for example `feature/chess-variants`,
`feature/bilingual-support`, `fix/save-corruption` or `perf/chess-engine`.
Do not invent feature work from examples in these instructions.

Reuse an appropriate current branch for small safe tasks. Do not create branches
unnecessarily. Keep `main` for accepted work; make large updates on a suitable
working branch rather than directly on `main`. Merge only reviewed/accepted work.
The pre-existing `feature/next-update` branch is retained, not deleted or rewritten;
prefer a task-specific name for future substantial work instead of that generic name.

## Versions and changelog

- Maintain the root `CHANGELOG.md` as part of each coherent improvement. Record
  actual additions, fixes, optimizations, compatibility changes, migrations and
  material limitations. Keep unreleased work under `Unreleased` until identified
  and verified as a release. Documentation-only tasks can use a documentation entry.
- Use annotated Git tags for clearly identified releases only after checking that
  the commit, canonical version and delivered artifacts match. Use names such as
  `v1.1.0-rc.1` for candidates and `v1.1.0` for accepted stable versions.
- Do not invent historical tags, create tags merely because a version string exists,
  or label a version stable while physical validation is still pending.
- Do not perform physical 3DS tests. Distinguish host checks, ARM builds, emulator
  observations and pending hardware tests. Never invent physical results.

## Final task report

State what changed, checks actually run and their outcome, branch used, short commit
hash, exact commit message and remaining issues. If no commit was made, say why.
For failures, identify the failed check and do not claim the commit is validated.

See `docs/GIT_LOCAL.md` for routine commands and non-destructive recovery.
