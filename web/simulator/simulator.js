// The face simulator on the project site: the settings page's clock preview, with the factory settings and a
// few display choices, and no clock. web/build.mjs puts data/config_initial.json in FACTORY_CONFIG.

/** @type {ClockConfig} */
const FACTORY_CONFIG = /*__CONFIG__*/null

// The settings the preview shows, with the same get/saveJson/on as the settings page's form. Every face is
// active, so the panel's left and right buttons reach them all.
const settings = (() => {
    const events = emitter()
    const zone = Intl.DateTimeFormat().resolvedOptions().timeZone || ""
    let draft = normalizeLoaded({ ...FACTORY_CONFIG, units: "mgdl", tz: zone, inactive_faces: [] })
    return {
        on: events.on,
        /**
         * Read one setting.
         * @param {string} key - Setting name.
         * @returns {any}
         */
        get: key => draft[key],
        /**
         * Change one setting and tell the preview.
         * @param {string} key - Setting name.
         * @param {JsonValue} value - New value.
         * @returns {void}
         */
        set(key, value) {
            draft = { ...draft, [key]: value }
            events.emit("change", key)
        },
        /**
         * The configuration as the clock would load it.
         * @returns {ClockConfig}
         */
        saveJson: () => buildSaveJson(draft),
        /**
         * Announce the starting settings to everything listening.
         * @returns {void}
         */
        load() { events.emit("load") },
    }
})()

/**
 * A card with a title, an optional line under it, and its body.
 * @param {string} title - Card title.
 * @param {string | null} subtitle - Line under the title.
 * @param {DomChild} body - Card contents.
 * @returns {HTMLElement}
 */
function card(title, subtitle, body) {
    return el("section.card", el("div.card-head", el("div", el("h2", title), subtitle ? el("p", subtitle) : null)), body)
}

/**
 * A setting with its label above the control.
 * @param {string} id - ID of the control the label names.
 * @param {string} label - Setting name.
 * @param {DomChild} control - The control, or a group of buttons.
 * @param {string} [help] - Line under the control.
 * @returns {HTMLElement}
 */
function field(id, label, control, help) {
    return el("div.field", el("label", { for: id }, label), control, help ? el("p.help", help) : null)
}

/**
 * Buttons for a setting with a few choices, one pressed.
 * @param {string} key - Setting name, also the group's ID.
 * @param {[string, string][]} options - Values and labels.
 * @returns {HTMLElement}
 */
function segmented(key, options) {
    const box = el("div.seg", { role: "group", id: key })
    const paint = () => $$("button", box).forEach(b => b.setAttribute("aria-pressed", String(b.dataset.value === settings.get(key))))
    for (const [value, text] of options) box.append(el("button", { type: "button", dataset: { value }, onclick: () => { settings.set(key, value); paint() } }, text))
    paint()
    return box
}

/**
 * A dropdown for a setting.
 * @param {string} key - Setting name, also the control's ID.
 * @param {[string, string][]} options - Values and labels.
 * @returns {HTMLSelectElement}
 */
function selectInput(key, options) {
    const s = el("select", { id: key })
    for (const [value, text] of options) s.add(new Option(text, value))
    s.value = settings.get(key)
    s.addEventListener("change", () => settings.set(key, s.value))
    settings.on("change", k => { if (k === key) s.value = settings.get(key) })
    return s
}

/**
 * The brightness modes, with the manual level as a slider (1 to 10) while manual is chosen.
 * @returns {HTMLElement}
 */
function brightnessField() {
    let lastManual = 5
    const seg = el("div.seg", { role: "group", id: "brightness_level" })
    const range = el("input", { type: "range", min: 1, max: 10, step: 1, id: "brightness_manual", "aria-label": "Manual brightness" })
    const out = el("output")
    const manualRow = el("div.range-row", el("label.label", { for: range.id }, "Level"), range, out)
    const paint = () => {
        const level = settings.get("brightness_level")
        $$("button", seg).forEach(b => b.setAttribute("aria-pressed", String(b.dataset.value === brightnessMode(level))))
        manualRow.hidden = brightnessMode(level) !== "manual"
        range.value = String(lastManual)
        out.textContent = String(lastManual)
    }
    for (const [mode, text, value] of BRIGHTNESS_MODES) {
        seg.append(el("button", { type: "button", dataset: { value: mode }, onclick: () => settings.set("brightness_level", value == null ? lastManual : value) }, text))
    }
    range.addEventListener("input", () => { lastManual = Number(range.value); settings.set("brightness_level", lastManual) })
    settings.on("change", k => { if (k === "brightness_level") paint() })
    paint()
    return el("div.stack", field(seg.id, "Brightness", seg, "Auto follows the light sensor: try the room light slider under the preview."), manualRow)
}

