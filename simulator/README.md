# Face Simulator

A pixel-exact, in-browser preview of all 16 clock faces — open `index.html`
in any browser. Animations run live, and the controls let you sweep glucose
value, trend, data age (stale at 20 min), units, 12/24 h clock, and the stale
color.

`index.html` is **self-contained** (fonts and bitmaps are embedded). It is a
generated file and is not committed; the release workflow generates it and
publishes it to GitHub Pages at
`https://ktomy.github.io/nightscout-clock/simulator/` together with each
release (see `.github/workflows/build_release.yml`).

## Regenerating

`index.html` is generated from `gen_sim.py` plus the data files in `fonts/`:

- `fonts/font_awtrix.json`, `fonts/font_mu.json` — the firmware's own font
  bitmaps (Awtrix + Mu Heavy), so text metrics match the device exactly.
- Static data (trend arrows, mood smileys, unicorn sprite) is
  transcribed from the firmware sources at the top of `gen_sim.py`.

After changing any face logic in `src/BGDisplayFace*.cpp`, mirror the change
in the matching `draw()` function inside `gen_sim.py`'s `JS_FACES` block,
then regenerate and open `simulator/index.html` to check the result:

```sh
python3 simulator/gen_sim.py
```

The script writes `simulator/index.html`, which Git ignores. Commit only the
generator and font changes.

## Fidelity notes

- The JS engine re-implements the firmware's Adafruit-GFX text rendering,
  RGB565 color math (`hsv`, `fade`, `blend`), and per-face drawing logic.
- Sample history is synthesized: 3 h of 5-minute readings ending at the
  chosen value/trend.
- Faces driven by wall-clock time (Clock, Time only, Diagnostics) use the
  browser's local time.
- Hardware-only behavior is not simulated: button handling (including the
  middle-button brightness cycle/overlay), the LDR light sensor, Wi-Fi,
  and OTA.
