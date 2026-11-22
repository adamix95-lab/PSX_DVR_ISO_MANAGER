# STAGE 2 — ręczna rejestracja gry ICO w Sony PSX XMB

**STAGE 1 i STAGE 2 — HARDWARE PASS na Sony PSX DESR-7000 (22 listopada 2026).** Użytkownik potwierdził, że skrót ICO pojawia się prawidłowo w XMB, ikona wygląda poprawnie, a uruchomienie wpisu startuje ICO.iso bez menu PDIM. Komunikat zakazujący usuwania przez XMB był zbyt kategoryczny i został poprawiony w szablonie nagłówka.

## BARDZO WAŻNE: ZAPIS APA MOŻE USZKODZIĆ SYSTEM DVR

**Opcja Inject Header w wLE-R3Z zapisuje surowe sektory nagłówka APA!** Wymagana jest pełna, zweryfikowana i odtwarzalna kopia sektorowa całego HDD. Nie zmieniaj partycji Sony ani PP.DVRISO. Użytkownik potwierdził, że korzysta z usuwania wpisów przez XMB i działa to prawidłowo; nasz wcześniejszy kategoryczny zakaz był błędny. Przed usunięciem upewnij się, że wskazujesz właściwy skrót ICO/PP.ICO. Nie wybieraj Inject All. Paczka nie ma automatycznego zapisu APA.

## Test STAGE 2

1. Rozpakuj ZIP na pendrive FAT32. Skopiuj ZAWARTOŚĆ folderu ICO_XMB_STAGE2_TEST do katalogu głównego USB. Sprawdź mass0:/__Headers/PP.ICO/system.cnf oraz icon.sys.
2. W wLE-R3Z 4.80 HDDManager potwierdź, że PP.ICO nadal istnieje po STAGE 1. Jeśli nie istnieje, PRZERWIJ, niczego nie twórz ani nie formatuj.
3. Skopiuj TYLKO zawartość PP.ICO-FILES/res/ do PP.ICO/res/, zastępując grafiki testowe. NIE nadpisuj działających BOOT.ELF, BOOT.CNF, EXECUTE.ELF i EXECUTE.CNF.
4. Przed modyfikacją APA ponownie uruchom mass0:/USB_TEST/ICOPFS.ELF. Jeżeli ICO nie wystartuje, PRZERWIJ.
5. Po weryfikacji kopii sektorowej: HDDManager -> wybierz TYLKO PP.ICO -> R1 -> Inject Header -> This partition only. Źródło musi być mass0:/__Headers/PP.ICO/.
6. Całkowicie wyłącz konsolę, uruchom XMB -> Games -> ICO. Sprawdź okładkę, opis, screenshoty i uruchomienie gry. W razie błędu nie kasuj partycji ani nie ponawiaj wstrzykiwania bez analizy.

## Uwagi

- Neutrino, moduły i konfiguracja nadal pozostają wyłącznie w PP.DVRISO; ISO znajduje się na DVR.
- EXECUTE.ELF jest niezmienioną kopią z PDIM HDD1.2. Start z XMB na PP.ICO został już potwierdzony na DESR-7000.
- Grafiki i metadane odnoszą się do europejskiego wydania PAL ICO SCES-50760. Faktyczny Game ID Twojego ICO.iso nie został sprawdzony.
- Źródło obrazów: [LaunchBox](https://gamesdb.launchbox-app.com/games/images/2128-ico); metadane: [PSXDatacenter](https://psxdatacenter.com/psx2/games2/SCES-50760.html).

Szablony: [system.cnf](stage2-templates/__Headers/PP.ICO/system.cnf), [icon.sys](stage2-templates/__Headers/PP.ICO/icon.sys), [info.sys](stage2-templates/info.sys), [man.xml](stage2-templates/man.xml).

Oficjalny PDIM 1.2 i gałąź main nie zostały zmienione.

## Poprawiony komunikat usuwania z XMB

Dotychczasowy tekst `Do not uninstall from XMB` niepotrzebnie zabraniał działania, które użytkownik normalnie wykonuje z XMB. W nowym `__Headers/PP.ICO/icon.sys` ustawiono `Delete ICO shortcut?` oraz `ICO.iso remains on DVR.`. Gra ISO jest na osobnym obszarze DVR, a Neutrino na PP.DVRISO. Udane usuwanie zgłoszone przez użytkownika nie oznacza gwarancji dla każdej konfiguracji APA. **Nie wstrzykuj ponownie nagłówka APA tylko po to, aby poprawić tekst.** Nowy plik na pendrivie sam nie aktualizuje już zainstalowanego komunikatu XMB.
