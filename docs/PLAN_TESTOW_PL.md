# Plan testów DESR7000-DVR

1. **Build**: uruchom workflow; sprawdź, czy powstały `neutrino.elf`, trzy moduły DVR oraz `config/bsd-dvr.toml`.
2. **Bez zmian HDD**: rozpakuj artefakt i sprawdź kompletność plików. Nie trzeba jeszcze uruchamiać Neutrino na konsoli.
3. **Ocena pojemności DVR**: w wLE-R3Z sprawdź dostępność `dvr_hdd0:__xcontents` oraz ilość wolnego miejsca. Nie zmieniaj partycji.
4. **Mały test**: dopiero po potwierdzeniu miejsca skopiuj jeden własny ISO do `__xcontents` przez obsługiwane narzędzie.
5. **Start z argumentami**: uruchom Neutrino z `-bsd=dvr -dvd=dvr_pfs0:/<nazwa>.iso` przy użyciu launchera obsługującego argumenty.
6. **Raport**: wersja `PR228_COMMIT.txt`, ekran błędu/startu gry, gra i zachowanie XMB po restarcie.

Etapy 3–5 wymagają potwierdzenia, że używany interfejs dostępu do DVR działa na danej konfiguracji DESR. Zielony status GitHub Actions dotyczy wyłącznie kompilacji.
