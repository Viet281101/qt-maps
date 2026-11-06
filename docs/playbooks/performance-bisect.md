# Performance bisect playbook

Use the owner-gated helper in `tools/i915-bisect-run.sh`. Start with its
default `--dry-run`; only `--run` with `generated/system/OWNER_APPROVED_GPU_RUN`
may launch a GPU workload. Compare clean worktrees, one commit pair at a time,
with one unchanged environment and a retained log for every result.
