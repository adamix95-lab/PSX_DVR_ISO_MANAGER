# G2 — QUIT GAME z uruchomionej gry, test A/B tylko z USB

**Sprzęt:** Sony PSX DESR-7000. **Stan:** wersja eksperymentalna, NIE jest działającym IGR ani instalatorem.  
**Gałąź:** `feature/native-quit-igr`, utworzona z wydania 1.0.

## Dlaczego G2?

Test G0 na prawdziwej DESR-7000 (2026-11-13) potwierdził:
- `sceCdInit(SCECdINoD)=1`
- `sceCdNoticeGameStart(1)=1`
- `MechaCon status=0x00000000`
- naciśnięcie fizycznego QUIT GAME w **samodzielnym diagnostycznym ELF** nie spowodowało żadnej reakcji; użytkownik wyłączył konsolę ręcznie.

Nie oznacza to jeszcze, że QUIT nie działa podczas **gry PS2**. Oryginalna procedura Sony uruchamia specjalny tryb gry / systemu przed NoticeGameStart (por. `OSD-Initialization-Libraries/osd/main.c`, `OSDMenu/common/src/psxinit.c`). Test G0 nie uruchamiał prawdziwej gry. Nie należy utożsamiać potwierdzonego statusu RPC z potwierdzonym powrotem do XMB.

G2 **nie powtarza** starego `feature/neutrino-desr-igr`, gdzie USB zakończyło się czarnym ekranem przed grą. Tamta implementacja dodawała nowe funkcje, próby, reakcje na błędy, a także dodatkowe wywołanie w późniejszym miejscu loadera. Teraz zmieniamy tylko **dwie wartości istniejącego wywołania** w Neutrino, zanim wystartuje ISO.

## Co jest w ZIP?

Paczka z CI zawiera **dwa oddzielne katalogi, każdy na pendrive**:
- `DESR_G2A/` — kontrolny: oficjalne, oryginalne Neutrino SYSTEM + ENGLISH z opublikowanej 1.0. Testowy Manager w tym katalogu jest identyczny logicznie z v1.0, lecz skierowany do `mass0:/DESR_G2A/`. Pozwala sprawdzić, czy sama kopia aplikacji do innej lokalizacji nadal startuje.
- `DESR_G2B/` — eksperyment: dwa zbudowane Neutrino SYSTEM + ENGLISH z dokładnie tego samego pinu upstream, z jedną minimalną zmianą w `ee/loader/src/main.c`. Testowy Manager korzysta z `mass0:/DESR_G2B/`.
- `G2_MANIFEST.txt` — kontrola zachowania oryginalnych plików i modułów.

**Zmiana w G2B:**

```diff
             sceCdInit(SCECdINoD);
-            uint8_t in[4] = {0};
+            uint8_t in[4] = {1};
             uint8_t out[16] = {};
-            sceCdApplySCmd(0x29, in, 4, out);
+            sceCdApplySCmd(0x29, in, 1, out);
             sceCdInit(SCECdEXIT);
```

Nie zmieniamy `sceCdInit`, kolejności resetów IOP, ładowania ISO, `dvrfile_fhi`, `ee_core`, języka ENGLISH, konfiguracji DVR ani żadnego hooka. Nie wprowadzamy drugiego wywołania. Kod nie wymusza błędu `-97` i nie przerywa startu z powodu odrzuconego SCMD — tak jak w stabilnej wersji, ale nie mamy tu osobnego odczytu statusu SCMD podczas gry.

## Jak testować

**Nie testuj podczas zapisywania save na karcie pamięci.** W tej próbie powrót do XMB NIE jest zagwarantowany.

