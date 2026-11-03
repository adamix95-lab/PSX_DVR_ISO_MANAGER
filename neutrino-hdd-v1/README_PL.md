# PSX DVR ISO MANAGER 1.0 HDD — plan integracji (jeszcze nie wydanie)

## Obecny podgląd interfejsu 1.0 HDD — tylko odczyt

Dodatkowy target CI `DVR_ISO_MANAGER_HDD_PREVIEW.ELF` jest kompilowany z **tych samych źródeł menu** co USB 0.8/1.0, z etykietą **1.0 HDD**, zachowanymi 12 tytułami/stronę i podglądem PNG. Ścieżki aplikacji i cache zostały przeniesione na `pfs0:/` = `hdd0:PP.DVRISO`: `ART/`, `COVERS.IDX`, `COVER_SCAN.LOG`, `NET_PROBE.CNF` i inne. Samo `dvr_pfs0:/ISO` nadal pozostaje tylko do odczytu.

To **jeszcze nie finalny Manager**. Podgląd z commita `b8061f1` po resecie IOP ładuje osobno `ps2dev9`, `ps2atad`, `ps2hdd` i `ps2fs` z dokładnymi argumentami DIAG G, następnie `fileXioInit`, moduły kontrolera, `sceCdInit` / `sceCdNoticeGameStart` oraz `dvrdrv` / `dvrfile`. Dopiero potem sprawdza `hdd0:`, brak kolizji `pfs0:`, montuje `PP.DVRISO` z `FIO_MT_RDONLY` i weryfikuje katalog główny PFS oraz `dvr_pfs0:/ISO`. Jeśli moduły HDD/PFS lub partycja są niedostępne, program tylko wyświetli kod błędu i zakończy test bez zapisu. X, KWADRAT i pobieranie okładek są celowo zablokowane; TRÓJKĄT wykonuje wyłącznie nowy odczyt listy gier, bez zapisania indeksu. To pierwszy etap sprzętowej weryfikacji urządzenia, nie paczka do instalacji XMB.

Do testowania tego targetu `PP.DVRISO` musi już istnieć i zawierać odpowiednie pliki; **nie tworzymy ani nie formatujemy partycji automatycznie**. Kompilacja obu wariantów (USB jako kontrola regresji, HDD jako read-only preview), testy generatora CNF i statyczne kontrole bezpieczeństwa zakończyły się sukcesem w GitHub Actions: https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions/runs/41699032345 . **Nie jest to jeszcze wynik testu na DESR-7000.** Kolejny etap to fizyczne sprawdzenie obu systemów plików bez zapisu, dopiero potem osobny test zapisu CNF/ART.

---


**Stan sprzętowy:** DIAG Z FIX95 uruchomił prawdziwą grę z HDD na DESR-7000 po przejściu przez OSDMenu i pcm720 Neutrino z DVR. Ten sukces dotyczy testowej partycji PP.TESTDVR oraz wejściowego launchera UDPFS. Nowe PP.DVRISO, pełny Manager HDD i instalator XMB **nie są jeszcze przetestowane**.


## Kontrolowany pierwszy test na DESR-7000 (bez zapisu)

- Artefakt: [HDD READONLY PREVIEW z udanej kompilacji](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions/runs/41699032345) → sekcja **Artifacts**, nie paczka v1.0 do instalacji.
- Warunek: partycja aplikacji `hdd0:PP.DVRISO` już istnieje i jest prawidłowa. Test **nie tworzy**, nie formatuje i nie naprawia partycji; nie testować na partycjach Sony. Na razie **nie kopiować** żadnych plików na HDD, jeśli nie przygotowano ich wcześniej w kontrolowany sposób.
- Uruchomić tylko wariant `DVR_ISO_MANAGER_HDD_PREVIEW.ELF`; sprawdzić ekran inicjalizacji modułów, `APA ROOT`, `PP.DVRISO RO MOUNT`, `PFS ROOT` i `DVR ISO`. Oczekiwane: kody nieujemne i `BOTH FILESYSTEMS READABLE (RO)`.
- W menu: STRZAŁKI/SELECT mogą nawigować; TRÓJKĄT tylko ponownie odczytuje listę ISO; **X/KWADRAT nie uruchamiają gier ani nic nie zapisują**; START kończy program i odmontowuje własny `pfs0:`.
- Jeśli program zawiesi się lub zwróci błąd, zanotować ostatni komunikat. Nie ponawiać prób zapisu ani nie zmieniać systemowych partycji PSX. Wynik tylko potwierdzi etap odczytu — nie autoryzuje jeszcze włączenia zapisu.

