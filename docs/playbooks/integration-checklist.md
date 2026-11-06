# Integration checklist

1. Rebase the next local Unit 07 branch onto current `main`.
2. Resolve only listed boundaries; run `make QT_EXPECTED_VERSION=5.15.18 test`.
3. Check generated artifacts are untracked and `git diff --check` is clean.
4. Create a linked PR only with owner approval; set milestone, labels, Project
   #6 fields, and squash merge policy. Rebase the next branch after squash.
