// Tests that the OpenAPI spec defines the button simulation endpoint correctly.
// This guards against the spec drifting from the firmware implementation.

import assert from "node:assert/strict";
import fs from "node:fs";
import path from "node:path";
import test from "node:test";
import { fileURLToPath } from "node:url";

const ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const read = file => fs.readFileSync(path.join(ROOT, file), "utf8");

const spec = read("openapi.yaml");

test("openapi.yaml defines GET /api/button/left/press", () => {
    assert.ok(spec.includes("  /api/button/left/press:"));
});

test("openapi.yaml defines GET /api/button/left/long", () => {
    assert.ok(spec.includes("  /api/button/left/long:"));
});

test("openapi.yaml defines GET /api/button/right/press", () => {
    assert.ok(spec.includes("  /api/button/right/press:"));
});

test("openapi.yaml defines GET /api/button/right/long", () => {
    assert.ok(spec.includes("  /api/button/right/long:"));
});

test("openapi.yaml defines GET /api/button/middle/press", () => {
    assert.ok(spec.includes("  /api/button/middle/press:"));
});

test("openapi.yaml defines GET /api/button/middle/long", () => {
    assert.ok(spec.includes("  /api/button/middle/long:"));
});

test("openapi.yaml no longer defines POST /api/button", () => {
    // Use indexOf instead of regex to avoid escaping issues
    const hasPost = spec.includes('  "/api/button":\n    post:');
    const hasPost2 = spec.includes('  "/api/button:\n  post:');
    assert.ok(!hasPost, "POST /api/button should not exist");
    assert.ok(!hasPost2, "POST /api/button should not exist (alt format)");
});

test("openapi.yaml has button routes under Device control", () => {
    // spec is a string, check for button routes by looking for /api/button/ patterns
    const buttonRoutes = [
        "  /api/button/left/press:",
        "  /api/button/left/long:",
        "  /api/button/right/press:",
        "  /api/button/right/long:",
        "  /api/button/middle/press:",
        "  /api/button/middle/long:"
    ];
    let found = 0;
    for (const route of buttonRoutes) {
        if (spec.includes(route)) {
            found++;
        }
    }
    assert.ok(found >= 1, `Expected at least 1 button route, got ${found}`);
});
