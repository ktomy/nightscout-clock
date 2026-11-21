// Builds data/clockemu.js.gz, the settings page's clock preview and the face simulator: the firmware's
// display code from src/ and lib/ compiled to WebAssembly with Emscripten, plus the stand-ins in this folder
// for the hardware. PlatformIO runs it before every firmware build (scripts/build_web.py); it skips the
// compile when none of its inputs changed since the last build.
//
//   node web/emulator/build.mjs [--libs <libdeps dir>] [--core <Arduino core dir>]
//   node web/emulator/build.mjs --emsdk-version
//
// Needs Emscripten EMSDK_VERSION: emcc on PATH, or the emsdk folder in EMSDK or ~/emsdk. The libraries and
// the Arduino core come from a PlatformIO build of the ulanzi_debug or ulanzi env.
import { spawn, spawnSync } from "node:child_process";
import crypto from "node:crypto";
import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import zlib from "node:zlib";
import { fileURLToPath } from "node:url";

const EMSDK_VERSION = "6.0.9";

const HERE = path.dirname(fileURLToPath(import.meta.url));
const ROOT = path.resolve(HERE, "../..");
const OUT = path.join(ROOT, ".pio", "emulator");
const TARGET = path.join(ROOT, "data", "clockemu.js.gz");
const WINDOWS = process.platform === "win32";

/**
 * Stop with a message for the person running the build.
 * @param {string} message - What went wrong and how to fix it.
 * @returns {never}
 */
function fail(message) {
    console.error(`clock preview: ${message}`);
    process.exit(1);
}

/**
 * Read a command-line option's value.
 * @param {string} name - Option name, such as "--libs".
 * @returns {string | undefined}
 */
function option(name) {
    const i = process.argv.indexOf(name);
    return i >= 0 ? process.argv[i + 1] : undefined;
}

/**
 * Find emcc and the environment it needs: on PATH, or in an emsdk folder from EMSDK or ~/emsdk.
 * @returns {{ emcc: string, emxx: string, env: NodeJS.ProcessEnv }}
 */
function findEmscripten() {
    const ext = WINDOWS ? ".bat" : "";
    const onPath = spawnSync(`emcc${ext}`, ["--version"], { shell: WINDOWS, encoding: "utf8" });
    if (onPath.status === 0) return { emcc: `emcc${ext}`, emxx: `em++${ext}`, env: process.env };
    for (const sdk of [process.env.EMSDK, path.join(os.homedir(), "emsdk")].filter(Boolean)) {
        const bin = path.join(sdk, "upstream", "emscripten");
        if (!fs.existsSync(path.join(bin, `emcc${ext}`))) continue;
        const env = { ...process.env, EMSDK: sdk, EM_CONFIG: path.join(sdk, ".emscripten") };
        env.PATH = [bin, process.env.PATH].join(path.delimiter);
        return { emcc: path.join(bin, `emcc${ext}`), emxx: path.join(bin, `em++${ext}`), env };
    }
    fail(`Emscripten ${EMSDK_VERSION} was not found. Install it as CONTRIBUTING.md describes, in ~/emsdk or `
        + "with EMSDK set to its folder.");
}

/**
 * Run a command and resolve when it succeeds; reject with its output when it fails.
 * @param {string} cmd - Program to run.
 * @param {string[]} args - Its arguments.
 * @param {NodeJS.ProcessEnv} env - Its environment.
 * @returns {Promise<void>}
 */
function run(cmd, args, env) {
    return new Promise((resolve, reject) => {
        const child = spawn(cmd, args, { env, shell: WINDOWS, stdio: ["ignore", "pipe", "pipe"] });
        let output = "";
        child.stdout.on("data", d => { output += d });
        child.stderr.on("data", d => { output += d });
        child.on("error", reject);
        child.on("close", code => (code === 0 ? resolve() : reject(new Error(`${path.basename(cmd)} failed:\n${output}`))));
    });
}

/**
 * Run tasks with at most `limit` at a time.
 * @param {(() => Promise<void>)[]} tasks - Work to run.
 * @param {number} limit - Concurrency.
 * @returns {Promise<void>}
 */
async function parallel(tasks, limit) {
    const queue = [...tasks];
    await Promise.all(Array.from({ length: Math.min(limit, queue.length) }, async () => {
        while (queue.length) await queue.shift()();
    }));
}

const list = (dir, test) => fs.readdirSync(dir).filter(test).sort().map(f => path.join(dir, f));

if (process.argv.includes("--emsdk-version")) {
    console.log(EMSDK_VERSION);
    process.exit(0);
}

const libsDir = option("--libs") || ["ulanzi_debug", "ulanzi"].map(e => path.join(ROOT, ".pio", "libdeps", e))
    .find(d => fs.existsSync(path.join(d, "ArduinoJson")));
const coreDir = option("--core") || path.join(process.env.PLATFORMIO_CORE_DIR || path.join(os.homedir(), ".platformio"),
    "packages", "framework-arduinoespressif32", "cores", "esp32");
if (!libsDir || !fs.existsSync(path.join(libsDir, "ArduinoJson"))) {
    fail("the firmware's libraries are missing: build the ulanzi_debug or ulanzi env once.");
}
if (!fs.existsSync(path.join(coreDir, "WString.cpp"))) fail(`no Arduino core in ${coreDir}: build the ulanzi env once.`);

