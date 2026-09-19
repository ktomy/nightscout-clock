// The settings cards, built from the form's working copy. Controls write straight back to the form.

const idFor = key => "f_" + key

// ---------- building blocks ----------
function field(key, label, control, help) {
    const target = control.matches("input,select") ? control : control.querySelector("input,select")
    return el("div.field", { dataset: { field: key } },
        label ? el("label", { for: target ? target.id : null }, label) : null,
        control,
        help ? el("p.help", help) : null,
        el("p.err", { hidden: true }))
}

function selectInput(key, options, { numeric = false } = {}) {
    const current = form.get(key)
    const s = el("select", { id: idFor(key), name: key })
    if (!options.some(([v]) => String(v) === String(current == null ? "" : current))) s.add(new Option("Choose…", ""))
    for (const [v, label] of options) s.add(new Option(label, v))
    s.value = current == null ? "" : String(current)
    s.addEventListener("change", () => {
        form.set(key, numeric && s.value !== "" ? Number(s.value) : s.value)
        form.touch(key)
    })
    return s
}

function textInput(key, { type = "text", placeholder, maxlength, autocomplete = "off", trim = false } = {}) {
    const input = el("input", { id: idFor(key), name: key, type, placeholder, maxlength, autocomplete, spellcheck: "false", autocapitalize: "off" })
    input.value = form.get(key) == null ? "" : String(form.get(key))
    input.addEventListener("input", () => form.set(key, input.value))
    input.addEventListener("change", () => {
        if (trim && input.value !== input.value.trim()) { input.value = input.value.trim(); form.set(key, input.value) }
        form.touch(key)
    })
    return type === "password" ? passwordGroup(input) : input
}

function passwordGroup(input) {
    const btn = el("button.btn.icon", { type: "button", "aria-label": "Show password" }, icon("eye"))
    btn.addEventListener("click", () => {
        const show = input.type === "password"
        input.type = show ? "text" : "password"
        btn.replaceChildren(icon(show ? "eyeOff" : "eye"))
        btn.setAttribute("aria-label", show ? "Hide password" : "Show password")
    })
    return el("div.input-group", input, btn)
}

// A number typed in the given glucose units (stored as mg/dl), or a plain whole number without units.
function numberInput(key, { units } = {}) {
    const glucose = units !== undefined
    const input = el("input", { id: idFor(key), name: key, type: "text", inputmode: units === "mmol" ? "decimal" : "numeric", autocomplete: "off" })
    input.value = glucose ? mgdlToText(form.get(key), units) : form.get(key) == null ? "" : String(form.get(key))
    input.addEventListener("input", () => {
        const v = glucose ? textToMgdl(input.value, units) : /^\d+$/.test(input.value.trim()) ? parseInt(input.value, 10) : NaN
        form.set(key, Number.isNaN(v) ? input.value : v)
    })
    input.addEventListener("change", () => form.touch(key))
    return glucose ? el("div.unit", input, el("span", unitLabel(units))) : input
}

function toggleRow(key, title, desc) {
    const input = el("input", { type: "checkbox", id: idFor(key), name: key, role: "switch" })
    input.checked = !!form.get(key)
    input.addEventListener("change", () => form.set(key, input.checked))
    return el("div.switch-row", { dataset: { field: key } },
        el("div.text", el("label", { for: input.id }, el("h3", title)), desc ? el("p.help", desc) : null),
        el("label.switch", input, el("span")))
}

function segmented(key, options, { numeric = false, label } = {}) {
    const box = el("div.seg", { role: "group", "aria-label": label, id: idFor(key) })
    const paint = () => $$("button", box).forEach(b => b.setAttribute("aria-pressed", String(String(form.get(key)) === b.dataset.value)))
    for (const [value, text] of options) {
        box.append(el("button", {
            type: "button", dataset: { value: String(value) },
            onclick: () => { form.set(key, numeric ? Number(value) : String(value)); form.touch(key); paint() },
        }, text))
    }
    paint()
    return box
}

// Calls fn(key) on form changes until a re-render removes `node` from the page.
function onChangeWhileAttached(node, fn) {
    const off = form.on("change", key => (node.isConnected ? fn(key) : off()))
}

