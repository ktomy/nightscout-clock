// Define setting options, value conversions, and validation independently of DOM rendering.
// Normalize incoming configuration for the form and prepare its outgoing save payload.

/** @typedef {'mgdl' | 'mmol' | ''} GlucoseUnits Empty before units have been selected. */
/** @typedef {'display' | 'glucose' | 'alarms' | 'system'} SettingsTab */
/** @typedef {[string | number, string]} SelectOption A stored value and its visible label. */
/** @typedef {Record<string, string>} ValidationErrors Error messages indexed by setting key. */

/**
 * One daily change of clock face and brightness, using the clock's configured timezone.
 * @typedef {Object} FaceScheduleEntry
 * @property {string} time Start time as HH:MM; may be empty while editing.
 * @property {number} face Registered clock-face ID.
 * @property {number} brightness Manual level 1–10, or automatic mode 100/101.
 */
/**
 * A recurring alarm window; an end earlier than its start crosses midnight.
 * @typedef {Object} AlertWindow
 * @property {string} days Start weekdays as digits, Sunday=0 through Saturday=6.
 * @property {string} from Start time as HH:MM; may be empty while editing.
 * @property {string} to End time as HH:MM; may be empty while editing.
 */
/**
 * Settings loaded from or saved to the clock; numeric fields may hold unfinished input text in the draft.
 * Additional firmware settings pass through the page using their existing JSON keys.
 * @typedef {Record<string, JsonValue> & {
 *   units: GlucoseUnits,
 *   face_schedule_enabled?: boolean,
 *   face_schedule?: FaceScheduleEntry[],
 *   face_cycle_enabled: boolean,
 *   inactive_faces: number[],
 *   brightness_level: number,
 *   default_face: number,
 *   tz: string,
 *   tz_libc: string,
 *   alarm_high_alert_windows: AlertWindow[],
 *   alarm_low_alert_windows: AlertWindow[],
 *   alarm_urgent_low_alert_windows: AlertWindow[]
 * }} ClockConfig
 */
/**
 * Components edited separately in the Nightscout address controls.
 * @typedef {Object} NightscoutAddress
 * @property {string} protocol URL scheme, normally http or https.
 * @property {string} host Hostname or IP address without scheme or port.
 * @property {string} port Port text, or empty to use the scheme's default.
 */
/**
 * One choice in the timezone picker, as supplied by tzdata.json.
 * @typedef {Object} TimezoneEntry
 * @property {string} name Display name, e.g. Europe/Amsterdam.
 * @property {string} value POSIX/libc timezone rule used by the firmware.
 */
/**
 * Shared metadata used to build a high, low, or urgent-low alarm card.
 * @typedef {Object} AlarmDescriptor
 * @property {'high' | 'low' | 'urgent_low'} t Alarm key suffix.
 * @property {string} name Display name.
 * @property {string} compare Label describing the threshold comparison.
 * @property {string} defaultMelody Default RTTTL sound.
 */

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

// The config stores the faces switched off, so a face added later starts active.
const activeFaceIds = inactive => FACES.map(f => f.id).filter(id => !(inactive || []).includes(id))

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

/**
 * Translate firmware brightness levels into UI modes: 100/101 are automatic; other levels are manual.
 * @param {number} level - Firmware brightness level: manual 1–10, automatic 100 or 101.
 * @returns {'auto_dimmed' | 'auto_linear' | 'manual'}
 */
const brightnessMode = level => (level === 101 ? "auto_dimmed" : level === 100 ? "auto_linear" : "manual")

const SOURCE_STATUS_TEXT = {
    connected: "Connected", initialized: "Connecting", not_initialized: "Not started",
    login_failed: "Login failed", invalid_credentials: "Wrong login", connection_error: "Can't connect",
    invalid_url: "Bad address", not_configured: "Not set up", no_connections: "No one followed",
    multiple_patients_no_match: "Choose a patient", get_connections_failed: "Can't list patients",
    get_glucose_failed: "No readings", get_history_failed: "No history", invalid_response: "Unexpected answer",
    deserialization_error: "Unreadable data",
}

