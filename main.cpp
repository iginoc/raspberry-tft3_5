/*
 * main.cpp - Versione C++ del display MQTT per Raspberry Pi
 *
 * Replica le funzionalità dello script main.py:
 * - Lettura config.json (compatibile con la versione Python)
 * - Client MQTT (Paho)
 * - Rendering su framebuffer (SDL2) a 16-bit
 *
 * 🛠️ INSTALLAZIONE DIPENDENZE (Esegui su Raspberry Pi):
 * sudo apt update
 * sudo apt install build-essential libsdl2-dev libsdl2-ttf-dev libpaho-mqtt-dev nlohmann-json3-dev fonts-dejavu-core
 *
 * 🔨 COMPILAZIONE:
 * g++ main.cpp -o main -lSDL2 -lSDL2_ttf -lpaho-mqtt3c -Wno-psabi
 *
 * 🚀 ESECUZIONE:
 * sudo ./main
 */

#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <mutex>
#include <chrono>
#include <csignal>
#include <vector>

// Librerie esterne
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <MQTTClient.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace std;

// --- Variabili Globali ---
const string CONFIG_FILE = "config.json";
string mqtt_message = "In attesa di dati...";
mutex msg_mutex; // Per proteggere l'accesso a mqtt_message tra thread
bool running = true;

// --- Gestione Segnali (CTRL+C) ---
void signal_handler(int) {
    cout << "\nInterruzione manuale (CTRL+C)" << endl;
    running = false;
}

