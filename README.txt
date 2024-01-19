Temat:
Komunikator internetowy typu IRC

-------------------------------------------------------------------------
Kompilacja:

-Serwer:
Plik serwera należy skompilować w terminalu następującym poleceniem:
gcc -Wall ./serwer.c -o ./<nazwa_serwera>

-Klient:
Po pobraniu pliku klienta, należy upewnić się, że na urządzeniu zainstalowany jest Python
-------------------------------------------------------------------------
Uruchamianie:

-Serwer:
Za pomocą wiersza poleceń przejść do lokalizacji pliku serwera i wpisać:
./<nazwa_serwera>

-Klient:
Za pomocą wiersza poleceń przejść do lokalizacji pliku klienta i wpisać:
python3 <nazwa_pliku_klienta.py>

Następnie w wierszu poleceń wyśweitlą się instrukcje, dla których należy podać kolejno:
nazwę użytkownika, adres serwera, port na którym działa serwer

Po podaniu wymaganych informacji zostanie przeprowadzona próba nawiązania połączenia z serwerem i wyświetlą się opcje i krótka instrukcja obsługi komunikatora. Od tego momentu można korzystać z okienka które pojawiło się wraz z uruchomieniem skryptu.
-------------------------------------------------------------------------
Protokół Komunikacyjny:

W projekcie zastosowano protokół komunikacyjny TCP. Zarówno klient jak i serwer tworzą gniazdo TCP w celu umożliwienia komunikacji.
Serwer za pomocą funkcji:
socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)
Klient za pomocą funkcji:
socket(AF_INET, SOCK_STREAM)
Po utworzeniu gniazd serwer przypisuje utworzonemu gniazdu adres i port przy użyciu funkcji 'bind', a następnie nasłuchuje przychodzące połączenia za pomocą funkcji 'listen'.
Klient podając adres serwera i port na którym nasłuchuje serwer, próbuje nawiązać z nim połączenie za pomocą funkcji 'connect', które następnie serwer może zaakceptować przy pomocy funkcji 'accept'. Po zaakceptowaniu serwer tworzy nowe gniazdo, używane do komunikacji z danym klientem.
-------------------------------------------------------------------------
Krótki opis implementacji i zawartości plików:

Implementacja serwera obsługuje różne funkcje związane z czatem grupowym, takie jak:
Dołączanie do pokoju, opuszczanie pokoju, wyświetlanie dostępnych pokoi, wyświetlanie członków pokoi, wyświetlanie globalnie dostępnych użytkowników oraz komunikację między członkami grupy. Dodatkowo w celu wizualizacji przesyłu danych i poprawności połączenia, wysyłane przez klientów wiadomości wyświetlane są na serwerze (wyświetlanie ich można zakomentować). Implementacja kodu klienta to proste GUI napisane w języku Python z wątkami do odbierania oraz wysyłania wiadomości.
-------------------------------------------------------------------------
TODO (Lista modyfikacji które w przyszłości chciałbym zaimplementować w projekcie):

-Enkrypcja komunikacji
-Dostęp do pokoi broniony hasłem
-Unikalność użytkownika i pokoi
-Implementacja kodu klienta na systemie Linux


