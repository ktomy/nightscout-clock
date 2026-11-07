// Tests that the OpenAPI spec matches the device API contract.

import assert from "node:assert/strict";
import fs from "node:fs";
import path from "node:path";
import test from "node:test";
import { fileURLToPath } from "node:url";

const ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const read = (file) => fs.readFileSync(path.join(ROOT, file), "utf8");

const spec = read("openapi.yaml");

test("openapi.yaml excludes simulated button routes", () => {
  assert.doesNotMatch(spec, /^  \/api\/button\//m);
});

test("openapi.yaml defines POST /api/face", () => {
  assert.ok(spec.includes("  /api/face:"));
  assert.ok(spec.includes("    post:"));
});

test("openapi.yaml defines GET /api/faces", () => {
  assert.match(spec, /  \/api\/faces:\n    get:/);
});

test("openapi.yaml has one definition for each face route", () => {
  for (const route of [
    "/api/face",
    "/api/faces",
    "/api/face/next",
    "/api/face/prev",
  ]) {
    const definitions =
      spec.match(new RegExp(`^  ${route.replaceAll("/", "\\/")}:$`, "gm")) ||
      [];
    assert.equal(definitions.length, 1, `${route} should be defined once`);
  }
  assert.doesNotMatch(spec, /^  \/api\/face\/:$/m);
  assert.doesNotMatch(spec, /^  \/api\/faces\/:$/m);
});

test("openapi.yaml defines POST /api/face/next", () => {
  assert.ok(spec.includes("  /api/face/next:"));
  assert.ok(spec.includes("    post:"));
});

test("openapi.yaml defines POST /api/face/prev", () => {
  assert.ok(spec.includes("  /api/face/prev:"));
  assert.ok(spec.includes("    post:"));
});

test("display mutation routes document setup-mode unavailability", () => {
  const unavailableResponses =
    spec.match(/'503':\n\s+\$ref: '#\/components\/responses\/DisplayControlUnavailable'/g) || [];
  assert.equal(unavailableResponses.length, 6);
});

test("openapi.yaml defines POST /api/brightness/next", () => {
  assert.match(spec, /  \/api\/brightness\/next:\n    post:/);
});

test("openapi.yaml defines POST /api/brightness/prev", () => {
  assert.match(spec, /  \/api\/brightness\/prev:\n    post:/);
});

test("openapi.yaml defines POST /api/brightness with the supported values", () => {
  assert.match(spec, /  \/api\/brightness:\n    post:/);
  assert.match(spec, /enum: \[1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 100, 101\]/);
});
