// Page start-up, the header, login, and Save.

function setState(state) { document.body.dataset.state = state }

// ---------- header ----------
function pill(id, dot, value) {
    const p = $(`#${id}`)
    $(".dot", p).className = `dot ${dot}`
    $("b", p).textContent = value
}

function renderStatus(s) {
    const wasLlu = ui.status && ui.status.bgSource === "LIBRELINKUP"
    ui.status = s
    pill("pill_wifi", s.isInAPMode ? "warn" : s.isConnected ? "ok" : "bad", s.isInAPMode ? "Initial mode" : s.isConnected ? "Connected" : "Not connected")
    pill("pill_internet", s.hasInternet ? "ok" : "bad", s.hasInternet ? "Yes" : "No")
    const st = s.bgSourceStatus
    const text = st === "connected" ? "Connected" : st === "initialized" ? (s.isInAPMode ? "Initial mode" : "Connecting") : `Error: ${SOURCE_STATUS_TEXT[st] || st || "unknown"}`
    pill("pill_source", st === "connected" ? "ok" : st === "initialized" ? "warn" : "bad", text)
    const units = form.get("units")
    pill("pill_reading", s.sgv ? "info" : "", s.sgv ? `${mgdlToText(s.sgv, units)} ${unitLabel(units)}` : "–")
    $("#clock_sub").textContent = `${ui.versions.current ? "v" + ui.versions.current + " · " : ""}${location.host}`
    if (s.bgSource === "LIBRELINKUP" && !wasLlu) form.setCtx("status", s.bgSource)
}

function renderStatusError() {
    ["pill_wifi", "pill_internet", "pill_source", "pill_reading"].forEach(id => pill(id, "", "Unknown"))
    $("#clock_sub").textContent = `${location.host} · not answering`
}

// ---------- save ----------
function renderDirty() {
    const changes = form.changes()
    const n = changes.length
    $("#dirty_text").textContent = n ? `${n} unsaved change${n > 1 ? "s" : ""}` : "No unsaved changes"
    $("#discard").hidden = !n
    $$("[data-tab]").forEach(b => {
        const dirty = changes.some(k => tabOfKey(k) === b.dataset.tab)
        const mark = $(".badge-dirty", b)
        if (dirty && !mark) b.append(el("span.badge-dirty", { "aria-label": "has unsaved changes" }))
        if (!dirty && mark) mark.remove()
    })
}

function overlay(title, text) {
    $("#overlay").hidden = !title
    if (!title) return
    $("#overlay_title").textContent = title
    $("#overlay_text").textContent = text || ""
}

async function save() {
    form.showAllErrors()
    applyErrors()
    const keys = Object.keys(form.errors)
    if (keys.length) {
        const first = keys[0]
        showTab(tabOfKey(first))
        const box = $(`#tab_${tabOfKey(first)} [data-field="${first}"]`)
        if (box) {
            box.scrollIntoView({ block: "center" })
            const control = $("input,select,button", box)
            if (control) control.focus({ preventScroll: true })
        }
        const msg = form.errors[first]
        return toast(keys.length === 1 ? msg : `${keys.length} settings need attention. First: ${msg}`, "bad", 7000)
    }
    const phases = {
        saving: ["Saving", "Sending the settings to the clock…"],
        restarting: ["Restarting", "The clock is restarting to apply the changes."],
    }
    const result = await api.saveSettings(form.saveJson(), {
        setupMode: !!(ui.status && ui.status.isInAPMode),
        onPhase: (phase, seconds) => overlay(phases[phase][0], phases[phase][1] + (seconds ? ` ${seconds} s` : "")),
    })
    if (!result.ok) {
        overlay(null)
        if (result.locked) showLock("Your login expired. Unlock and save again; your changes are still here.")
        return toast(result.error, "bad", 9000)
    }
    if (result.setupMode) {
        form.load(form.saveJson())
        overlay("Saved", `The clock is restarting and will join "${form.get("ssid")}". Connect this phone to that WiFi network. If the clock can't join it, its setup network comes back.`)
        $("#overlay .spinner").hidden = true
        return
    }
    await reloadAfterSave()
    overlay(null)
}

async function reloadAfterSave() {
    const auth = await api.authStatus().catch(() => null)
    if (auth && auth.ok && auth.data.enabled && !auth.data.authenticated) {
        form.load(form.saveJson())
        return showLock("Saved. The clock restarted; unlock to continue.")
    }
    const r = await api.loadConfig().catch(() => null)
    if (!(r && r.ok)) return toast("Saved and restarted, but the settings couldn't be read back. Reload the page.", "warn", 8000)
    form.load(r.data)
    ui.patients = null
    renderAll()
    toast("Saved. The clock restarted with the new settings.")
}

// ---------- login ----------
function showLock(message) {
    setState("locked")
    for (const id of ["#app", "#savebar", "#loading_screen"]) $(id).hidden = true
    $("#lock_screen").hidden = false
    $("#lock_message").textContent = message || "Authentication is enabled. Log in to change settings."
    $("#lock_password").value = ""
}

async function unlock(e) {
    e.preventDefault()
    const pw = $("#lock_password").value
    if (!pw) { $("#lock_message").textContent = "Password is required to unlock."; return }
    $("#lock_submit").disabled = true
    try {
        const r = await api.login(pw)
        if (r.ok && r.data && (r.data.status === "ok" || r.data.status === "disabled")) {
            $("#lock_screen").hidden = true
            $("#lock_btn").hidden = r.data.status !== "ok"
            if (form.loaded) showApp()
            else await loadSettings()
        } else {
            $("#lock_message").textContent = "Invalid credentials."
        }
    } catch (err) {
        $("#lock_message").textContent = err.message
    } finally {
        $("#lock_submit").disabled = false
    }
}

