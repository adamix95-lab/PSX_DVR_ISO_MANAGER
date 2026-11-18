# PDIM 1.2 HDD — instalacja ręczna

**Oficjalna paczka:** [HDD 1.2 MANUAL](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/releases/download/1.2/PSX-DVR-ISO-Manager-v1.2.0-HDD-MANUAL-OFFICIAL.zip). Przed zapisem wymagana jest pełna, odtwarzalna kopia sektorowa HDD.

Docelowo **hdd0:PP.DVRISO**. `PP.DVRISO12` była tylko partycją testową. Obrazy ISO pozostają na natywnej partycji DVR Sony.

1. Rozpakuj paczkę **MANUAL** na pendrive FAT32. Folder `__Headers/PP.DVRISO/` musi pozostać pod dokładnie tą nazwą — tak rozpoznaje go wLE-R3Z.
2. Uruchom wLE-R3Z 4.80 i sprawdź układ HDD. Jeżeli istnieje `PP.DVRISO`, **nie twórz jej ponownie, nie formatuj i nie usuwaj**. Jeżeli nie istnieje, po wykonaniu pełnego backupu ręcznie utwórz partycję 256 MiB PFS o dokładnej nazwie `PP.DVRISO` przez HDDManager; zweryfikuj nazwę.
3. Przy aktualizacji z HDD 1.0 zabezpiecz stary `DVR_ISO_MANAGER_HDD.ELF` (na PC lub USB). Zachowaj istniejące `BOOT.CNF`, `V12_NET.CNF`, `ART/`, indeksy oraz własne ustawienia.
4. Skopiuj **zawartość** `PP.DVRISO-FILES/` do katalogu głównego PFS, zachowując strukturę `config/`, `modules/`, `res/`. Nie twórz dodatkowego podfolderu `PP.DVRISO-FILES`. Pakiet zawiera 79 plików Core i 5 plików XMB.
5. Jeśli na HDD brakuje `V12_NET.CNF`, wykorzystaj dostarczony w głównym katalogu paczki manualnej plik `V12_NET.CNF` (IP=192.168.0.240, NETMASK=255.255.255.0, GATEWAY=192.168.0.1, DNS=1.1.1.1) i skopiuj go do `PP.DVRISO`. Te adresy nie są uniwersalne — w razie potrzeby dostosuj je do swojego routera. **Jeżeli na HDD istnieje już `V12_NET.CNF`, pozostaw go bez nadpisywania.** Plik celowo nie znajduje się w `PP.DVRISO-FILES/`, żeby zbiorcze kopiowanie nie podmieniło ustawień.
6. Uruchom ręcznie `DVR_ISO_MANAGER_HDD.ELF` przez wLE. Sprawdź tytuł 1.2 HDD, wykrywanie gier, SYSTEM/ENGLISH i start wybranej gry.
7. **Tylko jeśli wpis XMB nie jest zarejestrowany:** gdy `__Headers/PP.DVRISO/` jest na pendrive, wybierz w wLE-R3Z **HDDManager → PP.DVRISO → R1 → Inject Header → This partition only**. Pięć plików XMB musi już znajdować się na PFS. Jeśli wpis HDD 1.0 działa, nie wstrzykuj nagłówka ponownie bez powodu — podmiana Managera wystarcza do uruchomienia 1.2.
8. Wyłącz i ponownie włącz konsolę. Sprawdź wpis Games, brak `Corrupted Data`, start z XMB, dwie gry i ENGLISH. Przetestuj HTTPS przez pobranie tylko wcześniej zabezpieczonych, brakujących okładek. Potem ponowny skan powinien pominąć już istniejące PNG.

Przy jakimkolwiek błędzie zatrzymaj się, zapisz kod i nie formatuj dysku. Wariant ręczny nie zapewnia automatycznej kontroli CRC i zabezpieczonych kopii instalatora.
