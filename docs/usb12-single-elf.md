# PSX DVR ISO Manager 1.2 USB — jeden ELF

Wariant testowy na gałęzi `test/v1.2-usb-single-elf` bazuje na prototypie
`7e6bdaa`. Pobieracz jest wbudowany w ELF Managera i uruchamia się z pamięci.
Zachowuje osobną inicjalizację sterowników oraz dotychczasowe weryfikacje
Roughtime, TLS, PNG i zapisu na USB. Test 45/45 na DESR-7000 dotyczył
wcześniejszego prototypu z dwoma plikami. Uruchomienie pobieracza z pamięci
i powrót z niego wymagają próby sprzętowej.

## Instalacja i próba

1. Na pendrive FAT32 z działającym Managerem USB 1.0 skopiuj plik
   `DVR_ISO_MANAGER_V12_SINGLE.ELF` do `DESR_DVR/`.
2. Uruchom `mass0:/DESR_DVR/DVR_ISO_MANAGER_V12_SINGLE.ELF`. Na liście wciśnij
   Trójkąt. Pytanie o pobieranie pojawi się, gdy skan wykaże brak okładek.
3. W pytaniu **X** uruchamia dotychczasowe pobieranie przez aplikację na PC.
   **Trójkąt** uruchamia HTTPS na konsoli, maksymalnie 50 brakujących
   okładek w jednej serii. **Kółko** wraca do listy.
4. Okładki trafiają do `DESR_DVR/ART/`. Po powrocie z HTTPS Manager
   pokazuje podsumowanie do naciśnięcia X lub Kółka.

Nazwy plików wersji 1.0 i prototypu z dwoma ELF-ami pozostają dostępne
na tym samym pendrivie. Wszystkie wersje korzystają z tego samego ART.

## Plik entropii do HTTPS

HTTPS nadal używa `N4E.CA` i świeżego `N6G50.ENT` w katalogu głównym USB.
Zostaw istniejący `hdd0:PP.DVRISO12/V12_NET.CNF` z testu BATCH50.
Przed każdą serią wygeneruj plik. Podstaw literę swojego pendrive'a.

```powershell
py .\tools\generate_n6g_batch_entropy.py I:/N6G50.ENT
```

Plik zawiera 16 000 bajtów losowych z generatora systemowego PC. Zasila
osobno żądania Roughtime i generator wolfSSL. Pobieranie przez X nie
wymaga tego pliku.

Po jednorazowym przygotowaniu na PC można obsłużyć wiele serii przez pulę
niezależnych pakietów losowych na USB. Konsola musi oznaczyć następny
pakiet jako zużyty przed rozpoczęciem sieci i zachować tę informację po
utracie zasilania. Przywrócenie starej kopii nośnika wymaga świeżej puli.
Wersja bez przygotowania na PC potrzebuje niezależnego, sprawdzonego
źródła entropii dostępnego na konsoli.
