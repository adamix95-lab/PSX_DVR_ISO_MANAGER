# PSX DVR ISO MANAGER — XMB → HDD → gra (TEST 1)

> **EXPERIMENTAL / NOT HARDWARE TESTED.** This feature is **not** part of the validated HDD 1.0 release and is kept on a separate branch. There is a verified, working HDD Manager build; do not change its logic, Neutrino, configs or games.
>
> **Test model:** Sony PSX DESR-7000 (PSX1); **existing** `hdd0:PP.DVRISO` partition with the fully tested HDD Manager. This is **not** a HDD formatter or an automatic partition creator.

[English notes](README_EN.md) · [HDD 1.0 technical documentation](../docs/HDD_1_0_TECHNICAL_PL.md)

## Status i zakres

Celem jest przetestowanie **XMB (Gry) → PSX DVR ISO MANAGER → gra PS2 z `dvr_pfs0:/ISO/`** bez ingerencji w działający HDD 1.0. W paczce wygenerowanej do testu znajdują się *dokładnie te same binaria* `BOOT.ELF` (OSDMenu Launcher, osadzone w KELF) i `DVR_ISO_MANAGER_HDD.ELF`, które pochodziły z fizycznie sprawdzonej paczki HDD. Nie kompilujemy żadnego nowego Managera.

| Etap | Stan |
| --- | --- |
| HDD Manager: 44/44 okładki (40+4), 2 gry, ENGLISH | **PASS — przetestowane wcześniej na DESR** |
| Zawartość ZIP, format ELF/MIPS i KELF, indeksowane PNG | **PASS — walidacja offline** |
| Nowy wpis `PSX DVR ISO MANAGER` pojawia się w XMB | **NIEPRZETESTOWANE** |
| Uruchomienie KELF z XMB | **NIEPRZETESTOWANE** |
| Przekazanie do HDD Managera i uruchomienie gry | **NIEPRZETESTOWANE** |

## Co zainstaluje wLaunchELF_R3Z?

Używamy **wyłącznie** `HDDManager → R1 → Inject Header → This partition only` na *istniejącej* `PP.DVRISO`. Wstrzyknięcie nagłówka APA jest operacją **zapisującą bezpośrednio metadane dysku**. Zrób pełną kopię nagłówka tej partycji i plików przed operacją, nie przerywaj zapisu i **nie wybieraj Create / Format / Expand ani opcji dla wszystkich partycji**.

R3Z szuka plików w `mass:/__Headers/PP.DVRISO/`. Wymagany jest `system.cnf`; `icon.sys` jest metadanymi dla HDD-OSD; `info.sys` i `jkt_002.png` R3Z kopiuje na PFS do `res/`. Nie nadpisuje oryginalnego `BOOT.ELF`, `BOOT.CNF` ani plików gry.

### Właściwa rozdzielczość grafiki

**PSX XMB używa `jkt_002.png` o rozdzielczości 76 × 108 pikseli**, 8-bit PNG z paletą i kanałem alfa; nie 74 × 108. `jkt_001.png` ma **256 × 256** i jest dla PSBBN. Grafika jest autorskim projektem: ciemnogranatowe tło, talerz HDD, niebieskie obwódki, pomarańczowy akcent, napisy PSX / DVR ISO / MANAGER.

