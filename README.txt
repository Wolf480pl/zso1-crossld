Zadanie 1 z ZSO by Wojciech Dubiel
==================================

Zależności
----------

Do zbudowania potrzebne są:
 - standardowe narzędzia (gcc, binutils, make, sh)
 - nasm

Na VMce z pierwszych zajęć wystarczy zainstalować nasma:

    sudo apt install nasm


Budowanie
---------

Wystarczy odpalić make w katalogu w którym jest to README:

    make

Powinien powstać plik `libcrossld.so`

### Flagi

Jak ktoś chce, można podać CFLAGS do make'a, np:

    make CFLAGS="-O0 -g -std=gnu11 -Wall"

Domyślnie są `-O2 -std=gnu11`.

We flagach można też zdefiniować następujące makra:

DEBUG - włacza dużo debugowych printfów (na stdout)

CROSSLD_NOEXIT - wyłącza funkcję exit (powoduje że kończy ona program zamiast
                 zwracać z crossld_start).

CROSSLD_DUMP_FD=<fd> - włacza dumpowanie wygenerowanych trampolin na podany
                       deskryptor pliku

np:

    make CLFAGS="-g -DDEBUG -DCROSSLD_DUMP_FD=2"


Opis rozwiązania
----------------

W pliku `wrapper.asm` zdefiniowane jest dużo fragmentów kodu kopiowanego
później w różne miejsca przez resztę biblioteki, za pomocą stałych globalnych
zadeklarowanych we `wrapper.h`.

W pliku `trampolines.c` jest kod generujący trampoliny za pomocą fragmentów
z `wrapper.asm`, wysokopoziomowe funkcje do alokacji i przełączania stosu, itp.

W pliku `crossld.c` znajduje się loader ELFów oraz funkcja crossld_start.

### Trampoliny

Jedną z nieoczywistych rzeczy w bibliotece jest sposób generowania trampolin
przez funkcję `write_trampoline` z pliku `trampolines.c`.
 
Skleja ona kilka kawałków kodu maszynowego:

 - `crossld_call64_trampoline_start` - kod 32-bitowy zapisujący rejestry,
   wyrównujący stos, i przełaczający się do trybu 64-bitowego retf-em
   tuż za swój koniec.

 - Odpowiednie elementy `crossld_hunk_array` do konwersji argumentów

 - `crossld_call64_trampoline_mid` - kod 64-bitowy który wywołuje funkcję
   docelową, konwertuje wynik, i skacze do 32-bitowego epilogu wspólnego dla
   wszystkich trampolin.
   Domyślnie konwersja wyniku jest tutaj nop-ami, ale zależnie od potrzeb,
   po skopiowaniu nop-y zostają zastąpione przez jeden z fragmentów
   `crossld_check_*`.
   Cały ten fragment jest dziurawy jak ser szwajcarski, tzn. zawiera dużo
   miejsc w które trzeba coś wpisać po skopiowaniu go:
   - adres wrapowanej funkcji
   - fragment kodu do konwersji wyniku, jeśli jest potrzebna
   - adres wspólnego epilogu 32-bitowego
   - adres funkcji crossld_panic
   - adres struktury crossld_ctx do przekazania do crossld_panic

 - `crossld_call64_out` - wspólny epilog 32-bitowy, który występuje w jednej
   kopii (per wywołanie crossld_start) używanej przez wszystkie trampoliny
