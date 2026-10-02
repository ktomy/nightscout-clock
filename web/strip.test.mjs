// Focused tests for the comment strippers in strip.mjs.
//
// The production bundle ships comment-free, so these guard the scanner's
// contract: comments go away, everything else stays byte-identical.
// Run: node --test web/strip.test.mjs
import { describe, it } from "node:test";
import assert from "node:assert/strict";
import { stripCss, stripJs } from "./strip.mjs";

describe("stripJs", () => {
    it("keeps URLs and // inside strings", () => {
        assert.equal(
            stripJs('const u = "https://example.com//path?a=1//2"; // c'),
            'const u = "https://example.com//path?a=1//2"; '
        );
        assert.equal(
            stripJs("const s = '// not a comment'; /* c */"),
            "const s = '// not a comment';  "
        );
    });

    it("keeps escaped quotes from ending a string early", () => {
        assert.equal(
            stripJs('const s = "a\\"//b"; // real'),
            'const s = "a\\"//b"; '
        );
    });

    it("replaces block comments with a single space", () => {
        assert.equal(stripJs("a/*c*/b"), "a b");
        assert.equal(stripJs("a /* multi\nline */ b"), "a   b");
    });

    it("strips a line comment at EOF without a trailing newline", () => {
        assert.equal(stripJs("const a = 1; // done"), "const a = 1; ");
    });

    it("keeps regex literals intact, including comment-like content", () => {
        assert.equal(
            stripJs("const r = /https?:\\/\\/[^/]+/; // c"),
            "const r = /https?:\\/\\/[^/]+/; "
        );
        // Slashes and comment markers inside a character class.
        assert.equal(
            stripJs("const r = /[/]+\\/\\/*/; // c"),
            "const r = /[/]+\\/\\/*/; "
        );
        // Regex after a keyword that cannot end an expression.
        assert.equal(stripJs("return /ab+c/;"), "return /ab+c/;");
        assert.equal(stripJs("typeof /ab+c/;"), "typeof /ab+c/;");
    });

    it("does not mistake division for a regex", () => {
        assert.equal(stripJs("const r = a / b / c; // c"), "const r = a / b / c; ");
        // Division directly after a string literal (a string is an operand).
        assert.equal(stripJs('const r = "a" / b; // c'), 'const r = "a" / b; ');
        // Division directly after a regex literal.
        assert.equal(stripJs("const r = /a/ / 2; // c"), "const r = /a/ / 2; ");
    });

    it("handles nested template literals with ${} expressions", () => {
        // // d sits inside the inner template text, so it is NOT a comment.
        assert.equal(
            stripJs("`outer ${ `inner ${x /*c*/} // d` } end` // e"),
            "`outer ${ `inner ${x  } // d` } end` "
        );
        assert.equal(
            stripJs("`http://x/${y}//z`; // c"),
            "`http://x/${y}//z`; "
        );
    });

    it("handles braces inside template expressions", () => {
        assert.equal(
            stripJs("`${ {a: 1} /*c*/ }`; // d"),
            "`${ {a: 1}   }`; "
        );
    });
});

describe("stripCss", () => {
    it("strips block comments", () => {
        assert.equal(
            stripCss("a { color: red; /* c */ margin: 0; }"),
            "a { color: red;   margin: 0; }"
        );
    });

    it("keeps comment markers inside CSS strings", () => {
        assert.equal(
            stripCss('a::after { content: "/*"; } /* c */'),
            'a::after { content: "/*"; }  '
        );
        assert.equal(
            stripCss('a::after { content: "a\\"/*"; }'),
            'a::after { content: "a\\"/*"; }'
        );
    });

    it("does not treat // as a comment in CSS", () => {
        assert.equal(
            stripCss("a { color: red; } // not css"),
            "a { color: red; } // not css"
        );
    });
});
