# Safe benchmark playbook

1. Confirm owner approval, stable i915 configuration, and no inherited software
   GL/platform variables. Record kernel, Mesa, DPR, CPU frequency, temperature.
2. Use `systemd-inhibit` only after approval; warm up after `window_exposed`.
3. Run one fixed configuration at a time with cooldown; follow i915 logs.
4. Stop immediately on `GPU HANG`, retain all raw output, and do not mix runs.
