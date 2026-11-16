// Renders every clock face to a PNG for the README, using the clock preview the settings page runs:
// the firmware's own display code compiled to WebAssembly (data/clockemu.js.gz). Every face shows the same
// sample data at the same time, so the pictures stay consistent as faces are added.
//
//   node scripts/render_faces.mjs [--out <dir>] [--face <id>]
//
// Build the preview first with `node web/emulator/build.mjs` (see CONTRIBUTING.md). Writes
// docs/images/faces/<face id>.png by default. Needs only Node.js.
import fs from "node:fs"
import path from "node:path"
import vm from "node:vm"
import zlib from "node:zlib"
import { fileURLToPath } from "node:url"

const ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..")
const EMULATOR = path.join(ROOT, "data", "clockemu.js.gz")
const W = 32, H = 8
const CELL = 24             // screen pixels per LED: 768 x 192 images

// The sample every face shows: 10:42 on a fixed day, 118 mg/dl and flat, a minute old, after three hours of
// readings that rise to 168 and settle back, so the graph faces have a curve to draw and the delta is 0.
const CLOCK_EPOCH = Date.UTC(2026, 0, 15, 10, 42, 0) / 1000
const READING = { bg: 118, trend: 4, ageMinutes: 1 }
// Animated faces are shown this long after the reading arrives, partway through their motion.
const SETTLE_MS = 1500
const LUX = 40

/**
 * Stop with a message for the person running the script.
 * @param {string} message - What went wrong and how to fix it.
 * @returns {never}
 */
function fail(message) {
    console.error(`render_faces: ${message}`)
    process.exit(1)
}

/**
 * Read a command-line option's value.
 * @param {string} name - Option name, such as "--out".
 * @returns {string | undefined}
 */
function option(name) {
    const i = process.argv.indexOf(name)
    return i >= 0 ? process.argv[i + 1] : undefined
}

const read = file => fs.readFileSync(path.join(ROOT, file), "utf8")

// The settings page's face list and settings conversion, loaded the way the page loads its classic scripts.
const { FACES, buildSaveJson, normalizeLoaded } = vm.runInThisContext(`(function () {
    ${read("web/src/js/util.js")}
    ${read("web/src/js/schema.js")}
    return { FACES, buildSaveJson, normalizeLoaded }
})()`)

/**
 * Load the clock preview module from data/clockemu.js.gz.
 * @returns {Promise<any>}
 */
async function loadEmulator() {
    if (!fs.existsSync(EMULATOR)) fail("data/clockemu.js.gz is missing: run `node web/emulator/build.mjs` first.")
    // The module is built for browsers; these are the only browser globals it looks for.
    const context = vm.createContext({ console, WebAssembly, atob, TextDecoder, TextEncoder, setTimeout, clearTimeout,
        performance, URL, document: { currentScript: { src: "" } }, location: { href: "" } })
    context.window = context.self = context.globalThis = context
    vm.runInContext(zlib.gunzipSync(fs.readFileSync(EMULATOR)).toString("utf8"), context, { filename: "clockemu.js" })
    return context.createClockEmu({ print() {}, printErr() {} })
}

/**
 * Three hours of readings, 5 minutes apart, oldest first, ending at the sample reading.
 * @param {number} now - The clock's time, epoch seconds.
 * @returns {[number, number, number][]}
 */
function readings(now) {
    const rows = []
    for (let k = 35; k >= 0; k--) {
        const bg = Math.round(READING.bg + 25 * (1 - Math.cos(k / 6)))
        rows.push([bg, READING.trend, now - (READING.ageMinutes * 60 + k * 300)])
    }
    return rows
}

/**
 * The colors sent to the LEDs, as rows of [r, g, b].
 * @param {any} M - The emulator module.
 * @returns {number[][][]}
 */
function panel(M) {
    const ptr = M._emu_wire()
    const bytes = M.HEAPU8.subarray(ptr, ptr + W * H * 3)
    return Array.from({ length: H }, (_, y) => Array.from({ length: W }, (_, x) => {
        const i = M._emu_xy(x, y) * 3
        return [bytes[i], bytes[i + 1], bytes[i + 2]]
    }))
}

// As in the settings page's preview (web/src/js/preview.js): an LED at 1/255 is dim but visible, so low values
// are lifted to show on a screen, round LEDs on a near-black panel, unlit LEDs dark grey, lit ones with a glow.
const lift = c => (c ? Math.round(255 * (0.22 + 0.78 * Math.sqrt(c / 255))) : 0)
const BACKGROUND = [5, 5, 5]
const UNLIT = [26, 28, 30]

/**
 * Draw the panel as an RGB image.
 * @param {number[][][]} rows - LED colors.
 * @returns {{ width: number, height: number, pixels: Uint8Array }}
 */
