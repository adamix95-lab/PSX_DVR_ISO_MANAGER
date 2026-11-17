# PSX DVR ISO Manager 1.2 USB — test licznika CPU

Testowy wariant jednego ELF-a na gałęzi `test/v1.2-usb-cpu-entropy`.
Przygotowuje dane dla wolfSSL i Roughtime na konsoli, bez pliku
`N6G50.ENT`. Zachowuje dotychczasową weryfikację podpisanej odpowiedzi
Roughtime, łańcucha certyfikatów, nazwy hosta oraz PNG.

Pierwsza wersja pobrała na DESR-7000 komplet 45 okładek w dwóch próbach
(1 + 44). Po zakończeniu drugiej serii konsola zatrzymała się na zielonym
ekranie przy wczytywaniu Managera. R2 odczytuje ELF z USB do RAM przed
przełączeniem sterowników. Powrót tym sposobem czeka na próbę sprzętową.

## Uruchomienie na DESR-7000

1. Na pendrive z działającą wersją USB 1.0 skopiuj
   `DVR_ISO_MANAGER_V12_CPU.ELF` do `DESR_DVR/`.
2. Zostaw `N4E.CA` w katalogu głównym USB i konfigurację testu BATCH50
   w `hdd0:PP.DVRISO12/V12_NET.CNF`. Konsola potrzebuje dostępu do internetu.
3. Uruchom `mass0:/DESR_DVR/DVR_ISO_MANAGER_V12_CPU.ELF`.
   Trójkąt na liście skanuje okładki. Na pytaniu **Trójkąt** wybiera HTTPS
   bezpośrednio z konsoli, **X** dotychczasowy serwer na PC, **Kółko** wraca
   do listy.
4. Po pobraniu HTTPS okładki są w `DESR_DVR/ART/`. Na podsumowaniu sprawdź
   `ENTROPY: CPU COUNTER (EXPERIMENTAL)`, `SAVED` i `RET`. X lub Kółko
   wraca do listy. `RET=-735` oznacza brak poprawnych próbek licznika
   lub argumentu przekazywanego z menu.

Gdy Roughtime odrzuci ramkę lub podpisaną odpowiedź, R2 ponawia żądanie
raz z osobnym nonce. HTTPS rusza dopiero po przyjęciu poprawnej odpowiedzi.
Podsumowanie pokazuje `ROUGHTIME RETRIES`, `LAST REJECT` i `FINAL REJECT`.
Implementacja TXRyan opisuje celowe wadliwe odpowiedzi testowe jako
możliwy powód sporadycznych odrzuceń.
[Dokumentacja TXRyan](https://github.com/tannerryan/roughtime)

## Źródło pomysłu

Inspiracja: Daniel i jego projekt NULD. W [kodzie sieciowym NULD](https://github.com/danielnuld/nuld/blob/2951dc9d22541aa6a2d6c2fcf7705187e97eae92/net.c)
odczyty licznika COP0 Count zasilają generator wolfSSL. Kod Managera
korzysta z własnej implementacji. Łączy pomiar przy wyborze HTTPS z
próbkami z pobieracza podczas krótkich uśpień, potem wyprowadza osobne
wartości dla Roughtime i TLS przez BLAKE2b.

Licznik CPU może dawać przewidywalne wyniki. BLAKE2b rozdziela dane
wejściowe dla kolejnych połączeń i nie zwiększa faktycznej entropii.
Ten wariant służy do testów pobierania publicznych okładek. Wersja
oparta na `N6G50.ENT` nadal jest dostępna na poprzedniej gałęzi.
