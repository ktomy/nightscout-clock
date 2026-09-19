// What the clock's settings are: option lists, validation, and the JSON that is saved.

// IDs must match the registration order in BGDisplayManager::setup().
const FACES = [
    { id: 0, name: "Simple" },
    { id: 1, name: "Full glucose graph" },
    { id: 2, name: "Glucose graph and value" },
    { id: 3, name: "Big text" },
    { id: 4, name: "Value and delta" },
    { id: 5, name: "Current time and BG value" },
    { id: 6, name: "Unicorn" },
]

// "carelink" is not a clock source: choosing it explains the xDrip+ and Nightscout bridge instead.
const SOURCES = [
    ["dexcom", "Dexcom"],
    ["nightscout", "Nightscout"],
    ["librelinkup", "LibreLinkUp"],
    ["carelink", "Medtronic CareLink"],
    ["medtrum", "Medtrum Easy Follow"],
    ["api", "API (see project documentation)"],
]
const DEXCOM_SERVERS = [["us", "US"], ["ous", "Non-US"], ["jp", "Japan"]]
const LLU_REGIONS = [
    ["AE", "United Arab Emirates"], ["AP", "Asia Pacific"], ["AU", "Australia"], ["CA", "Canada"], ["DE", "Germany"],
    ["EU", "Europe"], ["EU2", "Europe 2"], ["FR", "France"], ["JP", "Japan"], ["US", "United States"],
    ["LA", "Latin America"], ["RU", "Russia"],
]
const UNITS = [["mgdl", "mg/dl"], ["mmol", "mmol/l"]]
// Lowest range first, in the clock's fixed colors. `limit` is the key that ends (or starts) the range.
const BANDS = [
    { name: "Urgent low", color: "red", limit: "low_urgent_mgdl", label: "Up to" },
    { name: "Low", color: "yellow", limit: "low_mgdl", label: "Up to" },
    { name: "In range", color: "green", limit: null, label: "" },
    { name: "High", color: "yellow", limit: "high_mgdl", label: "From" },
    { name: "Urgent high", color: "red", limit: "high_urgent_mgdl", label: "From" },
]
const LIMIT_KEYS = ["low_urgent_mgdl", "low_mgdl", "high_mgdl", "high_urgent_mgdl"]
const OLD_DATA_COLORS = [["gray", "Gray"], ["cyan", "Cyan"], ["magenta", "Magenta"], ["blue", "Blue"]]
const CYCLE_INTERVALS = [[10, "10 s"], [30, "30 s"], [60, "1 min"], [120, "2 min"], [180, "3 min"], [300, "5 min"]]
const TIME_FORMATS = [["24", "24h"], ["12", "AM/PM"]]
const SNOOZES = [[5, "5 minutes"], [10, "10 minutes"], [15, "15 minutes"], [30, "30 minutes"], [60, "1 hour"], [120, "2 hours"], [0, "Until next trigger"]]
const REPEATS = [[60, "1 min"], [120, "2 min"], [300, "5 min"]]
const WIFI_TYPES = [["wpa_psk", "WPA-PSK"], ["wpa_eap", "WPA-EAP"]]
const BRIGHTNESS_MODES = [["auto_linear", "Auto: balanced", 100], ["auto_dimmed", "Auto: for darker rooms", 101], ["manual", "Manual", null]]
// Mon..Sun as the firmware numbers them (Sunday = 0).
const DAYS = [[1, "Mon"], [2, "Tue"], [3, "Wed"], [4, "Thu"], [5, "Fri"], [6, "Sat"], [0, "Sun"]]

const ALARMS = [
    { t: "high", name: "High", compare: "Alert when above", defaultMelody: "high:d=4,o=5,b=125:4e7,p,4e7" },
    { t: "low", name: "Low", compare: "Alert when below", defaultMelody: "low:d=4,o=5,b=200:4e5,4p,4e5,4p,4e5" },
    { t: "urgent_low", name: "Urgent Low", compare: "Alert when below", defaultMelody: "urgent_low:d=4,o=5,b=230:4e6,4p,4e6,4p,4e6,4p,4e6" },
]
const MELODY_PRESETS = [
    ["Double beep", "doublebeep:d=8,o=6,b=180:c,p,c"],
    ["Triple beep", "triplebeep:d=16,o=6,b=200:c,p,c,p,c"],
    ["Two tone siren", "siren:d=4,o=5,b=100:a,d6,a,d6"],
    ["Urgent pulse", "urgent:d=32,o=7,b=220:c,p,c,p,c,p,c,p,c,p,c"],
    ["Soft ping", "ping:d=4,o=6,b=140:8e,16p,8c"],
    ["Long tone", "longtone:d=1,o=5,b=90:a"],
]

