// Build settings tabs and reusable controls from the form's editable draft.
// Controls write through form.set(); subscriptions rebuild dependent controls and display validation errors.

/**
 * Browser hints and edit behavior for a text or password control.
 * @typedef {Object} TextInputOptions
 * @property {string} [type='text'] HTML input type, such as text, email, or password.
 * @property {string} [placeholder] Hint displayed while the control is empty.
 * @property {number} [maxlength] Maximum number of input characters.
 * @property {string} [autocomplete='off'] Browser autocomplete hint.
 * @property {boolean} [trim=false] Trim surrounding whitespace when the change is committed.
 */

/**
 * Turn a settings key into the stable DOM ID used by input controls and their labels.
 * @param {string} key - Settings key to turn into an input ID.
 * @returns {string}
 */
const idFor = key => "f_" + key

/**
 * ---------- building blocks ----------
 * Wrap a control with its label, optional help, and an error message slot keyed to form validation.
 * @param {string} key - Validation key for this field.
 * @param {string | null} label - Label text, or null to omit it.
 * @param {HTMLElement} control - Input element or a container of controls.
 * @param {string} [help] - Optional explanation below the control.
 * @returns {HTMLElement}
 */
function field(key, label, control, help) {
    const target = control.matches("input,select") ? control : control.querySelector("input,select")
    return el("div.field", { dataset: { field: key } },
        label ? el("label", { for: target ? target.id : null }, label) : null,
        control,
        help ? el("p.help", help) : null,
        el("p.err", { hidden: true }))
}

/**
 * Build a dropdown from option/value pairs and the current draft value.
 * On selection, write back a string or number and mark the field as touched for validation.
 * @param {string} key - Draft setting to bind.
 * @param {SelectOption[]} options - Available values and labels.
 * @param {{numeric?: boolean}} [settings={}] - Whether selected values are stored as numbers.
 * @returns {HTMLSelectElement}
 */
function selectInput(key, options, { numeric = false } = {}) {
    const current = form.get(key)
    const s = el("select", { id: idFor(key), name: key })
    if (!options.some(([v]) => String(v) === String(current == null ? "" : current))) s.add(new Option("Choose…", ""))
    for (const [v, label] of options) s.add(new Option(label, v))
    s.value = current == null ? "" : String(current)
    // Save the selected option using the requested value type and reveal any validation error.
    s.addEventListener("change", () => {
        form.set(key, numeric && s.value !== "" ? Number(s.value) : s.value)
        form.touch(key)
    })
    return s
}

/**
 * Create a draft-bound text input with optional trimming and password visibility controls.
 * Update on typing and mark the field as touched when its change is committed.
 * @param {string} key - Draft setting to bind.
 * @param {TextInputOptions} [options={}] - Input type, browser hints, and trimming behavior.
 * @returns {HTMLElement}
 */
function textInput(key, { type = "text", placeholder, maxlength, autocomplete = "off", trim = false } = {}) {
    const input = el("input", { id: idFor(key), name: key, type, placeholder, maxlength, autocomplete, spellcheck: "false", autocapitalize: "off" })
    input.value = form.get(key) == null ? "" : String(form.get(key))
    input.addEventListener("input", () => form.set(key, input.value))
    // Trim committed text when requested and mark the field as ready to show errors.
    input.addEventListener("change", () => {
        if (trim && input.value !== input.value.trim()) { input.value = input.value.trim(); form.set(key, input.value) }
        form.touch(key)
    })
    return type === "password" ? passwordGroup(input) : input
}

/**
 * Wrap a password input with a button that switches between masked and visible text.
 * @param {HTMLInputElement} input - Password input to wrap with a visibility button.
 * @returns {HTMLElement}
 */
function passwordGroup(input) {
    const btn = el("button.btn.icon", { type: "button", "aria-label": "Show password" }, icon("eye"))
    // Toggle password masking and update the icon and accessible button label together.
    btn.addEventListener("click", () => {
        const show = input.type === "password"
        input.type = show ? "text" : "password"
        btn.replaceChildren(icon(show ? "eyeOff" : "eye"))
        btn.setAttribute("aria-label", show ? "Hide password" : "Show password")
    })
    return el("div.input-group", input, btn)
}

/**
 * Display glucose in the selected units or a plain integer, converting edits back into the draft.
 * Keep invalid text intact so validation can explain it instead of silently changing it.
 * @param {string} key - Draft setting to bind.
 * @param {{units?: GlucoseUnits}} [options={}] - Glucose units; omit for a plain integer input.
 * @returns {HTMLElement}
 */
