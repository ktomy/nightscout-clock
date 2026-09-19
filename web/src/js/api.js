// Talking to the clock without overloading it.
//
// The clock's web server runs every handler on one task, and /api/status blocks while it checks the
// internet (up to ~4 s). So the page sends one request at a time, polls status on a timer that starts after
// the previous answer and backs off when the clock doesn't answer, and waits for a restart with version.txt.

const STATUS_EVERY_MS = 15000
const STATUS_MAX_BACKOFF_MS = 60000

const api = (() => {
    const events = emitter()
    let chain = Promise.resolve()

    function enqueue(task) {
        const run = chain.then(task)
        chain = run.catch(() => {})
        return run
    }

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

    function schedulePoll(delay = pollDelay) {
        clearTimeout(pollTimer)
        pollTimer = setTimeout(async () => {
            await pollOnce()
            schedulePoll()
        }, delay)
    }

    function startStatus() {
        pollOnce().then(() => schedulePoll())
        document.addEventListener("visibilitychange", () => {
            if (!document.hidden) schedulePoll(250)
            else clearTimeout(pollTimer)
        })
    }

    // Hold the poll while saving.
    async function exclusive(fn) {
        paused++
        clearTimeout(pollTimer)
        try { return await fn() } finally {
            paused--
            if (!paused) schedulePoll()
        }
    }

    // After /api/reset the clock drops off WiFi for a few seconds, then serves files again.
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

    // ---- endpoints ----
    const authStatus = () => request("GET", "/api/auth/status")
    const login = password => request("POST", "/api/auth/login", { body: { password } })
    const logout = () => request("POST", "/api/auth/logout", { body: {} })
    const loadConfig = () => request("GET", "/config.json")
    const version = () => request("GET", "/version.txt?" + Date.now(), { raw: true }).then(r => (r.ok ? r.text() : ""))
    const timezones = () => request("GET", "/tzdata.json", { raw: true }).then(r => r.json())
    const tryAlarm = rtttl => request("POST", "/api/alarm", { body: { rtttl } })
    const patients = () => request("GET", "/api/llu/patients")
    const reset = () => request("POST", "/api/reset", { body: {}, timeout: 4000 })

    // Save the whole config, then restart the clock so it loads it. In setup mode the clock restarts onto the
    // home WiFi and its setup network goes away, so there is nothing to wait for.
    function saveSettings(config, { setupMode = false, onPhase = () => {} } = {}) {
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