// The display code and the headers it includes, with this folder's replacements copied over the
// network-facing ones, and the Arduino core's String, Print and Stream.
const src = path.join(ROOT, "src");
const lib = path.join(ROOT, "lib");
const sources = [
    ...list(src, f => f.endsWith(".h")),
    ...list(path.join(lib, "Fonts"), f => f.endsWith(".h")),
    ...list(path.join(lib, "LightResistor"), () => true),
    ...list(path.join(lib, "MelodyPlayer"), () => true),
    ...list(src, f => /^(BGDisplayFace.*|Settings.*|DisplayManager|BGDisplayManager|globals|PeripheryManager|BGAlarmManager)\.cpp$/.test(f)),
    ...list(HERE, f => f.endsWith(".cpp")),
    ...list(path.join(HERE, "override"), f => f.endsWith(".h")),
    ...["WString.h", "WString.cpp", "Print.h", "Print.cpp", "Printable.h", "Stream.h", "Stream.cpp", "stdlib_noniso.h",
        "stdlib_noniso.c"].map(f => path.join(coreDir, f)),
];
const libraryCpp = [
    path.join(libsDir, "Adafruit GFX Library", "Adafruit_GFX.cpp"),
    path.join(libsDir, "Framebuffer GFX", "Framebuffer_GFX.cpp"),
    path.join(libsDir, "FastLED NeoMatrix", "FastLED_NeoMatrix.cpp"),
];
const includeDirs = ["Adafruit GFX Library", "Framebuffer GFX", "FastLED NeoMatrix", path.join("ArduinoJson", "src")]
    .map(d => path.join(libsDir, d));
const stubs = list(path.join(HERE, "include"), () => true);

// Skip the compile when the inputs and the compiler are the ones the current file was built from.
const { emcc, emxx, env } = findEmscripten();
const version = spawnSync(emcc, ["--version"], { env, shell: WINDOWS, encoding: "utf8" }).stdout || "";
if (!version.includes(` ${EMSDK_VERSION} `)) fail(`Emscripten ${EMSDK_VERSION} is required; ${emcc} is a different version.`);
const hash = crypto.createHash("sha256").update(EMSDK_VERSION);
for (const file of [fileURLToPath(import.meta.url), ...stubs, ...sources, ...libraryCpp,
    ...includeDirs.flatMap(d => fs.readdirSync(d, { recursive: true }).map(f => path.join(d, f)))].sort()) {
    if (fs.statSync(file).isFile()) hash.update(path.basename(file)).update(fs.readFileSync(file));
}
const stamp = hash.digest("hex");
const stampFile = path.join(OUT, "inputs.sha256");
if (fs.existsSync(TARGET) && fs.existsSync(stampFile) && fs.readFileSync(stampFile, "utf8") === stamp) {
    console.log("data/clockemu.js.gz is up to date");
    process.exit(0);
}

fs.rmSync(OUT, { recursive: true, force: true });
fs.mkdirSync(path.join(OUT, "src"), { recursive: true });
fs.mkdirSync(path.join(OUT, "obj"));
for (const file of sources) fs.copyFileSync(file, path.join(OUT, "src", path.basename(file)));

console.log("Building the clock preview (data/clockemu.js.gz)...");
const flags = ["-Oz", "-w", "-DARDUINO=10819", "-DESP32", "-DULANZI", "-include", path.join(HERE, "include", "emu_prelude.h"),
    `-I${path.join(HERE, "include")}`, `-I${path.join(OUT, "src")}`, ...includeDirs.map(d => `-I${d}`)];
const units = [...list(path.join(OUT, "src"), f => /\.(c|cpp)$/.test(f)), ...libraryCpp];
const objects = units.map(f => path.join(OUT, "obj", path.basename(f).replace(/\.[^.]+$/, ".o")));
try {
    await parallel(units.map((file, i) => () => (file.endsWith(".c")
        ? run(emcc, ["-std=gnu11", ...flags, "-c", file, "-o", objects[i]], env)
        : run(emxx, ["-std=gnu++17", ...flags, "-c", file, "-o", objects[i]], env))), os.cpus().length);
    // emmalloc and Closure keep the download small.
    await run(emxx, ["-Oz", ...objects, "-o", path.join(OUT, "clockemu.js"), "-sMODULARIZE=1", "-sEXPORT_NAME=createClockEmu",
        "-sSINGLE_FILE=1", "-sENVIRONMENT=web", "-sALLOW_MEMORY_GROWTH=1", "-sINITIAL_MEMORY=4194304", "-sSTACK_SIZE=262144",
        "-sMALLOC=emmalloc", "-sEXPORTED_RUNTIME_METHODS=ccall,HEAPU8", "--closure", "1"], env);
} catch (e) {
    fail(e.message);
}

// Gzipped like web/build.mjs does, so every platform writes the same bytes.
const out = zlib.gzipSync(fs.readFileSync(path.join(OUT, "clockemu.js")), { level: 9 });
out[9] = 255;
fs.writeFileSync(TARGET, out);
fs.writeFileSync(stampFile, stamp);
console.log(`data/clockemu.js.gz ${out.length} B`);