// A block rebuilt whenever one of `deps` changes (switches and selects only; never text, it would lose focus).
function reactive(deps, build) {
    const box = el("div.stack")
    const draw = () => {
        box.replaceChildren(build())
        applyErrors(box)
    }
    draw()
    onChangeWhileAttached(box, key => {
        if (!deps.includes(key)) return
        const active = document.activeElement && document.activeElement.id
        draw()
        const again = active && document.getElementById(active)
        if (again && box.contains(again)) again.focus({ preventScroll: true })
    })
    return box
}

function card(title, subtitle, body, { aside, id } = {}) {
    return el("section.card", { id },
        el("div.card-head", el("h2", title), aside || null, subtitle ? el("p", subtitle) : null),
        body)
}

// Show the form's visible errors inside `root`.
function applyErrors(root = document) {
    const errors = form.visibleErrors()
    $$("[data-field]", root).forEach(box => {
        const msg = errors[box.dataset.field]
        box.classList.toggle("invalid", !!msg)
        const p = $(":scope > .err", box)
        if (p) { p.hidden = !msg; p.textContent = msg || "" }
        const control = $("input,select", box)
        if (control) control.setAttribute("aria-invalid", msg ? "true" : "false")
    })
}

function toast(message, kind = "ok", ms = 4500) {
    const t = $("#toast")
    t.replaceChildren(el(`div.notice.${kind}`, message))
    t.hidden = false
    clearTimeout(t._timer)
    t._timer = setTimeout(() => { t.hidden = true }, ms)
}

// ---------- Display ----------
function displayTab() {
    return el("div.stack", facesCard(), faceScheduleCard(), brightnessCard(), oldDataCard(), timeCard())
}

function facesCard() {
    const faces = reactive(["default_face", "face_cycle_enabled", "face_cycle_faces"], () => {
        const cycling = !!form.get("face_cycle_enabled")
        const cycle = form.get("face_cycle_faces") || []
        const list = el("div.faces", { role: "group", "aria-label": cycling ? "Faces to cycle" : "Default face" })
        for (const f of FACES) {
            const on = cycling ? cycle.includes(f.id) : form.get("default_face") === f.id
            const status = cycling ? (on ? `✓ ${cycle.indexOf(f.id) + 1} in cycle` : "Not in cycle") : on ? "✓ Default" : ""
            list.append(el("button.face", {
                type: "button", "aria-pressed": String(on), dataset: { face: f.id },
                onclick: () => {
                    if (!cycling) return form.set("default_face", f.id)
                    form.set("face_cycle_faces", on ? cycle.filter(x => x !== f.id) : [...cycle, f.id].sort((a, b) => a - b))
                    form.touch("face_cycle_faces")
                },
            }, el("span", f.name), status ? el("span.face-status", status) : null))
        }
        return el("div.field", { dataset: { field: cycling ? "face_cycle_faces" : "default_face" } },
            el("p.help", cycling ? "Tap faces to add them to the cycle. Select at least two; they cycle in the order shown."
                : "Tap a face to make it the default."),
            list, el("p.err", { hidden: true }))
    })
    const interval = reactive(["face_cycle_enabled"], () => form.get("face_cycle_enabled")
        ? field("face_cycle_interval_seconds", "Change face every", segmented("face_cycle_interval_seconds", CYCLE_INTERVALS, { numeric: true, label: "Change face every" }))
        : el("span", { hidden: true }))
    const cyclingToggle = reactive(["face_schedule_enabled"], () => {
        const row = toggleRow("face_cycle_enabled", "Cycle selected clock faces automatically",
            "The left and right buttons move only between the selected faces while cycling is on. Turn off the daily schedule to enable cycling.")
        $("input", row).disabled = !!form.get("face_schedule_enabled")
        return row
    })
    return card("Clock face", null, el("div.stack", faces, el("hr.divider"), cyclingToggle,
        interval), { id: "card_faces" })
}

