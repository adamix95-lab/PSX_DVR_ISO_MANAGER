# G3 — izolacja trybu i długości komendy Native QUIT (PSX DESR-7000)

## Co wykazały testy

- **G0**: `sceCdInit=1`, `sceCdNoticeGameStart(1)=1`, `status=0`. W samodzielnym ELF fizyczny QUIT niczego nie zmienił.
- **G2B**: po starcie gry pojawił się **czarny ekran**, ale naciśnięcie fizycznego **QUIT GAME** spowodowało po kilku sekundach **powrót do oryginalnego XMB Sony**. To pierwszy potwierdzony sprzętowo powrót do XMB w eksperymencie — jednak **gra nie wystartowała**.
- **G2C**: kompilacja Neutrino z dokładnego pinu, bez modyfikacji, dała bajtowo ten sam ELF co w oficjalnym USB 1.0. Część związana z ponowną kompilacją nie wyjaśnia regresji G2B.

**Nie jest to gotowa poprawka:** nie potwierdzono normalnego działania gry i fizycznego QUIT równocześnie.

## Jedna zmienna na test

Stabilne Neutrino v1.0 używa `sceCdApplySCmd(0x29, in, 4, out)` z `in[0]=0`. W G2B zmieniono jednocześnie tryb i rozmiar, co utrudnia ustalenie przyczyny czarnego ekranu.

| Wariant | Tryb (`in[0]`) | Długość (`inBuffSize`) | Co zmieniono względem 1.0 |
| --- | --- | --- | --- |
| 1.0 / G2A | 0 | 4 | nic |
| G2B (poprzedni test) | 1 | 1 | tryb i długość |
| **G3A (nowy)** | **1** | **4** | tylko tryb |
| **G3B (nowy)** | **0** | **1** | tylko długość |

G3A/G3B to **ten sam pinned Neutrino DVR PR228** co działająca wersja 1.0, skompilowany w tym samym kontenerze. Nie dodano żadnego resetu IOP, wywołania komendy, hooka, dodatkowych modułów ani kodu IGR.

## Instalacja — tylko USB

1. W GitHub Actions znajdź workflow `Native QUIT G3 - SCMD mode vs byte count USB only`, pobierz artefakt **DESR-QUIT-G3-MODE-VS-LENGTH-USB-UNTESTED**, następnie wewnętrzny `DESR_QUIT_G3_MODE_VS_LENGTH_USB.zip`.
2. Rozpakuj **`DESR_G3A/`** i **`DESR_G3B/`** bezpośrednio na pendrive FAT32.
3. **Nie** nadpisuj `mass0:/DESR_DVR/`, plików wersji 1.0, `BOOT.ELF`, HDD `PP.DVRISO`, ani instalacji XMB. Gra ISO nadal jest czytana z DVR, nie jest zapisywana.

Każdy katalog zawiera swoją kopię programu Manager, OSDMenu i identyczne moduły/config z opublikowanej USB 1.0. Testowy Manager zapisuje `BOOT.CNF` **tylko w swoim folderze USB**. Do testu wybieraj **SYSTEM**; ENGLISH pozostawiono z oficjalnego 1.0 i nie jest przedmiotem G3.

## Kolejność próby

1. Przez wLE-R3Z 4.80 uruchom `mass0:/DESR_G3A/DVR_ISO_MANAGER.ELF`. Nagłówek musi zawierać `1.0 USB G3A`. Wybierz tę samą, znaną działającą grę co w G2B w **SYSTEM** i naciśnij X.
2. Zapisz, czy **gra rzeczywiście wystartowała**, czy jest **czarny ekran** (a nie tylko wynik samego Managera). Dopiero gdy konsola nic nie zapisuje, naciśnij raz fizyczny QUIT GAME. Zanotuj: XMB / brak reakcji / reset do innego ekranu / zawieszenie oraz przybliżone opóźnienie.
3. Powtórz identyczny test z `mass0:/DESR_G3B/DVR_ISO_MANAGER.ELF` (nagłówek `1.0 USB G3B`).
4. Przekaż: **G3A: start gry [tak/nie], QUIT [wynik]; G3B: start gry [tak/nie], QUIT [wynik]**. Nazwa gry i język SYSTEM wystarczą.

Nie testuj przy zapisywaniu save na karcie pamięci, pobieraniu okładek ani zapisie na nośnik. Nie naciskaj wielokrotnie QUIT. W razie zawieszenia bez reakcji użyj zwykłego wyłączenia po upewnieniu się, że nie trwa zapis danych.

## Możliwe interpretacje

- Jeśli **G3A** potrafi uruchomić grę i wyjść do XMB, może to oznaczać, że zachowanie **długości 4 bajtów** pozwala uniknąć regresji z G2B, jednocześnie aktywując QUIT.
- Jeśli G3A również ma czarny ekran, ale QUIT działa, a **G3B** uruchamia grę bez QUIT, podejrzana jest **wartość trybu 1**, a nie długość argumentu.
- Jeśli obie wersje startują lub obie się zawieszają, dalszy wniosek zależy od wyników i wymaga instrumentacji. Wyniki fizycznego przycisku nie zastępują weryfikacji stabilnego startu gry.

## Bezpieczeństwo

- Zmiany tylko w `feature/native-quit-igr`, wydanie `1.0` i tag nietknięte.
- Testy są **USB-only** i nie tworzą partycji, nie montują `PP.DVRISO` do zapisu, nie aktualizują XMB ani BIOS.
- Moduły DVR/FHI, ee_core i config pochodzą bajtowo z opublikowanego, hardware-tested USB 1.0.
- Nie kopiuj elementów eksperymentalnych do stabilnego `DESR_DVR`, zanim gra i fizyczny QUIT nie przejdą testów.
