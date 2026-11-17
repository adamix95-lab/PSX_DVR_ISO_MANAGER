# PSX DVR ISO Manager 1.2 USB — uproszczony interfejs HTTPS (TEST)

**Wersja do prób sprzętowych, jeszcze NIE potwierdzona na DESR-7000.**
Oryginalny, sprawdzony R4 z 17.11.2026 pozostaje w historii jako
commit \`0bed4b1394b93a937420ff35fdcaded8bfe526e8\`.
W tym R4 pobrano 45/45 okładek w jednej serii, powrócono do listy
i uruchomiono Killzone; jego ELF ma SHA-256
\`81a0f045e2597719d58d798499378afcdce7466a6c7ca967546c1499e2355741\`.

## Co zmienia wersja UI TEST

Zmiany dotyczą wyłącznie wyświetlanych komunikatów w już działającym
pobieraczu HTTPS oraz ekranu podsumowania Managera. Obsługa HTTP/TLS,
Roughtime, źródeł OPL/CosmicScale, normalizacji PNG, pamięci EE i zapisu
\`.PART\` nie była celowo zmieniana. **Nowy ELF jest inny bajtowo od R4 i musi
ponownie przejść test na konsoli.**

Podczas pobierania ekran pokazuje tylko:

\`\`\`text
PSX DVR ISO MANAGER 1.2 USB

TRWA POBIERANIE BRAKUJACYCH OKLADEK
PROSZE CZEKAC...

1/45 OK
2/45 OK
3/45 POMINIETO
4/45 POBIERANIE...

POBRANO: 2  POMINIETO: 1
\`\`\`

Wynik kolejnej okładki pojawia się dopiero po zakończeniu całej próby
pobierania i bezpiecznego zapisu. Ekran zawiera najwyżej 12 ostatnich
wyników, żeby tekst nie wychodził poza obszar. Komunikaty są zapisane
ASCII ze względu na ograniczenia fontu PS2.

Po serii Manager wyświetla uproszczony ekran, np.:

\`\`\`text
POBIERANIE ZAKONCZONE

POBRANO: 43/45
POMINIETO: 2

X / O: POWROT DO LISTY GIER
\`\`\`

W razie błędu pojawia się \`POBIERANIE PRZERWANE\`. W normalnym widoku
nie ma logów TCP, TLS, certyfikatów, RET ani wewnętrznych liczników.
Szczegółowe wyniki nadal są przekazywane do Managera dotychczasowym
formatem argumentów. **Powrót z ekranu Managera następuje po X lub O,**
bez zegara i bez automatycznego zamykania tego ekranu. Przejście z
pobieracza do tego ekranu pozostaje takie jak w R4.

## Instalacja

Zachowaj kopię oryginalnego pendrive'a z działającym R4.
Z paczki UI TEST zastąp na pendrivie tylko plik
\`DESR_DVR/DVR_ISO_MANAGER_V12_CPU.ELF\`. Pozostałe pliki Managera 1.0,
\`ART\`, konfiguracje, \`BOOT.CNF\` i okładki pozostaw bez zmian.
Do działania wymagane są dotychczasowe publiczne \`N4E.CA\`,
\`hdd0:PP.DVRISO12/V12_NET.CNF\` oraz połączenie Ethernet. Nie zmieniaj
ich, jeśli R4 już działa.

Uruchom przez wLE-R3Z 4.80. W menu Managera wybierz Trójkąt (skan),
a następnie Trójkąt (HTTPS). Wybierz test z kilkoma brakującymi
okładkami. Sprawdź \`1/N OK\`, \`POMINIETO\`, podsumowanie oraz
powrót do listy przyciskami X i O, a na końcu uruchomienie gry.

**W razie zawieszenia lub regresji:** zachowaj ostatni ekran,
nie usuwaj \`.PART\` automatycznie. Przywróć pojedynczy ELF R4 z
artefaktu GitHub Actions
\`https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions/runs/44067601600\`.
Nie ma zmian w działającej wersji HDD 1.0 ani w XMB.

## Pakowanie

R4 odtwarzał bajtowo stary, sprawdzony worker. UI TEST buduje nowy
worker z aktualnych źródeł i umieszcza właśnie jego bajty w końcowej
aplikacji. Skrypt \`pack_usb12_tail.py pack-ui\` weryfikuje obecność
komunikatów UI w nowym ELF-ie i odrzuca pomyłkowe użycie starego R4.
Build zapisuje nowe SHA-256 oraz sprawdza zawartość ZIP-a.