## Układ nowej partycji hdd0:PP.DVRISO (pfs0:/)

    DVR_ISO_MANAGER.ELF
    BOOT.ELF                    # oryginalny OSDMenu pcm720
    BOOT.CNF                    # wybrana gra, automatyczna zmiana przy X
    BOOT_RECOVERY.CNF           # kopia awaryjna
    neutrino_hdd.elf           # tryb SYSTEM
    neutrino_hdd_lang.elf      # tryb ENGLISH
    NET_PROBE.CNF
    config/system.toml
    config/bsd-dvr.toml
    config/osdlang.toml
    modules/ee_core.elf
    modules/ee_core_lang.elf
    modules/dvrfile_fhi.irx
    modules/...                # komplet zgodnych modułów PR #228
    ART/<GAME_ID>_COV.png
    COVERS.IDX
    COVER_SCAN.LOG

Obrazy ISO pozostają bez zmian w dvr_pfs0:/ISO/ — nie przenosimy gier i nie formatujemy żadnej partycji Sony.

## Mechanizm uruchamiania

- USB 1.0: UI jak w 0.8, nagłówek 1.0 USB, dotychczasowy mass0:/DESR_DVR/.
- HDD 1.0: ten sam interfejs, nagłówek 1.0 HDD; zapisy CNF i ART tylko na PP.DVRISO.
- SYSTEM: neutrino_hdd.elf.
- ENGLISH: neutrino_hdd_lang.elf + istniejący, sprzętowo sprawdzony patch neutrino-v02/neutrino-osdlang-en.patch + dopasowane modules/ee_core_lang.elf i config/osdlang.toml. Nie zmieniamy trwale języka XMB/NVRAM.

Generator CNF (dvr-iso-manager/src/hdd_boot_config.c) jest już dodany i testowany host-side. Zachowuje -dev9=NICHDD na końcu argumentów i ignorowaną linię #PAD na końcu całego pliku. Przestrzega dokładnej wielkości liter w nazwie ISO.

## Budowanie Neutrino HDD

Eksperymentalny workflow build-neutrino-hdd-v1.yml kompiluje dwie osobne binarki z przypiętego commita pcm720 PR #228 aa8d80a1a0b5323cb407c814c0568c3489597c1c.

Wspólna poprawka patch_hdd_runtime.py zawiera potwierdzone na sprzęcie fileXioInit, ponowny mount PP.DVRISO jako pfs0: wyłącznie do odczytu, chdir pfs0:/ i oryginalne Neutrino. ENGLISH dodatkowo buduje istniejącą poprawkę językową oraz osobny EE core. Są to podglądowe, NIEPRZETESTOWANE na PP.DVRISO pliki; nie zastępować nimi działających instalacji.

## Etapy pozostające do zrobienia przed wydaniem

1. **Kod i CI gotowe; sprzęt PENDING:** uruchomić osobny `DVR_ISO_MANAGER_HDD_PREVIEW.ELF` na DESR-7000 z uprzednio istniejącą `PP.DVRISO`, potwierdzić komunikat `BOTH FILESYSTEMS READABLE (RO)`, listę ISO, odczyt indeksu i okładek oraz START → XMB. Przy błędzie zrobić zdjęcie kodu; nie nadpisywać żadnych plików.
2. Dopiero po teście odczytu włączyć bezpieczny zapis w PP.DVRISO: CNF z kopią awaryjną i weryfikacją; indeksy okładek i logi.
3. Przenieść istniejący downloader HTTP na zapis ART na HDD przez plik .PART i kontrolę przed podmianą. Gry i partycje Sony pozostają nietknięte.
4. Przetestować samodzielne uruchomienie Managera z HDD, SYSTEM i ENGLISH, ponowne wybieranie gier, ART i stabilność.
5. Dopiero wtedy instalator XMB — oddzielny, odwracalny projekt z własnym testem bezpieczeństwa.

Do tego czasu nie nadpisywać oryginalnego BOOT.CNF, BOOT.ELF, neutrino.elf, działającego USB v0.8 ani plików systemowych Sony. Diagnostyki zostają w Draft PR #26; stabilny main niezmieniony.
