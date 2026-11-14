# Face Simulator

An in-browser preview of the clock faces — open `index.html` in any browser.
Animations run live, and the controls let you sweep glucose value, trend, data
age (stale at 20 min), units, 12/24 h clock, and the stale color.

The simulator does not run the firmware. Each face is redrawn by a separate
JavaScript copy of its drawing code, so it shows only the faces someone chose
to add, and a face may look different from the current firmware.

`index.html` is **self-contained** (fonts and bitmaps are embedded). It is a
generated file and is not committed; the release workflow generates it and
publishes it to GitHub Pages at
`https://ktomy.github.io/nightscout-clock/simulator/` together with each
release (see `.github/workflows/build_release.yml`).

## Keeping a face in the simulator (optional)

Adding a face to the simulator, or keeping it up to date, is optional and up
to the face's author and the maintainers. Face changes in `src/` do not need a
matching simulator change, and pull requests are not held back for one.

`index.html` is generated from `gen_sim.py` plus the data files in `fonts/`:

- `fonts/font_awtrix.json`, `fonts/font_mu.json` — the firmware's own font
  bitmaps (Awtrix + Mu Heavy), so text metrics match the device exactly.
- Static data (trend arrows, mood smileys, unicorn sprite) is
  transcribed from the firmware sources at the top of `gen_sim.py`.

To add or update a face, write or change its `draw()` function inside
`gen_sim.py`'s `JS_FACES` block to match `src/BGDisplayFace*.cpp`, then
regenerate and open `simulator/index.html` to check the result:

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
- Faces driven by wall-clock time (Clock and value, Time only, Rainbow
  clock) use the browser's local time.
- Hardware-only behavior is not simulated: button handling (including the
  middle-button brightness cycle/overlay), the LDR light sensor, Wi-Fi,
  and OTA.