function numberInput(key, { units } = {}) {
    const glucose = units !== undefined
    const input = el("input", { id: idFor(key), name: key, type: "text", inputmode: units === "mmol" ? "decimal" : "numeric", autocomplete: "off" })
    input.value = glucose ? mgdlToText(form.get(key), units) : form.get(key) == null ? "" : String(form.get(key))
    // Convert typed text into the stored numeric units, preserving invalid input for validation.
    input.addEventListener("input", () => {
        const v = glucose ? textToMgdl(input.value, units) : /^\d+$/.test(input.value.trim()) ? parseInt(input.value, 10) : NaN
        form.set(key, Number.isNaN(v) ? input.value : v)
    })
    input.addEventListener("change", () => form.touch(key))
    return glucose ? el("div.unit", input, el("span", unitLabel(units))) : input
}

/**
 * Build a labelled on/off switch initialized from the draft and write its boolean value on change.
 * @param {string} key - Boolean setting to bind.
 * @param {string} title - Switch label.
 * @param {string | null} desc - Optional explanatory text.
 * @returns {HTMLElement}
 */
function toggleRow(key, title, desc) {
    const input = el("input", { type: "checkbox", id: idFor(key), name: key, role: "switch" })
    input.checked = !!form.get(key)
    input.addEventListener("change", () => form.set(key, input.checked))
    return el("div.switch-row", { dataset: { field: key } },
        el("div.text", el("label", { for: input.id }, el("h3", title)), desc ? el("p.help", desc) : null),
        el("label.switch", input, el("span")))
}

/**
 * Build a row of mutually exclusive option buttons and save the selected value to the draft.
 * Use aria-pressed to show the current choice and mark changes for validation.
 * @param {string} key - Draft setting to bind.
 * @param {SelectOption[]} options - Button values and labels.
 * @param {{numeric?: boolean, label?: string, prop?: string}} [settings={}] - Value conversion and accessible group label.
 * @returns {HTMLElement}
 */
function segmented(key, options, { numeric = false, label, prop } = {}) {
    const name = prop ? `${key}_${prop}` : key
    const get = () => (prop ? (form.get(key) || {})[prop] : form.get(key))
    const put = v => form.set(key, prop ? { ...form.get(key), [prop]: v } : v)
    const box = el("div.seg", { role: "group", "aria-label": label, id: idFor(name) })
    const paint = () => $$("button", box).forEach(b => b.setAttribute("aria-pressed", String(String(get()) === b.dataset.value)))
    for (const [value, text] of options) {
        box.append(el("button", {
            type: "button", dataset: { value: String(value) },
            onclick: () => { put(numeric ? Number(value) : String(value)); form.touch(name); paint() },
        }, text))
    }
    paint()
    return box
}

/**
 * Subscribe a DOM block to form changes and unsubscribe on the next change after it leaves the page.
 * @param {Node} node - Element whose attachment controls the subscription lifetime.
 * @param {(key: string) => void} fn - Callback for a changed setting or context key.
 * @returns {void}
 */
function onChangeWhileAttached(node, fn) {
    const off = form.on("change", key => (node.isConnected ? fn(key) : off()))
}

/**
 * Rebuild a DOM block when one of its dependency keys changes, restoring focus and visible errors.
 * Use this for structural changes, not typing dependencies that would replace an active text input.
 * @param {string[]} deps - Setting/context keys that trigger rebuilding.
 * @param {() => Node} build - Callback that creates the replacement block.
 * @returns {HTMLElement}
 */
function reactive(deps, build) {
    const box = el("div.stack")
    /**
     * Replace the block with newly built controls, then apply any validation messages to them.
     * @returns {void}
     */
    const draw = () => {
        box.replaceChildren(build())
        applyErrors(box)
    }
    draw()
    // Rebuild only for relevant changes and restore focus to a replacement control with the same ID.
    onChangeWhileAttached(box, key => {
        if (!deps.includes(key)) return
        const active = document.activeElement && document.activeElement.id
        draw()
        const again = active && document.getElementById(active)
        if (again && box.contains(again)) again.focus({ preventScroll: true })
    })
    return box
}

