import socket
import ssl
import sys

# Configuration du certificat SSL
certfile = 'server.crt'
keyfile = 'server.key'

# Configuration de l'adresse et du port d'écoute
address = '127.0.0.1'
port = 1292

# Création du socket
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# Liaison du socket à l'adresse et au port
sock.bind((address, port))

# Écoute des connexions entrantes
sock.listen(1)

# Accepter une connexion entrante
print("En attente de connexion...")
conn, addr = sock.accept()

# Création d'un contexte SSL
context = ssl.create_default_context(ssl.Purpose.CLIENT_AUTH)
context.load_cert_chain(certfile, keyfile)

# Conversion du socket en socket SSL
conn_ssl = context.wrap_socket(conn, server_side=True)

while True:
    cmd = input("cmd >> ")
    conn_ssl.send(cmd.encode())

    # Réception et traitement des données par blocs
    if cmd and cmd != "exit":
        data = b""
        while True:
            chunk = conn_ssl.recv(8192)
            if not chunk:
                break
            data += chunk

            # Vérification si la réception est terminée
            if len(chunk) < 8192:
                break
        print(data.decode('cp850'))
    else:
        break

# Fermeture de la connexion SSL
conn_ssl.close()

# Fermeture du socket
sock.close()
