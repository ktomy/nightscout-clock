// Small DOM and data helpers shared by the page.

const $ = (sel, root = document) => root.querySelector(sel)
const $$ = (sel, root = document) => Array.from(root.querySelectorAll(sel))

// el("div.card", {attrs}, children...): tag with optional .classes, attributes and children (nodes or text).
function el(spec, attrs, ...children) {
    const [tag, ...classes] = spec.split(".")
    const node = document.createElement(tag || "div")
    if (classes.length) node.className = classes.join(" ")
    if (attrs && (typeof attrs !== "object" || attrs instanceof Node || Array.isArray(attrs))) {
        children.unshift(attrs)
        attrs = null
    }
    for (const [k, v] of Object.entries(attrs || {})) {
        if (v == null || v === false) continue
        if (k.startsWith("on")) node.addEventListener(k.slice(2), v)
        else if (k === "dataset") Object.assign(node.dataset, v)
        else if (k in node && typeof v !== "string") node[k] = v
        else node.setAttribute(k, v === true ? "" : v)
    }
    for (const c of children.flat(Infinity)) {
        if (c == null || c === false) continue
        node.append(c instanceof Node ? c : document.createTextNode(String(c)))
    }
    return node
}

const clone = v => JSON.parse(JSON.stringify(v))
const sameJson = (a, b) => JSON.stringify(a) === JSON.stringify(b)
const sleep = ms => new Promise(r => setTimeout(r, ms))

// Minimal event hub.
function emitter() {
    const handlers = {}
    return {
        // Returns a function that removes the handler.
        on(name, fn) {
            (handlers[name] = handlers[name] || []).push(fn)
            return () => { handlers[name] = handlers[name].filter(h => h !== fn) }
        },
        emit(name, ...args) { (handlers[name] || []).slice().forEach(fn => fn(...args)) },
    }
}

// 24px stroke icons, inlined so the page needs no icon font.
const ICON_PATHS = {
    save: "M5 12l5 5L20 7",
    speaker: "M11 5L6 9H3v6h3l5 4V5zM15.5 8.5a5 5 0 0 1 0 7M18.5 5.5a9 9 0 0 1 0 13",
    lock: "M6 11h12v9H6zM8 11V8a4 4 0 0 1 8 0v3",
    eye: "M2 12s3.5-7 10-7 10 7 10 7-3.5 7-10 7S2 12 2 12zM12 15a3 3 0 1 0 0-6 3 3 0 0 0 0 6z",
    eyeOff: "M3 3l18 18M10.6 10.6a3 3 0 0 0 4.2 4.2M9.4 5.2A10.4 10.4 0 0 1 12 5c6.5 0 10 7 10 7a17 17 0 0 1-3.2 4.1M6.1 6.1A17 17 0 0 0 2 12s3.5 7 10 7a9.8 9.8 0 0 0 4.3-1",
    plus: "M12 5v14M5 12h14",
    trash: "M4 7h16M9 7V4h6v3M6 7l1 13h10l1-13",
    download: "M12 4v11M7 10l5 5 5-5M5 20h14",
    grid: "M4 4h7v7H4zM13 4h7v7h-7zM4 13h7v7H4zM13 13h7v7h-7z",
    bell: "M6 16V11a6 6 0 1 1 12 0v5l2 2H4l2-2zM10 20a2 2 0 0 0 4 0",
    drop: "M12 3s6 7 6 11a6 6 0 0 1-12 0c0-4 6-11 6-11z",
    chip: "M7 7h10v10H7zM10 3v4M14 3v4M10 17v4M14 17v4M3 10h4M3 14h4M17 10h4M17 14h4",
}
function icon(name) {
    const ns = "http://www.w3.org/2000/svg"
    const svg = document.createElementNS(ns, "svg")
    for (const [k, v] of Object.entries({ viewBox: "0 0 24 24", fill: "none", stroke: "currentColor", "stroke-width": "2",
        "stroke-linecap": "round", "stroke-linejoin": "round", "aria-hidden": "true", class: "icon-svg" })) svg.setAttribute(k, v)
    const path = document.createElementNS(ns, "path")
    path.setAttribute("d", ICON_PATHS[name] || "")
    svg.append(path)
    return svg
}

// Color names the firmware understands, as swatches on the page.
const COLOR_HEX = { green: "#22c55e", yellow: "#facc15", red: "#ef4444", cyan: "#22d3ee", blue: "#3b82f6", magenta: "#e879f9", gray: "#9ca3af" }
