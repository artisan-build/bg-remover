# Workflow - bg-remover

Check-only profile. It records how changes to this repository are verified; it does not
grant authority to merge, release or alter repository settings.

## Project

- C++ command-line tool (`src/bg-remover.cpp`) built with the `Makefile` and per-platform
  Dockerfiles (`Dockerfile.ubuntu`, `Dockerfile.ubuntu-arm64`); macOS builds natively.
- There is no unit-test suite in the repository.

## CI checks

Changes are checked by the pull request's own CI. There is no separate local full suite.

- Workflow: `.github/workflows/build.yml` ("Build Multi-Platform Binaries").
- Triggers: `pull_request` targeting `main`, `v*` tag pushes, and `workflow_dispatch`.
- Build checks (matrix-generated names): `Build ubuntu`, `Build linux-arm64`,
  `Build macos`. Each builds the platform binary, lists and inspects it, and generates a
  SHA256 checksum.
- The expected result is all three build checks passing for the pull request.

## Release

- The `release` job runs only for tags or manual dispatch and publishes the GitHub release.
- Release, tag and dispatch operations are not part of a workflow-change pull request.

## Runner selection

- `runs-on` for the `build` job defaults to the `runner` value in each matrix entry.
- For same-repository pull requests only, the `ubuntu` matrix entry may be redirected by
  the repository variable `CI_RUNNER`. Fork pull requests, tag/manual runs, `linux-arm64`
  and `macos` always use their matrix runner.

## Local checks for workflow edits

- Parse changed YAML (`python -c "import yaml,sys;[yaml.safe_load(open(f)) for f in sys.argv[1:]]" <files>`).
- Run `actionlint` on changed workflow files.
- Compare against `origin/main` and confirm only the intended lines changed.
- Do not run Docker builds locally for workflow-only changes; CI covers them.

## Contribution notes

- Keep matrix keys, job ids and check names stable, since they identify the CI checks.
