<!--- Copyright (C) 2023 - 2026 Advanced Micro Devices, Inc. --->
<!--- SPDX-License-Identifier: Apache-2.0 --->

# Contributing to AI Engine Graph API - Baremetal

## How to Submit a Patch

### A. Create a new branch

Forks are not enabled in Github EMU.

Instead, create a new branch named after your NTID:

```bash
$ git checkout -b <your-ntid>
```

### B. Make your change in your branch

Implement and commit your change on your own branch.

```bash
$ git add <files>
$ git commit -m "<description of the change>"
$ git push -u origin <your-ntid>
```

### C. Create a pull request

Open a PR from your branch into the target branch.

### D. Pass tests and get approval

The PR can move forward only after:

- All CI tests pass, and
- At least one reviewer approves the change.

### E. Trigger the merge process

After tests pass and a reviewer approves:

1. Wait for the notification email to be sent out.
2. Go to Code Collab and move the CI process forward for the merge.

### F. Urgent or low-risk patches

Some urgent or low-risk test patches may bypass step C and
step E.

## Automated Test Setup

- Normal PRs run 4 tests: `Testkernel CI`, `Testaie CI`, `QEMU Testkernel RTP`, and `QEMU Testkernel RTP Sync`.
- Large or complex PRs can run a larger test suite (see [`.github/test-suite.txt`](.github/test-suite.txt)). Add the `run-full-suite` label to the PR:
  - GitHub UI: PR sidebar → Labels → select `run-full-suite`, or
  - CLI: `gh pr edit <PR-number> --add-label run-full-suite`
- The full test suite also runs automatically every night at 12am (Pacific), but only if commits were pushed to `master` that day.
- New tests can be added to the test suite by adding a line to the file [`.github/test-suite.txt`](.github/test-suite.txt).

