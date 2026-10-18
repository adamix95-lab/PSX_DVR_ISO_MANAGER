## PSX_DVR_ISO_MANAGER

Proste narzędzie do uruchamiania gier w formacie .ISO z partycji DVR konsoli PSX DESR.
Jest to prototyp. Bez pięknego GUI i wielu opcji.

## Instalacja
W aktualnym wersji cała konfiguracja programu (patche, pliki OSDLauncher, Neutrino itd) MUSZĄ znajdować się w folderze 
"DESR DVR" na pamięci masowej USB. Program DVR_ISO_MANAGER.ELF może być uruchamiany z USB jak i z serwera UDPFS.

```text
mass0:/DESR_DVR/
│
├── BOOT.ELF                 # OSDMenu Launcher
├── BOOT.CNF                 # Konfiguracja uruchamianej gry
├── BOOT_RECOVERY.CNF        # Kopia zapasowa konfiguracji
├── DVR_TEST.CNF             # Testowa konfiguracja
│
├── neutrino.elf             # Neutrino DVR - SYSTEM
├── neutrino_lang.elf        # Neutrino DVR - ENGLISH
├── DVR_ISO_MANAGER.ELF      # Manager v0.3 - wprowadza zmiany w BOOT.CNF i uruchamia neutrino.elf lub neutrino_lang.elf
│
├── config/
│   ├── system.toml          # Podstawowa konfiguracja
│   ├── bsd-dvr.toml         # Obsługa DVR
│   └── osdlang.toml         # Konfiguracja języka ENGLISH
│
└── modules/
    ├── ee_core.elf          # Standardowy rdzeń Neutrino
    ├── ee_core_lang.elf     # Zmodyfikowany rdzeń ENGLISH
    ├── dvrdrv.irx           # Sterownik DVR
    ├── dvrfile.irx          # Obsługa plików DVR
    └── dvrfile_fhi.irx      # Odczyt ISO podczas gry
```

**Uwaga:** Lista zawiera wyłącznie najważniejsze pliki projektu. Nie obejmuje wszystkich modułów Neutrino.

Obrazy ISO muszą znajdować się na partycji DVR w `dvr_pfs0:/ISO/*`

## Obsługa aplikacji
<img width="640" height="480" alt="image" src="https://github.com/user-attachments/assets/344403f5-11b3-4a1d-925e-0c676ad2fbb2" />

- Wciśnij **UP** lub **DOWN** by wybrać grę z listy
- Wciśnij **X** aby uruchomić dany tytuł
- Wciśnij **^** aby ponownie sprawdzić zawartość folderu "ISO"
- Wciśnij **[]** aby zapisać testową konfigurację do *"DVR_TEST.CNF"*
- Wciśnij **SELECT** by zmienić język gry - funkcja dla gier które ustawiają język według menu konsoli (*testowane przy REZ i Metal Slug Anthology*)
- Wciśnij **START** by wyjść z aplikacji

## *TO DO* (Do zrobienia)
- uruchamianie całości z HDD jako aplikacja uruchamiana z XMB - nie umiem obejść problemu -19 z wskazywaniem ścieżki

## Zapraszamy do współpracy!

Jednym z pozostałych celów jest uruchamianie PSX DVR ISO Manager bezpośrednio z menu XMB konsoli PSX, z użyciem wewnętrznego HDD i bez konieczności korzystania z pamięci USB. W dotychczasowych próbach napotkaliśmy błąd `-19` przy wskazywaniu ścieżki aplikacji. Jego dokładna przyczyna nie została jeszcze ustalona.

Zapraszamy do dzielenia się wiedzą, zgłaszania pomysłów i przesyłania pull requestów — szczególnie osoby znające Sony PSX DESR, PS2SDK i uruchamianie aplikacji z XMB.

## Źródła i autorzy

- Neutrino: https://github.com/ps2max32/neutrino
- PR #228 (pcm720): https://github.com/ps2max32/neutrino/pull/228
- Berion - pomoc merytoryczna 

Ten repozytorium zawiera tylko konfigurację budowania i dokumentację projektu; właściwe źródła Neutrino są pobierane podczas kompilacji.
