#!/usr/bin/env python3
"""Render the current configuration UI with sample data, without a clock or server."""

import argparse
import gzip
import json
import mimetypes
from pathlib import Path
import subprocess
import sys
from urllib.parse import unquote, urlsplit


ROOT = Path(__file__).resolve().parents[1]
ORIGIN = "http://nsclock.test"
LATEST_VERSION_URL = (
    "https://raw.githubusercontent.com/ktomy/nightscout-clock/refs/heads/main/data/version.txt"
)


def positive_int(value):
    number = int(value)
    if number <= 0:
        raise argparse.ArgumentTypeError("must be greater than zero")
    return number


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--output", type=Path, default=ROOT / "docs/images/web-ui.png",
        help="PNG destination (default: docs/images/web-ui.png in the repository)",
    )
    parser.add_argument("--width", type=positive_int, default=1440, help="viewport width (default: 1440)")
    parser.add_argument("--height", type=positive_int, default=900, help="viewport height (default: 900)")
    parser.add_argument("--browser", type=Path, help="use an installed Chrome/Chromium executable")
    args = parser.parse_args()
    if args.output.suffix.lower() != ".png":
        parser.error("--output must have a .png extension")

    try:
        from playwright.sync_api import Error, sync_playwright
    except ImportError:
        parser.exit(1, "Install dependencies: python -m pip install playwright\n"
                    "Then install Chromium: python -m playwright install chromium\n")

    try:
        subprocess.run(["node", str(ROOT / "web/build.mjs")], cwd=ROOT, check=True)
    except FileNotFoundError:
        parser.exit(1, "Install Node.js using the version in .node-version.\n")
    except subprocess.CalledProcessError:
        parser.exit(1, "Web asset generation failed; screenshot cancelled.\n")

    # Always start from factory defaults, never a developer's private config.json.
    config = json.loads((ROOT / "data/config_initial.json").read_text())
    config.update({
        "ssid": "Home WiFi",
        "password": "example-password",
        "nightscout_url": "https://example.com",
        "units": "mgdl",
        "tz": "Europe/Amsterdam",
        "tz_libc": "CET-1CEST,M3.5.0,M10.5.0/3",
    })
    version = (ROOT / "data/version.txt").read_text().strip()
    errors = []

    def serve(route):
        url = urlsplit(route.request.url)
        path = unquote(url.path)
        if route.request.method != "GET":
            errors.append(f"Unexpected request: {route.request.method} {path}")
            route.abort()
        elif route.request.url.split("?", 1)[0] == LATEST_VERSION_URL:
            route.fulfill(content_type="text/plain", body=version)
        elif f"{url.scheme}://{url.netloc}" != ORIGIN:
            errors.append(f"Unexpected external request: {route.request.url}")
            route.abort()
        elif path == "/config.json":
            route.fulfill(json=config)
        elif path == "/api/auth/status":
            route.fulfill(json={"enabled": False, "authenticated": False})
        elif path == "/api/status":
            route.fulfill(json={
                "isInAPMode": False, "isConnected": True, "hasInternet": True,
                "bgSourceStatus": "connected", "sgv": 110,
            })
        else:
            asset = (ROOT / "data" / (path.lstrip("/") or "index.html")).resolve()
            if not asset.is_relative_to(ROOT / "data"):
                errors.append(f"Invalid asset path: {path}")
                route.abort()
                return
            compressed = asset.with_name(asset.name + ".gz")
            if compressed.is_file():
                body = gzip.decompress(compressed.read_bytes())
            elif asset.is_file():
                body = asset.read_bytes()
            else:
                errors.append(f"Missing UI asset: {path}")
                route.fulfill(status=404, body="Not found")
                return
            content_type = mimetypes.guess_type(asset.name)[0] or "application/octet-stream"
            route.fulfill(content_type=content_type, body=body)

    try:
        with sync_playwright() as playwright:
            options = {"headless": True}
            if args.browser:
                options["executable_path"] = str(args.browser.resolve())
            browser = playwright.chromium.launch(**options)
            context = browser.new_context(
                viewport={"width": args.width, "height": args.height},
                device_scale_factor=1, locale="en-US", timezone_id="Europe/Amsterdam",
                color_scheme="dark", reduced_motion="reduce", service_workers="block",
            )
            context.route("**/*", serve)
            page = context.new_page()
            page.on("pageerror", lambda error: errors.append(str(error)))
            page.goto(ORIGIN, wait_until="load")
            page.locator("body[data-state=ready]").wait_for(state="attached")
            page.wait_for_function("document.querySelector('#pill_wifi b').textContent === 'Connected'")
            page.wait_for_function(
                "document.querySelector('#fw_status').textContent === 'You are using the latest version.'"
            )
            page.evaluate("document.fonts.ready")
            if errors:
                raise RuntimeError("UI failed to render cleanly:\n" + "\n".join(errors))
            args.output.parent.mkdir(parents=True, exist_ok=True)
            page.screenshot(path=str(args.output), full_page=True, animations="disabled")
            browser.close()
    except (Error, RuntimeError, OSError) as error:
        print(f"Screenshot failed: {error}", file=sys.stderr)
        return 1
    print(f"Saved {args.output.resolve()}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
