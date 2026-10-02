// Strip comments from the web sources for the production page bundle.
//
// The shipped index.html.gz keeps the page a small fraction of the LittleFS
// partition, so the build drops every JS/CSS comment (the sources keep their
// full JSDoc for readability). This is comment removal only, not minification:
// no identifier is renamed and no whitespace inside code is collapsed, so the
// output stays debuggable and diffable.
//
// The scanner is a small state machine that understands string literals,
// template literals (including nested ${}), and regex literals, so comment
// markers inside them are never treated as comments.

const ID_CHAR = /[a-zA-Z0-9_$]/;

// After one of these keywords a "/" starts a regex literal, never division.
const REGEX_KEYWORDS = new Set([
    "return", "typeof", "case", "in", "of", "new", "delete", "void",
    "throw", "yield", "await", "do", "else",
]);

/**
 * Remove // and block comments from JavaScript source.
 * @param {string} src - JavaScript source text.
 * @returns {string} Source with comments removed.
 */
export function stripJs(src) {
    let out = "";
    let i = 0;
    const n = src.length;
    // Stack of open template-literal ${} expressions; each tracks inner braces.
    const tplStack = [];
    let state = "code"; // code | line | block | sq | dq | tpl | regex | rclass
    let prevSig = ""; // last significant char seen in code state
    let prevWord = ""; // last identifier read in code state

    const pushCode = c => {
        out += c;
        if (/\s/.test(c)) return;
        prevSig = c;
        if (ID_CHAR.test(c)) prevWord += c;
        else prevWord = "";
    };

    while (i < n) {
        const c = src[i];
        const d = i + 1 < n ? src[i + 1] : "";

        if (state === "line") {
            if (c === "\n") { state = "code"; out += c; prevSig = "\n"; }
            i++;
            continue;
        }
        if (state === "block") {
            if (c === "*" && d === "/") { state = "code"; i += 2; out += " "; }
            else i++;
            continue;
        }
        if (state === "sq" || state === "dq" || state === "tpl") {
            const q = state === "sq" ? "'" : state === "dq" ? '"' : "`";
            out += c;
            if (c === "\\") { out += d; i += 2; continue; }
            // A closed string/template is an operand, so a following "/" is
            // division, never a regex: report it like a closing paren.
            if (c === q) { state = "code"; prevSig = ")"; prevWord = ""; i++; continue; }
            if (state === "tpl" && c === "$" && d === "{") {
                out += d;
                tplStack.push(0);
                state = "code";
                prevSig = "{";
                prevWord = "";
                i += 2;
                continue;
            }
            i++;
            continue;
        }
        if (state === "regex") {
            out += c;
            if (c === "\\") { out += d; i += 2; continue; }
            if (c === "[") { state = "rclass"; i++; continue; }
            // A closed regex is an operand too: a following "/" divides.
            if (c === "/") { state = "code"; prevSig = ")"; prevWord = ""; i++; continue; }
            i++;
            continue;
        }
        if (state === "rclass") {
            out += c;
            if (c === "\\") { out += d; i += 2; continue; }
            if (c === "]") state = "regex";
            i++;
            continue;
        }

        // state === "code"
        if (c === "{" && tplStack.length) {
            tplStack[tplStack.length - 1]++;
            pushCode(c);
            i++;
            continue;
        }
        if (c === "}" && tplStack.length) {
            const depth = tplStack[tplStack.length - 1];
            pushCode(c);
            if (depth === 0) {
                // Closes the ${ ... } expression: back inside the template.
                tplStack.pop();
                state = "tpl";
            } else {
                tplStack[tplStack.length - 1]--;
            }
            i++;
            continue;
        }
        if (c === "/" && d === "/") { state = "line"; i += 2; continue; }
        if (c === "/" && d === "*") { state = "block"; i += 2; continue; }
        if (c === "/") {
            const afterOperand = ID_CHAR.test(prevSig) || prevSig === ")" || prevSig === "]";
            const afterKeyword = ID_CHAR.test(prevSig) && REGEX_KEYWORDS.has(prevWord);
            if (!afterOperand || afterKeyword || prevSig === "}" || prevSig === "") {
                state = "regex";
                out += c;
                i++;
                continue;
            }
            pushCode(c);
            i++;
            continue;
        }
        if (c === "'") { state = "sq"; out += c; i++; continue; }
        if (c === '"') { state = "dq"; out += c; i++; continue; }
        if (c === "`") { state = "tpl"; out += c; i++; continue; }
        pushCode(c);
        i++;
    }
    return out;
}

/**
 * Remove block comments from CSS source. ("//" is not a comment in CSS.)
 * @param {string} src - CSS source text.
 * @returns {string} Source with block comments removed.
 */
export function stripCss(src) {
    let out = "";
    let i = 0;
    const n = src.length;
    let q = null; // open string quote, if any
    while (i < n) {
        const c = src[i];
        const d = i + 1 < n ? src[i + 1] : "";
        if (q) {
            out += c;
            if (c === "\\") { out += d; i += 2; continue; }
            if (c === q) q = null;
            i++;
            continue;
        }
        if (c === '"' || c === "'") { q = c; out += c; i++; continue; }
        if (c === "/" && d === "*") {
            i += 2;
            while (i < n && !(src[i] === "*" && src[i + 1] === "/")) i++;
            i += 2;
            out += " ";
            continue;
        }
        out += c;
        i++;
    }
    return out;
}