function faceScheduleCard() {
    const key = "face_schedule"
    const toggle = reactive(["face_cycle_enabled"], () => {
        const row = toggleRow("face_schedule_enabled", "Change face and brightness on a daily schedule",
            "Turn off face cycling to enable the schedule. Times use the clock's time zone and repeat every day.")
        $("input", row).disabled = !!form.get("face_cycle_enabled")
        return row
    })
    const body = reactive(["face_schedule_enabled"], () => {
        if (!form.get("face_schedule_enabled")) return el("span", { hidden: true })
        const rows = el("div.stack")
        const list = () => clone(form.get(key))
        const brightnessOptions = [[100, "Auto: balanced"], [101, "Auto: for darker rooms"],
            ...Array.from({ length: 10 }, (_, i) => [i + 1, `Manual: ${i + 1}`])]
        const draw = () => {
            rows.replaceChildren(...list().map((entry, i) => {
                const update = patch => {
                    const next = list()
                    next[i] = { ...next[i], ...patch }
                    form.set(key, next)
                    form.touch(key)
                }
                const time = el("input", { id: `schedule_${i}_time`, type: "time", step: 60 })
                time.value = entry.time
                time.addEventListener("input", () => update({ time: time.value }))
                const select = (name, options) => {
                    const control = el("select", { id: `schedule_${i}_${name}` })
                    for (const [value, label] of options) control.add(new Option(label, value))
                    control.value = String(entry[name])
                    control.addEventListener("change", () => update({ [name]: Number(control.value) }))
                    return control
                }
                return el("div.stack",
                    el("div.grid",
                        field(key + "_time", "Time", time),
                        field(key + "_face", "Clock face", select("face", FACES.map(f => [f.id, f.name]))),
                        field(key + "_brightness", "Brightness", select("brightness", brightnessOptions))),
                    el("button.btn.sm", { type: "button", "aria-label": `Remove scheduled time ${i + 1}`, onclick: () => {
                        const next = list()
                        next.splice(i, 1)
                        form.set(key, next)
                        form.touch(key)
                        draw()
                    } }, icon("trash"), "Remove time"))
            }))
            add.disabled = list().length >= 8
        }
        const add = el("button.btn.sm", { type: "button", onclick: () => {
            form.set(key, [...list(), { time: "22:00", face: 0, brightness: 100 }])
            form.touch(key)
            draw()
        } }, icon("plus"), "Add time")
        draw()
        return field(key, null, el("div.stack", rows, add),
            "Add up to 8 different times. Each row applies its face and brightness until the next scheduled time, including overnight.")
    })
    return card("Daily schedule", null, el("div.stack", toggle, body), { id: "card_schedule" })
}

function brightnessCard() {
    let lastManual = form.get("brightness_level") >= 1 && form.get("brightness_level") <= 10 ? form.get("brightness_level") : 5
    const body = reactive(["brightness_level"], () => {
        const level = form.get("brightness_level")
        const mode = level === 100 ? "auto_linear" : level === 101 ? "auto_dimmed" : "manual"
        const seg = el("div.seg", { role: "group", "aria-label": "Brightness", id: idFor("brightness_level") },
            ...BRIGHTNESS_MODES.map(([m, text, value]) => el("button", {
                type: "button", "aria-pressed": String(mode === m),
                onclick: () => form.set("brightness_level", value == null ? lastManual : value),
            }, text)))
        if (mode !== "manual") return el("div.stack", field("brightness_level", null, seg, "Follows the clock's light sensor."))
        const range = el("input", { type: "range", min: 1, max: 10, step: 1, id: "f_brightness_manual" })
        range.value = String(level)
        const out = el("output", String(level))
        range.addEventListener("input", () => {
            out.textContent = range.value
            lastManual = Number(range.value)
            form.set("brightness_level", lastManual)
        })
        return el("div.stack", field("brightness_level", null, seg),
            el("div.range-row", el("label.label", { for: range.id }, "Level"), range, out))
    })
    return card("Brightness level", null, body, { id: "card_brightness" })
}

function oldDataCard() {
    const box = el("div.swatches", { role: "group", "aria-label": "Color when data is old", id: idFor("data_old_color") })
    const paint = () => $$("button", box).forEach(b => b.setAttribute("aria-pressed", String(b.dataset.value === (form.get("data_old_color") || "gray"))))
    for (const [value, text] of OLD_DATA_COLORS) {
        box.append(el("button.swatch", { type: "button", dataset: { value }, onclick: () => { form.set("data_old_color", value); paint() } },
            el("i", { style: `background:${COLOR_HEX[value]}` }), text))
    }
    paint()
    return card("When data is old", null, el("div.stack",
        field("data_old_color", "Color", box,
            "Also used for the \"no data\" screen. Gray is not visible at the lowest brightness, so if you run the clock dim, choose one of the others to keep stale readings readable."),
        el("hr.divider"),
        toggleRow("custom_nodatatimer_enable", "Custom no data timer", "Minutes without a reading before the clock shows data as old. Otherwise 20 minutes."),
        reactive(["custom_nodatatimer_enable"], () => form.get("custom_nodatatimer_enable")
            ? el("div.grid", field("custom_nodatatimer", "Minutes (6 to 60)", numberInput("custom_nodatatimer")))
            : el("span", { hidden: true }))),
        { id: "card_old_data" })
}

