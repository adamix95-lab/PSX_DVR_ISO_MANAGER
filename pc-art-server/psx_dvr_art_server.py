"""PSX DVR ART Server v0.8 — HTTPS OPL/PSBBN artwork to LAN HTTP.

The PC handles internet downloads and caches PNGs locally. Console and
USB contents are accessed only by the Manager, not by this server.
"""
from __future__ import annotations

import argparse
import io
import os
import queue
import re
import socket
import ssl
import threading
import urllib.error
import urllib.parse
import urllib.request
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from typing import Callable

from PIL import Image, ImageOps, UnidentifiedImageError

GAME_PATH = re.compile(r"^/([A-Z]{4}_[0-9]{3}\.[0-9]{2})_COV\.png$")
MAX_REMOTE_BYTES = 4 * 1024 * 1024
MAX_PNG_BYTES = 256 * 1024
# Primary artwork source: the OPL Manager archive used by CosmicScale.
OPL_SOURCE = (
    "https://archive.org/download/OPLM_ART_2024_09/"
    "OPLM_ART_2024_09.zip/PS2/{id}/{id}_COV.png"
)
# Fallback: CosmicScale's separate PSBBN square artwork collection.
PSBBN_SOURCE = (
    "https://raw.githubusercontent.com/CosmicScale/"
    "psbbn-art-database/main/art/{id}.png"
)


def trusted_archive_url(url: str) -> bool:
    try:
        parts = urllib.parse.urlsplit(url)
    except ValueError:
        return False
    host = (parts.hostname or "").lower()
    return (parts.scheme == "https" and parts.username is None and
            parts.password is None and parts.port in (None, 443) and
            (host == "archive.org" or host.endswith(".archive.org")))


class ArchiveRedirects(urllib.request.HTTPRedirectHandler):
    """HTTPS only, trusted archive.org hosts only, including CDN redirects."""

    def redirect_request(self, req, fp, code, msg, headers, newurl):
        if not trusted_archive_url(newurl):
            raise urllib.error.URLError("Rejected untrusted redirect")
        return super().redirect_request(req, fp, code, msg, headers, newurl)


def download_from_archive(game_id: str) -> bytes:
    url = OPL_SOURCE.format(id=game_id)
    if not trusted_archive_url(url):
        raise ValueError("Untrusted artwork URL")
    request = urllib.request.Request(
        url,
        headers={"User-Agent": "PSX-DVR-ART-Server/0.8",
                 "Accept": "image/png"})
    opener = urllib.request.build_opener(
        urllib.request.HTTPSHandler(context=ssl.create_default_context()),
        ArchiveRedirects())
    with opener.open(request, timeout=15) as response:
        if not trusted_archive_url(response.geturl()):
            raise ValueError("Untrusted redirect destination")
        if response.status != 200:
            raise ValueError(f"Archive HTTP {response.status}")
        raw = response.read(MAX_REMOTE_BYTES + 1)
    if len(raw) > MAX_REMOTE_BYTES:
        raise ValueError("Remote artwork exceeds 4 MiB")
    return raw


def trusted_psbbn_url(url: str, game_id: str) -> bool:
    """Allow only the verified CosmicScale artwork path for this Game ID."""
    if not GAME_PATH.fullmatch(f"/{game_id}_COV.png"):
        return False
    try:
        parts = urllib.parse.urlsplit(url)
    except ValueError:
        return False
    return (
        parts.scheme == "https"
        and (parts.hostname or "").lower() == "raw.githubusercontent.com"
        and parts.username is None
        and parts.password is None
        and parts.port in (None, 443)
        and not parts.query
        and not parts.fragment
        and parts.path
        == f"/CosmicScale/psbbn-art-database/main/art/{game_id}.png"
    )


class PsbbnRedirects(urllib.request.HTTPRedirectHandler):
    """Never follow PSBBN HTTP redirects outside the exact HTTPS art path."""

    def __init__(self, game_id: str):
        self.game_id = game_id

    def redirect_request(self, req, fp, code, msg, headers, newurl):
        if not trusted_psbbn_url(newurl, self.game_id):
            raise urllib.error.URLError("Rejected untrusted PSBBN redirect")
        return super().redirect_request(req, fp, code, msg, headers, newurl)


