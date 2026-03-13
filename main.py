#prova schermo
import os
import sys
import time
import pygame

# --- Configurazione Ambiente ---
# Imposta il driver video su framebuffer console
os.environ["SDL_VIDEODRIVER"] = "dummy"
# os.environ["SDL_VIDEO_DEVICE"] = "/dev/dri/card1"

# Specifica il dispositivo framebuffer. 
# Di solito gli schermi SPI 3.5" sono su /dev/fb1
# Se non funziona, prova /dev/fb0
# os.environ["SDL_FBDEV"] = "/dev/fb1"

# --- Inizializzazione Pygame ---
try:
    pygame.init()
except pygame.error as e:
    print(f"Impossibile inizializzare Pygame: {e}")
    sys.exit()

# --- Dimensioni ---
# Le dimensioni ora corrispondono all'output di fbset: 480x320
WIDTH = 480
HEIGHT = 320
size = (WIDTH, HEIGHT)

try:
    # Usa depth=16 perché questi schermi solitamente lavorano a 16-bit (RGB565)
    screen = pygame.display.set_mode(size, 0, 16)
    
    # Workaround: Il driver 'dummy' tende a usare 32-bit anche se chiediamo 16.
    # Creiamo una superficie esplicita a 16-bit per la scrittura sicura sul framebuffer.
    fb_surface = pygame.Surface(size, 0, 16)
    
    # Apri direttamente il file del dispositivo framebuffer per scriverci dentro
    # Questo viene fatto dopo set_mode per assicurarsi che pygame sia inizializzato
    f_fb = open("/dev/fb1", "wb")

except FileNotFoundError:
    print("Errore: Dispositivo /dev/fb1 non trovato. Lo schermo è collegato correttamente?")
    sys.exit()
except PermissionError:
    print("Errore permessi: Impossibile scrivere su /dev/fb1. Verifica i gruppi utente.")
    sys.exit()
except pygame.error as e:
    print("Errore apertura display. Controlla che SDL_FBDEV punti al device corretto.")
    print(f"Errore Pygame: {e}")
    sys.exit()

# Nascondi il cursore del mouse (dato che siamo in console)
pygame.mouse.set_visible(False)
# Tenta di nascondere il cursore lampeggiante della console testuale
try:
    with open("/dev/tty1", "w") as tty:
        tty.write("\033[?25l") # Nasconde il cursore
        tty.write("\033[9;0]") # Disabilita lo spegnimento schermo (blanking)
        tty.write("\033[2J")   # Pulisce la schermata di login sotto la grafica
except (PermissionError, FileNotFoundError) as e:
    print(f"Info: Impossibile agire su tty1 ({e}). Lo sfarfallio potrebbe continuare.")
    print("      Consiglio: Esegui 'sudo usermod -a -G tty igino' e riavvia.")

# Stampa il driver video che è stato effettivamente scelto
print(f"Driver video in uso: {pygame.display.get_driver()}")

# --- Definizione Colori e Font ---
BLACK = (0, 0, 0)
WHITE = (255, 255, 255)
RED = (255, 0, 0)

# Usa un font di default. Puoi caricare un .ttf specifico se vuoi.
font_large = pygame.font.Font(None, 50)
font_small = pygame.font.Font(None, 30)

# --- Ciclo Principale ---
running = True
try:
    while running:
        # 1. Pulisci lo schermo (sfondo nero)
        screen.fill(BLACK)

        # 2. Crea le superfici di testo
        text_title = font_large.render("Raspberry Pi", True, WHITE)
        text_info = font_small.render("Modalità Console", True, RED)

        # 3. Calcola la posizione per centrare il testo
        rect_title = text_title.get_rect(center=(WIDTH // 2, HEIGHT // 2 - 30))
        rect_info = text_info.get_rect(center=(WIDTH // 2, HEIGHT // 2 + 20))

        # 4. Disegna il testo sullo schermo bufferizzato
        screen.blit(text_title, rect_title)
        screen.blit(text_info, rect_info)

        # 5. Aggiorna il display (necessario per il driver dummy)
        pygame.display.update()

        # 6. Copia la schermata sulla superficie sicura a 16-bit
        fb_surface.blit(screen, (0, 0))

        # 7. Copia manuale del buffer grafico sul dispositivo fisico
        f_fb.seek(0)
        f_fb.write(fb_surface.get_buffer())
        f_fb.flush()

        # Gestione eventi per uscire (es. CTRL+C da tastiera collegata)
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
        
        # Pausa per non consumare 100% CPU
        time.sleep(0.1)
except KeyboardInterrupt:
    print("Interruzione manuale (CTRL+C)")
finally:
    if 'f_fb' in locals() and f_fb is not None:
        f_fb.close()

pygame.quit()
