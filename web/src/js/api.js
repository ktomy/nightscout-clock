// Functions for interacting with the clock: reading status and settings, authentication,
// alarm previews, and saving settings and waiting for the clock to restart.

/**
 * Parsed clock response; non-JSON bodies are returned as text and empty bodies as null.
 * @template [T=JsonValue]
 * @typedef {Object} ApiReply
 * @property {boolean} ok Whether the HTTP status is successful.
 * @property {number} status HTTP status code.
 * @property {T | {text: string} | null} data Parsed response body, text fallback, or no body.
 */
/**
 * Options for a queued HTTP request to the clock.
 * @typedef {Object} RequestOptions
 * @property {JsonValue} [body] JSON payload, or an already serialized string.
 * @property {number} [timeout=8000] Request timeout in milliseconds after it leaves the queue.
 * @property {boolean} [raw=false] Return the fetch Response instead of parsing its body.
 */
/**
 * Connection and glucose state returned by /api/status.
 * @typedef {Object} ClockStatus
 * @property {boolean} isConnected Whether the clock is connected to WiFi.
 * @property {boolean} hasInternet Whether its connectivity probe succeeded.
 * @property {boolean} isInAPMode Whether the clock is providing its setup WiFi network.
 * @property {string} bgSource Active firmware source name, such as LIBRELINKUP.
 * @property {string} bgSourceStatus Source state/error code, such as connected or initialized.
 * @property {number} sgv Latest glucose in mg/dl, or zero when no reading is available.
 */
/**
 * Password protection and session state reported by the clock.
 * @typedef {Object} AuthStatus
 * @property {boolean} enabled Whether web password protection is enabled.
 * @property {boolean} authenticated Whether this browser session is logged in.
 */
/**
 * Result body for clock actions such as login, save, or alarm preview.
 * @typedef {Object} ActionStatus
 * @property {string} status Outcome code such as ok or disabled.
 * @property {string} [error] Optional explanation when the action fails.
 */
/**
 * A followed LibreLinkUp patient offered by the clock.
 * @typedef {Object} PatientEntry
 * @property {string} patientId Identifier stored when selecting this patient.
 * @property {string} firstName First name for the picker label.
 * @property {string} lastName Last name for the picker label.
 */
/**
 * Timing and progress options for detecting when the clock returns after a restart.
 * @typedef {Object} RestartWaitOptions
 * @property {number} [firstWaitMs=4000] Initial delay before probing.
 * @property {number} [everyMs=2000] Delay between unsuccessful probes.
 * @property {number} [giveUpMs=90000] Elapsed-time limit checked before starting each probe.
 * @property {(seconds: number) => void} [onTick] Callback receiving elapsed seconds before a probe.
 */
/**
 * Options controlling save/restart behavior and progress display.
 * @typedef {Object} SaveOptions
 * @property {boolean} [restart=true] Restart after saving settings that require it.
 * @property {boolean} [setupMode=false] Skip reconnect probing when moving off the setup WiFi network.
 * @property {(phase: 'saving' | 'restarting', seconds?: number) => void} [onPhase] Progress callback.
 */
/**
 * Save/restart outcome returned to the page; success does not include reloading the configuration.
 * @typedef {Object} SaveResult
 * @property {boolean} ok Whether saving and the applicable restart wait succeeded.
 * @property {boolean} [setupMode] True when the browser must join the newly configured WiFi network.
 * @property {boolean} [locked] True when saving was rejected because authentication is required.
 * @property {string} [error] Explanation suitable for display when the operation fails.
 */

const STATUS_EVERY_MS = 15000
const STATUS_MAX_BACKOFF_MS = 60000