/**
 * ---------- units ----------
 * Format stored mg/dl values in the selected units, rounding mmol/l to one decimal place.
 * Keep unfinished input visible instead of replacing it with a number.
 * @param {number | string | null | undefined} mgdl - Stored value or unfinished input.
 * @param {GlucoseUnits} units - Units selected for display.
 * @returns {string}
 */
const mgdlToText = (mgdl, units) => {
    if (mgdl === "" || mgdl == null || !isFinite(Number(mgdl))) return mgdl == null ? "" : String(mgdl)
    return units === "mmol" ? (Math.round(Number(mgdl) / 1.8) / 10).toFixed(1) : String(Math.round(Number(mgdl)))
}
/**
 * Parse input in the selected glucose units into integer mg/dl; return NaN for unsupported formats.
 * @param {string} text - User-entered glucose value.
 * @param {GlucoseUnits} units - Units used by the input.
 * @returns {number} Integer mg/dl, or NaN for invalid input.
 */
function textToMgdl(text, units) {
    const s = String(text).trim()
    if (units === "mmol") return /^\d{1,2}(\.\d)?$/.test(s) ? Math.round(parseFloat(s) * 18) : NaN
    return /^\d{1,3}$/.test(s) ? parseInt(s, 10) : NaN
}
/**
 * Convert the stored units code into the human-readable label displayed beside glucose values.
 * @param {GlucoseUnits} units - Stored units code.
 * @returns {string}
 */
const unitLabel = units => (units === "mmol" ? "mmol/l" : "mg/dl")

/**
 * ---------- Nightscout address ----------
 * Extract protocol, hostname, and port for separate input controls; use empty HTTPS defaults if unmatched.
 * @param {string} url - Saved or pasted Nightscout address.
 * @returns {NightscoutAddress}
 */
