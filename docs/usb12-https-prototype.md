# PSX DVR ISO Manager 1.2 USB — prototyp

Dodatek do [wydania 1.0 USB](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/releases/tag/1.0).
Pobieracz bazuje na BATCH50 R3 (`314ed49752e42251c00ecfebc8fb0e3710745037`).
Na DESR-7000 prototyp 1.2 zapisał 45 z 45 okładek w dwóch podejściach.
Pierwszą serię przerwał błąd weryfikacji odpowiedzi Roughtime (`RET=-508`)
po 12 zapisach. Manager wrócił do listy i pomijał zapisane okładki przy
ponowieniu. Nowy ekran podsumowania czeka na próbę na konsoli.

## Instalacja na testowym USB FAT32

1. Na pendrivie z działającą wersją 1.0 skopiuj oba nowe pliki ELF do
   `DESR_DVR/`, obok `DVR_ISO_MANAGER.ELF`.
2. Dla HTTPS użyj działającego `N4E.CA` w katalogu głównym USB oraz istniejącego
   `hdd0:PP.DVRISO12/V12_NET.CNF` z testu BATCH50. Pobieracz czyta tę partycję
   aplikacji i katalog DVR ISO w trybie tylko do odczytu.
3. Przed każdą próbą HTTPS uruchom na PC generator z paczki. Zastąp `I:`
   literą pendrive'a.

   ```powershell
   py .\tools\generate_n6g_batch_entropy.py I:/N6G50.ENT
   ```

   Powstaje prywatny plik na jedno wywołanie, 16 000 bajtów. Wygeneruj go
   ponownie także przed ponowieniem próby. Do pobierania przez PC przygotowanie
   plików HTTPS jest zbędne.

## Próba na konsoli

1. Z programu uruchamiającego ELF otwórz
   `mass0:/DESR_DVR/DVR_ISO_MANAGER_V12.ELF`.
2. Wciśnij Trójkąt, aby przeskanować listę i wyświetlić pytanie o brakujące
   okładki. Pytanie pojawia się, gdy skan wykryje braki. Użyj testowej kopii
   USB z co najmniej jedną brakującą okładką w `DESR_DVR/ART`.
3. Na ekranie pytania X uruchamia dotychczasowe pobieranie przez aplikację
   na PC, Trójkąt uruchamia pobieranie HTTPS z konsoli (do 50 brakujących
   okładek), a Kółko wraca do listy.
4. HTTPS zapisuje nowe PNG w `DESR_DVR/ART/`. Po automatycznym powrocie
   Manager odświeża indeks, jeśli przybyły pliki, i pokazuje wynik serii.
   Ekran pozostaje widoczny do wciśnięcia X lub Kółka. Przy błędzie zapisz
   `RET`, `LAST` i, jeśli jest widoczny, kod weryfikacji Roughtime.
   Jeśli konsola pozostanie na ekranie pobieracza, zanotuj `SAVED` i `RET`,
   uruchom Manager 1.2 ponownie i wciśnij Trójkąt, żeby odświeżyć indeks.

Do uruchomienia wersji 1.0 wybierz dotychczasowy `DVR_ISO_MANAGER.ELF`.
