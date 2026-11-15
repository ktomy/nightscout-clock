// The preview column's controls: a reading to show, the clock's time, the room light, the buttons, and the
// notes under the panel. Shared by the settings page and the face simulator on the project site, so it reads
// settings only through the small interface below.

/**
 * The settings the preview shows: the settings page's form, or the simulator's few choices.
 * @typedef {Object} PreviewSettings
 * @property {(key: string) => any} get Current value of one setting.
 * @property {() => ClockConfig} saveJson The whole configuration as the clock would load it.
 * @property {(name: "load" | "change", fn: EventHandler) => (() => void)} on "load" after a new config; "change" with the changed key.
 */

/**
 * Connect the preview column's controls (preview.html) to the preview and keep them in step with the settings.
 * @param {PreviewSettings} settings - Source of the settings the preview shows.
 * @returns {void}
 */
function wirePreview(settings) {
    const bg = $("#sim_bg"), bgOut = $("#sim_bg_out"), age = $("#sim_age"), lux = $("#sim_lux"), luxOut = $("#sim_lux_out")
    const trends = $("#sim_trends")
    const ARROWS = [[1, "⇈", "Rising fast"], [2, "↑", "Rising"], [3, "↗", "Rising slowly"], [4, "→", "Steady"], [5, "↘", "Falling slowly"], [6, "↓", "Falling"], [7, "⇊", "Falling fast"]]
    let trend = 4
    trends.replaceChildren(...ARROWS.map(([v, a, label]) => el("button", {
        type: "button", "aria-pressed": String(v === trend), "aria-label": label, title: label,
        onclick: () => { trend = v; $$("button", trends).forEach((b, i) => b.setAttribute("aria-pressed", String(ARROWS[i][0] === v))); push() },
    }, a)))
    const luxValue = () => Math.round(Math.pow(10, Number(lux.value) / 25 - 1) * 10) / 10
    const labelAges = () => {
        const n = noDataMinutes(settings.saveJson())
        age.querySelector('option[value="stale"]').textContent = `Past the no-data time (${n + 1} min)`
        $("#sim_age_help").textContent = `Readings are 5 minutes apart. The age bars fill up to 5 minutes; the old-data color appears after ${n} minutes (the no-data time).`
    }
    const push = () => {
        const minutes = age.value === "stale" ? noDataMinutes(settings.saveJson()) + 1 : age.value === "none" ? 1 : Number(age.value)
        bgOut.textContent = mgdlToText(Number(bg.value), settings.get("units"))
        const history = age.value === "none" ? "none" : trend < 4 ? "rising" : trend > 4 ? "falling" : "steady"
        preview.setScenario({ bg: Number(bg.value), trend, age: minutes, history })
    }
    bg.addEventListener("input", push)
    age.addEventListener("change", push)
    lux.addEventListener("input", () => { luxOut.textContent = `${luxValue()} lx`; preview.setLux(luxValue()) })
    luxOut.textContent = `${luxValue()} lx`
    const buttons = { sim_left: "LEFT", sim_select: "SELECT", sim_right: "RIGHT" }
    for (const [id, button] of Object.entries(buttons)) $(`#${id}`).addEventListener("click", () => preview.press(button))

    // A strip in the range colors under the glucose slider, so you can see where the limits fall.
    const drawBands = () => {
        const min = 40, max = 400
        const lim = LIMIT_KEYS.map(k => settings.get(k))
        if (!lim.every(isInt) || !lim.every((v, i) => i === 0 || lim[i - 1] < v)) return $("#sim_bg_bands").replaceChildren()
        const edges = [min, ...lim.map(v => Math.min(max, Math.max(min, v))), max]
        $("#sim_bg_bands").replaceChildren(...BANDS.map((b, i) => el("i", {
            title: b.name,
            style: `width:${((edges[i + 1] - edges[i]) / (max - min)) * 100}%;background:${COLOR_HEX[settings.get(b.color)]}`,
        })))
    }
    settings.on("load", drawBands)
    settings.on("change", k => { if (LIMIT_KEYS.includes(k) || BANDS.some(b => b.color === k)) drawBands() })
    $("#sim_live").addEventListener("change", e => preview.setLive(e.target.checked))

    // Clock time: set a time and run it faster to watch the clock react.
    const timeMode = $("#sim_time_mode"), timeInput = $("#sim_time"), timeOut = $("#sim_time_out"), speed = $("#sim_speed")
    // Times are the clock's local time, in the time zone chosen in the settings.
    const epochFromInput = (hhmm = timeInput.value) => {
        const [h, m] = (hhmm || "00:00").split(":").map(Number)
        const now = Math.floor(Date.now() / 1000)
        const local = new Date((now + preview.tzOffsetAt(now)) * 1000)
        const guess = Date.UTC(local.getUTCFullYear(), local.getUTCMonth(), local.getUTCDate(), h, m) / 1000
        return guess - preview.tzOffsetAt(guess)
    }
    const fmt = epoch => {
        const d = new Date((epoch + preview.tzOffsetAt(epoch)) * 1000)
        const h = d.getUTCHours(), mm = String(d.getUTCMinutes()).padStart(2, "0")
        return settings.get("time_format") === "12" ? `${h % 12 || 12}:${mm} ${h < 12 ? "AM" : "PM"}` : `${String(h).padStart(2, "0")}:${mm}`
    }
    const showSetTime = set => {
        timeInput.disabled = !set
        timeInput.hidden = !set
        $("#sim_speed_row").hidden = !set
    }
    timeMode.addEventListener("change", () => {
        const set = timeMode.value === "set"
        showSetTime(set)
        preview.setTime(set ? { mode: "set", epoch: epochFromInput() } : { mode: "now" })
    })
    timeInput.addEventListener("change", () => preview.setTime({ epoch: epochFromInput() }))
    const setSpeed = s => {
        $$("button", speed).forEach(x => x.setAttribute("aria-pressed", String(Number(x.dataset.speed) === s)))
        preview.setTime({ speed: s })
        // A running clock needs the firmware's loop running.
        if (s > 0 && !$("#sim_live").checked) { $("#sim_live").checked = true; preview.setLive(true) }
    }
    $$("button", speed).forEach(b => b.addEventListener("click", () => setSpeed(Number(b.dataset.speed))))
    preview.on("time", epoch => { timeOut.textContent = fmt(epoch) })
    // Show the clock's time straight away, not only once the clock runs.
    preview.on("render", () => { if (!$("#sim_live").checked) timeOut.textContent = fmt(preview.clockEpoch()) })
    $("#sim_sound").addEventListener("change", e => preview.setSound(e.target.checked))

    const warnSeen = []
    let warnKey = ""
    preview.on("render", s => {
        document.body.dataset.preview = "ready"
        const face = FACES.find(f => f.id === s.face)
        $("#sim_face").textContent = face ? face.name : `Face ${s.face}`
        const level = settings.get("brightness_level")
        $("#sim_brightness").textContent = level === 100 ? "auto" : level === 101 ? "auto, darker" : `level ${level}`
        $("#sim_note").textContent = s.displayOn ? `${s.drawn} LEDs lit` : "display off"
        // An animated face can lose a channel on a few frames only, so the warning shows the worst of the last
        // 3 s; otherwise it blinks on and off and moves the page. A new face, brightness or setting starts afresh.
        const now = Date.now(), key = `${s.face}|${s.brightness}`
        if (key !== warnKey) { warnKey = key; warnSeen.length = 0 }
        warnSeen.push({ t: now, lost: s.lost, partial: s.partial, drawn: s.drawn })
        while (now - warnSeen[0].t > 3000) warnSeen.shift()
        const worst = warnSeen.reduce((a, w) => ({
            lost: Math.max(a.lost, w.lost), partial: Math.max(a.partial, w.partial), drawn: Math.max(a.drawn, w.drawn),
        }), { lost: 0, partial: 0, drawn: 0 })
        const warn = $("#sim_warn")
        warn.hidden = !worst.lost && !worst.partial
        warn.textContent = worst.lost
            ? `${worst.lost} of ${worst.drawn} LEDs this face draws get 0 at this brightness: they are invisible on the clock.`
            : worst.partial ? `${worst.partial} LEDs lose a color channel at this brightness, so the color shifts on the clock.` : ""
        $("#sim_lux_row").hidden = !(level === 100 || level === 101)
    })
    preview.on("error", e => { $("#sim_note").textContent = e.message })
    settings.on("change", key => {
        warnSeen.length = 0
        if (key === "units") push()
        if (key === "tz") preview.setTimeZone(settings.get("tz"))
        // The "past the no-data time" choice follows the timer setting.
        if (key === "custom_nodatatimer" || key === "custom_nodatatimer_enable") { labelAges(); if (age.value === "stale") push() }
    })
    settings.on("load", () => { preview.setTimeZone(settings.get("tz")); labelAges(); push() })
}