function timeCard() {
    const tz = reactive(["ctx.tzNames"], () => {
        const list = ui.timezones
        if (!list) return field("tz", "Clock time zone", el("select", { id: idFor("tz"), disabled: true }, el("option", form.get("tz") || "Loading time zones…")))
        const s = el("select", { id: idFor("tz"), name: "tz" })
        s.add(new Option("Choose…", ""))
        for (const z of list) s.add(new Option(z.name, z.name))
        s.value = ui.timezoneNames.has(form.get("tz")) ? form.get("tz") : ""
        s.addEventListener("change", () => {
            const z = list.find(x => x.name === s.value)
            form.set("tz_libc", z ? z.value : "")
            form.set("tz", z ? z.name : "")
            form.touch("tz")
        })
        return field("tz", "Clock time zone", s, list.length ? null : "The time zone list could not be loaded.")
    })
    return card("Time", null, el("div.grid", tz,
        field("time_format", "Time format", segmented("time_format", TIME_FORMATS, { label: "Time format" }))), { id: "card_time" })
}

// ---------- Glucose ----------
function glucoseTab() {
    return el("div.stack", sourceCard(), rangesCard())
}

function sourceCard() {
    const body = reactive(["data_source"], () => {
        const src = form.get("data_source")
        const parts = [field("data_source", "Glucose data source", selectInput("data_source", SOURCES))]
        if (src === "nightscout") parts.push(nightscoutFields())
        if (src === "dexcom") parts.push(el("p.help", "Use the same credentials you use to log into the Dexcom app on your phone."), el("div.grid",
            field("dexcom_username", "Dexcom username", textInput("dexcom_username", { autocomplete: "username", placeholder: "Your Dexcom app login" })),
            field("dexcom_password", "Dexcom password", textInput("dexcom_password", { type: "password", autocomplete: "current-password", placeholder: "Your Dexcom app password" })),
            field("dexcom_server", "Dexcom server", selectInput("dexcom_server", DEXCOM_SERVERS))))
        if (src === "librelinkup") parts.push(el("div.grid",
            field("librelinkup_email", "LibreLink Up email", textInput("librelinkup_email", { type: "email", autocomplete: "username" })),
            field("librelinkup_password", "LibreLink Up password", textInput("librelinkup_password", { type: "password", autocomplete: "current-password" })),
            field("librelinkup_region", "LibreLink Up server", selectInput("librelinkup_region", LLU_REGIONS))), patientPicker())
        if (src === "medtrum") parts.push(el("div.grid",
            field("medtrum_email", "Medtrum email", textInput("medtrum_email", { type: "email", autocomplete: "username" })),
            field("medtrum_password", "Medtrum password", textInput("medtrum_password", { type: "password", autocomplete: "current-password" }))))
        if (src === "carelink") parts.push(el("div.notice.info",
            "Medtronic CareLink is not configured directly on the clock. Install xDrip+ on the phone connected to the sensor, connect xDrip+ to a Nightscout site, then connect the clock to that same Nightscout site by selecting the Nightscout source here. More details: ",
            el("a", { href: "https://github.com/ktomy/nightscout-clock/discussions/53", target: "_blank", rel: "noopener noreferrer" }, "GitHub discussion #53"), "."))
        if (src === "api") parts.push(el("div.notice.info", "Send readings to the clock with POST ", el("code", `${location.origin}/api/v1/entries`), " (see the project documentation)."))
        return el("div.stack", ...parts)
    })
    return card("Glucose data source", null, body, { id: "card_source" })
}

