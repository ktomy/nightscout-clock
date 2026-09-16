// The clock preview: the firmware's display code, compiled to WebAssembly by web/emulator/build.sh, runs in
// this browser with the settings on the page, unsaved changes included. The clock only serves the file.
// It loads after the settings are usable, and the page works without it.

const preview = (() => {
    const W = 32, H = 8, CELL = 12
    const ROOM_LUX = 40
    // History slope in mg/dl per 5 minutes for each trend arrow.
    const SLOPES = { 1: 15, 2: 10, 3: 5, 4: 0, 5: -5, 6: -10, 7: -15 }
    let createClock = null
    let clock = null
    let bootSeq = 0, bootTimer = null, dirty = true
    let timer = null, onScreen = false
    let lastLoop = 0, lastMinute = -1, lastFrame = ""
    let viewedFace = null

    // The clock's UTC offset in its own time zone; the firmware is given an offset instead of a POSIX rule.
    function tzOffset(epoch) {
        const minute = epoch - (epoch % 60)
        try {
            const parts = new Intl.DateTimeFormat("en-US", { timeZone: form.get("tz") || undefined, hourCycle: "h23",
                year: "numeric", month: "numeric", day: "numeric", hour: "numeric", minute: "numeric" }).formatToParts(new Date(minute * 1000))
            const p = Object.fromEntries(parts.map(x => [x.type, Number(x.value)]))
            return Date.UTC(p.year, p.month - 1, p.day, p.hour, p.minute) / 1000 - minute
        } catch (e) {
            return -new Date(minute * 1000).getTimezoneOffset() * 60
        }
    }

    // Three hours of readings ending in the chosen one, as a data source would deliver them.
    function showReading(now) {
        const bg = Number($("#preview_bg").value), trend = Number($("#preview_trend").value), age = $("#preview_age").value
        const rows = []
        if (age !== "none") {
            for (let k = 35; k >= 0; k--) rows.push([Math.min(400, Math.max(40, bg - SLOPES[trend] * k)), trend, now - Number(age) * 60 - k * 300])
        }
        clock.ccall("emu_show_readings", null, ["string"], [JSON.stringify(rows)])
    }

    // A dim LED is still visible on the clock, so low values are lifted to show on a screen.
    const lift = c => (c ? Math.round(255 * (0.22 + 0.78 * Math.sqrt(c / 255))) : 0)

    function draw() {
        const ptr = clock._emu_wire()
        const bytes = clock.HEAPU8.slice(ptr, ptr + W * H * 3)
        const frame = bytes.join()
        if (frame === lastFrame) return
        lastFrame = frame
        const ctx = $("#preview_canvas").getContext("2d")
        ctx.fillStyle = "#050505"
        ctx.fillRect(0, 0, W * CELL, H * CELL)
        for (let y = 0; y < H; y++) {
            for (let x = 0; x < W; x++) {
                const i = clock._emu_xy(x, y) * 3
                const [r, g, b] = [bytes[i], bytes[i + 1], bytes[i + 2]]
                ctx.fillStyle = r || g || b ? `rgb(${lift(r)},${lift(g)},${lift(b)})` : "#1a1c1e"
                ctx.beginPath()
                ctx.arc(x * CELL + CELL / 2, y * CELL + CELL / 2, CELL * 0.38, 0, Math.PI * 2)
                ctx.fill()
            }
        }
        const face = FACES.find(f => f.id === clock._emu_get_face())
        $("#preview_face").textContent = face ? face.name : ""
    }

    // One pass of the firmware's loop per second while the preview is on screen; nothing runs otherwise.
    function tick() {
        timer = null
        if (!clock || !onScreen || document.hidden) return
        const now = Math.floor(Date.now() / 1000), ms = performance.now()
        clock._emu_set_clock(now, tzOffset(now))
        // Re-sent every minute so the reading keeps its chosen age.
        if (Math.floor(now / 60) !== lastMinute) {
            lastMinute = Math.floor(now / 60)
            showReading(now)
        }
        clock._emu_loop(ms - lastLoop)
        lastLoop = ms
        draw()
        timer = setTimeout(tick, 1000)
    }

    // A fresh emulated clock for every settings change, as the device restarts after a save.
    async function boot() {
        const seq = ++bootSeq
        let next
        try {
            next = await createClock({ print() {}, printErr() {} })
            const now = Math.floor(Date.now() / 1000)
            next._emu_set_clock(now, tzOffset(now))
            if (!next.ccall("emu_boot", "number", ["string", "number"], [JSON.stringify(form.saveJson()), ROOM_LUX])) throw new Error("boot")
        } catch (e) {
            if (seq === bootSeq) $("#preview_note").textContent = "The preview can't show these settings."
            return resume()
        }
        if (seq !== bootSeq) return
        clock = next
        if (viewedFace != null) clock._emu_set_face(viewedFace)
        $("#preview_note").textContent = "Drawn by the clock's own code with the settings on this page, unsaved changes included."
        lastLoop = performance.now()
        lastMinute = -1
        lastFrame = ""
        clearTimeout(timer)
        tick()
    }

    function resume() {
        if (!createClock || !onScreen || document.hidden) return
        if (dirty) {
            dirty = false
            return boot()
        }
        if (clock && !timer) tick()
    }

    function changed() {
        dirty = true
        clearTimeout(bootTimer)
        bootTimer = setTimeout(resume, 150)
    }

    function showBg() {
        const units = form.get("units")
        $("#preview_bg_out").textContent = `${mgdlToText(Number($("#preview_bg").value), units)} ${unitLabel(units)}`
    }

    function tryReading() {
        showBg()
        if (!clock) return
        showReading(Math.floor(Date.now() / 1000))
        draw()
    }

    let started = false
    async function start() {
        if (started) return
        started = true
        try {
            await new Promise((resolve, reject) => document.head.append(el("script", { src: "clockemu.js", onload: resolve, onerror: reject })))
            createClock = window.createClockEmu
            if (!createClock) throw new Error("no emulator")
        } catch (e) {
            $("#preview_note").textContent = "The clock preview is not available."
            return
        }
        $("#preview_controls").hidden = false
        showBg()
        $$("[data-button]").forEach(b => b.addEventListener("click", () => {
            if (!clock) return
            clock._emu_button(Number(b.dataset.button))
            viewedFace = clock._emu_get_face()
            draw()
        }))
        $("#preview_bg").addEventListener("input", tryReading)
        $("#preview_trend").addEventListener("change", tryReading)
        $("#preview_age").addEventListener("change", tryReading)
        form.on("change", key => {
            if (key.startsWith("ctx.")) return
            // A setting that chooses faces, not a face's own settings object, lets the clock pick the face again.
            const value = form.get(key)
            if (key.includes("face") && (Array.isArray(value) || typeof value !== "object")) viewedFace = null
            if (key === "units") showBg()
            changed()
        })
        form.on("load", () => { viewedFace = null; showBg(); changed() })
        new IntersectionObserver(entries => {
            onScreen = entries[entries.length - 1].isIntersecting
            resume()
        }).observe($("#preview_canvas"))
        document.addEventListener("visibilitychange", resume)
    }

    return { start }
})()