/**
 * Build a settings section with a heading, optional subtitle/action, and supplied content.
 * @param {string} title - Section heading.
 * @param {string | null} subtitle - Optional subtitle.
 * @param {HTMLElement} body - Controls to place in the card.
 * @param {{aside?: Node, id?: string}} [options={}] - Optional heading action and element ID.
 * @returns {HTMLElement}
 */
function card(title, subtitle, body, { aside, id } = {}) {
    return el("section.card", { id },
        el("div.card-head", el("h2", title), aside || null, subtitle ? el("p", subtitle) : null),
        body)
}

/**
 * Match visible form errors to data-field containers, updating messages, styling, and aria-invalid.
 * @param {ParentNode} [root=document] - Container whose field errors should be refreshed.
 * @returns {void}
 */
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

/**
 * Show a temporary status notice and replace any existing dismissal timer so the latest message stays visible.
 * @param {string} message - Status text to display.
 * @param {string} [kind="ok"] - Notice style such as ok, warn, or bad.
 * @param {number} [ms=4500] - Time before hiding the notice, in milliseconds.
 * @returns {void}
 */
function toast(message, kind = "ok", ms = 4500) {
    const t = $("#toast")
    t.replaceChildren(el(`div.notice.${kind}`, message))
    t.hidden = false
    clearTimeout(t._timer)
    t._timer = setTimeout(() => { t.hidden = true }, ms)
}

/**
 * ---------- Display ----------
 * Assemble the Display tab from face, schedule, brightness, stale-data, and time cards.
 * @returns {HTMLElement}
 */
function displayTab() {
    return el("div.stack", facesCard(), faceScheduleCard(), brightnessCard(), oldDataCard(), timeCard())
}

/**
 * Build active-face selection, default-face, and cycling controls.
 * Disable cycling while daily scheduling is enabled and show the interval only while cycling.
 * @returns {HTMLElement}
 */
function facesCard() {
    // Rebuild face buttons when the default, cycling switch, or active faces change.
    const faces = reactive(["default_face", "face_cycle_enabled", "inactive_faces"], () => {
        const cycling = !!form.get("face_cycle_enabled")
        const inactive = form.get("inactive_faces") || []
        const list = el("div.faces", { role: "group", "aria-label": "Active clock faces" })
        for (const f of FACES) {
            const on = !inactive.includes(f.id)
            const status = !on ? "Not active" : !cycling && form.get("default_face") === f.id ? "✓ Active, default" : "✓ Active"
            list.append(el("button.face", {
                type: "button", "aria-pressed": String(on), dataset: { face: f.id },
                // Toggle this face in the sorted inactive selection.
                onclick: () => {
                    const next = on ? [...inactive, f.id].sort((a, b) => a - b) : inactive.filter(x => x !== f.id)
                    const active = activeFaceIds(next)
                    // The default face is one of the active ones, so it moves before the list it is picked from.
                    if (active.length && !active.includes(form.get("default_face"))) form.set("default_face", active[0])
                    form.set("inactive_faces", next)
                    form.touch("inactive_faces")
                },
            }, el("span", f.name), el("span.face-status", status)))
        }
        return el("div.field", { dataset: { field: "inactive_faces" } },
            el("p.help", "Tap the faces you use. The left and right buttons move only between these, and cycling runs through them in the order shown."),
            list, el("p.err", { hidden: true }))
    })
    const defaultFace = reactive(["face_cycle_enabled", "inactive_faces"], () => {
        if (form.get("face_cycle_enabled")) return el("span", { hidden: true })
        const active = FACES.filter(f => activeFaceIds(form.get("inactive_faces")).includes(f.id))
        return field("default_face", "Face shown by default", selectInput("default_face", active.map(f => [f.id, f.name]), { numeric: true }))
    })
    // Show the cycling interval buttons only when automatic face cycling is enabled.
    const interval = reactive(["face_cycle_enabled"], () => form.get("face_cycle_enabled")
        ? field("face_cycle_interval_seconds", "Change face every", segmented("face_cycle_interval_seconds", CYCLE_INTERVALS, { numeric: true, label: "Change face every" }))
        : el("span", { hidden: true }))
    // Rebuild the cycling switch when scheduling changes so the two modes cannot be enabled together.
    const cyclingToggle = reactive(["face_schedule_enabled"], () => {
        const row = toggleRow("face_cycle_enabled", "Cycle through the active faces automatically",
            "Cycling needs at least two active faces. Turn off the daily schedule to enable cycling.")
        $("input", row).disabled = !!form.get("face_schedule_enabled")
        return row
    })
    return card("Clock faces", null, el("div.stack", faces, defaultFace, el("hr.divider"), cyclingToggle,
        interval, faceDrawers()), { id: "card_faces" })
}

