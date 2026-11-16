# N6G ARENA na Sony PSX DESR-7000

Próba na fizycznym DESR-7000 zakończyła się wynikiem `RET=0` i `SAVED=1`. Użytkownik potwierdził pojawienie się nowego PNG w USB `ART` oraz brak pliku `.PART` po zakończeniu. Była to próba pobrania i zapisu jednej okładki.

- ELF `V12_N6G_ARENA.ELF`, rewizja kodu `b356d31c511ed8723373f98c73095d8f91c797b3`.
- SHA-256 `6e3c75bce1a3746bb22159d762657bb521ef6760bebb37a36b7b5e19a4e2e691`.
- [GitHub Actions z przetestowanym plikiem](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions/runs/43831464011).

Skopiuj ELF i działający `N4E.CA` do katalogu głównego FAT32 USB. Przed każdą próbą wygeneruj świeże 288 B entropii, podstawiając literę USB:

```powershell
py .\tools\generate_n6c_entropy.py I:/N6C.ENT
```

Pozostaw pendrive bez `N6G.NEWLIB`, żeby włączyć pulę ARENA. Wybierz do próby grę bez okładki w USB `ART` i uruchom ELF przez wLaunchELF-R3Z. Program wybiera jedną pozycję, weryfikuje TLS, normalizuje do 140×200 RGB i weryfikuje odczyt pliku przed publikacją PNG. Istniejące PNG, JPG i PART pozostają na miejscu.

Obejście sprawdziło się na fizycznej konsoli. Dokładny mechanizm wcześniejszego zawieszenia wymaga porównania trybu ARENA i NEWLIB w tym samym binarnym ELF-ie.
