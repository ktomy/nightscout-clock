// The settings being edited: the loaded config, the working copy, what changed, and validation.

/**
 * Page-only values used for validation and rebuilding controls; never saved as clock settings.
 * @typedef {Object} FormContext
 * @property {boolean} openNetwork Whether the primary WiFi network intentionally has no password.
 * @property {Set<string> | null} tzNames Known timezone names, or null before loading them.
 * @property {string} [status] Last source name announced to dependent controls.
 * @property {number} [patients] Patient count used to notify the patient picker of newly loaded data.
 */

// Keep the saved configuration, editable draft, validation errors, and subscriptions private.
// Expose methods that update the draft and notify the UI when values or errors change.
const form = (() => {
    const events = emitter()
    let original = null
    let draft = null
    let errors = {}
    const touched = new Set()
    let showAll = false
    // Page-only state that shapes validation or rendering but isn't a config key.
    const ctx = { openNetwork: false, tzNames: null }

    /**
     * Validate the current draft using page context and notify subscribers that errors may have changed.
     * @returns {void}
     */
    function revalidate() {
        errors = draft ? validateConfig(draft, ctx) : {}
        events.emit("errors")
    }

    /**
     * Replace the draft with a copy of the supplied config and reset validation visibility.
     * Infer open-WiFi mode, validate the new draft, and announce that settings were loaded.
     * @param {ClockConfig} config - Configuration used as the new draft baseline.
     * @returns {void}
     */
    function reset(config) {
        draft = clone(config)
        ctx.openNetwork = !String(draft.password || "").trim() && !!String(draft.ssid || "").trim()
        touched.clear()
        showAll = false
        revalidate()
        events.emit("load")
    }

    return {
        on: events.on,
        ctx,
        /**
         * Normalize configuration from the clock, remember it as the saved baseline, and reset the draft.
         * @param {ClockConfig} config - Configuration used as the new draft baseline.
         * @returns {void}
         */
        load(config) {
            original = normalizeLoaded(config)
            reset(original)
        },
        /**
         * Restore the saved baseline, dropping unsaved edits and resetting validation visibility.
         * @returns {void}
         */
        discard() { reset(original) },
        /**
         * Report whether a configuration draft exists so rendering can wait until settings are loaded.
         * @returns {boolean}
         */
        get loaded() { return !!draft },
        /**
         * Read a setting from the draft, returning undefined before configuration has loaded.
         * @param {string} key - Setting key to read from the draft.
         * @returns {JsonValue | undefined}
         */
        get(key) { return draft ? draft[key] : undefined },
        /**
         * Update a draft setting only when its value changes, then validate and announce the changed key.
         * @param {string} key - Setting key to update.
         * @param {JsonValue} value - Replacement draft value.
         * @returns {void}
         */
        set(key, value) {
            if (sameJson(draft[key], value)) return
            draft[key] = value
            revalidate()
            events.emit("change", key)
        },
        /**
         * Update page-only context, revalidate, and emit a ctx-prefixed key for dependent UI blocks.
         * @param {keyof FormContext} name - Context property to update.
         * @param {FormContext[keyof FormContext]} value - New page-only context value.
         * @returns {void}
         */
        setCtx(name, value) {
            ctx[name] = value
            revalidate()
            events.emit("change", `ctx.${name}`)
        },
        /**
         * Compare the original and draft save payloads and return the keys whose serialized values differ.
         * @returns {string[]}
         */
        changes() {
            if (!draft) return []
            const a = buildSaveJson(original), b = buildSaveJson(draft)
            return [...new Set([...Object.keys(a), ...Object.keys(b)])].filter(k => !sameJson(a[k], b[k]))
        },
        /**
         * Mark a field as interacted with so its validation errors can appear before a Save attempt.
         * @param {string} key - Setting whose validation message may now be shown.
         * @returns {void}
         */
        touch(key) {
            if (touched.has(key)) return
            touched.add(key)
            events.emit("errors")
        },
        /**
         * Expose all validation errors and revalidate, typically when the user tries to save.
         * @returns {void}
         */
        showAllErrors() {
            showAll = true
            revalidate()
        },
        /**
         * Expose all current validation errors so Save can reject invalid settings, including untouched fields.
         * @returns {ValidationErrors}
         */
        get errors() { return errors },
        /**
         * Return all errors after a Save attempt, otherwise only errors for fields marked as touched.
         * @returns {ValidationErrors}
         */
        visibleErrors() {
            return showAll ? errors : Object.fromEntries(Object.entries(errors).filter(([k]) => touched.has(k)))
        },
        /**
         * Create the normalized save payload from the current draft without changing the draft itself.
         * @returns {ClockConfig}
         */
        saveJson() { return buildSaveJson(draft) },
    }
})()