function nightscoutFields() {
    const parts = () => parseNightscoutUrl(form.get("nightscout_url"))
    const write = patch => form.set("nightscout_url", buildNightscoutUrl({ ...parts(), ...patch }))
    const p0 = parts()
    const protocol = el("select", { id: "f_ns_protocol" }, new Option("HTTPS", "https"), new Option("HTTP", "http"))
    protocol.value = p0.protocol
    protocol.addEventListener("change", () => write({ protocol: protocol.value }))
    const host = el("input", { id: "f_ns_host", type: "text", placeholder: "mybloodsugar.heroku.com", autocomplete: "off", spellcheck: "false", autocapitalize: "off" })
    host.value = p0.host
    host.addEventListener("input", () => {
        // Pasting a whole address fills all three parts.
        const pasted = /^https?:\/\//i.test(host.value.trim()) ? parseNightscoutUrl(host.value) : null
        if (pasted) { protocol.value = pasted.protocol; port.value = pasted.port; host.value = pasted.host; write(pasted) } else write({ host: host.value })
    })
    host.addEventListener("change", () => form.touch("ns_host"))
    const port = el("input", { id: "f_ns_port", type: "text", inputmode: "numeric", autocomplete: "off" })
    port.value = p0.port
    port.addEventListener("input", () => write({ port: port.value }))
    port.addEventListener("change", () => form.touch("ns_port"))
    return el("div.stack",
        el("div.grid",
            field("ns_protocol", "Protocol", protocol),
            field("ns_host", "Nightscout hostname", host, "Hostname, not URL."),
            field("ns_port", "Port (optional)", port)),
        field("api_secret", "API secret", textInput("api_secret", { type: "password" }), "Only needed when viewing Nightscout data is protected."),
        toggleRow("nightscout_simplified_api", "Use simplified API", "To be used with xDrip+ Open Web Service or similar."))
}

// The clock lists LibreLinkUp patients once it runs on that source; the choice matters only with several.
function patientPicker() {
    return reactive(["ctx.patients", "ctx.status"], () => {
        if (!(ui.status && ui.status.bgSource === "LIBRELINKUP")) return el("span", { hidden: true })
        if (!ui.patients) { loadPatients(); return el("span", { hidden: true }) }
        if (ui.patients.length <= 1) return el("span", { hidden: true })
        return field("librelinkup_patient_id", "Select patient", selectInput("librelinkup_patient_id",
            ui.patients.map(p => [p.patientId, `${p.firstName} ${p.lastName}`])))
    })
}

async function loadPatients(attempt = 0) {
    if (ui.patientsLoading) return
    ui.patientsLoading = true
    try {
        const r = await api.patients()
        if (r.ok && Array.isArray(r.data) && (r.data.length || attempt >= 5)) {
            ui.patients = r.data
            form.setCtx("patients", r.data.length)
            return
        }
    } catch (e) { /* retry below */ } finally { ui.patientsLoading = false }
    if (attempt < 5) setTimeout(() => loadPatients(attempt + 1), 10000)
}

function rangesCard() {
    const loadBtn = el("button.btn.sm", { type: "button", id: "load_from_ns", onclick: loadFromNightscout }, icon("download"), "Load from Nightscout")
    const syncBtn = () => { loadBtn.disabled = form.get("data_source") !== "nightscout" }
    syncBtn()
    onChangeWhileAttached(loadBtn, k => { if (k === "data_source") syncBtn() })

    // The bar and the in-range text follow the limits as they are typed.
    const bar = el("div.bandbar", { "aria-hidden": "true" }, ...BANDS.map(b => el("i", { style: `background:${COLOR_HEX[b.color]}` })))
    const normal = el("p.help")
    const drawBar = () => {
        const units = form.get("units")
        const lim = LIMIT_KEYS.map(k => form.get(k))
        const ordered = lim.every(isInt) && lim.every((v, i) => i === 0 || lim[i - 1] < v)
        const edges = ordered ? [40, ...lim.map(v => Math.min(400, Math.max(40, v))), 400] : [0, 1, 2, 3, 4, 5]
        const widths = edges.slice(1).map((v, i) => Math.max(v - edges[i], 4))
        const sum = widths.reduce((a, b) => a + b, 0)
        $$("i", bar).forEach((seg, i) => { seg.style.width = `${(widths[i] / sum) * 100}%` })
        normal.textContent = `${mgdlToText(form.get("low_mgdl"), units)} – ${mgdlToText(form.get("high_mgdl"), units)}`
    }
    drawBar()
    onChangeWhileAttached(bar, k => { if (LIMIT_KEYS.includes(k) || k === "units") drawBar() })

    const bands = reactive(["units"], () => el("div.bands", ...BANDS.map(b => el("div.band",
        el("div.band-top", el("i", { style: `background:${COLOR_HEX[b.color]}` }), el("span", b.name)),
        b.limit ? field(b.limit, b.label, numberInput(b.limit, { units: form.get("units") })) : normal))))
    return card("Glucose-related settings", null, el("div.stack",
        field("units", "Blood glucose units", segmented("units", UNITS, { label: "Blood glucose units" })), bar, bands),
        { aside: loadBtn, id: "card_ranges" })
}