1. Pobierz artefakt `DESR-QUIT-G2-AB-USB-UNTESTED` z workflow G2 na GitHub Actions. Rozpakuj **oba** katalogi `DESR_G2A` i `DESR_G2B` do katalogu głównego **FAT32 USB**. Nie nadpisuj `mass0:/DESR_DVR/`, nie kopiuj nic na HDD, nie zmieniaj instalacji XMB. Nie są potrzebne nowe partycje.
2. Przez wLE-R3Z uruchom **najpierw** `mass0:/DESR_G2A/DVR_ISO_MANAGER.ELF`. Upewnij się, że wyświetla nagłówek **1.0 USB G2A**.
3. W G2A wybierz dowolną **znaną działającą** grę (najpierw SYSTEM) i naciśnij X. Jeśli G2A **nie uruchomi gry**, zatrzymaj test i zgłoś, gdzie nastąpił błąd. Nie przechodź do G2B: to problem relokacji/środowiska, nie dowód skutków zmiany SCMD.
4. Jeśli G2A poprawnie uruchomi grę, sprawdź, czy fizyczny QUIT GAME działa, gdy gra jest uruchomiona i nic nie zapisuje. Zapamiętaj wynik. Jeśli nie działa, zrestartuj konsolę tradycyjnie.
5. Uruchom z wLE `mass0:/DESR_G2B/DVR_ISO_MANAGER.ELF` (nagłówek **1.0 USB G2B**), wybierz **tę samą grę, SYSTEM**, naciśnij X. Najpierw sprawdź, czy gra się uruchamia. Dopiero gdy działa i nie zapisuje, naciśnij **raz** fizyczny QUIT GAME. Zapisz: XMB / reset bez XMB / brak reakcji / czarny ekran.
6. Dopiero po poprawnym teście G2B SYSTEM można opcjonalnie porównać ENGLISH z tą samą grą. Nie testuj ENGLISH, jeśli SYSTEM ma problem z bootowaniem.

**Raport (wystarczy w wiadomości):** G2A gra + QUIT, G2B gra + QUIT, SYSTEM czy ENGLISH, nazwa gry, ewentualne zdjęcie ostatniego ekranu. Jeśli pojawi się czarny ekran, zanotuj, czy nastąpił przed startem gry, czy po QUIT.

## Izolacja i cofnięcie zmian

- Każdy testowy Manager aktualizuje tylko swój plik **USB** `DESR_G2A/BOOT.CNF` albo `DESR_G2B/BOOT.CNF`, używając zwykłej procedury v1.0; może pobierać okładki do swojego folderu, jeżeli podejmiesz akcję ART. To NIE są programy bez zapisu na USB.
- **Nie** zapisuje nic w `hdd0:PP.DVRISO` i nie dotyka gry ISO na `dvr_pfs0:/ISO/`. Pozostawia oryginalny `mass0:/DESR_DVR` nietknięty.
- Po teście uruchom pierwotną `mass0:/DESR_DVR/DVR_ISO_MANAGER.ELF`, albo aktualny Manager z XMB. Katalogi `DESR_G2A` i `DESR_G2B` można usunąć z USB.
- Fizyczny QUIT może spowodować natychmiastowy reset. Pozostałe ryzyka to typowe niestabilności eksperymentalnego loadera; dlatego używamy tylko USB i znanej gry.

## Budowa i weryfikacja (bez dotykania wydania)

Workflow `.github/workflows/native-quit-g2-ab.yml`:
1. Pobiera Neutrino PR228 z SHA `aa8d80a1a0b5323cb407c814c0568c3489597c1c`; buduje osobne SYSTEM/ENGLISH z dwoma zmienionymi wartościami, nie kopiuje nowych modułów.
2. Kompiluje dwa testowe Managery ze źródeł tagu 1.0, zmieniając tylko folder `DESR_G2A` lub `DESR_G2B`.
3. Pobiera publiczne wydanie 1.0 USB i **sprawdza dokładny SHA-256 archiwum**. Montuje dwie kopie, zachowując binarnie identyczne `BOOT.ELF`, moduły, konfiguracje, oryginalny SYSTEM/ENGLISH w G2A.
4. Sprawdza, że między folderami A/B różnią się **wyłącznie** trzy pliki: `DVR_ISO_MANAGER.ELF`, `neutrino.elf`, `neutrino_lang.elf`.
5. Publikuje wyłącznie artefakt eksperymentalny. **Nie** publikuje wydania 1.0, nie wykonuje merge, nie instaluje XMB ani nie dotyka HDD.

Test kompilacji / archiwum nie jest dowodem działania na prawdziwej DESR-7000.
