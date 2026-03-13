# Display MQTT per Raspberry Pi su Schermo SPI

Uno script Python per visualizzare i messaggi da un topic MQTT su un piccolo schermo SPI (es. 3.5") collegato a un Raspberry Pi. Il rendering avviene direttamente sul framebuffer della console, senza la necessità di un ambiente desktop.

![Schermata del progetto in azione](immagine.jpg)

## 📜 Descrizione

Questo progetto è ideale per creare un piccolo display di stato o un monitor di dati per sistemi domotici o progetti IoT. Alla prima esecuzione, lo script guida l'utente nella configurazione dei parametri di connessione al broker MQTT, che vengono poi salvati in un file `config.json` per gli avvii successivi.

Lo script è ottimizzato per funzionare in un ambiente minimale (Raspberry Pi OS Lite) e include workaround specifici per l'uso di `pygame` con il driver "dummy" e la scrittura diretta sul framebuffer a 16-bit.

## ✨ Funzionalità

*   **Visualizzazione Dati MQTT**: Si connette a un broker MQTT e mostra in tempo reale i messaggi ricevuti su un topic specifico.
*   **Configurazione Guidata**: Al primo avvio, richiede interattivamente i dati del server MQTT (IP, utente, password, topic).
*   **Rendering senza Desktop**: Utilizza `pygame` per disegnare testo direttamente sul framebuffer (es. `/dev/fb1`), eliminando la necessità di un server X.
*   **Ottimizzato per Schermi SPI**: Gestisce la profondità di colore a 16-bit (RGB565), comune per questi display.
*   **Basso Consumo di Risorse**: Progettato per essere leggero e funzionare in background.

## 📋 Requisiti

*   Raspberry Pi (qualsiasi modello).
*   Uno schermo SPI da 3.5" configurato per funzionare come dispositivo framebuffer (es. `/dev/fb1`).
*   Sistema operativo Raspberry Pi OS (la versione Lite è sufficiente).
*   Python 3.
*   Librerie Python: `pygame` e `paho-mqtt`.

## 🛠️ Installazione

1.  **Clona il repository:**
    ```bash
    git clone <URL_DEL_TUO_REPOSITORY>
    cd <NOME_DELLA_CARTELLA>
    ```

2.  **Installa le dipendenze:**
    ```bash
    sudo apt update
    sudo apt install python3-pygame python3-paho-mqtt
    ```

3.  **Configura i permessi utente:**
    Per permettere allo script di scrivere sul framebuffer e nascondere il cursore della console, l'utente deve far parte dei gruppi corretti.
    ```bash
    sudo usermod -a -G tty $USER
    sudo usermod -a -G video $USER
    ```
    Dopo aver eseguito questi comandi, **è necessario riavviare il Raspberry Pi** per rendere effettive le modifiche.

## ⚙️ Configurazione

Al primo avvio dello script, ti verranno chiesti i dettagli per la connessione MQTT:

*   **Server MQTT (IP)**: L'indirizzo IP del tuo broker MQTT.
*   **User**: L'username per l'autenticazione (lascia vuoto se non richiesto).
*   **Password**: La password per l'autenticazione (lascia vuoto se non richiesta).
*   **Topic da visualizzare**: Il topic MQTT a cui sottoscriversi.

Queste informazioni verranno salvate nel file `config.json`. Se in futuro avrai bisogno di modificarle, potrai editare direttamente questo file.

## 🚀 Utilizzo

Esegui lo script direttamente dalla console del tuo Raspberry Pi:

```bash
python3 main.py
```

Lo schermo inizierà a mostrare i messaggi provenienti dal topic MQTT configurato. Per terminare l'esecuzione, premi `CTRL+C` nella console.

## 🔧 Troubleshooting

*   **"Errore: Dispositivo /dev/fb1 non trovato"**:
    Questo errore indica che lo schermo non è stato rilevato all'indirizzo previsto. Verifica che i driver dello schermo siano installati e attivi. Prova a modificare la riga `f_fb = open("/dev/fb1", "wb")` in `main.py` con `/dev/fb0`.

*   **"Errore permessi: Impossibile scrivere su /dev/fb1"**:
    L'utente con cui esegui lo script non ha i permessi di scrittura sul framebuffer. Assicurati di aver aggiunto l'utente al gruppo `video` come descritto nella sezione "Installazione" e di aver riavviato.

*   **Sfarfallio del cursore o testo della console visibile sotto la grafica**:
    Questo accade se lo script non riesce a nascondere il cursore della TTY. Assicurati di aver aggiunto il tuo utente al gruppo `tty` e di aver riavviato il sistema.