def download_from_psbbn(game_id: str) -> bytes:
    """Second source used by CosmicScale: square PSBBN artwork by Game ID."""
    url = PSBBN_SOURCE.format(id=game_id)
    if not trusted_psbbn_url(url, game_id):
        raise ValueError("Untrusted PSBBN artwork URL")
    request = urllib.request.Request(
        url,
        headers={
            "User-Agent": "PSX-DVR-ART-Server/0.8",
            "Accept": "image/png",
        },
    )
    opener = urllib.request.build_opener(
        urllib.request.HTTPSHandler(context=ssl.create_default_context()),
        PsbbnRedirects(game_id),
    )
    with opener.open(request, timeout=15) as response:
        if not trusted_psbbn_url(response.geturl(), game_id):
            raise ValueError("Untrusted PSBBN redirect destination")
        if response.status != 200:
            raise ValueError(f"PSBBN HTTP {response.status}")
        raw = response.read(MAX_REMOTE_BYTES + 1)
    if len(raw) > MAX_REMOTE_BYTES:
        raise ValueError("PSBBN artwork exceeds 4 MiB")
    return raw


def normalize_png(raw: bytes) -> bytes:
    """Reject corrupt/giant images and output PS2-friendly 140x200 RGB PNG."""
    if not raw or len(raw) > MAX_REMOTE_BYTES:
        raise ValueError("Empty/oversized artwork")
    if not raw.startswith(b"\x89PNG\r\n\x1a\n"):
        raise ValueError("Only PNG artwork is accepted")
    Image.MAX_IMAGE_PIXELS = 2_000_000
    try:
        with Image.open(io.BytesIO(raw)) as check:
            if check.format != "PNG":
                raise ValueError("Not a PNG")
            check.verify()
        with Image.open(io.BytesIO(raw)) as source:
            source.load()
            if source.width < 1 or source.height < 1:
                raise ValueError("Invalid image dimensions")
            rgb = source.convert("RGB")
            fitted = ImageOps.contain(
                rgb, (140, 200), method=Image.Resampling.LANCZOS)
            output = Image.new("RGB", (140, 200), "black")
            output.paste(fitted, ((140 - fitted.width) // 2,
                                  (200 - fitted.height) // 2))
            buf = io.BytesIO()
            output.save(buf, format="PNG", optimize=True)
            result = buf.getvalue()
    except (UnidentifiedImageError, OSError, SyntaxError) as exc:
        raise ValueError("Corrupt image or failed PNG decode") from exc
    if len(result) > MAX_PNG_BYTES:
        raise ValueError("Normalized PNG exceeds 256 KiB")
    return result


def default_cache() -> Path:
    root = os.environ.get("LOCALAPPDATA") or str(Path.home())
    return Path(root) / "PSX_DVR_ART_SERVER" / "cache"


class CoverService:
    def __init__(
        self,
        cache_dir: Path,
        log: Callable[[str], None] = print,
        fetch: Callable[[str], bytes] = download_from_archive,
        fallback_fetch: Callable[[str], bytes] = download_from_psbbn,
    ):
        self.cache_dir = Path(cache_dir)
        self.log = log
        self.fetch = fetch
        self.fallback_fetch = fallback_fetch
        self._lock = threading.Lock()
        self.cache_dir.mkdir(parents=True, exist_ok=True)

    def get(self, game_id: str) -> bytes | None:
        if not GAME_PATH.fullmatch(f"/{game_id}_COV.png"):
            return None
        destination = self.cache_dir / f"{game_id}_COV.png"
        with self._lock:
            if destination.exists():
                try:
                    data = destination.read_bytes()
                    # A valid local cache was normalized on its first download.
                    if len(data) <= MAX_PNG_BYTES and data.startswith(
                            b"\x89PNG\r\n\x1a\n"):
                        self.log(f"CACHE  {game_id}")
                        return data
                except OSError:
                    pass
                self.log(f"CACHE INVALID: {game_id}")
                return None  # Never overwrite existing cache files.

            # Prefer OPL covers; use square PSBBN artwork only as fallback.
            self.log(f"HTTPS OPL  {game_id} — querying archive...")
            try:
                raw = self.fetch(game_id)
                data = normalize_png(raw)
                source = "OPL"
            except (OSError, ValueError, urllib.error.URLError) as primary_error:
                self.log(f"OPL UNAVAILABLE  {game_id}: {primary_error}")
                self.log(f"HTTPS PSBBN FALLBACK  {game_id} — square artwork...")
                try:
                    raw = self.fallback_fetch(game_id)
                    data = normalize_png(raw)
                    source = "PSBBN"
                except (OSError, ValueError, urllib.error.URLError) as fallback_error:
                    self.log(f"NOT FOUND  {game_id}: PSBBN fallback: {fallback_error}")
                    return None

            try:
                # Exclusive creation: never overwrite an existing cached PNG.
                with destination.open("xb") as file:
                    file.write(data)
            except OSError as exc:
                self.log(f"CACHE SAVE FAILED  {game_id}: {exc}")
                return None
            self.log(f"SOURCE  {game_id}: {source}")
            self.log(f"READY  {game_id} ({len(data)} bytes)")
            return data


class CoverHandler(BaseHTTPRequestHandler):
    server_version = "DESR-Art-Server/0.8"
    protocol_version = "HTTP/1.0"

    def do_GET(self):
        service = self.server.service
        client_ip = self.client_address[0]
        if client_ip != self.server.console_ip:
            service.log(f"BLOCKED client: {client_ip}")
            self.send_error(403, "Console IP not allowed")
            return
        match = GAME_PATH.fullmatch(self.path)
        if not match:
            self.send_error(404, "Unknown cover path")
            return
        game_id = match.group(1)
        image = service.get(game_id)
        if image is None:
            self.send_error(404, "No verified PNG cover for this Game ID")
            return
        self.send_response(200)
        self.send_header("Content-Type", "image/png")
        self.send_header("Content-Length", str(len(image)))
        self.send_header("Cache-Control", "private, max-age=3600")
        self.send_header("Connection", "close")
        self.end_headers()
        try:
            self.wfile.write(image)
        except (BrokenPipeError, ConnectionResetError):
            service.log(f"CLIENT DISCONNECTED  {game_id}")

    def log_message(self, fmt, *args):
        self.server.service.log(
            f"{self.client_address[0]}  {fmt % args}")


def create_server(
    bind_ip: str,
    port: int,
    console_ip: str,
    service: CoverService,
) -> ThreadingHTTPServer:
    # Reject zero-address wildcard and accidental public-facing binds.
    for value in (bind_ip, console_ip):
        socket.inet_aton(value)
        if value == "0.0.0.0":
            raise ValueError("Use a specific local IPv4 address, not 0.0.0.0")
    if not 1 <= port <= 65535:
        raise ValueError("Port must be in range 1..65535")
    server = ThreadingHTTPServer((bind_ip, port), CoverHandler)
    server.daemon_threads = True
    server.service = service
    server.console_ip = console_ip
    return server


def run_gui() -> None:
    import tkinter as tk
    from tkinter import messagebox, ttk

    events: queue.Queue[str] = queue.Queue()
    root = tk.Tk()
    root.title("PSX DVR ART Server v0.8")
    root.geometry("760x540")
    root.minsize(660, 480)

    frame = ttk.Frame(root, padding=14)
    frame.pack(fill="both", expand=True)
    ttk.Label(frame, text="PSX DVR ART Server  •  v0.8",
              font=("Segoe UI", 15, "bold")).pack(anchor="w")
    ttk.Label(frame, text="Cover Sources",
              font=("Segoe UI", 10, "bold")).pack(anchor="w", pady=(5, 3))
    ttk.Label(
        frame,
        text="Primary — OPL Manager Archive (Archive.org): PS2 box-cover artwork.",
        wraplength=720,
    ).pack(anchor="w")
    ttk.Label(
        frame,
        text="Fallback — CosmicScale PSBBN Art Database (GitHub): square game "
             "artwork when the primary source has no valid cover.",
        wraplength=720,
    ).pack(anchor="w", pady=(2, 2))
    ttk.Label(
        frame,
        text="Delivery: PC downloads over HTTPS; PSX receives PNG over local LAN HTTP.",
        wraplength=720,
    ).pack(anchor="w", pady=(2, 12))

    form = ttk.Frame(frame)
    form.pack(fill="x")
    bind_ip = tk.StringVar(value="192.168.0.122")
    console_ip = tk.StringVar(value="192.168.0.240")
    port = tk.StringVar(value="8000")
    for col, (label, variable) in enumerate(
            (("PC IPv4 Address", bind_ip),
             ("Console IPv4 Address", console_ip), ("HTTP Port", port))):
        ttk.Label(form, text=label).grid(row=0, column=col, sticky="w", padx=(0, 12))
        ttk.Entry(form, textvariable=variable, width=22 if col < 2 else 10).grid(
            row=1, column=col, sticky="we", padx=(0, 12))
        form.columnconfigure(col, weight=1)

    ttk.Label(frame, text=f"PNG Cache: {default_cache()}",
              wraplength=690).pack(anchor="w", pady=(12, 7))
    status = tk.StringVar(value="Server stopped")
    ttk.Label(frame, textvariable=status,
              font=("Segoe UI", 10, "bold")).pack(anchor="w")

    buttons = ttk.Frame(frame)
    buttons.pack(fill="x", pady=(6, 8))
    console = tk.Text(frame, height=13, wrap="word", state="disabled")
    console.pack(fill="both", expand=True)

    service = CoverService(default_cache(), log=events.put)
    current: dict[str, ThreadingHTTPServer | None] = {"server": None}

    def start():
        if current["server"] is not None:
            return
        try:
            web_server = create_server(
                bind_ip.get().strip(), int(port.get()),
                console_ip.get().strip(), service)
        except (OSError, ValueError) as exc:
            messagebox.showerror("Unable to start HTTP server", str(exc))
            return
        current["server"] = web_server
        threading.Thread(target=web_server.serve_forever, daemon=True).start()
        status.set(f"Listening: {bind_ip.get()}:{port.get()} | Console: {console_ip.get()}")
        events.put("HTTP server started. Waiting for console requests.")
        start_btn.configure(state="disabled")
        stop_btn.configure(state="normal")

    def stop():
        server = current["server"]
        if server is None:
            return
        current["server"] = None
        # shutdown() is called from a separate thread so the GUI stays live.
        def finish():
            server.shutdown()
            server.server_close()
            events.put("HTTP server stopped.")
        threading.Thread(target=finish, daemon=True).start()
        status.set("Server stopped")
        start_btn.configure(state="normal")
        stop_btn.configure(state="disabled")

    start_btn = ttk.Button(buttons, text="Start HTTP Server", command=start)
    start_btn.pack(side="left", padx=(0, 8))
    stop_btn = ttk.Button(buttons, text="Stop Server", command=stop, state="disabled")
    stop_btn.pack(side="left")
    ttk.Label(buttons, text="Private LAN only — no port forwarding").pack(
        side="right")

    def pump():
        lines = []
        while True:
            try:
                lines.append(events.get_nowait())
            except queue.Empty:
                break
        if lines:
            console.configure(state="normal")
            console.insert("end", "\n".join(lines) + "\n")
            console.see("end")
            console.configure(state="disabled")
        root.after(120, pump)

    def on_close():
        server = current["server"]
        if server is not None:
            current["server"] = None
            server.shutdown()
            server.server_close()
        root.destroy()

    root.protocol("WM_DELETE_WINDOW", on_close)
    pump()
    root.mainloop()


def main() -> None:
    ap = argparse.ArgumentParser(description="Local Sony PSX OPL-artwork HTTPS gateway")
    ap.add_argument("--headless", action="store_true")
    ap.add_argument("--bind", default="192.168.0.122")
    ap.add_argument("--console", default="192.168.0.240")
    ap.add_argument("--port", type=int, default=8000)
    args = ap.parse_args()
    if not args.headless:
        run_gui()
        return
    server = create_server(args.bind, args.port, args.console,
                           CoverService(default_cache()))
    print(f"Listening on {args.bind}:{args.port} for {args.console}")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()


if __name__ == "__main__":
    main()