Źródło: [wLaunchELF_R3Z — APA header injection](https://github.com/NathanNeurotic/wLaunchELF_R3Z#hdd-apa-header-injection).

## Dlaczego potrzebny jest KELF, a nie sam ELF Managera?

HDD Manager ma **4 185 356 B**, więc nie nadaje się do osadzenia w niewielkim obszarze APA. W PFS musi pozostać samodzielnym ELF-em. Wariant testowy używa:

```text
XMB (Gry) / PP.DVRISO
  └─ BOOT2 = pfs:/EXECUTE.ELF
       └─ EXECUTE.ELF — podpisany kontener KELF (2 026 464 B)
            └─ istniejący OSDMenu Launcher BOOT.ELF (197 396 B)
                 └─ EXECUTE.CNF (Quickboot)
                      └─ path=hdd0:PP.DVRISO:pfs:/DVR_ISO_MANAGER_HDD.ELF
                           └─ Manager HDD 1.0
                                └─ BOOT.CNF → OSDMenu → Neutrino FIX95 → gra
```

Kontener `EXECUTE.ELF` zawiera **KELF**, mimo rozszerzenia `.ELF`. Nazwa jest zamierzona: standardowy OSDMenu Launcher szuka odpowiednika `.CNF` tylko wtedy, gdy `argv[0]` ma rozszerzenie `.ELF`. W ten sposób nie modyfikujemy ani jednego bajtu znanego, działającego OSDMenu Launcher.

**Uwaga — istotne ryzyko integracyjne:** zachowanie przekazanego przez XMB `argv[0]` oraz uruchomienie kontenera `.ELF` w tym trybie nie zostały jeszcze zweryfikowane na fizycznym DESR. Jeśli pojawi się czarny ekran, jest to temat do diagnozy KELF/XMB/Quickboot, a nie powód do zmiany działającego Managera lub Neutrino.

## Pierwszy test — kroki

1. Zapisz kopię HDD / obecnego nagłówka APA `PP.DVRISO`; zanotuj aktualną zawartość partycji.
2. Potwierdź start `DVR_ISO_MANAGER_HDD.ELF` bezpośrednio przez wLE-R3Z i uruchomienie gry (znany punkt odniesienia).
3. Rozpakuj *paczę instalacyjną testu* na pendrive FAT32 **z zachowaniem katalogu `__Headers` w korzeniu USB**.
4. Skopiuj z `PFS_COPY_TO_PP.DVRISO/` na istniejącą `PP.DVRISO` wyłącznie `EXECUTE.ELF` i `EXECUTE.CNF`. Nie uruchamiaj podpisanego KELF z poziomu zwykłego polecenia „Execute ELF” w wLE.
5. Jeśli brak istniejącego Managera, użyj opcjonalnego `OPTIONAL_MANAGER_IF_MISSING/DVR_ISO_MANAGER_HDD.ELF`. **Nie podmieniaj** już działającego pliku.
6. W `wLaunchELF_R3Z → HDDManager` wybierz dokładnie `PP.DVRISO`, `R1 → Inject Header`, źródło `usb:/` lub `mass:/`, tryb **This partition only**. *Nie* twórz dodatkowych partycji.
7. Po zakończeniu odłącz zasilanie konsoli, uruchom ponownie XMB. Otwórz sekcję Gry, wybierz **PSX DVR ISO MANAGER**.
8. Jeśli uruchomi się Manager, sprawdź jedną znaną grę w SYSTEM i jedną w ENGLISH.
9. Jeśli wpis nie pojawia się lub ekran jest czarny, zrób zdjęcie. **Nie reinstaluj w ciemno**, nie formatuj `PP.DVRISO`. Przywróć z backupu wyłącznie nagłówek APA, jeśli chcesz cofnąć wpis; usunięcie plików `EXECUTE` nie przywraca nagłówka.

## Źródła i prawa autorów

- [RepairBox PSX HDD + Apps Setup v1.3](https://github.com/DanielFergisz/RepairBox-PSX-HDD-Setup) — sprawdzony schemat opakowania małego ELF w KELF i rejestrowania aplikacji w XMB. **NIE UŻYWAJ opcji L1/R1 do ponownego formatowania systemu**.
- [hdl-dump KELF template](https://github.com/DanielFergisz/RepairBox-PSX-HDD-Setup/tree/main/third_party/hdl-dump) — szablon i licencja GPLv2; minimalny odpowiadający kod źródłowy i GPLv2 są załączone do testowego ZIP.
- [wLaunchELF_R3Z](https://github.com/NathanNeurotic/wLaunchELF_R3Z) — właściwy mechanizm iniekcji APA wybranej partycji.
- [OSDMenu Launcher](https://github.com/pcm720/OSDMenu/tree/main/launcher) — niezmodyfikowany ELF do uruchamiania programu z PFS przez Quickboot.
- [PFS-BatchKit Manager](https://github.com/GDX-X/PFS-BatchKit-Manager) — historyczne wsparcie zasobów XMB/PSBBN; projekt oznaczony przez autorów jako zakończony.

### Ścisła izolacja od stabilnego HDD 1.0

Ten eksperyment jest na gałęzi `feature/hdd-xmb-installer-test`. Docelowa gałąź `feature/v1.0-hdd` i stabilny kod Managera/Neutrino pozostają bez zmian aż do sprzętowego PASS. Nie wstawiaj gotowego KELF do głównego wydania bez przetestowania XMB → Manager → gra.