function draw(rows) {
    const width = W * CELL, height = H * CELL
    const glow = new Float32Array(width * height * 3)
    const radius = CELL * 0.38, glowRadius = CELL * 0.9
    // The glow first, added up around each lit LED and fading with distance.
    rows.forEach((row, y) => row.forEach((p, x) => {
        if (!(p[0] || p[1] || p[2])) return
        const c = p.map(lift), cx = x * CELL + CELL / 2, cy = y * CELL + CELL / 2
        for (let py = Math.max(0, Math.floor(cy - glowRadius)); py < Math.min(height, Math.ceil(cy + glowRadius)); py++) {
            for (let px = Math.max(0, Math.floor(cx - glowRadius)); px < Math.min(width, Math.ceil(cx + glowRadius)); px++) {
                const d = Math.hypot(px + 0.5 - cx, py + 0.5 - cy)
                if (d <= radius || d >= glowRadius) continue
                const a = 0.35 * (1 - (d - radius) / (glowRadius - radius)) ** 2
                for (let k = 0; k < 3; k++) glow[(py * width + px) * 3 + k] += a * c[k]
            }
        }
    }))
    const pixels = new Uint8Array(width * height * 3)
    for (let i = 0; i < width * height; i++) {
        for (let k = 0; k < 3; k++) pixels[i * 3 + k] = Math.min(255, Math.round(BACKGROUND[k] + glow[i * 3 + k]))
    }
    // Then each LED, its edge smoothed by how much of the screen pixel it covers.
    rows.forEach((row, y) => row.forEach((p, x) => {
        const c = p[0] || p[1] || p[2] ? p.map(lift) : UNLIT, cx = x * CELL + CELL / 2, cy = y * CELL + CELL / 2
        for (let py = Math.floor(cy - radius - 1); py <= Math.ceil(cy + radius + 1); py++) {
            for (let px = Math.floor(cx - radius - 1); px <= Math.ceil(cx + radius + 1); px++) {
                const cover = Math.min(1, Math.max(0, radius + 0.5 - Math.hypot(px + 0.5 - cx, py + 0.5 - cy)))
                if (!cover) continue
                const i = (py * width + px) * 3
                for (let k = 0; k < 3; k++) pixels[i + k] = Math.round(pixels[i + k] * (1 - cover) + c[k] * cover)
            }
        }
    }))
    return { width, height, pixels }
}

/**
 * Encode an RGB image as PNG. The same pixels always give the same file, so unchanged faces stay unchanged.
 * @param {{ width: number, height: number, pixels: Uint8Array }} image - Image to encode.
 * @returns {Buffer}
 */
function png({ width, height, pixels }) {
    const chunk = (type, data) => {
        const length = Buffer.alloc(4), crc = Buffer.alloc(4), body = Buffer.concat([Buffer.from(type, "ascii"), data])
        length.writeUInt32BE(data.length)
        crc.writeUInt32BE(zlib.crc32(body))
        return Buffer.concat([length, body, crc])
    }
    const header = Buffer.alloc(13)
    header.writeUInt32BE(width, 0)
    header.writeUInt32BE(height, 4)
    header.set([8, 2, 0, 0, 0], 8)  // 8-bit RGB, no interlace
    const raw = Buffer.alloc((width * 3 + 1) * height)
    for (let y = 0; y < height; y++) raw.set(pixels.subarray(y * width * 3, (y + 1) * width * 3), y * (width * 3 + 1) + 1)
    return Buffer.concat([Buffer.from([137, 80, 78, 71, 13, 10, 26, 10]), chunk("IHDR", header),
        chunk("IDAT", zlib.deflateSync(raw, { level: 9 })), chunk("IEND", Buffer.alloc(0))])
}

const out = path.resolve(option("--out") || path.join(ROOT, "docs", "images", "faces"))
const only = option("--face")

// The factory settings with every face active, as the face simulator shows them.
const factory = JSON.parse(read("data/config_initial.json"))
const config = buildSaveJson(normalizeLoaded({ ...factory, units: "mgdl", inactive_faces: [] }))

const M = await loadEmulator()
M._emu_set_clock(CLOCK_EPOCH, 0)
if (!M.ccall("emu_boot", "number", ["string", "number"], [JSON.stringify(config), LUX])) fail("the emulator refused the factory settings.")
const ids = JSON.parse(M.ccall("emu_face_ids", "string", [], []))

// Every face the firmware has must be on the settings page, and the other way round, so none is left out here.
const named = FACES.map(f => f.id)
const unnamed = ids.filter(id => !named.includes(id)), missing = named.filter(id => !ids.includes(id))
if (unnamed.length || missing.length) {
    fail(`the firmware's faces and FACES in web/src/js/schema.js differ (firmware only: ${unnamed.join(", ") || "none"}; `
        + `settings page only: ${missing.join(", ") || "none"}). Rebuild the preview, or add the face to FACES.`)
}
if (only && !ids.includes(only)) fail(`no face "${only}"; the faces are ${ids.join(", ")}.`)

fs.mkdirSync(out, { recursive: true })
for (const id of ids) {
    if (only && id !== only) continue
    M._emu_set_clock(CLOCK_EPOCH, 0)
    M.ccall("emu_set_face", null, ["string"], [id])
    M.ccall("emu_show_readings", null, ["string"], [JSON.stringify(readings(CLOCK_EPOCH))])
    for (let ms = 0; ms < SETTLE_MS; ms += 50) M._emu_loop(50)
    const file = path.join(out, `${id}.png`)
    fs.writeFileSync(file, png(draw(panel(M))))
    console.log(`${path.relative(ROOT, file)}  ${FACES.find(f => f.id === id).name}`)
}