function parseNightscoutUrl(url) {
    const m = /^(https?):\/\/([^:/?#]*)(?::([^/?#]*))?/i.exec(String(url || "").trim())
    return m ? { protocol: m[1].toLowerCase(), host: m[2], port: m[3] || "" } : { protocol: "https", host: "", port: "" }
}
/**
 * Combine the URL controls into a trimmed Nightscout address with an optional port and trailing slash.
 * @param {NightscoutAddress} address - Protocol, hostname, and optional port.
 * @returns {string}
 */
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
/**
 * Check that a time uses 24-hour HH:MM format with valid hour and minute ranges.
 * @param {string} s - Time to validate as HH:MM.
 * @returns {boolean}
 */
const isTime = s => /^([01][0-9]|2[0-3]):[0-5][0-9]$/.test(s)
/**
 * Check that a value is an integer number rather than numeric text or a fractional value.
 * @param {unknown} v - Value to check without coercion.
 * @returns {boolean}
 */
const isInt = v => Number.isInteger(v)
/**
 * Check option membership by comparing string values, allowing numeric and string IDs to match.
 * @param {string | number} v - Value to find.
 * @param {SelectOption[]} options - Available value/label pairs.
 * @returns {boolean}
 */
const inOptions = (v, options) => options.some(o => String(o[0]) === String(v))
/**
 * Require integer mg/dl storage and check that its displayed value fits the selected units' input range.
 * @param {number | string} mgdl - Stored value or unfinished input.
 * @param {GlucoseUnits} units - Units whose displayed range must be valid.
 * @returns {boolean}
 */
const isGlucose = (mgdl, units) => isInt(mgdl) && (units === "mmol" ? RX.bgMmol : RX.bgMgdl).test(mgdlToText(mgdl, units))
const isOpenNetwork = c => !String(c.password || "").trim() && !!String(c.ssid || "").trim()
// A settings object such as a face's own settings, as opposed to a list.
const isBlock = v => v !== null && typeof v === "object" && !Array.isArray(v)

/**
 * Check the melody's name, default duration/octave/tempo fields, and allowed note characters.
 * This is a basic RTTTL format check, not a full music parser.
 * @param {string} text - RTTTL melody to validate.
 * @returns {boolean}
 */
function isValidRtttl(text) {
    const parts = String(text || "").trim().split(":")
    if (parts.length !== 3) return false
    const [name, defaults, notes] = parts
    const d = defaults.toLowerCase()
    return /^[a-zA-Z0-9 _-]{1,20}$/.test(name.trim()) && /d=\d+/.test(d) && /o=\d+/.test(d) && /b=\d+/.test(d) &&
        /^[a-grpA-GRP0-9#.,]+$/.test(notes.trim())
}

/**
 * Return errors keyed by setting, validating the selected source and enabled features.
 * Use page context for open WiFi and available timezone choices.
 * @param {ClockConfig} c - Editable settings to validate.
 * @param {FormContext} ctx - Page-only WiFi and timezone context.
 * @returns {ValidationErrors}
 */
function validateConfig(c, ctx) {
    const e = {}
    const units = c.units
    /**
     * Record the first failed check for a field, preserving the most useful error if later checks also fail.
     * @param {string} key - Setting whose error is recorded.
     * @param {boolean} ok - Whether this validation check passed.
     * @param {string} message - Error to record if the check failed.
     * @returns {void}
     */
    const need = (key, ok, message) => { if (!ok && !e[key]) e[key] = message }
    /**
     * Convert a value to text for validation, treating null or missing values as empty input.
     * @param {unknown} v - Value to convert; missing values become empty strings.
     * @returns {string}
     */
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
    const active = activeFaceIds(c.inactive_faces)
    need("inactive_faces", active.length >= 1, "No faces active. Tap at least one face before saving.")
    if (c.face_cycle_enabled) {
        need("inactive_faces", active.length >= 2, "1 face active. Cycling needs at least two.")
    } else if (active.length) {
        need("default_face", active.includes(c.default_face), "Please select default clock face.")
    }
    if (c.face_schedule_enabled) {
        const rows = c.face_schedule
        need("face_schedule", !c.face_cycle_enabled, "Turn off face cycling before enabling the schedule.")
        need("face_schedule", rows.length >= 1 && rows.length <= 8, "Add between 1 and 8 scheduled times, or turn the schedule off.")
        need("face_schedule", rows.every(row => active.includes(row.face)), "Choose an active clock face for every scheduled time.")
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

/**
 * Copy the draft, derive its brightness mode, and omit incomplete alert windows before saving.
 * Preserve other settings, including keys not edited by this page.
 * @param {ClockConfig} c - Configuration to copy and normalize.
 * @returns {ClockConfig}
 */
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

/**
 * Copy incoming settings, convert numeric strings, initialize absent schedules, and normalize cycle options.
 * Supply supported interval defaults and remove duplicate or unknown cycling face IDs.
 * @param {ClockConfig} c - Configuration to copy and normalize.
 * @returns {ClockConfig}
 */
function normalizeLoaded(c) {
    const out = clone(c)
    out.face_schedule ??= []
    /**
     * Convert one integer-string setting to a number in the copied configuration, leaving other values alone.
     * @param {string} k - Key to convert within the copied configuration.
     * @returns {void}
     */
    const num = k => {
        if (typeof out[k] === "string" && /^-?\d+$/.test(out[k].trim())) out[k] = parseInt(out[k], 10)
    }
    ;[...LIMIT_KEYS, "brightness_level", "default_face", "face_cycle_interval_seconds", "alarm_repeat_interval_seconds", "custom_nodatatimer",
        ...ALARMS.flatMap(a => [`alarm_${a.t}_value`, `alarm_${a.t}_snooze_interval`])].forEach(num)
    if (!inOptions(out.face_cycle_interval_seconds, CYCLE_INTERVALS)) out.face_cycle_interval_seconds = 60
    if (!inOptions(out.alarm_repeat_interval_seconds, REPEATS)) out.alarm_repeat_interval_seconds = 300
    const inactive = Array.isArray(out.inactive_faces) ? out.inactive_faces : []
    out.inactive_faces = [...new Set(inactive.map(Number).filter(id => FACES.some(f => f.id === id)))]
    const active = activeFaceIds(out.inactive_faces)
    if (active.length && !active.includes(out.default_face)) out.default_face = active[0]
    return out
}

/**
 * Map a setting key to its tab so dirty badges and validation navigation point to the right section.
 * @param {string} key - Setting or validation key to locate.
 * @returns {SettingsTab}
 */
function tabOfKey(key) {
    if (/^(ssid|password|additional_|custom_hostname|web_auth)/.test(key)) return "system"
    if (/^alarm_/.test(key)) return "alarms"
    if (/^(data_source|ns_|api_secret|nightscout|dexcom|librelinkup|medtrum|units|low_|high_)/.test(key)) return "glucose"
    return "display"
}

// ---------- settings file ----------
// Never taken from a file, so a file can't lock anyone out of the clock.
const NEVER_FROM_FILE = ["web_auth_enable", "web_auth_password"]
// Taken only when asked: another clock's file would move this clock to that network.
const NETWORK_KEYS = ["ssid", "password", "dhcp", "ip", "netmask", "gateway", "dns1", "dns2",
    "additional_wifi_enable", "additional_wifi_type", "additional_ssid", "additional_wifi_username", "additional_wifi_password"]
// The list the page offers for each setting that is picked from one.
const KEY_OPTIONS = {
    data_source: SOURCES, dexcom_server: DEXCOM_SERVERS, librelinkup_region: LLU_REGIONS, units: UNITS, data_old_color: OLD_DATA_COLORS,
    face_cycle_interval_seconds: CYCLE_INTERVALS, time_format: TIME_FORMATS, alarm_repeat_interval_seconds: REPEATS, additional_wifi_type: WIFI_TYPES,
    default_face: FACES.map(f => [f.id]), inactive_faces: FACES.map(f => [f.id]),
    brightness_level: [...Array.from({ length: 10 }, (_, i) => [i + 1]), ...BRIGHTNESS_MODES.filter(m => m[2] != null).map(m => [m[2]])],
    ...Object.fromEntries(ALARMS.map(a => [`alarm_${a.t}_snooze_interval`, SNOOZES])),
}
// Alert windows, as the day buttons and time inputs write them; the clock's own list is often empty.
const KEY_ITEMS = Object.fromEntries(ALARMS.map(a => [`alarm_${a.t}_alert_windows`,
    w => isBlock(w) && typeof w.days === "string" && /^[0-6]+$/.test(w.days) && isTime(w.from) && isTime(w.to)]))

KEY_ITEMS.face_schedule = row => isBlock(row) && isTime(row.time) && isInt(row.face)
    && inOptions(row.face, KEY_OPTIONS.default_face) && isInt(row.brightness)
    && inOptions(row.brightness, KEY_OPTIONS.brightness_level)

// Whether a value from a file has the shape of the clock's own value and, for a list, is one of its options.
// A block's settings take their options from "<key>.<setting>".
function fitsSetting(key, value, clockValue) {
    const options = KEY_OPTIONS[key]
    if (typeof clockValue === "number") return (isInt(value) || /^\d+$/.test(typeof value === "string" ? value.trim() : "")) && (!options || inOptions(value, options))
    if (Array.isArray(clockValue)) {
        if (!Array.isArray(value)) return false
        if (KEY_ITEMS[key]) return value.every(KEY_ITEMS[key])
        // Items shaped like the clock's first item; an empty list takes numbers or blocks, and the checks decide.
        const [sample] = clockValue
        return value.every(item => sample === undefined ? (isInt(item) && (!options || inOptions(item, options))) || isBlock(item)
            : typeof item === typeof sample && fitsSetting(key, item, sample) && (!isBlock(sample) || Object.keys(item).length === Object.keys(sample).length))
    }
    // A block: only settings the clock has, each of the clock's type.
    if (isBlock(clockValue)) {
        return isBlock(value) && Object.keys(value).every(p => p in clockValue && typeof value[p] === typeof clockValue[p] && fitsSetting(`${key}.${p}`, value[p], clockValue[p]))
    }
    return clockValue !== null && typeof value === typeof clockValue && (!options || value === "" || inOptions(value, options))
}

// The clock's settings with a file's values applied. A value that doesn't fit, or that the checks run before a save
// reject, keeps the clock's value and is listed in `kept`. Keys the clock doesn't have are ignored.
function mergeSettingsFile(file, clock, { network, tzNames }) {
    const config = clone(clock)
    const taken = [], kept = []
    for (const key of Object.keys(clock)) {
        if (!(key in file) || NEVER_FROM_FILE.includes(key) || (!network && NETWORK_KEYS.includes(key))) continue
        // A number in the file for a text setting (an older page wrote time_format as 24) is read as that text.
        const value = typeof clock[key] === "string" && typeof file[key] === "number" ? String(file[key]) : file[key]
        if (sameJson(value, clock[key])) continue
        if (fitsSetting(key, value, clock[key])) {
            config[key] = isBlock(clock[key]) ? { ...clone(clock[key]), ...clone(file[key]) } : clone(value)
            taken.push(key)
        } else {
            kept.push(key)
        }
    }
    // Alarms and the no-data timer are checked as if switched on, so a file can't carry a bad value in one that is off.
    const allOn = { custom_nodatatimer_enable: true, ...Object.fromEntries(ALARMS.map(a => [`alarm_${a.t}_enabled`, true])) }
    const errorKeys = key => (key === "nightscout_url" ? ["ns_host", "ns_port"] : key === "tz_libc" ? ["tz"] : [key])
    // A block's checks report as "<key>_<setting>".
    const hasError = (errors, key) => errorKeys(key).some(k => errors[k] || (isBlock(clock[key]) && Object.keys(errors).some(e => e.startsWith(`${k}_`))))
    for (;;) {
        const c = normalizeLoaded(config)
        const errors = validateConfig({ ...c, ...allOn }, { openNetwork: isOpenNetwork(c), tzNames })
        const rejected = taken.filter(key => !kept.includes(key) && hasError(errors, key))
        if (!rejected.length) return { config: c, kept }
        rejected.forEach(key => { config[key] = clone(clock[key]); kept.push(key) })
    }
}

// SHA-1 for Nightscout's api-secret header; crypto.subtle needs HTTPS and the clock serves plain HTTP.
/**
 * Hash a UTF-8 string into SHA-1 hex for Nightscout's api-secret header.
 * Implement SHA-1 locally because crypto.subtle is unavailable on the clock's plain HTTP page.
 * @param {string} text - API secret to hash as UTF-8.
 * @returns {string}
 */
function sha1Hex(text) {
    const bytes = new TextEncoder().encode(text)
    const words = []
    for (let i = 0; i < bytes.length; i++) words[i >> 2] |= bytes[i] << (24 - (i % 4) * 8)
    const bitLen = bytes.length * 8
    words[bitLen >> 5] |= 0x80 << (24 - (bitLen % 32))
    words[(((bitLen + 64) >> 9) << 4) + 15] = bitLen
    let [a, b, c, d, e] = [0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0]
    /**
     * Rotate a 32-bit word left, wrapping shifted bits around for the SHA-1 rounds.
     * @param {number} n - 32-bit word to rotate.
     * @param {number} s - Number of bit positions.
     * @returns {number}
     */
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
