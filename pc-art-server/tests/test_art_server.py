import io
import tempfile
import threading
import unittest
import urllib.error
import urllib.request
from http.server import ThreadingHTTPServer
from pathlib import Path

from PIL import Image
from psx_dvr_art_server import (
    CoverHandler, CoverService, GAME_PATH, MAX_PNG_BYTES, normalize_png,
    trusted_archive_url, ArchiveRedirects, trusted_psbbn_url,
    PsbbnRedirects, download_from_psbbn, PSBBN_SOURCE,
)


def fixture_png():
    buff = io.BytesIO()
    Image.new("RGB", (140, 200), (123, 55, 211)).save(buff, format="PNG")
    return buff.getvalue()


class ArtworkTests(unittest.TestCase):
    def test_id_whitelist(self):
        self.assertIsNotNone(GAME_PATH.fullmatch("/SLES_538.86_COV.png"))
        for name in ("/../SLES_538.86_COV.png", "/bad.png",
                     "/SLES_538.86_COV.png?x=1", "/scus_974.02_COV.png"):
            self.assertIsNone(GAME_PATH.fullmatch(name))

    def test_trusted_source(self):
        self.assertTrue(trusted_archive_url("https://archive.org/download/a"))
        self.assertTrue(trusted_archive_url("https://ia902300.us.archive.org/a"))
        for url in ("http://archive.org/", "https://archive.org.evil.com/",
                    "https://example.com/a", "https://evil.org@archive.org@x.com/"):
            self.assertFalse(trusted_archive_url(url))
        handler = ArchiveRedirects()
        req = urllib.request.Request("https://archive.org/foo")
        with self.assertRaises(urllib.error.URLError):
            handler.redirect_request(req, None, 302, "Found", {}, "http://127.0.0.1/a")

    def test_psbbn_source_is_exact_game_id_and_https_only(self):
        url = PSBBN_SOURCE.format(id="SLES_538.86")
        self.assertEqual(
            url,
            "https://raw.githubusercontent.com/CosmicScale/"
            "psbbn-art-database/main/art/SLES_538.86.png",
        )
        self.assertTrue(trusted_psbbn_url(url, "SLES_538.86"))
        for bad in (
            "http://raw.githubusercontent.com/CosmicScale/"
            "psbbn-art-database/main/art/SLES_538.86.png",
            "https://evil.example/CosmicScale/"
            "psbbn-art-database/main/art/SLES_538.86.png",
            "https://raw.githubusercontent.com/CosmicScale/"
            "psbbn-art-database/main/art/SCUS_974.02.png",
            url + "?download=1",
        ):
            self.assertFalse(trusted_psbbn_url(bad, "SLES_538.86"))
        request = urllib.request.Request(url)
        with self.assertRaises(urllib.error.URLError):
            PsbbnRedirects("SLES_538.86").redirect_request(
                request, None, 302, "Found", {},
                "https://untrusted.example/art.png",
            )

    def test_opl_success_never_uses_psbbn_fallback(self):
        attempted = []
        with tempfile.TemporaryDirectory() as path:
            def primary(game_id):
                attempted.append(("OPL", game_id))
                return fixture_png()
            def secondary(game_id):
                attempted.append(("PSBBN", game_id))
                return fixture_png()
            svc = CoverService(
                Path(path), log=lambda _: None,
                fetch=primary, fallback_fetch=secondary,
            )
            self.assertIsNotNone(svc.get("SLES_538.86"))
            self.assertEqual(attempted, [("OPL", "SLES_538.86")])

    def test_psbbn_fallback_for_missing_opl_with_square_art(self):
        tried = []
        logs = []
        square = io.BytesIO()
        Image.new("RGB", (256, 256), (255, 127, 0)).save(
            square, format="PNG")
        with tempfile.TemporaryDirectory() as path:
            def missing_opl(game_id):
                tried.append(("OPL", game_id))
                raise urllib.error.HTTPError(
                    "https://archive.org/missing", 404, "not found", {}, None)
            def psbbn(game_id):
                tried.append(("PSBBN", game_id))
                return square.getvalue()
            svc = CoverService(
                Path(path), log=logs.append,
                fetch=missing_opl, fallback_fetch=psbbn,
            )
            image = svc.get("SLES_538.86")
            self.assertIsNotNone(image)
            self.assertEqual(
                tried,
                [("OPL", "SLES_538.86"), ("PSBBN", "SLES_538.86")],
            )
            with Image.open(io.BytesIO(image)) as png:
                self.assertEqual((png.size, png.mode), ((140, 200), "RGB"))
                self.assertEqual(png.getpixel((0, 0)), (0, 0, 0))
                self.assertEqual(png.getpixel((70, 100)), (255, 127, 0))
            self.assertTrue(any("SOURCE" in msg and "PSBBN" in msg for msg in logs))
            self.assertEqual(svc.get("SLES_538.86"), image)
            self.assertEqual(len(tried), 2)  # second request from cache

    def test_both_missing_keeps_cache_empty(self):
        with tempfile.TemporaryDirectory() as path:
            def missing(_):
                raise urllib.error.URLError("server unavailable")
            svc = CoverService(
                Path(path), log=lambda _: None,
                fetch=missing, fallback_fetch=missing,
            )
            self.assertIsNone(svc.get("SLES_538.86"))
            self.assertFalse(
                (Path(path) / "SLES_538.86_COV.png").exists())

    def test_normalize_png(self):
        output = normalize_png(fixture_png())
        self.assertLess(len(output), MAX_PNG_BYTES)
        with Image.open(io.BytesIO(output)) as image:
            self.assertEqual((image.size, image.mode), ((140, 200), "RGB"))
        with self.assertRaises(ValueError):
            normalize_png(b"<html>error</html>")

    def test_cache_and_request(self):
        count = []
        with tempfile.TemporaryDirectory() as path:
            def fetch(game_id):
                count.append(game_id)
                return fixture_png()
            service = CoverService(Path(path), log=lambda _: None, fetch=fetch)
            first = service.get("SLES_538.86")
            second = service.get("SLES_538.86")
            self.assertEqual(first, second)
            self.assertEqual(count, ["SLES_538.86"])
            self.assertEqual(
                (Path(path) / "SLES_538.86_COV.png").read_bytes(), first)
            self.assertIsNone(service.get("../evil"))

    def test_existing_corrupt_cache_not_overwritten(self):
        with tempfile.TemporaryDirectory() as path:
            existing = Path(path) / "SLES_538.86_COV.png"
            existing.write_bytes(b"not PNG")
            svc = CoverService(Path(path), log=lambda _: None,
                               fetch=lambda _: fixture_png())
            self.assertIsNone(svc.get("SLES_538.86"))
            self.assertEqual(existing.read_bytes(), b"not PNG")

    def test_http_end_to_end_and_wrong_client(self):
        with tempfile.TemporaryDirectory() as path:
            srv = ThreadingHTTPServer(("127.0.0.1", 0), CoverHandler)
            srv.daemon_threads = True
            srv.service = CoverService(Path(path), log=lambda _: None,
                                       fetch=lambda _: fixture_png())
            srv.console_ip = "127.0.0.1"
            worker = threading.Thread(target=srv.serve_forever, daemon=True)
            worker.start()
            try:
                url = f"http://127.0.0.1:{srv.server_port}/SLES_538.86_COV.png"
                with urllib.request.urlopen(url, timeout=5) as response:
                    self.assertEqual(response.status, 200)
                    self.assertEqual(response.getheader("Content-Type"), "image/png")
                    body = response.read()
                    self.assertEqual(int(response.getheader("Content-Length")), len(body))
                self.assertTrue(body.startswith(b"\x89PNG\r\n\x1a\n"))
                with self.assertRaises(urllib.error.HTTPError) as exc:
                    urllib.request.urlopen(f"http://127.0.0.1:{srv.server_port}/../../x")
                self.assertEqual(exc.exception.code, 404)
                srv.console_ip = "192.168.0.240"
                with self.assertRaises(urllib.error.HTTPError) as exc:
                    urllib.request.urlopen(url)
                self.assertEqual(exc.exception.code, 403)
            finally:
                srv.shutdown()
                srv.server_close()
                worker.join(timeout=3)


if __name__ == "__main__":
    unittest.main()
