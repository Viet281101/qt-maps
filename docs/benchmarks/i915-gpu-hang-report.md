# i915 GPU Hang Investigation — 2026-11-06

## Status

**Blocked pending a local privileged terminal.** This report records evidence and
the reversible next experiment; it does not claim that the GPU hang is fixed.
No package, kernel, GRUB setting, or QtMaps rendering behaviour was changed.

## Evidence collected

| Item | Observation | Evidence |
| --- | --- | --- |
| Hardware | Dell Latitude 5480; Intel HD 520 (Skylake, PCI `00:02.0`); i5-6300U; 16 GiB RAM | `generated/system/{uname-before,dpkg-gpu-kernel-before,vga-driver-before}.txt` |
| Running platform | Ubuntu 26.04.1; kernel `7.0.0-43-generic`; Qt 5.15.18; Mesa 26.0.8 | same snapshots |
| i915 failure | `qtmaps` hit two `rcs0` pre-emption timeouts at 09:29:42 and 09:34:29, followed by context resets and GPU HANG ecode `9:1:84df7efc` | `generated/system/i915-prior-boots.log` |
| Heat / memory | Earlier matrix telemetry was 39–51 C; no OOM or kernel panic was found in inspected boots | retained matrix logs and kernel log |
| Current mitigations | `i915.enable_psr=0 i915.enable_dc=0 i915.enable_fbc=0` were already present | `generated/system/{cmdline-before,i915-parameter-prechange}.txt` |
| Available kernels | Bootable images: 7.0.0-42 and 7.0.0-43; no bootable 6.x image remains | `generated/system/kernel-grub-upgrades.txt` |
| Available updates | No Mesa or Linux package was upgradable at inspection time | `generated/system/kernel-grub-upgrades.txt` |

The driver saved error state at `/sys/class/drm/card1/error`; it and debugfs
were not readable without elevation. The preservation snapshot is retained in
`generated/system/`, including the A+hooks binary at commit `937f68a` and its
SHA-256 (`a-hooks-937f68a.qtmaps.sha256`), rather than in `/tmp`.

## Benchmark validity

The interrupted backend matrix at
`generated/benchmarks/20261106-backend-matrix/` must not be used as a hardware
comparison. Two early runs are invalid (Qt runtime mismatch; fixture-only
routing). Later runs were interrupted by hard reboots, including an xcb
navigation/offline run; all raw runs are retained and none were deleted.

| Probe | Result | Interpretation |
| --- | --- | --- |
| HEAD, xcb hardware, 5 s warm-up + 15 s | 56.20 FPS average; 21.05 FPS 1% low; no new i915 line during this short probe | Insufficient to establish stability |
| A+hooks, xcb hardware, 5 s + 15 s | about 1 FPS, 15 swaps | Invalid as a stability comparison: `937f68a` intentionally limits the instrumentation to liveness tracing and this run's frame cadence is anomalous |
| HEAD, xcb, `LIBGL_ALWAYS_SOFTWARE=1`, 5 s + 60 s | 13.76 FPS average; 6.90 FPS 1% low; no GPU hang | A diagnostic fallback only, not comparable with GPU results |

The benchmark shell and user systemd environment were checked for inherited
`LIBGL_ALWAYS_SOFTWARE`, `MESA_*`, `QT_QPA_PLATFORM`, `QSG_RENDER_LOOP`, and
`QTMAPS_*`; none were present before the hardware probes.

## Next reversible experiment

The owner approved trying **only** `i915.enable_guc=0`, conditional on this
privileged preflight:

```bash
cat /sys/module/i915/parameters/enable_guc
sudo dmesg | grep -iE 'guc|huc'
```

If `enable_guc` is already `0`, stop: do not edit GRUB. Otherwise, after a
local user enters the sudo password in a real terminal, make a dated backup
and append just the one parameter:

```bash
sudo cp -a /etc/default/grub /etc/default/grub.bak-20261106-i915-guc
sudo sed -i 's/\(GRUB_CMDLINE_LINUX_DEFAULT="[^"]*\)"/\1 i915.enable_guc=0"/' /etc/default/grub
sudo update-grub
sudo reboot
```

Rollback is exact and reversible:

```bash
sudo cp -a /etc/default/grub.bak-20261106-i915-guc /etc/default/grub
sudo update-grub
sudo reboot
```

After reboot, record `uname -r` and `/proc/cmdline`, then run five 60-second
online-navigation xcb probes with cooldown while following i915 messages. A
new hang requires immediate rollback and an owner decision before trying the
next listed parameter (`i915.enable_guc=0` is the final untried approved
parameter); no automatic escalation is permitted.

## Matrix restart gate

Only after 20 continuous minutes without a hang should a new, separately dated
matrix begin. It must use one unchanged kernel/Mesa/parameter configuration,
run xcb before Wayland, interleave A+hooks and HEAD, use `systemd-inhibit` for
idle and sleep, and write that configuration to `environment.md`. On any new
hang, stop the matrix rather than mixing configurations. Suspend after a
completed matrix still requires a locally authorised system command.
