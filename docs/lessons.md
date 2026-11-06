# Lessons

- Never use `apt autoremove` on this workstation: it previously removed Qt
  runtime packages.
- `/tmp` is cleared by reboot; retain temporary binaries and evidence under
  `generated/`.
- `937f68a` limits A+hooks to liveness tracing and its ~1 FPS run is not a
  performance or stability reference.
- Wayland DPR and xcb/XWayland DPI change pixel workload and visual scale; do
  not compare their FPS as equal rendering conditions.