async function lock() {
    try { await api.logout() } catch (e) { /* the clock may be restarting */ }
    location.reload()
}

// ---------- start ----------
function showApp() {
    setState("ready")
    $("#loading_screen").hidden = true
    $("#lock_screen").hidden = true
    $("#app").hidden = false
    $("#savebar").hidden = false
}

async function loadSettings() {
    setState("loading")
    const r = await api.loadConfig().catch(e => ({ ok: false, error: e }))
    if (r.status === 401) return showLock()
    if (!r.ok) {
        $("#loading_text").textContent = r.error ? r.error.message : `The clock answered ${r.status}.`
        $("#loading_retry").hidden = false
        return setState("error")
    }
    form.load(r.data)
    renderAll()
    renderDirty()
    // First setup (no WiFi yet) starts where the WiFi settings are.
    if (!String(r.data.ssid || "").trim()) ui.tab = "system"
    try { const t = sessionStorage.getItem("tab"); if (TABS[t]) ui.tab = t } catch (e) { /* private mode */ }
    showTab(ui.tab)
    showApp()
    // Background loads, after the form is usable.
    await loadTimezones()
    loadVersions()
}

async function loadTimezones() {
    try {
        const list = await api.timezones()
        ui.timezones = list
        ui.timezoneNames = new Set(list.map(z => z.name))
        const zone = Intl.DateTimeFormat().resolvedOptions().timeZone
        if (!form.get("tz") && ui.timezoneNames.has(zone)) {
            // No zone saved yet: use this browser's (it shows as an unsaved change).
            form.set("tz_libc", list.find(z => z.name === zone).value)
            form.set("tz", zone)
        } else if (ui.timezoneNames.has(form.get("tz"))) {
            // Saving writes the list's rule for the zone, as the zone list may carry a corrected one.
            const libc = list.find(z => z.name === form.get("tz")).value
            if (libc !== form.get("tz_libc")) form.set("tz_libc", libc)
        }
    } catch (e) {
        ui.timezones = []
        ui.timezoneNames = new Set(form.get("tz") ? [form.get("tz")] : [])
    }
    form.setCtx("tzNames", ui.timezoneNames)
}

function compareVersions(a, b) {
    const pa = String(a).split(".").map(Number), pb = String(b).split(".").map(Number)
    for (let i = 0; i < Math.max(pa.length, pb.length); i++) if ((pa[i] || 0) !== (pb[i] || 0)) return (pa[i] || 0) - (pb[i] || 0)
    return 0
}

// The latest version comes from GitHub when this browser has internet; without it (setup mode) the check
// gives up after a few seconds and says so.
async function loadVersions() {
    const v = ui.versions
    v.current = (await api.version().catch(() => "")).trim()
    const ctrl = new AbortController()
    const timer = setTimeout(() => ctrl.abort(), 5000)
    try {
        const res = await fetch("https://raw.githubusercontent.com/ktomy/nightscout-clock/refs/heads/main/data/version.txt?" + Date.now(), { cache: "no-store", signal: ctrl.signal })
        v.latest = res.ok ? (await res.text()).trim() : ""
    } catch (e) {
        v.latest = ""
    } finally {
        clearTimeout(timer)
    }
    v.update = !!(v.latest && v.current && compareVersions(v.current, v.latest) < 0)
    v.status = !v.current ? "Could not read the current version." : !v.latest ? "Could not check for updates." : v.update ? "" : "You are using the latest version."
    const set = (id, text) => { const n = $(id); if (n) n.textContent = text }
    set("#fw_current", v.current || "unknown")
    set("#fw_latest", v.latest || "unknown")
    const status = $("#fw_status")
    if (status) status.replaceChildren(...versionStatusNodes())
}

async function start() {
    $$("[data-tab]").forEach(b => b.addEventListener("click", () => {
        showTab(b.dataset.tab)
        window.scrollTo({ top: 0 })
    }))
    const lockInput = $("#lock_password")
    lockInput.parentNode.insertBefore(passwordGroup(lockInput), null)
    $$("[data-icon]").forEach(n => n.prepend(icon(n.dataset.icon)))
    $("#save").addEventListener("click", save)
    $("#discard").addEventListener("click", () => { form.discard(); renderAll() })
    $("#lock_form").addEventListener("submit", unlock)
    $("#lock_btn").addEventListener("click", lock)
    $("#loading_retry").addEventListener("click", () => location.reload())

    form.on("change", renderDirty)
    form.on("load", renderDirty)
    form.on("errors", () => applyErrors())
    api.on("status", renderStatus)
    api.on("status-error", renderStatusError)
    api.on("locked", () => { if (document.body.dataset.state === "ready") showLock("Your login expired. Unlock to continue; your changes are still here.") })
    window.addEventListener("beforeunload", e => { if (form.loaded && form.changes().length && document.body.dataset.state === "ready") { e.preventDefault(); e.returnValue = "" } })

    // Status can take seconds while the clock checks the internet, so it starts after the settings. It needs
    // no login, so the badges work on the lock screen too.
    const auth = await api.authStatus().catch(() => null)
    if (auth && auth.ok && auth.data) {
        $("#lock_btn").hidden = !(auth.data.enabled && auth.data.authenticated)
        if (auth.data.enabled && !auth.data.authenticated) {
            showLock()
            return api.startStatus()
        }
    }
    await loadSettings()
    api.startStatus()
}

start()