// Brightness: 1-10 manual, 100 "Auto: balanced", 101 "Auto: for darker rooms".
const brightnessMode = level => (level === 101 ? "auto_dimmed" : level === 100 ? "auto_linear" : "manual")

const SOURCE_STATUS_TEXT = {
    connected: "Connected", initialized: "Connecting", not_initialized: "Not started",
    login_failed: "Login failed", invalid_credentials: "Wrong login", connection_error: "Can't connect",
    invalid_url: "Bad address", not_configured: "Not set up", no_connections: "No one followed",
    multiple_patients_no_match: "Choose a patient", get_connections_failed: "Can't list patients",
    get_glucose_failed: "No readings", get_history_failed: "No history", invalid_response: "Unexpected answer",
    deserialization_error: "Unreadable data",
}

// ---------- units ----------
// Every glucose number is stored in mg/dl; mmol/l is shown with one decimal.
const mgdlToText = (mgdl, units) => {
    if (mgdl === "" || mgdl == null || !isFinite(Number(mgdl))) return mgdl == null ? "" : String(mgdl)
    return units === "mmol" ? (Math.round(Number(mgdl) / 1.8) / 10).toFixed(1) : String(Math.round(Number(mgdl)))
}
function textToMgdl(text, units) {
    const s = String(text).trim()
    if (units === "mmol") return /^\d{1,2}(\.\d)?$/.test(s) ? Math.round(parseFloat(s) * 18) : NaN
    return /^\d{1,3}$/.test(s) ? parseInt(s, 10) : NaN
}
const unitLabel = units => (units === "mmol" ? "mmol/l" : "mg/dl")