async function loadFromNightscout() {
    const ns = parseNightscoutUrl(form.get("nightscout_url"))
    if (!ns.host) return toast("Please fill in the Nightscout hostname before loading the thresholds.", "warn")
    const btn = $("#load_from_ns")
    btn.disabled = true
    try {
        const headers = {}
        if (form.get("api_secret")) headers["api-secret"] = sha1Hex(form.get("api_secret"))
        const ctrl = new AbortController()
        const timer = setTimeout(() => ctrl.abort(), 10000)
        const res = await fetch(`${buildNightscoutUrl(ns)}api/v1/status.json`, { headers, signal: ctrl.signal })
        clearTimeout(timer)
        const data = res.ok ? await res.json() : null
        const t = data && data.settings && data.settings.thresholds
        if (!t) throw new Error(res.ok ? "no settings in the answer" : `HTTP ${res.status}`)
        form.set("units", data.settings.units === "mmol" ? "mmol" : "mgdl")
        const pairs = [["low_urgent_mgdl", t.bgLow], ["low_mgdl", t.bgTargetBottom], ["high_mgdl", t.bgTargetTop], ["high_urgent_mgdl", t.bgHigh]]
        for (const [k, v] of pairs) if (isFinite(Number(v))) form.set(k, Math.round(Number(v)))
        rerenderTab("glucose")
        toast("Blood glucose thresholds loaded from Nightscout. Save to keep them.")
    } catch (e) {
        toast(`Failed to load settings from Nightscout (${e.name === "AbortError" ? "no answer" : e.message}).`, "bad", 7000)
    } finally {
        btn.disabled = form.get("data_source") !== "nightscout"
    }
}

// ---------- Alarms ----------
function alarmsTab() {
    return el("div.stack",
        el("div.notice.info", el("b", "How alarms work. "),
            "The volume is moderate, so test each sound with Try on clock. An alarm beeps 2 times (high), 3 times (low) or 4 times (urgent low), then pauses for the repeat interval; intensive mode keeps beeping without a pause. This continues until glucose is back in range or you press the clock's middle button to snooze, which shows SNOOZED for the snooze time. An alert with alert windows stays silent outside them."),
        ...ALARMS.map(alarmCard),
        card("Repeat", null, el("div.stack",
            reactive(["alarm_intensive_mode"], () => {
                const seg = segmented("alarm_repeat_interval_seconds", REPEATS, { numeric: true, label: "Repeat every" })
                if (form.get("alarm_intensive_mode")) $$("button", seg).forEach(b => { b.disabled = true })
                return field("alarm_repeat_interval_seconds", "Repeat every", seg, "Time between alarm repeats when not snoozed. Intensive mode overrides this setting.")
            }),
            toggleRow("alarm_intensive_mode", "Intensive mode", "The alarm sounds continuously until snoozed.")),
            { id: "card_repeat" }))
}

function alarmCard(a) {
    const key = s => `alarm_${a.t}_${s}`
    const fs = el("fieldset")
    // A switched-off alarm's settings are disabled, but its sound can still be tried.
    const sync = () => {
        const off = !form.get(key("enabled"))
        fs.classList.toggle("off", off)
        $$("input,select,button", fs).forEach(x => { if (!x.dataset.alwaysOn) x.disabled = off })
    }

    const body = reactive(["units"], () => {
        const units = form.get("units")
        // Presets are the melody strings themselves; anything else is "Custom".
        const presets = [[a.defaultMelody, "Default"], ...MELODY_PRESETS.map(([n, m]) => [m, n]), ["custom", "Custom"]]
        const preset = el("select", { id: idFor(key("melody_preset")), "aria-label": `${a.name} alert sound` })
        for (const [v, n] of presets) preset.add(new Option(n, v))
        const melody = el("input", { id: idFor(key("melody")), type: "text", placeholder: a.defaultMelody, spellcheck: "false", autocomplete: "off", autocapitalize: "off" })
        melody.value = form.get(key("melody")) || ""
        const syncPreset = () => { preset.value = presets.some(([v]) => v === melody.value.trim()) ? melody.value.trim() : "custom" }
        syncPreset()
        preset.addEventListener("change", () => {
            if (preset.value === "custom") return melody.focus()
            melody.value = preset.value
            form.set(key("melody"), melody.value)
            form.touch(key("melody"))
        })
        melody.addEventListener("input", () => { form.set(key("melody"), melody.value.trim()); syncPreset() })
        melody.addEventListener("change", () => form.touch(key("melody")))

        const tryIt = el("button.btn.sm", { type: "button", dataset: { alwaysOn: "1" }, onclick: async () => {
            if (!isValidRtttl(melody.value)) return toast("Please enter a valid RTTTL melody before testing.", "warn")
            try {
                const r = await api.tryAlarm(melody.value.trim())
                const ok = r.ok && r.data && r.data.status === "ok"
                toast(ok ? "You should hear the alert playing." : "Could not play the alert.", ok ? "ok" : "bad")
            } catch (e) { toast(e.message, "bad") }
        } }, icon("speaker"), "Try on clock")

        return el("div.stack",
            el("div.grid",
                field(key("value"), a.compare, numberInput(key("value"), { units })),
                field(key("snooze_interval"), "Snooze for", selectInput(key("snooze_interval"), SNOOZES, { numeric: true }))),
            field(key("melody"), "Alert sound", el("div.stack", el("div.row.melody-row", preset, tryIt), melody), "Choose a sound or enter a custom RTTTL melody."),
            alertWindows(a))
    })
    fs.append(body)
    sync()
    onChangeWhileAttached(fs, k => { if (k === key("enabled") || k === "units") sync() })
    return el("section.card.alarm", { id: `card_alarm_${a.t}` }, toggleRow(key("enabled"), `${a.name} alert`, null), fs)
}

