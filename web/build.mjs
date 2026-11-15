// Builds the settings page and compresses source assets for the clock's LittleFS.
//
// The clock serves "<file>.gz" with Content-Encoding: gzip, so the page ships as ONE gzipped file with its
// CSS and JS inlined: one request, no libraries, nothing loaded from the internet.
//
//   node web/build.mjs
//
// With --simulator, it builds the face simulator for the project site instead: the same preview code with
// the factory settings, as <outdir>/index.html and <outdir>/clockemu.js. data/ is left as it is.
//
//   node web/build.mjs --simulator www/simulator
import fs from "node:fs";
import path from "node:path";
import zlib from "node:zlib";
import { fileURLToPath } from "node:url";

const HERE = path.dirname(fileURLToPath(import.meta.url));
const ROOT = path.resolve(HERE, "..");
const SRC = path.join(HERE, "src");
const SIM = path.join(HERE, "simulator");
const DATA = path.join(ROOT, "data");

// Order matters: later files use what earlier ones define.
const JS = ["util.js", "api.js", "schema.js", "form.js", "preview.js", "preview-panel.js", "cards.js", "app.js"];
// The simulator page: the preview and the schema's constants, without the clock's API or the settings form.
const SIM_JS = ["util.js", "schema.js", "preview.js", "preview-panel.js", path.join(SIM, "simulator.js")];

// Gzipped page budget. The whole LittleFS partition is 1 MB.
const BUDGET_BYTES = 200000;
// The clock preview emulator, built by web/emulator/build.mjs.
const EMULATOR_BUDGET_BYTES = 70000;

/**
 * Compress an asset at maximum gzip compression and normalize the OS header byte for consistent output.
 * @param {Uint8Array} buf - Uncompressed asset bytes.
 * @returns {Buffer}
 */
function gzip(buf) {
    const out = zlib.gzipSync(buf, { level: 9 });
    out[9] = 255; // "unknown OS" header byte, so every platform builds the same bytes
    return out;
}

/**
 * Read UTF-8 source with normalized line endings so CRLF and LF checkouts produce the same assets.
 * @param {string} file - Source file path.
 * @returns {string}
 */
const read = file => fs.readFileSync(file, "utf8").replace(/\r\n/g, "\n");

/**
 * Build one self-contained page: the preview column at "<!--__PREVIEW__: note-->" (the note ends the line
 * under the panel), then the CSS and the JS files inlined.
 * @param {string} template - Page template path.
 * @param {string[]} files - JS files in load order; a bare name is in web/src/js.
 * @param {(js: string) => string} [prepare] - Changes to the joined JS before it is inlined.
 * @returns {string}
 */
function buildPage(template, files, prepare = js => js) {
    const css = read(path.join(SRC, "app.css"));
    const preview = read(path.join(SRC, "preview.html"));
    const js = prepare(files.map(f => `// ---- ${path.basename(f)} ----\n` + read(path.resolve(SRC, "js", f))).join("\n;\n"));
    return read(template)
        .replace(/^[ \t]*<!--__PREVIEW__:\s*(.*?)-->\n/m, (_, note) => preview.replace("<!--__NOTE__-->", () => note))
        .replace("/*__CSS__*/", () => css)
        .replace("/*__JS__*/", () => `(function () {\n"use strict";\n${js}\n})();\n`.replace(/<\/script/gi, "<\\/script"));
}

/**
 * Write the face simulator for the project site: the page, and the emulator it loads by relative URL.
 * @param {string} outDir - Output directory, created if missing.
 * @returns {void}
 */
function buildSimulator(outDir) {
    const config = JSON.parse(read(path.join(DATA, "config_initial.json")));
    const html = buildPage(path.join(SIM, "index.html"), SIM_JS, js => js.replace("/*__CONFIG__*/null", () => JSON.stringify(config)));
    fs.mkdirSync(outDir, { recursive: true });
    fs.writeFileSync(path.join(outDir, "index.html"), html);
    fs.writeFileSync(path.join(outDir, "clockemu.js"), zlib.gunzipSync(fs.readFileSync(path.join(DATA, "clockemu.js.gz"))));
    console.log(`simulator written to ${outDir}`);
}

if (!fs.existsSync(path.join(DATA, "clockemu.js.gz"))) {
    throw new Error("data/clockemu.js.gz is missing: run node web/emulator/build.mjs, or a filesystem build");
}

const simulatorAt = process.argv.indexOf("--simulator");
if (simulatorAt >= 0) {
    const outDir = process.argv[simulatorAt + 1];
    if (!outDir) throw new Error("usage: node web/build.mjs --simulator <outdir>");
    buildSimulator(path.resolve(outDir));
    process.exit();
}

const page = gzip(Buffer.from(buildPage(path.join(SRC, "index.html"), JS)));
if (page.length > BUDGET_BYTES) {
    throw new Error(`index.html.gz is over budget: ${page.length} B (limit ${BUDGET_BYTES} B)`);
}
const timezoneData = read(path.join(SRC, "assets", "tzdata.json"));
JSON.parse(timezoneData);
const favicon = fs.readFileSync(path.join(SRC, "assets", "favicon.ico"));
fs.writeFileSync(path.join(DATA, "index.html.gz"), page);
fs.writeFileSync(path.join(DATA, "tzdata.json.gz"), gzip(Buffer.from(timezoneData)));
fs.writeFileSync(path.join(DATA, "favicon.ico.gz"), gzip(favicon));

const total = fs.readdirSync(DATA, { recursive: true })
    .map(f => path.join(DATA, f)).filter(f => fs.statSync(f).isFile())
    .reduce((n, f) => n + fs.statSync(f).size, 0);
const emulator = fs.statSync(path.join(DATA, "clockemu.js.gz")).size;
console.log(`index.html.gz ${page.length} B (budget ${BUDGET_BYTES} B)`);
console.log(`clockemu.js.gz ${emulator} B (budget ${EMULATOR_BUDGET_BYTES} B)`);
console.log(`data/ total   ${total} B`);
if (emulator > EMULATOR_BUDGET_BYTES) {
    console.error("over budget");
    process.exitCode = 1;
}
