// Builds the settings page into data/index.html.gz for the clock's LittleFS.
//
// The clock serves "<file>.gz" with Content-Encoding: gzip, so the page ships as ONE gzipped file with its
// CSS and JS inlined: one request, no libraries, nothing loaded from the internet.
//
//   node web/build.mjs
import fs from "node:fs";
import path from "node:path";
import zlib from "node:zlib";
import { fileURLToPath } from "node:url";

const HERE = path.dirname(fileURLToPath(import.meta.url));
const ROOT = path.resolve(HERE, "..");
const SRC = path.join(HERE, "src");
const DATA = path.join(ROOT, "data");

// Order matters: later files use what earlier ones define.
const JS = ["util.js", "api.js", "schema.js", "form.js", "cards.js", "app.js"];

// Gzipped page budget. The whole LittleFS partition is 1 MB.
const BUDGET_BYTES = 40000;

function gzip(buf) {
    const out = zlib.gzipSync(buf, { level: 9 });
    out[9] = 255; // "unknown OS" header byte, so every platform builds the same bytes
    return out;
}

// LF only, so a CRLF checkout builds the same bytes.
const read = file => fs.readFileSync(file, "utf8").replace(/\r\n/g, "\n");

const css = read(path.join(SRC, "app.css"));
const js = JS.map(f => `// ---- ${f} ----\n` + read(path.join(SRC, "js", f))).join("\n;\n");
let html = read(path.join(SRC, "index.html"));
html = html
    .replace("/*__CSS__*/", () => css)
    .replace("/*__JS__*/", () => `(function () {\n"use strict";\n${js}\n})();\n`.replace(/<\/script/gi, "<\\/script"));

const page = gzip(Buffer.from(html));
fs.writeFileSync(path.join(DATA, "index.html.gz"), page);

const total = fs.readdirSync(DATA, { recursive: true })
    .map(f => path.join(DATA, f)).filter(f => fs.statSync(f).isFile())
    .reduce((n, f) => n + fs.statSync(f).size, 0);
console.log(`index.html.gz ${page.length} B (budget ${BUDGET_BYTES} B)`);
console.log(`data/ total   ${total} B`);
if (page.length > BUDGET_BYTES) {
    console.error("index.html.gz is over budget");
    process.exitCode = 1;
}
