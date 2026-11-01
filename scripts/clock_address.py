"""Find the clock used for local testing.

An address given on the command line wins. Otherwise it is read from the
git-ignored .local_clock_ip file in the repository root, which holds one IP
address or hostname, for example 192.168.1.50.
"""
from pathlib import Path

ADDRESS_FILE = Path(__file__).resolve().parent.parent / ".local_clock_ip"


def resolve_clock_url(url=None):
    """Return the clock's base URL with a trailing slash, or None if no address is configured."""
    if not url and ADDRESS_FILE.is_file():
        url = ADDRESS_FILE.read_text().strip()
    if not url:
        return None
    if "://" not in url:
        url = "http://" + url
    return url.rstrip("/") + "/"