function alertWindows(a) {
    const key = `alarm_${a.t}_alert_windows`
    const box = el("div.windows")
    const list = () => clone(form.get(key) || [])
    const draw = () => {
        box.replaceChildren(...list().map((w, i) => {
            const update = patch => { const next = list(); next[i] = { ...next[i], ...patch }; form.set(key, next); form.touch(key); draw() }
            const bad = !w.days || !w.from || !w.to || w.from === w.to
            const days = el("div.days", ...DAYS.map(([n, label]) => el("button", {
                type: "button", "aria-pressed": String(String(w.days || "").includes(String(n))),
                onclick: () => {
                    const set = new Set(String(list()[i].days || "").split("").filter(Boolean))
                    set.has(String(n)) ? set.delete(String(n)) : set.add(String(n))
                    update({ days: [...set].sort().join("") })
                },
            }, label)))
            const from = el("input", { type: "time", step: 60, "aria-label": "From" })
            const to = el("input", { type: "time", step: 60, "aria-label": "To" })
            from.value = w.from || ""
            to.value = w.to || ""
            from.addEventListener("change", () => update({ from: from.value }))
            to.addEventListener("change", () => update({ to: to.value }))
            return el(bad ? "div.window.invalid" : "div.window",
                el("div", days, el("div.times", from, el("span.muted", "to"), to)),
                el("button.btn.ghost.icon", { type: "button", "aria-label": "Remove alert window", onclick: () => { const next = list(); next.splice(i, 1); form.set(key, next); form.touch(key); draw() } }, icon("trash")))
        }))
        const off = !form.get(`alarm_${a.t}_enabled`)
        $$("input,button", box).forEach(x => { x.disabled = off })
    }
    draw()
    const add = el("button.btn.sm", { type: "button", onclick: () => { form.set(key, [...list(), { days: "0123456", from: "08:00", to: "22:00" }]); form.touch(key); draw() } }, icon("plus"), "Add window")
    return el("div.field", { dataset: { field: key } },
        el("div.row.spread", el("span.label", "Alert windows"), add),
        box,
        el("p.help", `Leave this empty to alert at any time. With one or more windows the ${a.name} alert only sounds inside them. A window whose end time is earlier than its start time runs past midnight into the next morning, and the days you pick are the days it starts on.`),
        el("p.err", { hidden: true }))
}

// ---------- System ----------
function systemTab() {
    return el("div.stack", wifiCard(), extraWifiCard(), hostnameCard(), loginCard(), versionCard())
}

