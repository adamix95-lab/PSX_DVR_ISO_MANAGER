# PSX DVR ISO MANAGER 1.0 — XMB

**PASS — Sony PSX DESR-7000:** XMB uruchamia HDD Managera. Ikona PDIM (dysk HDD i DVD) w formacie `jkt_002.png`, PNG8 76 × 108. Wersja HDD jest od 1.0 podstawowym kierunkiem rozwoju.

To instalacja manualna: istniejąca `PP.DVRISO`, wLaunchELF_R3Z i wstrzyknięcie nagłówka APA tylko tej partycji. Najpierw wykonaj odtwarzalną kopię nagłówka APA i danych. Nie twórz ani nie formatuj partycji.

1. Z pakietu XMB skopiuj `EXECUTE.ELF` (KELF) i `EXECUTE.CNF` do katalogu głównego już istniejącej `PP.DVRISO`.
2. Umieść `__Headers/PP.DVRISO/` w korzeniu USB FAT32.
3. W wLE-R3Z wybierz `HDDManager → PP.DVRISO → R1 → Inject Header → This partition only`.
4. Uruchom konsolę do XMB, wybierz `PSX DVR ISO MANAGER`.

Wpis XMB działa z oryginalnym, sprawdzonym Managerem; późniejsze aktualizacje wymagają tylko podmiany `DVR_ISO_MANAGER_HDD.ELF`. Nie reinjektuj nagłówka, jeśli wpis już działa. KELF używa niezmodyfikowanego OSDMenu Launcher. Licencja i źródło szablonu KELF (hdl-dump GPLv2) dołączone do paczki wydania.

[Opis wydania](../docs/RELEASE_NOTES_v1.0.0_PL.md) · [Neutrino FIX95 i HDD NETFIX](../docs/HDD_1_0_TECHNICAL_PL.md).