// Create the shared clock client with a private request queue, polling state, and event subscriptions.
const api = (() => {
    const events = emitter()
    let chain = Promise.resolve()

    /**
     * Serialize clock requests through a promise chain; a failed request does not block later ones.
     * The status endpoint checks internet connectivity synchronously and can delay other requests.
     * @template T
     * @param {() => Promise<T>} task - Request operation to run after earlier requests.
     * @returns {Promise<T>}
     */
    function enqueue(task) {
        const run = chain.then(task)
        chain = run.catch(() => {})
        return run
    }

    /**
     * Queue a fetch with a timeout and optional JSON body; parse the reply unless raw is requested.
     * Notify the UI on HTTP 401 and turn network failures into readable errors.
     * @param {string} method - HTTP method.
     * @param {string} path - Device-relative request path.
     * @param {RequestOptions} [options={}] - Body, timeout, and raw-response options.
     * @returns {Promise<ApiReply | Response>}
     */
    function request(method, path, { body, timeout = 8000, raw = false } = {}) {
        return enqueue(async () => {
            const ctrl = new AbortController()
            const timer = setTimeout(() => ctrl.abort(), timeout)
            try {
                const init = { method, signal: ctrl.signal, credentials: "same-origin", cache: "no-cache" }
                if (body !== undefined) {
                    init.headers = { "Content-Type": "application/json" }
                    init.body = typeof body === "string" ? body : JSON.stringify(body)
                }
                const res = await fetch(path, init)
                if (res.status === 401) events.emit("locked")
                if (raw) return res
                const text = await res.text()
                let data = null
                try { data = text ? JSON.parse(text) : null } catch (e) { data = { text } }
                return { ok: res.ok, status: res.status, data }
            } catch (e) {
                throw new Error(e.name === "AbortError" ? "The clock did not answer in time." : "Could not reach the clock.")
            } finally {
                clearTimeout(timer)
            }
        })
    }

    // ---- status poll ----
    let pollTimer = null, pollDelay = STATUS_EVERY_MS, paused = 0, polling = false

    /**
     * Fetch status unless polling is paused, already running, or the page is hidden.
     * Publish successful replies; double the retry delay after network failures, up to 60 seconds.
     * @returns {Promise<void>}
     */
    async function pollOnce() {
        if (polling || paused || document.hidden) return
        polling = true
        try {
            const r = await request("GET", "/api/status", { timeout: 7000 })
            if (r.ok && r.data) {
                pollDelay = STATUS_EVERY_MS
                events.emit("status", r.data)
            }
        } catch (e) {
            pollDelay = Math.min(pollDelay * 2, STATUS_MAX_BACKOFF_MS)
            events.emit("status-error", e)
        } finally {
            polling = false
        }
    }

    /**
     * Schedule the next status request after a delay, then schedule again only once that poll finishes.
     * @param {number} [delay] - Delay in milliseconds; defaults to the current retry delay.
     * @returns {void}
     */
    function schedulePoll(delay = pollDelay) {
        clearTimeout(pollTimer)
        pollTimer = setTimeout(async () => {
            await pollOnce()
            schedulePoll()
        }, delay)
    }

    /**
     * Start status polling and watch tab visibility to pause hidden pages and refresh on return.
     * @returns {void}
     */
    function startStatus() {
        pollOnce().then(() => schedulePoll())
        // Stop the scheduled poll when hidden; request fresh status shortly after the user returns.
        document.addEventListener("visibilitychange", () => {
            if (!document.hidden) schedulePoll(250)
            else clearTimeout(pollTimer)
        })
    }

    /**
     * Pause status polling while an operation runs, then resume it even if the operation fails.
     * @template T
     * @param {() => Promise<T>} fn - Operation to run with status polling paused.
     * @returns {Promise<T>}
     */
    async function exclusive(fn) {
        paused++
        clearTimeout(pollTimer)
        try { return await fn() } finally {
            paused--
            if (!paused) schedulePoll()
        }
    }

    /**
     * After a restart, wait four seconds and probe version.txt until the clock responds or time runs out.
     * Report elapsed seconds through onTick so the UI can show restart progress.
     * @param {RestartWaitOptions} [options={}] - Polling delays, deadline, and progress callback.
     * @returns {Promise<boolean>}
     */
    async function waitForClock({ firstWaitMs = 4000, everyMs = 2000, giveUpMs = 90000, onTick } = {}) {
        const start = Date.now()
        await sleep(firstWaitMs)
        while (Date.now() - start < giveUpMs) {
            onTick && onTick(Math.round((Date.now() - start) / 1000))
            try {
                const r = await request("GET", "/version.txt?" + Date.now(), { timeout: 2500, raw: true })
                if (r.ok) return true
            } catch (e) { /* still restarting */ }
            await sleep(everyMs)
        }
        return false
    }

    /**
     * Ask the clock whether password protection is enabled and the current browser session is authenticated.
     * @returns {Promise<ApiReply<AuthStatus>>}
     */
    const authStatus = () => request("GET", "/api/auth/status")
    /**
     * Submit the password to establish an authenticated browser session with the clock.
     * @param {string} password - Password entered on the login screen.
     * @returns {Promise<ApiReply<ActionStatus>>}
     */
    const login = password => request("POST", "/api/auth/login", { body: { password } })
    /**
     * Ask the clock to end the current authenticated browser session.
     * @returns {Promise<ApiReply<ActionStatus>>}
     */
    const logout = () => request("POST", "/api/auth/logout", { body: {} })
    /**
     * Read the clock configuration that will become the form's original and editable settings.
     * @returns {Promise<ApiReply<ClockConfig>>}
     */
    const loadConfig = () => request("GET", "/config.json")
    /**
     * Read the installed firmware version as text, using a timestamp to avoid a cached response.
     * @returns {Promise<string>}
     */
    const version = () => request("GET", "/version.txt?" + Date.now(), { raw: true }).then(r => (r.ok ? r.text() : ""))
    /**
     * Load the timezone name/rule pairs as JSON for the timezone picker.
     * @returns {Promise<TimezoneEntry[]>}
     */
    const timezones = () => request("GET", "/tzdata.json", { raw: true }).then(r => r.json())
    /**
     * Send an RTTTL melody to the clock for immediate playback without saving settings.
     * @param {string} rtttl - Melody to play on the clock.
     * @returns {Promise<ApiReply<ActionStatus>>}
     */
    const tryAlarm = rtttl => request("POST", "/api/alarm", { body: { rtttl } })
    /**
     * Fetch LibreLinkUp patient choices from the clock's active connection.
     * @returns {Promise<ApiReply<PatientEntry[]>>}
     */
    const patients = () => request("GET", "/api/llu/patients")
    /**
     * Request a clock restart; the connection may close before a response arrives.
     * @returns {Promise<ApiReply<ActionStatus>>}
     */
    const reset = () => request("POST", "/api/reset", { body: {}, timeout: 4000 })

    /**
     * Pause polling, save the configuration, optionally restart the clock, and report progress or failure.
     * Wait for it to return unless initial setup moves it onto a different WiFi network.
     * @param {ClockConfig} config - Configuration payload to save.
     * @param {SaveOptions} [options={}] - Initial-setup flag and progress callback.
     * @returns {Promise<SaveResult>}
     */
    function saveSettings(config, { restart = true, setupMode = false, onPhase = () => {} } = {}) {
        return exclusive(async () => {
            onPhase("saving")
            try {
                const r = await request("POST", "/api/save", { body: config, timeout: 10000 })
                if (r.status === 401) return { ok: false, locked: true, error: "Your login has expired. Unlock settings and save again." }
                if (!r.ok || !r.data || r.data.status !== "ok") {
                    const reason = (r.data && (r.data.error || r.data.status)) || `HTTP ${r.status}`
                    return { ok: false, error: `The clock refused the settings: ${reason}` }
                }
            } catch (e) {
                return { ok: false, error: `${e.message} The settings may not have been saved; save again.` }
            }
            if (!restart) return { ok: true }
            onPhase("restarting")
            try { await reset() } catch (e) { /* the clock may restart before it answers */ }
            if (setupMode) return { ok: true, setupMode: true }
            if (!(await waitForClock({ onTick: s => onPhase("restarting", s) }))) {
                return { ok: false, error: "Saved, but the clock has not come back after 90 seconds. It may have joined a different network." }
            }
            return { ok: true }
        })
    }

    return { on: events.on, startStatus, saveSettings, authStatus, login, logout, loadConfig, version, timezones, tryAlarm, patients }
})()
