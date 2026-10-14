# DESR7000-DVR – DVR ISO Manager (etap 1 / wersja testowa 0.1)

Ta paczka jest **dodatkiem do repozytorium `adamix95-lab/DESR7000-DVR`**, nie nowym repozytorium.
Nie zastępuje sprawdzonej kompilacji Neutrino z PR #228 ani istniejącego workflow.

## Co robi obecny etap

- Resetuje IOP i próbuje zainicjalizować sterowniki USB oraz DVR.
- Próbuje otworzyć `dvr_pfs0:/ISO` (czyli `dvr_hdd0:/__xcontents/ISO` w wLE-R3Z).
- Wyświetla uporządkowaną listę zwykłych plików `.iso`/`.ISO` z tego katalogu (bez podkatalogów).
- Po wciśnięciu X tworzy `mass0:/DESR_DVR/BOOT.CNF` z **dokładną nazwą wybranego ISO**. Plik jest tworzony bez znacznika BOM.
- W ogóle nie zapisuje na partycjach DVR ani APA. Jedyny zapis to **nadpisanie BOOT.CNF na USB** po zatwierdzeniu X.

**Jeszcze nie uruchamia Neutrino automatycznie.** Po wyborze gry uruchom z wLE działający `mass0:/DESR_DVR/BOOT.ELF` (OSDMenu Launcher). To celowo rozdziela test skanowania DVR od uruchamiania programu. Automatyczne uruchamianie to następny etap, jeśli wykrywanie ISO zadziała.

## Układ plików na USB

```
mass0:/DESR_DVR/
  BOOT.ELF                <-- Twój działający OSDMenu Launcher (z USB)
  BOOT.CNF                <-- istniejący, może zostać nadpisany przez Manager
  neutrino.elf            <-- Twoja działająca wersja Neutrino DVR PR #228
  config/                 <-- pełny katalog Neutrino
  modules/                <-- pełny katalog Neutrino
  DVR_ISO_MANAGER.ELF     <-- wynik nowego workflow
```

Nie zastępuj modułów Neutrino plikami .irx z paczki źródłowej Managera. Są one **osadzone wewnątrz ELF Managera**, jedynie na potrzeby jego skanowania DVR.

## Kontroler

- D-pad góra/dół: wybór gry
- Trójkąt: ponowne skanowanie DVR
- X: nadpisanie `mass0:/DESR_DVR/BOOT.CNF` dla wybranej gry
- START: zamknięcie aplikacji

## Instalacja w GitHub Desktop

1. W repozytorium `DESR7000-DVR` utwórz katalog `dvr-iso-manager` i skopiuj do niego całą zawartość folderu `dvr-iso-manager` z tego ZIP.
2. Z ZIP skopiuj też `.github/workflows/build-dvr-iso-manager.yml` do `.github/workflows/` repozytorium. Nie kasuj starszego workflow Neutrino.
3. Commit i Push do `main`.
4. GitHub → Actions → **Build DVR ISO Manager v0.1** → Run workflow.
5. Jeśli zadanie będzie zielone, pobierz jego artefakt. Jeśli będzie czerwone, prześlij logi kompilacji.

## Ważne ograniczenia

- Kod **nie został jeszcze zbudowany toolchainem PS2 ani uruchomiony na DESR-7000**. Testy komputerowe dotyczą tylko generowania nazw/konfiguracji.
- DVR sterowniki pochodzą z wcześniej przesłanego, działającego artefaktu Neutrino PR #228 (commit `aa8d80a1a0b5323cb407c814c0568c3489597c1c`). Nie mamy jeszcze potwierdzenia, że ich inicjalizacja w samodzielnym programie działa identycznie jak w Neutrino.
- Ponieważ kod wykonuje reset IOP, wLE i Manager mogą mieć inny zestaw sterowników. Jeśli `dvr_pfs0:/ISO` nie jest dostępne, nie modyfikuj partycji DVR; prześlij zdjęcie ekranu z komunikatami.
- `BOOT.CNF` jest nadpisywany tylko po wciśnięciu X. Zachowaj jego kopię, jeśli jest Ci potrzebna.
- Długi katalog (>128 gier) zostanie ograniczony do pierwszych 128 odczytanych wpisów.
- **Pierwsze testy wykonujemy wyłącznie z USB**. Nie używamy Inject Header ani kanału XMB.

## Autorstwo i licencje

To niezależny prototyp. Osadzone moduły DVR pochodzą ze skompilowanej wersji Neutrino PR #228, autorstwa społeczności Neutrino/pcm720, na warunkach AFL-3.0 (patrz `NEUTRINO_LICENSE_AFL3.txt`). Źródła projektu Neutrino: https://github.com/ps2max32/neutrino/pull/228.