// ---------- Nightscout address ----------
function parseNightscoutUrl(url) {
    const m = /^(https?):\/\/([^:/?#]*)(?::([^/?#]*))?/i.exec(String(url || "").trim())
    return m ? { protocol: m[1].toLowerCase(), host: m[2], port: m[3] || "" } : { protocol: "https", host: "", port: "" }
}
const buildNightscoutUrl = ({ protocol, host, port }) => `${protocol}://${host.trim()}${String(port).trim() ? ":" + String(port).trim() : ""}/`

// ---------- checks ----------
const RX = {
    ssid: /^[\x20-\x7E]{1,32}$/,
    wifiPassword: /^.{8,}$/,
    dexcomUsername: /^.{6,}$/,
    password: /^.{8,20}$/,
    nsHostname: /(^(?:[0-9]{1,3}\.){3}[0-9]{1,3}$)|(^(?:[a-z0-9](?:[a-z0-9-]{0,61}[a-z0-9])?\.)+[a-z0-9][a-z0-9-]{0,61}[a-z0-9]$)/,
    nsPort: /(^$)|(.{3,5})/,
    apiSecret: /(^$)|(.{12,})/,
    bgMgdl: /^[3-9][0-9]$|^[1-3][0-9][0-9]$/,
    bgMmol: /^(([2-9])|([1-2][0-9]))(\.[0-9])?$/,
    email: /^[\w-\.]+(\+[A-Za-z0-9]+)?@([\w-]+\.)+[\w-]{2,4}$/,
    timezone: /^.{2,}$/,
    noDataMinutes: /^(?:[6-9]|[1-5][0-9]|60)$/,
    webPassword: /^.{8,64}$/,
}
const isTime = s => /^([01][0-9]|2[0-3]):[0-5][0-9]$/.test(s)
const isInt = v => Number.isInteger(v)
const inOptions = (v, options) => options.some(o => String(o[0]) === String(v))
// A glucose value as typed in the selected units.
const isGlucose = (mgdl, units) => isInt(mgdl) && (units === "mmol" ? RX.bgMmol : RX.bgMgdl).test(mgdlToText(mgdl, units))

function isValidRtttl(text) {
    const parts = String(text || "").trim().split(":")
    if (parts.length !== 3) return false
    const [name, defaults, notes] = parts
    const d = defaults.toLowerCase()
    return /^[a-zA-Z0-9 _-]{1,20}$/.test(name.trim()) && /d=\d+/.test(d) && /o=\d+/.test(d) && /b=\d+/.test(d) &&
        /^[a-grpA-GRP0-9#.,]+$/.test(notes.trim())
}

// Returns {key: message}. ctx: {openNetwork, tzNames}
function validateConfig(c, ctx) {
    const e = {}
    const units = c.units
    const need = (key, ok, message) => { if (!ok && !e[key]) e[key] = message }
    const text = v => (v == null ? "" : String(v))

    // WiFi
    need("ssid", RX.ssid.test(text(c.ssid)), "Valid network name (SSID) is required.")
    if (!ctx.openNetwork) need("password", RX.wifiPassword.test(text(c.password)), "Password is required and must be at least 8 characters long.")

    // Data source
    const src = c.data_source
    need("data_source", src !== "carelink" && SOURCES.some(([v]) => v === src), src === "carelink"
        ? "Medtronic CareLink requires xDrip+ and Nightscout. Select Nightscout after setting up that bridge."
        : "Please select glucose data source.")
    if (src === "nightscout") {
        const ns = parseNightscoutUrl(c.nightscout_url)
        need("ns_host", RX.nsHostname.test(ns.host), "Please enter a valid hostname.")
        need("ns_port", RX.nsPort.test(ns.port), "Port must be either empty or a valid TCP port.")
        need("api_secret", RX.apiSecret.test(text(c.api_secret)), "API secret must be at least 12 characters long.")
    }
    if (src === "dexcom") {
        need("dexcom_username", RX.dexcomUsername.test(text(c.dexcom_username)), "Dexcom username is required (at least 6 characters).")
        need("dexcom_password", RX.password.test(text(c.dexcom_password)), "Dexcom password is required (8 to 20 characters).")
        need("dexcom_server", inOptions(c.dexcom_server, DEXCOM_SERVERS), "Please select Dexcom server.")
    }
    if (src === "librelinkup") {
        need("librelinkup_email", RX.email.test(text(c.librelinkup_email)), "LibreLink Up email is required.")
        need("librelinkup_password", RX.password.test(text(c.librelinkup_password)), "LibreLink Up password is required (8 to 20 characters).")
        need("librelinkup_region", !!c.librelinkup_region, "Please select LibreLink Up server.")
    }
    if (src === "medtrum") {
        need("medtrum_email", RX.email.test(text(c.medtrum_email)), "Medtrum email is required.")
        need("medtrum_password", RX.password.test(text(c.medtrum_password)), "Medtrum password is required (8 to 20 characters).")
    }

    // Glucose
    need("units", inOptions(units, UNITS), "Please select blood glucose units type.")
    for (const k of LIMIT_KEYS) need(k, isGlucose(c[k], units), units === "mmol" ? "Enter 2.0 to 29.9." : "Enter 30 to 399.")
    if (c.custom_nodatatimer_enable) need("custom_nodatatimer", RX.noDataMinutes.test(text(c.custom_nodatatimer)), "A valid time between 6 and 60 minutes is required.")

    // Display
    need("default_face", FACES.some(f => f.id === c.default_face), "Please select default clock face.")
    if (c.face_cycle_enabled) {
        const n = new Set(c.face_cycle_faces || []).size
        need("face_cycle_faces", n >= 2, n === 1 ? "1 face selected. Select one more face before saving." : "0 faces selected. Select at least two faces before saving.")
    }
    if (c.face_schedule_enabled) {
        const rows = c.face_schedule
        need("face_schedule", !c.face_cycle_enabled, "Turn off face cycling before enabling the schedule.")
        need("face_schedule", rows.length >= 1 && rows.length <= 8, "Add between 1 and 8 scheduled times, or turn the schedule off.")
        need("face_schedule", rows.every(row => isTime(row.time)), "Every row needs a valid time.")
        need("face_schedule", new Set(rows.map(row => row.time)).size === rows.length, "Two rows have the same time.")
    }
    need("tz", RX.timezone.test(text(c.tz_libc)) && (!ctx.tzNames || ctx.tzNames.has(c.tz)), "Please select your time zone.")
    need("time_format", inOptions(c.time_format, TIME_FORMATS), "Please select the time format (AM/PM or 24h).")

    // Alarms
    for (const a of ALARMS) {
        if (!c[`alarm_${a.t}_enabled`]) continue
        need(`alarm_${a.t}_value`, isGlucose(c[`alarm_${a.t}_value`], units), `${a.name} alert threshold value is required.`)
        need(`alarm_${a.t}_snooze_interval`, inOptions(c[`alarm_${a.t}_snooze_interval`], SNOOZES), `Please select ${a.name} alert snooze interval.`)
        need(`alarm_${a.t}_melody`, isValidRtttl(c[`alarm_${a.t}_melody`]), `Enter a valid RTTTL string (e.g. ${a.defaultMelody}).`)
        const bad = (c[`alarm_${a.t}_alert_windows`] || []).find(w => !w.days || !w.from || !w.to || w.from === w.to)
        if (bad) {
            need(`alarm_${a.t}_alert_windows`, false, !bad.days ? "Choose at least one day for every alert window."
                : !bad.from || !bad.to ? "Every alert window needs a start and an end time."
                : "An alert window cannot start and end at the same time.")
        }
    }

    // Web interface authentication
    if (c.web_auth_enable) need("web_auth_password", RX.webPassword.test(text(c.web_auth_password)), "Password is required and must be 8 to 64 characters long.")
    return e
}

// The config exactly as it will be posted. Keys this page doesn't know pass through untouched.
function buildSaveJson(c) {
    const out = clone(c)
    out.brightness_mode = brightnessMode(out.brightness_level)
    // Incomplete alert windows are dropped; the firmware ignores them anyway.
    for (const a of ALARMS) {
        const k = `alarm_${a.t}_alert_windows`
        if (Array.isArray(out[k])) out[k] = out[k].filter(w => w && w.days && isTime(w.from) && isTime(w.to) && w.from !== w.to)
    }
    return out
}

// Loaded config -> the form's working copy: numbers as numbers, stored values the firmware would refuse
// replaced by the page's defaults.
function normalizeLoaded(c) {
    const out = clone(c)
    out.face_schedule ??= []
    const num = k => {
        if (typeof out[k] === "string" && /^-?\d+$/.test(out[k].trim())) out[k] = parseInt(out[k], 10)
    }
    ;[...LIMIT_KEYS, "brightness_level", "default_face", "face_cycle_interval_seconds", "alarm_repeat_interval_seconds", "custom_nodatatimer",
        ...ALARMS.flatMap(a => [`alarm_${a.t}_value`, `alarm_${a.t}_snooze_interval`])].forEach(num)
    if (!inOptions(out.face_cycle_interval_seconds, CYCLE_INTERVALS)) out.face_cycle_interval_seconds = 60
    if (!inOptions(out.alarm_repeat_interval_seconds, REPEATS)) out.alarm_repeat_interval_seconds = 300
    const fallbackFace = FACES.some(f => f.id === out.default_face) ? out.default_face : 0
    const faces = Array.isArray(out.face_cycle_faces) ? out.face_cycle_faces : [fallbackFace]
    out.face_cycle_faces = [...new Set(faces.map(Number).filter(id => FACES.some(f => f.id === id)))]
    return out
}

// Which tab a validation key lives on.
function tabOfKey(key) {
    if (/^(ssid|password|additional_|custom_hostname|web_auth)/.test(key)) return "system"
    if (/^alarm_/.test(key)) return "alarms"
    if (/^(data_source|ns_|api_secret|nightscout|dexcom|librelinkup|medtrum|units|low_|high_)/.test(key)) return "glucose"
    return "display"
}

// SHA-1 for Nightscout's api-secret header; crypto.subtle needs HTTPS and the clock serves plain HTTP.
function sha1Hex(text) {
    const bytes = new TextEncoder().encode(text)
    const words = []
    for (let i = 0; i < bytes.length; i++) words[i >> 2] |= bytes[i] << (24 - (i % 4) * 8)
    const bitLen = bytes.length * 8
    words[bitLen >> 5] |= 0x80 << (24 - (bitLen % 32))
    words[(((bitLen + 64) >> 9) << 4) + 15] = bitLen
    let [a, b, c, d, e] = [0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0]
    const rol = (n, s) => (n << s) | (n >>> (32 - s))
    for (let i = 0; i < words.length; i += 16) {
        const w = []
        const [oa, ob, oc, od, oe] = [a, b, c, d, e]
        for (let j = 0; j < 80; j++) {
            w[j] = j < 16 ? words[i + j] | 0 : rol(w[j - 3] ^ w[j - 8] ^ w[j - 14] ^ w[j - 16], 1)
            const f = j < 20 ? (b & c) | (~b & d) : j < 40 ? b ^ c ^ d : j < 60 ? (b & c) | (b & d) | (c & d) : b ^ c ^ d
            const k = j < 20 ? 0x5a827999 : j < 40 ? 0x6ed9eba1 : j < 60 ? 0x8f1bbcdc : 0xca62c1d6
            const t = (rol(a, 5) + f + e + k + w[j]) | 0
            e = d; d = c; c = rol(b, 30); b = a; a = t
        }
        a = (a + oa) | 0; b = (b + ob) | 0; c = (c + oc) | 0; d = (d + od) | 0; e = (e + oe) | 0
    }
    return [a, b, c, d, e].map(n => (n >>> 0).toString(16).padStart(8, "0")).join("")
}