// Settings that belong to one face, by face id, shown in a drawer while that face is active.
const FACE_DRAWERS = { 3: bigTextSettings, 6: unicornSettings }

function faceDrawers() {
    return reactive(["inactive_faces"], () => {
        const active = activeFaceIds(form.get("inactive_faces"))
        const faces = FACES.filter(f => FACE_DRAWERS[f.id] && active.includes(f.id))
        if (!faces.length) return el("span", { hidden: true })
        return el("div.stack", el("hr.divider"), ...faces.map(f => drawer(f.name, FACE_DRAWERS[f.id]())))
    })
}

// Open the drawer when a setting needs attention.
function drawer(title, body) {
    const panel = el("div.stack", { hidden: !ui.openDrawers.has(title) }, body)
    const toggle = el("button.btn.sm", { type: "button" })
    const setOpen = open => {
        panel.hidden = !open
        toggle.textContent = open ? "Hide" : "Show"
        toggle.setAttribute("aria-expanded", String(open))
        open ? ui.openDrawers.add(title) : ui.openDrawers.delete(title)
    }
    toggle.addEventListener("click", () => setOpen(panel.hidden))
    setOpen(!panel.hidden)
    const off = form.on("errors", () => {
        if (!panel.isConnected) return off()
        if (panel.hidden && $(".invalid", panel)) setOpen(true)
    })
    return el("div.drawer", el("div.row.spread", el("h3", title), toggle), panel)
}

function bigTextSettings() {
    return el("div.stack",
        field("face_big_text_early_stale_color", "Color when a reading is late", segmented("face_big_text", EARLY_STALE_COLORS, { prop: "early_stale_color", label: "Color when a reading is late" }),
            "Color late readings until the old-data threshold. Off keeps the usual glucose colors."),
        field("face_big_text_early_stale_minutes", "Late after", segmented("face_big_text", EARLY_STALE_MINUTES, { numeric: true, prop: "early_stale_minutes", label: "Late after" })))
}

function unicornSettings() {
    return reactive(["face_unicorn"], () => el("div.stack",
        field("face_unicorn_mane", "Mane", segmented("face_unicorn", MANE_MODES, { prop: "mane", label: "Mane" }),
            "A moving mane rolls its colors while the reading is fresh, and stops in the old data color when data is old."),
        (form.get("face_unicorn") || {}).mane === "moving"
            ? field("face_unicorn_speed", "Speed", segmented("face_unicorn", ANIMATION_SPEEDS, { prop: "speed", label: "Speed" }))
            : null))
}

/**
 * Build editable daily time/face/brightness rows backed by the schedule array in the form.
 * Keep scheduling exclusive with cycling and limit additions to eight rows.
 * @returns {HTMLElement}
 */
