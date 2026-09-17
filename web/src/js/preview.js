// The clock preview: the firmware's display code, compiled to WebAssembly by web/emulator/build.sh, runs in
// this browser with the settings on the page, unsaved changes included. The clock only serves the file, so
// the preview works in setup mode and costs the clock nothing after the one download.

const preview = (() => {
    const W = 32, H = 8
    // PeripheryManager's buttons, as web/emulator/emu.cpp numbers them.
    const BUTTON = { LEFT: 0, SELECT: 1, RIGHT: 2, LEFT_HOLD: 3, RIGHT_HOLD: 4, SELECT_DOUBLE: 5 }
    const events = emitter()
    let loading = null
    let config = null
    let main = null          // module driving the big panel; only the person's choices change its face
    let thumbModule = null   // a second module for the face cards, so drawing them never moves the big panel
    let faceCount = 0        // faces this firmware has; a face the page knows and the firmware lacks gets no picture
    let bootSeq = 0
    let thumbSeq = 0
    let liveTimer = null
    let sound = false
    let scenario = { bg: 118, trend: 4, age: 1, history: "steady" }
    let lux = 40
    let canvas = null
    let faceIndex = null     // face the big panel shows; null = the config's default face
    let thumbs = []          // [{canvas, face}] drawn after each boot
    let toneLog = []         // buzzer changes while the preview ran: [[ms, hz], ...]

    function loadScript() {
        if (window.createClockEmu) return Promise.resolve()
        loading = loading || new Promise((resolve, reject) => document.head.append(el("script", {
            src: "clockemu.js", onload: resolve, onerror: () => reject(new Error("The clock preview could not be loaded.")),
        })))
        return loading
    }

    async function start(panelCanvas) {
        canvas = panelCanvas
        await whenIdle()
        await loadScript()
        if (config) await reboot()
    }

    // The clock's time: the real time, or a time the person sets that can run faster than real time.
    let timeMode = "now"
    let timeBase = 0          // epoch seconds at timeStartedMs
    let timeStartedMs = 0
    let timeSpeed = 0         // emulated seconds per real second; 0 = paused
    function clockEpoch() {
        if (timeMode === "now") return Math.floor(Date.now() / 1000)
        return Math.floor(timeBase + ((performance.now() - timeStartedMs) / 1000) * timeSpeed)
    }
    // The clock's own time zone (settings), not this browser's: its UTC offset in seconds at a moment.
    let timeZone = null
    function tzOffsetAt(epoch) {
        try {
            const parts = Object.fromEntries(new Intl.DateTimeFormat("en-US", {
                timeZone: timeZone || undefined, hourCycle: "h23", year: "numeric", month: "2-digit", day: "2-digit", hour: "2-digit", minute: "2-digit", second: "2-digit",
            }).formatToParts(new Date(epoch * 1000)).map(p => [p.type, p.value]))
            const asUtc = Date.UTC(+parts.year, +parts.month - 1, +parts.day, +parts.hour % 24, +parts.minute, +parts.second)
            return Math.round((asUtc / 1000 - epoch) / 60) * 60
        } catch (e) {
            return -new Date(epoch * 1000).getTimezoneOffset() * 60
        }
    }

    // Three hours of readings, 5 minutes apart, the newest `age` minutes before the clock's time.
    function buildReadings(s, now) {
        if (s.history === "none") return []
        const slope = s.history === "rising" ? 4 : s.history === "falling" ? -4 : 0
        const rows = []
        for (let k = 35; k >= 0; k--) rows.push([Math.max(40, Math.min(400, Math.round(s.bg - slope * k))), s.trend, now - (s.age * 60 + k * 300)])
        return rows
    }

    function showReadings(M, now = clockEpoch()) {
        M._emu_set_clock(now, tzOffsetAt(now))
        M.ccall("emu_show_readings", null, ["string"], [JSON.stringify(buildReadings(scenario, now))])
    }

    // The panel as rows of [r, g, b]: sent to the LEDs, or the frame before brightness.
    function panel(M, frame) {
        const ptr = frame ? M._emu_frame() : M._emu_wire()
        const bytes = M.HEAPU8.subarray(ptr, ptr + W * H * 3)
        return Array.from({ length: H }, (_, y) => Array.from({ length: W }, (_, x) => {
            const i = M._emu_xy(x, y) * 3
            return [bytes[i], bytes[i + 1], bytes[i + 2]]
        }))
    }

    // An LED at 1/255 is dim but visible, so low values are lifted to show on a screen; 0 stays dark.
    const lift = c => (c ? Math.round(255 * (0.22 + 0.78 * Math.sqrt(c / 255))) : 0)

    function drawPanel(target, M, cell, glow) {
        const rows = panel(M)
        const ctx = target.getContext("2d")
        target.width = W * cell
        target.height = H * cell
        ctx.fillStyle = "#050505"
        ctx.fillRect(0, 0, target.width, target.height)
        rows.forEach((row, y) => row.forEach((p, x) => {
            const lit = p[0] || p[1] || p[2]
            ctx.beginPath()
            ctx.arc(x * cell + cell / 2, y * cell + cell / 2, cell * 0.38, 0, Math.PI * 2)
            ctx.fillStyle = lit ? `rgb(${lift(p[0])},${lift(p[1])},${lift(p[2])})` : "#1a1c1e"
            ctx.shadowBlur = lit && glow ? cell * 0.5 : 0
            ctx.shadowColor = ctx.fillStyle
            ctx.fill()
        }))
        ctx.shadowBlur = 0
    }

    // drawn: LEDs the face draws; lost: drawn but 0 on every channel at this brightness (invisible on the clock);
    // partial: still lit but a channel went to 0 (the colour shifts).
    function stats(M) {
        const frame = panel(M, true).flat(), wire = panel(M).flat()
        let drawn = 0, lost = 0, partial = 0
        frame.forEach((f, i) => {
            const w = wire[i]
            if (!(f[0] || f[1] || f[2])) return
            drawn++
            if (!(w[0] || w[1] || w[2])) lost++
            else if (f.some((c, k) => c && !w[k])) partial++
        })
        return { drawn, lost, partial, brightness: M._emu_brightness(), face: M._emu_get_face(), displayOn: M._emu_display_on() === 1 }
    }

    // Buzzer changes, played as a square wave like the clock's PWM buzzer.
    let audio = null
    function playTones(list) {
        const Ctx = window.AudioContext || window.webkitAudioContext
        if (!list.length || !Ctx) return
        audio = audio || new Ctx()
        const startAt = audio.currentTime + 0.05, first = list[0][0]
        list.forEach(([ms, hz], i) => {
            if (!hz) return
            const end = i + 1 < list.length ? list[i + 1][0] : ms + 300
            const osc = audio.createOscillator(), gain = audio.createGain()
            osc.type = "square"
            osc.frequency.value = hz
            gain.gain.value = 0.05
            osc.connect(gain).connect(audio.destination)
            osc.start(startAt + (ms - first) / 1000)
            osc.stop(startAt + (end - first) / 1000)
        })
    }
    function takeNewTones(M) {
        const list = JSON.parse(M.ccall("emu_drain_tones", "string", [], []))
        toneLog.push(...list)
        if (toneLog.length > 2000) toneLog = toneLog.slice(-1000)
        return list
    }

    async function boot(cfg) {
        const M = await window.createClockEmu({ print() {}, printErr() {} })
        const now = clockEpoch()
        M._emu_set_clock(now, tzOffsetAt(now))
        if (!M.ccall("emu_boot", "number", ["string", "number"], [JSON.stringify(cfg), lux])) throw new Error("The preview can't show these settings.")
        return M
    }

    // A fresh emulated clock for every settings change, as the device loads a save.
    async function reboot() {
        if (!window.createClockEmu || !config) return
        const seq = ++bootSeq
        let M, T
        try {
            M = await boot(config)
            T = await boot(config)
        } catch (e) {
            if (seq === bootSeq) events.emit("error", e)
            return
        }
        if (seq !== bootSeq) return
        main = M
        thumbModule = T
        faceCount = M._emu_face_count()
        if (faceIndex != null && faceIndex < faceCount) M._emu_set_face(faceIndex)
        showReadings(M)
        draw()
        drawThumbs()
    }

    function draw() {
        if (!main || !canvas) return
        drawPanel(canvas, main, 12, true)
        events.emit("render", stats(main))
    }

    // Face cards: their own module, one face at a time, yielding between faces. A newer call stops an older loop.
    async function drawThumbs() {
        const seq = ++thumbSeq
        const T = thumbModule
        if (!T) return
        for (const t of thumbs) {
            if (seq !== thumbSeq || T !== thumbModule) return
            t.canvas.hidden = t.face >= faceCount
            if (t.canvas.hidden) continue
            T._emu_set_face(t.face)
            showReadings(T)
            drawPanel(t.canvas, T, 6, false)
            await nextFrame()
        }
    }
    const scheduleThumbs = debounce(drawThumbs, 120)
    const scheduleReboot = debounce(reboot, 150)

    // A change of scene: the readings follow the clock's time, and the face cards follow the big panel.
    function refresh() {
        if (!main) return
        showReadings(main)
        draw()
        scheduleThumbs()
    }

    return {
        on: events.on,
        start,
        // Keep showing the face the person is looking at; only a new default face switches it.
        setConfig(next) {
            if (!config || next.default_face !== config.default_face) faceIndex = null
            config = clone(next)
            scheduleReboot()
        },
        setScenario(next) {
            scenario = { ...scenario, ...next }
            refresh()
        },
        setLux(next) {
            lux = next
            if (!main) return
            main._emu_set_lux(lux)
            draw()
        },
        showFace(id) {
            faceIndex = id
            if (!main || id >= faceCount) return
            main._emu_set_face(id)
            showReadings(main)
            draw()
        },
        press(button) {
            if (!main) return
            main._emu_button(BUTTON[button])
            faceIndex = main._emu_get_face()
            draw()
        },
        setThumbs(list) { thumbs = list; drawThumbs() },
        setLive(on) {
            clearInterval(liveTimer)
            liveTimer = null
            if (!on) return
            let lastMinute = -1
            // The firmware's loop (display, alarms, light sensor) at the clock's time, with readings that keep
            // their age.
            liveTimer = setInterval(() => {
                if (!main || document.hidden) return
                const now = clockEpoch()
                if (Math.floor(now / 60) !== lastMinute) {
                    lastMinute = Math.floor(now / 60)
                    showReadings(main, now)
                }
                main._emu_set_clock(now, tzOffsetAt(now))
                main._emu_loop(250)
                const tones = takeNewTones(main)
                if (sound) playTones(tones)
                draw()
                events.emit("time", now)
            }, 250)
        },
        // mode "now" | "set"; epoch in seconds (for "set"); speed in emulated seconds per real second.
        setTime({ mode, epoch, speed }) {
            if (mode) timeMode = mode
            if (epoch != null) { timeBase = epoch; timeStartedMs = performance.now() }
            if (speed != null) { timeBase = clockEpoch(); timeStartedMs = performance.now(); timeSpeed = speed }
            refresh()
            events.emit("time", clockEpoch())
        },
        clockEpoch,
        tzOffsetAt,
        setTimeZone(name) {
            timeZone = name || null
            refresh()
        },
        setSound(on) { sound = on },
        // What the clock plays for "Try on clock", through the firmware's own melody code.
        playMelody(rtttl) {
            if (!main) return false
            main.ccall("emu_play_rtttl", null, ["string"], [rtttl])
            const started = performance.now()
            const pump = () => {
                main._emu_loop(50)
                playTones(takeNewTones(main))
                if (performance.now() - started < 8000) setTimeout(pump, 50)
            }
            pump()
            return true
        },
        get faceCount() { return faceCount },
        stats() { return main ? stats(main) : null },
        // For tests: buzzer changes since the last call, and the panel as rows of [r, g, b] on the LEDs.
        takeTones() { const t = toneLog; toneLog = []; return t },
        wire() { return main ? panel(main) : null },
    }
})()
