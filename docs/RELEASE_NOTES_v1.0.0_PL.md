# PSX DVR ISO MANAGER v1.0.0 — opis wydania

**HDD staje się główną wersją rozwojową, a USB pozostaje wspierane.** W testach Sony PSX DESR-7000 HDD pobrał 44/44 okładki w dwóch seriach (40 + 4), uruchomił dwie gry (jedna z wymuszonym angielskim) i wystartował z XMB z ikoną PDIM HDD+DVD. Nagłówek Managera pokazuje `1.0 HDD` bez dopisku NETFIX; poprawka sieciowa wciąż działa.

## Pliki wydania

- `PSX-DVR-ISO-Manager-v1.0.0-HDD-COMPLETE.zip` — **identyczna bajt po bajcie ze sprawdzoną** paczka HDD, Neutrino SYSTEM/ENGLISH, OSDMenu, moduły, konfiguracja, `ART/`.
- `PSX-DVR-ISO-Manager-v1.0.0-USB-COMPLETE.zip` — osobna paczka USB z nagłówkiem `1.0 USB`, utworzona na bazie sprawdzonej wersji 0.8.
- `PSX-DVR-ISO-Manager-v1.0.0-XMB-PDIM-INSTALL.zip` — **opcjonalne** uruchamianie z XMB przez wLaunchELF_R3Z; ikona PDIM 76 × 108 PNG8.
- `SHA256SUMS.txt` — sumy plików ZIP.

Kompatybilny [ART Server v0.8 dla Windows](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/releases/tag/v0.8) pobiera się osobno.

## Instalacja

**HDD:** Zrób kopię zapasową. Skopiuj zawartość `PP.DVRISO-FILES/` na **istniejącą** partycję aplikacji `hdd0:PP.DVRISO`, nie zastępując własnych danych `ART/`, `BOOT.CNF`, `NET_PROBE.CNF` i indeksów bez potrzeby. Uruchom `DVR_ISO_MANAGER_HDD.ELF` z wLE. Obrazy ISO pozostają na `dvr_pfs0:/ISO/`.

**XMB opcjonalnie:** Po wykonaniu **kopii nagłówka APA**, skopiuj `EXECUTE.ELF` i `EXECUTE.CNF` na `PP.DVRISO`; użyj wLaunchELF_R3Z: **HDDManager → PP.DVRISO → R1 → Inject Header → This partition only**, ze źródła USB `__Headers/PP.DVRISO/`. **Nigdy nie wybieraj Format/Create/Expand ani iniekcji na wszystkie partycje.** Gdy XMB już działa, podmiana samego Managera nie wymaga ponownej iniekcji.

**USB:** Skopiuj folder `DESR_DVR/` z ZIP-a na pendrive FAT32 i uruchom `mass0:/DESR_DVR/DVR_ISO_MANAGER.ELF`.

## Testy i ograniczenia

HDD 1.0 oraz start XMB → Manager: **PASS** na DESR-7000. **USB 1.0: PASS — potwierdzone działanie na DESR-7000 dnia 7.11.2026.** Szczegółowe wyniki **USB v0.8** (40/40 okładek, REZ po angielsku) pochodzą z wcześniejszego testu i nie są przypisywane nowemu USB 1.0. Nie sprawdzono startu wszystkich 44 gier ani innych modeli DESR. Automatyczny instalator XMB i NATIVE QUIT nie są częścią wydania.

## Technika i autorzy

[Dokumentacja HDD 1.0, FIX95 i NETFIX](HDD_1_0_TECHNICAL_PL.md) · [English](HDD_1_0_TECHNICAL_EN.md). Neutrino i podstawowa emulacja DVD pochodzą z upstream, DVR FHI od pcm720 ([PR #228](https://github.com/ps2max32/neutrino/pull/228)); OSDMenu, PS2SDK, wLaunchELF_R3Z i hdl-dump mają swoich autorów i licencje. Do paczki XMB dołączono źródło i licencję szablonu GPLv2.