// --- Callback MQTT ---
int msgarrvd(void *, char *topicName, int, MQTTClient_message *message) {
    lock_guard<mutex> lock(msg_mutex);
    if (message->payloadlen > 0) {
        mqtt_message = string((char*)message->payload, message->payloadlen);
    } else {
        mqtt_message = "";
    }
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

void connlost(void *, char *cause) {
    lock_guard<mutex> lock(msg_mutex);
    mqtt_message = "Conn Lost";
    cout << "Connessione MQTT persa. Causa: " << (cause ? cause : "Sconosciuta") << endl;
}

int main() {
    // Registra gestore per CTRL+C
    signal(SIGINT, signal_handler);

    // --- 1. Configurazione ---
    json config;
    ifstream f(CONFIG_FILE);
    if (!f.good()) {
        cout << "\n--- Configurazione MQTT Primo Avvio (C++) ---" << endl;
        string server, user, pw, topic;
        
        cout << "Server MQTT (IP): "; cin >> server;
        
        cout << "User (invio per nessuno): "; 
        cin.ignore(); getline(cin, user); 
        
        cout << "Password (invio per nessuno): "; 
        getline(cin, pw);
        
        cout << "Topic da visualizzare: "; 
        getline(cin, topic);
        
        config = {
            {"server", server},
            {"user", user},
            {"password", pw},
            {"topic", topic}
        };
        
        ofstream o(CONFIG_FILE);
        o << config.dump(4) << endl;
        cout << "Configurazione salvata in " << CONFIG_FILE << ".\n" << endl;
    } else {
        f >> config;
    }

    // --- 2. Setup MQTT ---
    MQTTClient client;
    MQTTClient_create(&client, config["server"].get<string>().c_str(), "RPi_Display_CPP", MQTTCLIENT_PERSISTENCE_NONE, NULL);
    
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    
    string user = config["user"];
    string pw = config["password"];
    if (!user.empty()) {
        conn_opts.username = user.c_str();
        conn_opts.password = pw.c_str();
    }

    MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, NULL);

    if (MQTTClient_connect(client, &conn_opts) != MQTTCLIENT_SUCCESS) {
        cerr << "Errore: Impossibile connettersi al broker MQTT." << endl;
        mqtt_message = "Err MQTT";
    } else {
        cout << "Connesso a MQTT. Sottoscrizione a: " << config["topic"] << endl;
        MQTTClient_subscribe(client, config["topic"].get<string>().c_str(), 0);
    }

    // --- 3. Setup Grafica e Framebuffer ---
    // Configurazione Hint per Input su Driver Dummy
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "1"); // Tratta il touch come mouse
    SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "1"); // Viceversa
    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1"); // Evita conflitti con i segnali di sistema
    
    // Stampa i driver video disponibili per debug
    cout << "Driver SDL disponibili: ";
    for (int i = 0; i < SDL_GetNumVideoDrivers(); ++i) {
        cout << SDL_GetVideoDriver(i) << " ";
    }
    cout << endl;

    // Tentativo 1: KMSDRM (Standard per Raspberry Pi recenti con driver VC4)
    setenv("SDL_VIDEODRIVER", "kmsdrm", 1);
    
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        cout << "Driver kmsdrm fallito (" << SDL_GetError() << "). Tentativo 2: evdev..." << endl;
        
        // Tentativo 2: evdev (Visto che è presente nella tua lista driver!)
        // Questo driver è specifico per gestire input senza output video grafico su embedded
        setenv("SDL_VIDEODRIVER", "evdev", 1);
        
        if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
            cout << "Driver evdev fallito (" << SDL_GetError() << "). Tentativo 3: fbcon..." << endl;
            
            // Tentativo 3: fbcon
            setenv("SDL_VIDEODRIVER", "fbcon", 1);
            setenv("SDL_FBDEV", "/dev/fb0", 1);

            if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
                cout << "Driver fbcon fallito (" << SDL_GetError() << "). Tentativo 4: dummy..." << endl;

                // Tentativo 4: dummy (Fallback assoluto)
                setenv("SDL_VIDEODRIVER", "dummy", 1);
                if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
                    cerr << "Errore critico SDL: " << SDL_GetError() << endl;
                    return 1;
                }
            }
        }
    }
    
    cout << "Video Driver in uso: " << SDL_GetCurrentVideoDriver() << endl;

    if (TTF_Init() == -1) {
        cerr << "TTF Init Error: " << TTF_GetError() << endl;
        return 1;
    }

    // --- FIX INPUT: Creazione Finestra Fittizia ---
    // Anche se usiamo il driver "dummy", SDL richiede una finestra logica per
    // agganciare il focus della tastiera e del mouse/touch.
    SDL_Window *window = SDL_CreateWindow("MQTT Touch Receiver", 0, 0, 480, 320, SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN);
    
    // Forza la cattura dell'input sulla finestra
    if (window) {
        SDL_SetWindowGrab(window, SDL_TRUE);
        SDL_RaiseWindow(window); // Tenta di portare la finestra in primo piano (focus)
    }

    // Debug: Verifica se SDL vede il touch
    int num_touch = SDL_GetNumTouchDevices();
    cout << "Info: Dispositivi Touch rilevati da SDL: " << num_touch << endl;
    for(int i=0; i<num_touch; i++) {
        cout << "  - Touch Device ID: " << SDL_GetTouchDevice(i) << endl;
    }

    // Creiamo una surface a 16-bit (RGB565) esplicita
    SDL_Surface *screen = SDL_CreateRGBSurfaceWithFormat(0, 480, 320, 16, SDL_PIXELFORMAT_RGB565);
    if (!screen) {
        cerr << "Errore creazione surface: " << SDL_GetError() << endl;
        return 1;
    }

    // Apertura diretta del device framebuffer
    const char* fb_path = "/dev/fb1";
    FILE* f_fb = fopen(fb_path, "wb");
    if (!f_fb) {
        cerr << "Attenzione: " << fb_path << " non trovato. Provo /dev/fb0..." << endl;
        fb_path = "/dev/fb0";
        f_fb = fopen(fb_path, "wb");
        if (!f_fb) {
            cerr << "Errore critico: Impossibile aprire device framebuffer." << endl;
            return 1;
        }
    }

    // Nascondi cursore console TTY
    ofstream tty("/dev/tty1");
    if (tty.good()) {
        tty << "\033[?25l" << "\033[9;0]" << "\033[2J"; 
    }

    // Caricamento Font (cerca percorsi standard RPi)
    TTF_Font *font_large = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 50);
    TTF_Font *font_small = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 30);
    
    if (!font_large || !font_small) {
        cerr << "Warning: Font DejaVuSans non trovato. Il testo potrebbe non apparire." << endl;
        cerr << "Installa con: sudo apt install fonts-dejavu-core" << endl;
    }

    SDL_Color white = {255, 255, 255, 255};
    SDL_Color red = {255, 0, 0, 255};

    int touch_x = -1, touch_y = -1;

    // --- 4. Ciclo Principale ---
    while (running) {
        // Gestione Eventi (Touchscreen/Mouse)
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            // --- DEBUG INPUT: Decommenta per vedere i codici evento ---
            cout << "Evento ricevuto: " << event.type << endl; 

            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
                cout << "Tasto premuto: " << event.key.keysym.sym << endl; // Debug tastiera
                if (event.key.keysym.sym == SDLK_MINUS || event.key.keysym.sym == SDLK_KP_MINUS) {
                    running = false;
                }
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                // Ignoriamo i click del mouse 'grezzi' per il disegno perché spesso
                // hanno coordinate fuori scala (es. 0-4096) con il driver evdev.
                // Ci affidiamo a SDL_FINGER* qui sotto.
                cout << "Mouse Raw: " << event.button.x << ", " << event.button.y << endl;
            } else if (event.type == SDL_FINGERDOWN || event.type == SDL_FINGERMOTION) {
                float x = event.tfinger.x;
                float y = event.tfinger.y;

                // --- CALIBRAZIONE TOUCH ---
                // Se il tocco non corrisponde al dito, modifica queste flag:
                bool swap_xy = true;  // Metti 'true' se muovendo X si muove Y (assi scambiati)
                bool invert_x = false; // Metti 'true' se il tocco va al contrario in orizzontale
                bool invert_y = true; // Metti 'true' se il tocco va al contrario in verticale

                if (swap_xy) std::swap(x, y);
                if (invert_x) x = 1.0f - x;
                if (invert_y) y = 1.0f - y;

                // Conversione da 0.0-1.0 a Pixel reali (480x320)
                touch_x = (int)(x * 480);
                touch_y = (int)(y * 320);

                // Verifica uscita (Angolo alto destra)
                if (event.type == SDL_FINGERDOWN && touch_x > 430 && touch_y < 50) running = false;

            } else if (event.type == SDL_FINGERUP) {
                touch_x = -1;
                touch_y = -1;
            } else if (event.type == SDL_MOUSEMOTION) {
                if (event.motion.state & SDL_BUTTON_LMASK) {
                    touch_x = event.motion.x;
                    touch_y = event.motion.y;
                }
            } else if (event.type == SDL_MOUSEBUTTONUP) {
                touch_x = -1;
                touch_y = -1;
            }
        }

        // Pulisci schermo (Nero)
        SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0));

        // Disegna Pulsante Esci (Rosso in alto a destra)
        SDL_Rect btn_exit = {430, 0, 50, 50};
        SDL_FillRect(screen, &btn_exit, SDL_MapRGB(screen->format, 200, 0, 0));
        
        if (font_small) {
            SDL_Surface* txt_x = TTF_RenderUTF8_Blended(font_small, "X", white);
            if (txt_x) {
                SDL_Rect r_x = { 430 + (50 - txt_x->w)/2, (50 - txt_x->h)/2, 0, 0 };
                SDL_BlitSurface(txt_x, NULL, screen, &r_x);
                SDL_FreeSurface(txt_x);
            }
        }

        // Prepara testi
        string current_msg;
        { lock_guard<mutex> lock(msg_mutex); current_msg = mqtt_message; }
        
        if (font_small && font_large) {
            // Titolo Topic
            string topic_str = "Topic: " + config["topic"].get<string>();
            SDL_Surface* txt_title = TTF_RenderUTF8_Blended(font_small, topic_str.c_str(), white);
            if (txt_title) {
                SDL_Rect r = { (480 - txt_title->w) / 2, (320 / 2) - 30 - (txt_title->h / 2), 0, 0 };
                SDL_BlitSurface(txt_title, NULL, screen, &r);
                SDL_FreeSurface(txt_title);
            }

            // Messaggio MQTT
            SDL_Surface* txt_msg = TTF_RenderUTF8_Blended(font_large, current_msg.c_str(), red);
            if (txt_msg) {
                SDL_Rect r = { (480 - txt_msg->w) / 2, (320 / 2) + 20 - (txt_msg->h / 2), 0, 0 };
                SDL_BlitSurface(txt_msg, NULL, screen, &r);
                SDL_FreeSurface(txt_msg);
            }
        }

        // Disegna pallino bianco sul tocco (sopra tutto)
        if (touch_x >= 0 && touch_y >= 0) {
            SDL_Rect r_dot = { touch_x - 5, touch_y - 5, 10, 10 };
            SDL_FillRect(screen, &r_dot, SDL_MapRGB(screen->format, 255, 255, 255));
        }

        // Scrittura su Framebuffer fisico
        rewind(f_fb);
        fwrite(screen->pixels, 1, screen->w * screen->h * screen->format->BytesPerPixel, f_fb);
        fflush(f_fb);

        // Pausa (100ms = 10 FPS)
        SDL_Delay(100);
    }

    // Cleanup
    fclose(f_fb);
    if (font_large) TTF_CloseFont(font_large);
    if (font_small) TTF_CloseFont(font_small);
    if (window) SDL_DestroyWindow(window);
    SDL_FreeSurface(screen);
    TTF_Quit();
    SDL_Quit();
    MQTTClient_disconnect(client, 1000);
    MQTTClient_destroy(&client);

    return 0;
}