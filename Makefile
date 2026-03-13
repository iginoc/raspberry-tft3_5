# Makefile per il progetto MQTT Display C++

# Compilatore e flag
CXX = g++
CXXFLAGS = -Wall -Wextra -O2 -Wno-psabi
LDFLAGS = -lSDL2 -lSDL2_ttf -lpaho-mqtt3c

# File sorgente e eseguibile di destinazione
SRCS = main.cpp
TARGET = main

# Target di default: compila il programma
all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS) $(LDFLAGS)

# Pulisce i file compilati
clean:
	rm -f $(TARGET)

# Installa il servizio systemd
install: all
	@echo "Copiando il file di servizio in /etc/systemd/system..."
	sudo cp mqtt-display.service /etc/systemd/system/
	@echo "Ricaricando il demone systemd..."
	sudo systemctl daemon-reload
	@echo "Servizio installato. Per abilitarlo e avviarlo: sudo systemctl enable --now mqtt-display.service"

# Rende i target non associati a file
.PHONY: all clean install