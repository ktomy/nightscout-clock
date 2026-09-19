// The settings being edited: the loaded config, the working copy, what changed, and validation.

const form = (() => {
    const events = emitter()
    let original = null
    let draft = null
    let errors = {}
    const touched = new Set()
    let showAll = false
    // Page-only state that shapes validation or rendering but isn't a config key.
    const ctx = { openNetwork: false, tzNames: null }

    function revalidate() {
        errors = draft ? validateConfig(draft, ctx) : {}
        events.emit("errors")
    }

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
        load(config) {
            original = normalizeLoaded(config)
            reset(original)
        },
        discard() { reset(original) },
        get loaded() { return !!draft },
        get(key) { return draft ? draft[key] : undefined },
        set(key, value) {
            if (sameJson(draft[key], value)) return
            draft[key] = value
            revalidate()
            events.emit("change", key)
        },
        setCtx(name, value) {
            ctx[name] = value
            revalidate()
            events.emit("change", `ctx.${name}`)
        },
        // Keys whose saved value would differ from what the clock has now.
        changes() {
            if (!draft) return []
            const a = buildSaveJson(original), b = buildSaveJson(draft)
            return [...new Set([...Object.keys(a), ...Object.keys(b)])].filter(k => !sameJson(a[k], b[k]))
        },
        touch(key) {
            if (touched.has(key)) return
            touched.add(key)
            events.emit("errors")
        },
        showAllErrors() {
            showAll = true
            revalidate()
        },
        get errors() { return errors },
        // Errors shown now: everything after a Save attempt, otherwise only fields the user has left.
        visibleErrors() {
            return showAll ? errors : Object.fromEntries(Object.entries(errors).filter(([k]) => touched.has(k)))
        },
        saveJson() { return buildSaveJson(draft) },
    }
})()