function faceScheduleCard() {
    const key = "face_schedule"
    // Disable the schedule switch while automatic cycling is enabled.
    const toggle = reactive(["face_cycle_enabled"], () => {
        const row = toggleRow("face_schedule_enabled", "Change face and brightness on a daily schedule",
            "Turn off face cycling to enable the schedule. Times use the clock's time zone and repeat every day.")
        $("input", row).disabled = !!form.get("face_cycle_enabled")
        return row
    })
    // Create the row editor when scheduling is enabled and hide it otherwise.
    const body = reactive(["face_schedule_enabled", "inactive_faces"], () => {
        if (!form.get("face_schedule_enabled")) return el("span", { hidden: true })
        const rows = el("div.stack")
        /**
         * Copy the schedule array before changing rows so form.set can detect the edit.
         * @returns {FaceScheduleEntry[]}
         */
        const list = () => clone(form.get(key))
        const brightnessOptions = [[100, "Auto: balanced"], [101, "Auto: for darker rooms"],
            ...Array.from({ length: 10 }, (_, i) => [i + 1, `Manual: ${i + 1}`])]
        /**
         * Rebuild schedule rows from the draft and disable Add time at the eight-row limit.
         * @returns {void}
         */
        const draw = () => {
            rows.replaceChildren(...list().map((entry, i) => {
                /**
                 * Merge an edit into this schedule row, save the copied array, and expose validation errors.
                 * @param {Partial<FaceScheduleEntry>} patch - Row properties to replace in the copied array.
                 * @returns {void}
                 */
                const update = patch => {
                    const next = list()
                    next[i] = { ...next[i], ...patch }
                    form.set(key, next)
                    form.touch(key)
                }
                const time = el("input", { id: `schedule_${i}_time`, type: "time", step: 60 })
                time.value = entry.time
                time.addEventListener("input", () => update({ time: time.value }))
                /**
                 * Build a row-specific dropdown whose changes are stored as numeric face or brightness values.
                 * @param {'face' | 'brightness'} name - Schedule entry property to edit.
                 * @param {SelectOption[]} options - Available numeric values and labels.
                 * @returns {HTMLSelectElement}
                 */
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
                        field(key + "_face", "Clock face", select("face", FACES.filter(f => activeFaceIds(form.get("inactive_faces")).includes(f.id)).map(f => [f.id, f.name]))),
                        field(key + "_brightness", "Brightness", select("brightness", brightnessOptions))),
                    // Remove the chosen row from a copied array, validate it, and rebuild row indices and controls.
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
        // Append a default schedule row, validate the array, and redraw the editor.
        const add = el("button.btn.sm", { type: "button", onclick: () => {
            form.set(key, [...list(), { time: "22:00", face: activeFaceIds(form.get("inactive_faces"))[0], brightness: 100 }])
            form.touch(key)
            draw()
        } }, icon("plus"), "Add time")
        draw()
        return field(key, null, el("div.stack", rows, add),
            "Add up to 8 different times. Each row applies its face and brightness until the next scheduled time, including overnight.")
    })
    return card("Daily schedule", null, el("div.stack", toggle, body), { id: "card_schedule" })
}

/**
 * Build automatic brightness choices and a manual slider, remembering the last manual level within the card.
 * @returns {HTMLElement}
 */
function brightnessCard() {
    let lastManual = form.get("brightness_level") >= 1 && form.get("brightness_level") <= 10 ? form.get("brightness_level") : 5
    // Rebuild the mode choices and optional manual slider whenever the stored brightness level changes.
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
        // Remember and save the slider level as a number so switching back to manual restores it.
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

/**
 * Build stale-data color choices and conditionally show the custom no-data timer input.
 * @returns {HTMLElement}
 */
function oldDataCard() {
    const box = el("div.swatches", { role: "group", "aria-label": "Color when data is old", id: idFor("data_old_color") })
    /**
     * Mark the color swatch matching the draft as selected, defaulting to gray when unset.
     * @returns {void}
     */
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

/**
 * Build timezone and time-format controls, storing both the selected zone name and its firmware rule.
 * @returns {HTMLElement}
 */
function timeCard() {
    // Rebuild the timezone picker after the available zone list loads or changes.
    const tz = reactive(["ctx.tzNames"], () => {
        const list = ui.timezones
        if (!list) return field("tz", "Clock time zone", el("select", { id: idFor("tz"), disabled: true }, el("option", form.get("tz") || "Loading time zones…")))
        const s = el("select", { id: idFor("tz"), name: "tz" })
        s.add(new Option("Choose…", ""))
        for (const z of list) s.add(new Option(z.name, z.name))
        s.value = ui.timezoneNames.has(form.get("tz")) ? form.get("tz") : ""
        // Save both the selected timezone name and its matching libc rule, then mark it for validation.
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

/**
 * ---------- Glucose ----------
 * Assemble the Glucose tab from data-source controls and glucose range settings.
 * @returns {HTMLElement}
 */
function glucoseTab() {
    return el("div.stack", sourceCard(), rangesCard())
}

/**
 * Build the source selector and rebuild its credentials/help fields when the selected provider changes.
 * @returns {HTMLElement}
 */
function sourceCard() {
    // Build only the credentials and instructions relevant to the selected glucose provider.
    const body = reactive(["data_source"], () => {
        const src = form.get("data_source")
        // Parse the latest draft URL so editing one address component preserves the other components.
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

/**
 * Split the Nightscout URL into editable protocol, host, and port controls that update one stored URL.
 * Also expose the API secret and simplified-API switch.
 * @returns {HTMLElement}
 */
function nightscoutFields() {
    /**
     * Parse the latest draft URL so editing one address component preserves the other components.
     * @returns {NightscoutAddress}
     */
    const parts = () => parseNightscoutUrl(form.get("nightscout_url"))
    /**
     * Merge an edited URL component into the current address and write the rebuilt URL to the form.
     * @param {Partial<NightscoutAddress>} patch - Address components to replace.
     * @returns {void}
     */
    const write = patch => form.set("nightscout_url", buildNightscoutUrl({ ...parts(), ...patch }))
    const p0 = parts()
    const protocol = el("select", { id: "f_ns_protocol" }, new Option("HTTPS", "https"), new Option("HTTP", "http"))
    protocol.value = p0.protocol
    protocol.addEventListener("change", () => write({ protocol: protocol.value }))
    const host = el("input", { id: "f_ns_host", type: "text", placeholder: "mybloodsugar.heroku.com", autocomplete: "off", spellcheck: "false", autocapitalize: "off" })
    host.value = p0.host
    // Accept either a hostname or a pasted full URL and synchronize the separate address controls.
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

/**
 * Show patient selection only when the clock reports LibreLinkUp and multiple patients are available.
 * Request patient data lazily and rebuild the picker when status or patient context changes.
 * @returns {HTMLElement}
 */
function patientPicker() {
    // Request patient choices when needed and render a selector only if there is more than one patient.
    return reactive(["ctx.patients", "ctx.status"], () => {
        if (!(ui.status && ui.status.bgSource === "LIBRELINKUP")) return el("span", { hidden: true })
        if (!ui.patients) { loadPatients(); return el("span", { hidden: true }) }
        if (ui.patients.length <= 1) return el("span", { hidden: true })
        return field("librelinkup_patient_id", "Select patient", selectInput("librelinkup_patient_id",
            ui.patients.map(p => [p.patientId, `${p.firstName} ${p.lastName}`])))
    })
}

/**
 * Fetch patient choices while preventing overlapping loads, then notify the picker through form context.
 * Retry empty or failed replies up to five more times at ten-second intervals.
 * @param {number} [attempt=0] - Zero-based retry attempt.
 * @returns {Promise<void>}
 */
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

/**
 * Build unit selection, glucose thresholds, the range visualization, and Nightscout import action.
 * Refresh displayed units and range proportions as the draft changes.
 * @returns {HTMLElement}
 */
function rangesCard() {
    const loadBtn = el("button.btn.sm", { type: "button", id: "load_from_ns", onclick: loadFromNightscout }, icon("download"), "Load from Nightscout")
    /**
     * Enable Nightscout range import only when Nightscout is the selected data source.
     * @returns {void}
     */
    const syncBtn = () => { loadBtn.disabled = form.get("data_source") !== "nightscout" }
    syncBtn()
    onChangeWhileAttached(loadBtn, k => { if (k === "data_source") syncBtn() })

    // The bar and the in-range text follow the limits as they are typed.
    const bar = el("div.bandbar", { "aria-hidden": "true" }, ...BANDS.map(b => el("i", { style: `background:${COLOR_HEX[b.color]}` })))
    const normal = el("p.help")
    /**
     * Calculate color-band widths from ordered glucose limits, using equal widths for invalid ordering.
     * Update the displayed in-range values in the currently selected units.
     * @returns {void}
     */
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

/**
 * Fetch Nightscout status directly from the browser, adding a hashed API secret when configured.
 * Copy returned glucose thresholds into the draft and report import errors.
 * @returns {Promise<void>}
 */
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

/**
 * ---------- Alarms ----------
 * Assemble alarm cards and shared repeat controls, disabling repeat-interval choices in intensive mode.
 * @returns {HTMLElement}
 */
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

/**
 * Build one alarm's threshold, snooze, melody, preview, and time-window controls from its descriptor.
 * Disable settings when the alarm is off while keeping sound preview available.
 * @param {AlarmDescriptor} a - Alarm type and labels used to bind its settings.
 * @returns {HTMLElement}
 */
function alarmCard(a) {
    /**
     * Build the configuration key for a field belonging to this alarm type.
     * @param {string} s - Alarm field suffix, such as melody or enabled.
     * @returns {string}
     */
    const key = s => `alarm_${a.t}_${s}`
    const fs = el("fieldset")
    /**
     * A switched-off alarm's settings are disabled, but its sound can still be tried.
     * @returns {void}
     */
    const sync = () => {
        const off = !form.get(key("enabled"))
        fs.classList.toggle("off", off)
        $$("input,select,button", fs).forEach(x => { if (!x.dataset.alwaysOn) x.disabled = off })
    }

    // Rebuild alarm value controls when units change, using the same underlying mg/dl settings.
    const body = reactive(["units"], () => {
        const units = form.get("units")
        // Presets are the melody strings themselves; anything else is "Custom".
        const presets = [[a.defaultMelody, "Default"], ...MELODY_PRESETS.map(([n, m]) => [m, n]), ["custom", "Custom"]]
        const preset = el("select", { id: idFor(key("melody_preset")), "aria-label": `${a.name} alert sound` })
        for (const [v, n] of presets) preset.add(new Option(n, v))
        const melody = el("input", { id: idFor(key("melody")), type: "text", placeholder: a.defaultMelody, spellcheck: "false", autocomplete: "off", autocapitalize: "off" })
        melody.value = form.get(key("melody")) || ""
        /**
         * Match the melody text against known presets, selecting Custom when none matches.
         * @returns {void}
         */
        const syncPreset = () => { preset.value = presets.some(([v]) => v === melody.value.trim()) ? melody.value.trim() : "custom" }
        syncPreset()
        // Copy a chosen preset melody into the draft, or focus the text input for a custom melody.
        preset.addEventListener("change", () => {
            if (preset.value === "custom") return melody.focus()
            melody.value = preset.value
            form.set(key("melody"), melody.value)
            form.touch(key("melody"))
        })
        melody.addEventListener("input", () => { form.set(key("melody"), melody.value.trim()); syncPreset() })
        melody.addEventListener("change", () => form.touch(key("melody")))

        // Validate and play the current melody on the clock, reporting the response without saving settings.
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

/**
 * Build each alarm's weekday and time windows from its draft array, with add/remove actions.
 * Write changes through the form and redraw rows to reflect updated days, times, and validity.
 * @param {AlarmDescriptor} a - Alarm type and labels used to bind its settings.
 * @returns {HTMLElement}
 */
function alertWindows(a) {
    const key = `alarm_${a.t}_alert_windows`
    const box = el("div.windows")
    /**
     * Copy this alarm's window array before editing, treating an absent array as empty.
     * @returns {AlertWindow[]}
     */
    const list = () => clone(form.get(key) || [])
    /**
     * Rebuild window controls, highlight incomplete windows, and disable them when the alarm is off.
     * @returns {void}
     */
    const draw = () => {
        box.replaceChildren(...list().map((w, i) => {
            /**
             * Merge changes into this window, validate through the form, and redraw the window list.
             * @param {Partial<AlertWindow>} patch - Row properties to replace in the copied array.
             * @returns {void}
             */
            const update = patch => { const next = list(); next[i] = { ...next[i], ...patch }; form.set(key, next); form.touch(key); draw() }
            const bad = !w.days || !w.from || !w.to || w.from === w.to
            const days = el("div.days", ...DAYS.map(([n, label]) => el("button", {
                type: "button", "aria-pressed": String(String(w.days || "").includes(String(n))),
                // Toggle the selected weekday in the window's stored day string and save it in sorted order.
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
                // Remove this alert window from the draft and rebuild the remaining rows.
                el("button.btn.ghost.icon", { type: "button", "aria-label": "Remove alert window", onclick: () => { const next = list(); next.splice(i, 1); form.set(key, next); form.touch(key); draw() } }, icon("trash")))
        }))
        const off = !form.get(`alarm_${a.t}_enabled`)
        $$("input,button", box).forEach(x => { x.disabled = off })
    }
    draw()
    // Append an everyday 08:00–22:00 window and redraw the controls with validation feedback.
    const add = el("button.btn.sm", { type: "button", onclick: () => { form.set(key, [...list(), { days: "0123456", from: "08:00", to: "22:00" }]); form.touch(key); draw() } }, icon("plus"), "Add window")
    return el("div.field", { dataset: { field: key } },
        el("div.row.spread", el("span.label", "Alert windows"), add),
        box,
        el("p.help", `Leave this empty to alert at any time. With one or more windows the ${a.name} alert only sounds inside them. A window whose end time is earlier than its start time runs past midnight into the next morning, and the days you pick are the days it starts on.`),
        el("p.err", { hidden: true }))
}

/**
 * ---------- System ----------
 * Assemble network, hostname, authentication, and firmware-version cards for the system tab.
 * @returns {HTMLElement}
 */
function systemTab() {
    return el("div.stack", wifiCard(), extraWifiCard(), hostnameCard(), loginCard(), versionCard())
}

/**
 * Build primary WiFi credentials and an open-network switch that clears/disables the password input.
 * @returns {HTMLElement}
 */
function wifiCard() {
    const open = el("input", { type: "checkbox", id: "f_open_network" })
    open.checked = form.ctx.openNetwork
    const pw = textInput("password", { type: "password", autocomplete: "new-password" })
    const warn = el("div.notice.warn", "An open WiFi network is not secure and not recommended. If the network has a captive portal (a page that opens when you join it, e.g. to enter an email), the clock will not be able to reach the internet.")
    /**
     * Disable password controls and show the warning while open-network mode is selected.
     * @returns {void}
     */
    const sync = () => { $("input", pw).disabled = open.checked; $("button", pw).disabled = open.checked; warn.hidden = !open.checked }
    // Clear the password when selecting an open network and update validation context and controls.
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

/**
 * Build optional backup-network controls, showing fields appropriate to WPA-PSK or WPA-EAP.
 * @returns {HTMLElement}
 */
function extraWifiCard() {
    return card("Additional WiFi network", null, el("div.stack",
        toggleRow("additional_wifi_enable", "Use an additional WiFi network", "Tried when the main network can't be joined."),
        // Rebuild backup-network inputs when enabled or when its authentication method changes.
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

/**
 * Build the custom-hostname switch and show its text input only when enabled.
 * @returns {HTMLElement}
 */
function hostnameCard() {
    return card("Custom hostname", null, el("div.stack",
        toggleRow("custom_hostname_enable", "Use a custom hostname", "The clock's name on your network, and the name of its setup WiFi network."),
        // Show the hostname input only while custom naming is enabled.
        reactive(["custom_hostname_enable"], () => form.get("custom_hostname_enable")
            ? el("div.grid", field("custom_hostname", "Custom hostname", textInput("custom_hostname")))
            : el("span", { hidden: true }))), { id: "card_hostname" })
}

/**
 * Build web password protection controls and show the password field only when protection is enabled.
 * @returns {HTMLElement}
 */
function loginCard() {
    return card("Web interface authentication", null, el("div.stack",
        toggleRow("web_auth_enable", "Require a password to change settings", "Settings stay locked until someone unlocks them with this password; a login lasts 10 minutes."),
        // Show the password input only while web authentication is enabled.
        reactive(["web_auth_enable"], () => form.get("web_auth_enable")
            ? el("div.grid", field("web_auth_password", "Password", textInput("web_auth_password", { type: "password", autocomplete: "new-password", trim: true })))
            : el("span", { hidden: true }))), { id: "card_login" })
}

/**
 * Build current/latest firmware labels and update information from the shared version state.
 * @returns {HTMLElement}
 */
function versionCard() {
    return card("Version", null, el("div.stack",
        el("dl.kv", el("dt", "Current version"), el("dd", { id: "fw_current" }, ui.versions.current || "…"),
            el("dt", "Latest version"), el("dd", { id: "fw_latest" }, ui.versions.latest || "…")),
        el("p.help", { id: "fw_status" }, ...versionStatusNodes())), { id: "card_version" })
}

/**
 * Return installer/changelog links when an update exists, otherwise the current version-check message.
 * @returns {Array<Node | string>}
 */
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
const ui = { tab: "display", timezones: null, timezoneNames: null, status: null, patients: null, patientsLoading: false, versions: {}, openDrawers: new Set() }

/**
 * Replace one tab's contents using its builder and reapply visible validation errors.
 * @param {SettingsTab} name - Tab to render or select.
 * @returns {void}
 */
function rerenderTab(name) {
    const panel = $(`#tab_${name}`)
    if (!panel || !form.loaded) return
    panel.replaceChildren(TABS[name]())
    applyErrors(panel)
}
/**
 * Rebuild every settings tab from the current form, used after loading settings or discarding edits.
 * @returns {void}
 */
function renderAll() { Object.keys(TABS).forEach(rerenderTab) }

/**
 * Show only the chosen tab, update navigation accessibility state, and remember it for this browser session.
 * @param {SettingsTab} name - Tab to render or select.
 * @returns {void}
 */
function showTab(name) {
    ui.tab = name
    for (const n of Object.keys(TABS)) $(`#tab_${n}`).hidden = n !== name
    $$("[data-tab]").forEach(b => b.setAttribute("aria-selected", String(b.dataset.tab === name)))
    try { sessionStorage.setItem("tab", name) } catch (e) { /* private mode */ }
}
