PSX DVR ISO Manager v0.8 — kompletna paczka USB

NOWA INSTALACJA
1. Rozpakuj ZIP. Skopiuj caly folder DESR_DVR do katalogu glownego
   pamieci USB. Nie kopiuj plikow pojedynczo w inne miejsca.
2. Uruchom mass0:/DESR_DVR/DVR_ISO_MANAGER.ELF przez wLE / UDPFS.
3. Obrazy ISO trzymaj na partycji DVR w dvr_pfs0:/ISO.
   Program nie zapisuje do DVR, APA ani obrazow ISO.
4. Po nacisnieciu TROJKAT program tworzy COVERS.IDX i COVER_SCAN.LOG.
   Aby pobierac okladki, uruchom na Windows osobny PSX DVR ART Server v0.8
   (plik EXE w tym samym wydaniu GitHub).
5. Skonfiguruj NET_PROBE.CNF pod wlasna siec. Domyslne IP w paczce
   odpowiadaja tylko przetestowanemu przykladowi:
   PSX 192.168.0.240, PC 192.168.0.122, port 8000.
   W Windows wpisz ipconfig /all. NETMASK=Subnet Mask,
   GATEWAY=Default Gateway, SERVER_IP=IPv4 Address komputera.
   IP w NET_PROBE.CNF to wolny, statyczny adres PSX (nie PC).

AKTUALIZACJA ISTNIEJACEJ INSTALACJI
Najpierw zapisz na PC kopie CALEGO folderu DESR_DVR.
Nie nadpisuj przypadkowo swoich plikow BOOT.CNF, BOOT_RECOVERY.CNF,
NET_PROBE.CNF, ART/, COVERS.IDX ani COVER_SCAN.LOG.
Jezeli dotychczasowe OSDMenu/Neutrino dziala, zachowaj swoje ustawienia.

ZAWARTOSC
Wszystkie pliki konfiguracyjne i moduly OSDMenu/Neutrino
pochodza bez zmian z opublikowanej paczki v0.3.0, z jednym wyjatkiem:
plik BOOT.elf ma nazwe BOOT.ELF, zgodna ze sciezka Managera.
Manager ELF zostal wymieniony na oficjalne wydanie v0.8.
Dodano folder ART/, przykladowy NET_PROBE.CNF i ten plik.

Gry ISO i grafiki ART nie sa dolaczone. Okladki mozna pobrac przez PC.
Pelna instrukcja po polsku: README_PL.md w repozytorium GitHub.