function wifiCard() {
    const open = el("input", { type: "checkbox", id: "f_open_network" })
    open.checked = form.ctx.openNetwork
    const pw = textInput("password", { type: "password", autocomplete: "new-password" })
    const warn = el("div.notice.warn", "An open WiFi network is not secure and not recommended. If the network has a captive portal (a page that opens when you join it, e.g. to enter an email), the clock will not be able to reach the internet.")
    const sync = () => { $("input", pw).disabled = open.checked; $("button", pw).disabled = open.checked; warn.hidden = !open.checked }
    open.addEventListener("change", () => {
        if (open.checked) { $("input", pw).value = ""; form.set("password", "") }
        form.setCtx("openNetwork", open.checked)
        sync()
    })
    sync()
    return card("Wireless network", null, el("div.stack",
        el("div.grid",
            field("ssid", "WiFi network name (SSID)", textInput("ssid", { maxlength: 32 })),
            field("password", "WiFi password", pw)),
        el("label.check", open, "Open WiFi network (no password)"),
        warn), { id: "card_wifi" })
}

function extraWifiCard() {
    return card("Additional WiFi network", null, el("div.stack",
        toggleRow("additional_wifi_enable", "Use an additional WiFi network", "Tried when the main network can't be joined."),
        reactive(["additional_wifi_enable", "additional_wifi_type"], () => {
            if (!form.get("additional_wifi_enable")) return el("span", { hidden: true })
            const type = form.get("additional_wifi_type")
            const known = type === "wpa_psk" || type === "wpa_eap"
            return el("div.grid",
                field("additional_wifi_type", "WiFi type", selectInput("additional_wifi_type", WIFI_TYPES)),
                known ? field("additional_ssid", "WiFi network name (SSID)", textInput("additional_ssid", { maxlength: 32 })) : null,
                type === "wpa_eap" ? field("additional_wifi_username", "WiFi username", textInput("additional_wifi_username")) : null,
                known ? field("additional_wifi_password", "WiFi password", textInput("additional_wifi_password", { type: "password", autocomplete: "new-password" })) : null)
        })), { id: "card_wifi2" })
}

function hostnameCard() {
    return card("Custom hostname", null, el("div.stack",
        toggleRow("custom_hostname_enable", "Use a custom hostname", "The clock's name on your network, and the name of its setup WiFi network."),
        reactive(["custom_hostname_enable"], () => form.get("custom_hostname_enable")
            ? el("div.grid", field("custom_hostname", "Custom hostname", textInput("custom_hostname")))
            : el("span", { hidden: true }))), { id: "card_hostname" })
}

function loginCard() {
    return card("Web interface authentication", null, el("div.stack",
        toggleRow("web_auth_enable", "Require a password to change settings", "Settings stay locked until someone unlocks them with this password; a login lasts 10 minutes."),
        reactive(["web_auth_enable"], () => form.get("web_auth_enable")
            ? el("div.grid", field("web_auth_password", "Password", textInput("web_auth_password", { type: "password", autocomplete: "new-password", trim: true })))
            : el("span", { hidden: true }))), { id: "card_login" })
}

function versionCard() {
    return card("Version", null, el("div.stack",
        el("dl.kv", el("dt", "Current version"), el("dd", { id: "fw_current" }, ui.versions.current || "…"),
            el("dt", "Latest version"), el("dd", { id: "fw_latest" }, ui.versions.latest || "…")),
        el("p.help", { id: "fw_status" }, ...versionStatusNodes())), { id: "card_version" })
}

function versionStatusNodes() {
    const v = ui.versions
    if (v.update) {
        return [el("a", { href: "https://ktomy.github.io/nightscout-clock/", target: "_blank", rel: "noopener noreferrer" }, `Update to ${v.latest}`), " · ",
            el("a", { href: "https://github.com/ktomy/nightscout-clock/tree/main?tab=readme-ov-file#changes", target: "_blank", rel: "noopener noreferrer" }, "Changes")]
    }
    return [v.status || "Checking for updates…"]
}

// ---------- tabs ----------
const TABS = { display: displayTab, glucose: glucoseTab, alarms: alarmsTab, system: systemTab }
const ui = { tab: "display", timezones: null, timezoneNames: null, status: null, patients: null, patientsLoading: false, versions: {} }

function rerenderTab(name) {
    const panel = $(`#tab_${name}`)
    if (!panel || !form.loaded) return
    panel.replaceChildren(TABS[name]())
    applyErrors(panel)
}
function renderAll() { Object.keys(TABS).forEach(rerenderTab) }

function showTab(name) {
    ui.tab = name
    for (const n of Object.keys(TABS)) $(`#tab_${n}`).hidden = n !== name
    $$("[data-tab]").forEach(b => b.setAttribute("aria-selected", String(b.dataset.tab === name)))
    try { sessionStorage.setItem("tab", name) } catch (e) { /* private mode */ }
}
