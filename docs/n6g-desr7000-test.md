# N6G na Sony PSX DESR-7000

Pakiet eksperymentalny uruchamiany z wLaunchELF-R3Z 4.80. Finalny ELF zapisuje najwyżej jedną nową okładkę w `mass0:/ART/`. Warianty diagnostyczne kończą pracę w RAM. DVR ISO oraz `PP.DVRISO12` są montowane tylko do odczytu.

## Następna próba z ekranem uruchamianym na końcu

Uruchom `V12_N6G_HEADLESS.ELF` ze świeżym `N6C.ENT` i jedną grą bez okładki w USB `ART`. Ekran debugowy uruchomi się dopiero po zakończeniu próby, także gdy program zwróci błąd. W międzyczasie ekran może pozostać czarny lub wyświetlać ostatni obraz z programu uruchamiającego. Zapisz `RET` i `SAVED`. Jeśli po dwóch minutach nadal brak podsumowania, sprawdź na komputerze, czy w USB `ART` przybył PNG lub plik częściowy.

## Próba porównawcza SCREEN

Uruchom `V12_N6G_SCREEN.ELF` ze świeżym `N6C.ENT`. Na pierwszą próbę pozostaw katalog główny USB bez pliku `N6G.SCREEN`. Po wybraniu gry powinien pojawić się napis `N6G SCREEN TEST: QUIET`. Ekran pozostaje wtedy na tym napisie aż do podsumowania. Daj konsoli dwie minuty. Zapisz `RET` i `SAVED`, a gdy ekran się nie zmieni, sprawdź po zakończeniu próby, czy powstała okładka.

Ten sam ELF działa w trybie `VERBOSE` po umieszczeniu pustego pliku `N6G.SCREEN` w katalogu głównym USB. W obu trybach użyj znanego ID gry i świeżej entropii. Jeżeli QUIET zapisał okładkę, użyj osobnej kopii przygotowanej zawartości USB do porównania trybów na tym samym ID. Oba tryby zapisują maksymalnie jedną nową okładkę.

## Przygotowanie USB i wykonanie prób

1. Użyj pendrive'a FAT32. Skopiuj wybrany ELF do katalogu głównego USB obok działającego z N6E pliku `N4E.CA`. Zostaw działający `PP.DVRISO12/V12_NET.CNF` na swoim miejscu.
2. Do próby finalnej przygotuj grę bez `GAMEID_COV.png`, `GAMEID_COV.jpg` ani `GAMEID_COV.png.PART` w katalogu `ART` na USB. Zachowaj istniejące okładki i pliki częściowe. Najłatwiej użyć oddzielnego pendrive'a testowego i porównywać próby dla tego samego ID gry.
3. Wygeneruj nowy prywatny `N6C.ENT` przed **każdym** uruchomieniem. Plik musi mieć 288 bajtów. Polecenie w Windows, po podstawieniu litery USB:

   ```powershell
   py .\tools\generate_n6c_entropy.py I:/N6C.ENT
   ```

4. Podłącz Ethernet, wyłącz konsolę i włącz ją ponownie przed każdą próbą. Uruchom po kolei kontrolę N6E (jeżeli ją masz), A, B, następnie wariant finalny. Dla porównania wykonaj drugi przebieg A i B, zawsze ze świeżą entropią.
5. Zapisz ID gry, wynik Roughtime, powrót LOW i HIGH, końcowy `RET` oraz ostatni komunikat widoczny przy zawieszeniu. W razie zawieszenia podaj też, czy serwer odpowiedział na pakiety TLS (jeśli masz przechwycenie na routerze). Nie dołączaj `N6C.ENT`.

| ELF | Zmiana względem kontrolnego N6E | Pożądany wynik na ekranie |
| --- | --- | --- |
| `V12_N6G_A.ELF` | Dodatkowe 349 216 B bufora statycznego | `TLS LOW/HIGH: VERIFIED PASS`, `HTTP=200`, `CRC=PASS`, `RET=0` |
| `V12_N6G_B.ELF` | Oryginalny normalizator z libpng/zlib i jego bufory dołączone do ELF. Kod normalizatora pozostaje niewywołany. | Jak w A |
| `V12_N6G_TRACE.ELF` | Jak B, z komunikatami od `LOW CLEAN 1` do `LOW CLEAN 6` po udanej weryfikacji LOW | Jak w A, z widocznymi etapami zamknięcia połączenia |
| `V12_N6G_FREETRACE.ELF` | Jak TRACE, z osobnym buildem wolfSSL pokazującym kroki wewnątrz `wolfSSL_free()` | Jak w TRACE, z komunikatami `FREE` po udanym LOW |
| `V12_N6G_FINAL.ELF` | Normalizacja po zwolnieniu TLS i zweryfikowany zapis USB | `HTTP=200`, `BATCH=1 SAVED=1`, `HTTPS RAW PNG + RGB 140x200: PASS`, `USB ART .PART READBACK+RENAME: PASS`, `RET=0` |
| `V12_N6G_SCREEN.ELF` | Ścieżka FINAL z przełącznikiem ekranu QUIET/VERBOSE wybieranym na USB | Jak w FINAL, z napisem o trybie ekranu |
| `V12_N6G_HEADLESS.ELF` | Ścieżka FINAL z pierwszym uruchomieniem ekranu debugowego przy podsumowaniu | Jak w FINAL, z nagłówkiem `N6G HEADLESS` |

## Sprawdzenie zapisu

Po zakończeniu zapisu odczytaj plik na komputerze. Powinien być PNG 140×200 w trybie RGB. W końcowym przebiegu powinna przybyć dokładnie jedna okładka, a jej `.PART` powinien zostać przemianowany. `RET=0` przy `SAVED=0` może oznaczać, że wszystkie gry na USB mają już okładki. Jeśli zostanie `.PART` po błędzie, zachowaj go do inspekcji.

Do sprawdzenia ochrony plików użyj kopii testowego USB. Pozostaw kolejno istniejące PNG, JPG lub PART z rozpoznawalną zawartością i porównaj jego sumę SHA-256 po próbie. Jeżeli na USB są inne gry bez okładek, program może wybrać jedną z nich. Do sprawdzenia obu źródeł wybierz osobno grę z okładką OPL i grę bez prawidłowej okładki OPL, ale z okładką CosmicScale.

Wyniki kompilacji, mapy i pomiary EE są w `evidence/`. Wynik prób na fizycznym DESR-7000 wymaga zapisania obserwacji z konsoli.