/**
 * Color swatches for a setting.
 * @param {string} key - Setting name, also the group's ID.
 * @param {[string, string][]} options - Color names and labels.
 * @returns {HTMLElement}
 */
function swatches(key, options) {
    const box = el("div.swatches", { role: "group", id: key })
    const paint = () => $$("button", box).forEach(b => b.setAttribute("aria-pressed", String(b.dataset.value === settings.get(key))))
    for (const [value, text] of options) {
        box.append(el("button.swatch", { type: "button", dataset: { value }, onclick: () => { settings.set(key, value); paint() } },
            el("i", { style: `background:${COLOR_HEX[value]}` }), text))
    }
    paint()
    return box
}

/**
 * The display choices: default face, units, time format, brightness, and colors.
 * @returns {HTMLElement}
 */
function settingsCard() {
    const ranges = el("div.bands", ...BANDS.map(b => {
        const dot = el("i")
        const paint = () => { dot.style.background = COLOR_HEX[settings.get(b.color)] }
        settings.on("change", k => { if (k === b.color) paint() })
        paint()
        return el("div.band", el("div.band-top", dot, el("span", b.name)), selectInput(b.color, BAND_COLORS))
    }))
    const limits = el("p.help")
    const showLimits = () => {
        const [urgentLow, low, high, urgentHigh] = LIMIT_KEYS.map(k => mgdlToText(settings.get(k), settings.get("units")))
        limits.textContent = `Factory limits: urgent low up to ${urgentLow}, low up to ${low}, high from ${high}, urgent high from ${urgentHigh} ${unitLabel(settings.get("units"))}.`
    }
    settings.on("change", k => { if (k === "units") showLimits() })
    showLimits()
    return card("Clock settings", "A few of the clock's display settings. The rest stay at their factory values.", el("div.stack",
        field("default_face", "Face", selectInput("default_face", FACES.map(f => [f.id, f.name])),
            "The clock's left and right buttons, under the preview, also move between the faces."),
        el("div.grid",
            field("units", "Units", segmented("units", UNITS)),
            field("time_format", "Time format", segmented("time_format", TIME_FORMATS))),
        brightnessField(),
        field("data_old_color", "Color when data is old", swatches("data_old_color", OLD_DATA_COLORS),
            "Try \"Past the no-data time\" under the preview to see it."),
        el("div.field", el("span.label", "Glucose range colors"), ranges, limits)))
}

/**
 * Every face as a picture; tapping one shows it.
 * @returns {HTMLElement}
 */
function facesCard() {
    const thumbs = []
    const list = el("div.faces", { role: "group", "aria-label": "Clock faces" })
    for (const f of FACES) {
        const canvas = el("canvas", { width: 192, height: 48, "aria-hidden": "true" })
        thumbs.push({ canvas, face: f.id })
        list.append(el("button.face", { type: "button", dataset: { face: f.id }, onclick: () => settings.set("default_face", f.id) },
            canvas, el("div.face-name", el("span", f.name)), el("p.help", f.about)))
    }
    const paint = () => $$("button", list).forEach(b => b.setAttribute("aria-pressed", String(b.dataset.face === settings.get("default_face"))))
    settings.on("change", k => {
        if (k !== "default_face") return
        paint()
        // On a phone the preview is above the faces, so bring it into view.
        if (window.matchMedia("(max-width: 980px)").matches) $(".sim-col").scrollIntoView({ behavior: "smooth", block: "start" })
    })
    paint()
    preview.setThumbs(thumbs)
    return card("Faces", "Tap a face to show it on the preview. The pictures use the reading chosen under the preview.", list)
}

/**
 * Build the page, then start the preview with the factory settings.
 * @returns {void}
 */
function start() {
    $("#sim_page").append(settingsCard(), facesCard())
    wirePreview(settings)
    settings.on("change", () => preview.setConfig(settings.saveJson()))
    settings.load()
    preview.setConfig(settings.saveJson())
    preview.start($("#sim_canvas")).catch(e => { $("#sim_note").textContent = e.message })
}

// For poking at the page from the console.
window.nsclock = { settings, preview }

start()
