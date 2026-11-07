// Tests the settings page's real conversion and validation code from web/src/js/schema.js.
import assert from "node:assert/strict"
import fs from "node:fs"
import path from "node:path"
import test from "node:test"
import vm from "node:vm"
import { fileURLToPath } from "node:url"

const ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..")
const read = file => fs.readFileSync(path.join(ROOT, file), "utf8")

// The page's scripts are classic scripts sharing one scope, so load them the same way.
const { isValidRtttl, mergeSettingsFile, mgdlToText, normalizeLoaded, parseNightscoutUrl, tabOfKey, textToMgdl, validateConfig } =
    vm.runInThisContext(`(function () {
        ${read("web/src/js/util.js")}
        ${read("web/src/js/schema.js")}
        return { isValidRtttl, mergeSettingsFile, mgdlToText, normalizeLoaded, parseNightscoutUrl, tabOfKey, textToMgdl, validateConfig }
    })()`)

const TZ_NAMES = new Set(["Europe/Berlin"])

/** Factory settings with the fields a user must fill in before saving. */
function validConfig(overrides = {}) {
    return normalizeLoaded({
        ...JSON.parse(read("data/config_initial.json")),
        ssid: "home",
        password: "password1",
        nightscout_url: "https://ns.example.com/",
        units: "mgdl",
        tz: "Europe/Berlin",
        tz_libc: "CET-1CEST,M3.5.0,M10.5.0/3",
        ...overrides,
    })
}
const errorsOf = config => validateConfig(config, { openNetwork: false, tzNames: TZ_NAMES })

test("factory settings are valid once the required fields are filled in", () => {
    assert.deepEqual(errorsOf(validConfig()), {})
})

test("the no data timer accepts 6 to 60 minutes only when it is enabled", () => {
    const timerError = minutes => errorsOf(validConfig({ custom_nodatatimer_enable: true, custom_nodatatimer: minutes })).custom_nodatatimer
    assert.ok(timerError(5))
    assert.equal(timerError(6), undefined)
    assert.equal(timerError(60), undefined)
    assert.ok(timerError(61))
    assert.equal(errorsOf(validConfig({ custom_nodatatimer_enable: false, custom_nodatatimer: 5 })).custom_nodatatimer, undefined)
})

test("a custom MAC address must be well formed and unicast", () => {
    const macError = mac => errorsOf(validConfig({ custom_mac: mac })).custom_mac
    assert.equal(macError(""), undefined)
    assert.equal(macError("A4:83:E7:2B:10:9C"), undefined)
    assert.ok(macError("A4:83:E7:2B:10"))
    assert.ok(macError("A5:83:E7:2B:10:9C"))
})

test("textToMgdl converts both units and rejects other input", () => {
    assert.equal(textToMgdl("110", "mgdl"), 110)
    assert.equal(textToMgdl("5.5", "mmol"), 99)
    assert.ok(Number.isNaN(textToMgdl("5.5", "mgdl")))
    assert.ok(Number.isNaN(textToMgdl("abc", "mmol")))
})

test("mgdlToText converts mmol/l the way Nightscout and the clock show it", () => {
    assert.equal(mgdlToText(54, "mmol"), "3.0")
    assert.equal(mgdlToText(55, "mmol"), "3.1")
    assert.equal(mgdlToText(99, "mmol"), "5.5")
    assert.equal(mgdlToText(109, "mmol"), "6.1")
    assert.equal(mgdlToText(181, "mmol"), "10.0")
    assert.equal(mgdlToText(250, "mmol"), "13.9")
    assert.equal(mgdlToText(110, "mgdl"), "110")
})

test("every mmol/l input from 2.0 to 29.9 reads back unchanged", () => {
    for (let tenths = 20; tenths <= 299; tenths++) {
        const text = (tenths / 10).toFixed(1)
        assert.equal(mgdlToText(textToMgdl(text, "mmol"), "mmol"), text)
    }
})

test("parseNightscoutUrl splits an address and falls back to empty HTTPS", () => {
    assert.deepEqual(parseNightscoutUrl("HTTP://ns.example.com:1337/"), { protocol: "http", host: "ns.example.com", port: "1337" })
    assert.deepEqual(parseNightscoutUrl("not a url"), { protocol: "https", host: "", port: "" })
})

test("isValidRtttl requires a name, defaults and notes", () => {
    assert.ok(isValidRtttl("beep:d=4,o=5,b=100:c,e,g"))
    assert.ok(!isValidRtttl("beep:d=4,o=5:c"))
    assert.ok(!isValidRtttl("just text"))
})

test("tabOfKey finds the tab that shows a setting", () => {
    assert.equal(tabOfKey("custom_hostname"), "system")
    assert.equal(tabOfKey("alarm_low_enabled"), "alarms")
    assert.equal(tabOfKey("low_mgdl"), "glucose")
    assert.equal(tabOfKey("brightness_level"), "display")
})

test("a settings file cannot change the login, and changes the network only when asked", () => {
    const clock = validConfig()
    const file = { web_auth_enable: true, web_auth_password: "password1", ssid: "other", not_a_setting: 1 }

    const merged = mergeSettingsFile(file, clock, { network: false, tzNames: TZ_NAMES })
    assert.equal(merged.config.web_auth_enable, clock.web_auth_enable)
    assert.equal(merged.config.ssid, "home")
    assert.ok(!("not_a_setting" in merged.config))

    assert.equal(mergeSettingsFile(file, clock, { network: true, tzNames: TZ_NAMES }).config.ssid, "other")
})

test("a settings file value that fails validation keeps the clock's value", () => {
    const merged = mergeSettingsFile({ custom_nodatatimer: 5 }, validConfig({ custom_nodatatimer: 20 }), { network: false, tzNames: TZ_NAMES })
    assert.equal(merged.config.custom_nodatatimer, 20)
    assert.deepEqual(merged.kept, ["custom_nodatatimer"])
})
