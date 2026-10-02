// Builds the settings page and compresses source assets for the clock's LittleFS.
//
// The clock serves "<file>.gz" with Content-Encoding: gzip, so the page ships as ONE gzipped file with its
// CSS and JS inlined: one request, no libraries, nothing loaded from the internet.
//
//   node web/build.mjs
import fs from "node:fs";
import path from "node:path";
import zlib from "node:zlib";
import { execFileSync } from "node:child_process";
import { fileURLToPath } from "node:url";
import { stripCss, stripJs } from "./strip.mjs";

const HERE = path.dirname(fileURLToPath(import.meta.url));
const ROOT = path.resolve(HERE, "..");
const SRC = path.join(HERE, "src");
const DATA = path.join(ROOT, "data");

// Order matters: later files use what earlier ones define.
const JS = ["util.js", "api.js", "schema.js", "form.js", "cards.js", "app.js"];

// Gzipped page budget. The LittleFS partition is 448 KB; the page must stay a
// small fraction of it so settings, timezone data and favicon always fit.
const BUDGET_BYTES = 46000;

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

const css = stripCss(read(path.join(SRC, "app.css")));
const jsFiles = JS.map(f => ({ name: f, src: stripJs(read(path.join(SRC, "js", f))) }));
// Fail the build if comment stripping ever breaks the syntax of a source file.
for (const { name, src } of jsFiles) {
    const tmp = path.join(SRC, "js", `.check-${name}`);
    fs.writeFileSync(tmp, src);
    try {
        execFileSync(process.execPath, ["--check", tmp], { stdio: "pipe" });
    } finally {
        fs.unlinkSync(tmp);
    }
}
const js = jsFiles.map(({ src }) => src).join("\n;\n");
const bundle = `(function () {\n"use strict";\n${js}\n})();\n`;
const tmpBundle = path.join(SRC, "js", ".check-bundle.js");
fs.writeFileSync(tmpBundle, bundle);
try {
    execFileSync(process.execPath, ["--check", tmpBundle], { stdio: "pipe" });
} finally {
    fs.unlinkSync(tmpBundle);
}
let html = read(path.join(SRC, "index.html"));
html = html
    .replace("/*__CSS__*/", () => css)
    .replace("/*__JS__*/", () => bundle.replace(/<\/script/gi, "<\\/script"));

const page = gzip(Buffer.from(html));
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
console.log(`index.html.gz ${page.length} B (budget ${BUDGET_BYTES} B)`);
console.log(`data/ total   ${total} B`);
