# N6G BATCH50 R3 na Sony PSX DESR-7000

Próba 17 listopada 2026 r. na fizycznym DESR-7000 zakończyła się zapisem 45 z 45 brakujących okładek. Przed próbą katalog `mass0:/ART/` na USB był pusty. ELF pracował w domyślnej kolejności SSL FIRST.

```text
DVR ISO=45 ID=45 USB MISSING AT START=45
BATCH=45 ATTEMPTED=45 SAVED=45 UNAVAILABLE=0
LAST=SLES_503.30 HTTP=200 TLSERR=0
RET=0
```

`SAVED` jest zwiększany po walidacji PNG 140×200 RGB, zapisaniu `.PART`, porównaniu odczytu z pamięcią i udanym przemianowaniu na PNG. Limit 50 pozycji potwierdził test skanera z 60 symulowanymi grami. Fizyczna próba objęła całą bibliotekę 45 gier.

- ELF `V12_N6G_BATCH50.ELF`, kod `314ed49752e42251c00ecfebc8fb0e3710745037`.
- SHA-256 `9b11ca11a5792a67428ea68bd92407e2d4260548d59ebe0e0622a1c792b48f21`.
- [GitHub Actions z przetestowanym ELF-em](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions/runs/43882223379).

## Uruchomienie

1. Skopiuj ELF i działający `N4E.CA` do katalogu głównego pendrive'a FAT32. Użyj trybu SSL FIRST, pozostawiając katalog główny bez pliku `N6G.CAFIRST`. Konsola odczytuje dotychczasowy `PP.DVRISO12/V12_NET.CNF`.
2. Wygeneruj świeże 16 000 B entropii przed każdą próbą. Podstaw literę swojego USB:

   ```powershell
   py .\tools\generate_n6g_batch_entropy.py I:/N6G50.ENT
   ```

3. Podłącz Ethernet, włącz konsolę i uruchom ELF przez wLaunchELF-R3Z. Czekaj na `FINAL RESULT`. Program wybiera do 50 brakujących gier i pomija istniejące okładki. Przy przerwaniu próby kolejne uruchomienie ze świeżym `N6G50.ENT` pomija wcześniej zapisane PNG, a istniejący `.PART` pozostawia do inspekcji.

R3 ukończył przebieg sprzętowy z pełną weryfikacją CA, hosta i podpisanego czasu. Wynik potwierdza działanie tej kompilacji i konfiguracji. Mechanizm poprzedniego zawieszenia wewnątrz inicjalizacji TLS pozostaje nierozstrzygnięty.
