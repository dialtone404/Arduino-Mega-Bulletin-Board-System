/*
 * Arduino Mega Terminal OS - Enhanced BBS Style System v3.0
 * Requires: Arduino Mega + Official Arduino Ethernet Shield with SD Card
 * 
 * Connect via PuTTY using Telnet protocol on port 23
 * Default admin user: admin / password: admin123
 * 
 * Features:
 * - Working Text Editor with proper file save/load
 * - Functional Calculator with multiple operations
 * - Weather information fetcher
 * - Stock market data viewer
 * - Games: Number Guessing, Trivia
 * - User notes system
 * - System logs
 * - Improved SD card file structure
 * - Created By: https://github.com/dialtone404/
 */

#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>

// Network Configuration
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
EthernetServer server(23);
EthernetClient client;

// SD Card Configuration
const int SD_CS_PIN = 4;
const int ETHERNET_CS_PIN = 10;

// System Configuration
#define MAX_CMD_LENGTH 128
#define MAX_USERNAME 16
#define MAX_PASSWORD 16
#define TIMEOUT_MS 600000
#define MAX_EDITOR_LINES 40
#define MAX_LINE_LENGTH 75

// Menu States
enum MenuState {
  MENU_MAIN,
  MENU_MESSAGES,
  MENU_FILES,
  MENU_EDITOR,
  MENU_EDITOR_MENU,
  MENU_CALCULATOR,
  MENU_NEWS,
  MENU_STATS,
  MENU_SETTINGS,
  MENU_COMMAND,
  MENU_WEATHER,
  MENU_STOCKS,
  MENU_GAMES,
  MENU_GAME_GUESS,
  MENU_UTILITIES
};

// STEP 1: Update the Session struct (add new variable)
struct Session {
  bool loggedIn;
  char username[MAX_USERNAME];
  bool isAdmin;
  unsigned long lastActivity;
  MenuState currentMenu;
  MenuState previousMenu;
  char editorBuffer[MAX_EDITOR_LINES][MAX_LINE_LENGTH];
  int editorLines;
  char editorFilename[32];
  int guessNumber;
  int guessAttempts;
  float calcMemory;
  bool waitingForContinue;  // ADD THIS
  MenuState returnToMenu;   // ADD THIS - stores which menu to return to
} session;

// Command buffer
char cmdBuffer[MAX_CMD_LENGTH];
int cmdIndex = 0;

// System Stats
struct SystemStats {
  unsigned long bootTime;
  unsigned long totalConnections;
  unsigned long messagesPosted;
  unsigned long filesCreated;
  unsigned long commandsExecuted;
} sysStats;

// ANSI Color Codes
const char* CLR_RESET = "\033[0m";
const char* CLR_GREEN = "\033[32m";
const char* CLR_CYAN = "\033[36m";
const char* CLR_YELLOW = "\033[33m";
const char* CLR_RED = "\033[31m";
const char* CLR_BLUE = "\033[34m";
const char* CLR_MAGENTA = "\033[35m";
const char* CLR_WHITE = "\033[37m";
const char* CLR_BRIGHT_GREEN = "\033[92m";
const char* CLR_BRIGHT_CYAN = "\033[96m";
const char* CLR_BRIGHT_YELLOW = "\033[93m";
const char* CLR_BRIGHT_WHITE = "\033[97m";

// Function declarations
void showMainMenu();
void showCurrentMenu();
void showPrompt();
void drawBox(const char* color, const char* title);
int freeRam();
void printUptime(unsigned long seconds);
void printUptimeFormatted(unsigned long seconds);
void printUptimeToFile(File &f, unsigned long seconds);

void setup() {
  Serial.begin(9600);
  
  // Initialize system stats
  sysStats.bootTime = millis();
  sysStats.totalConnections = 0;
  sysStats.messagesPosted = 0;
  sysStats.filesCreated = 0;
  sysStats.commandsExecuted = 0;
  
  // Disable Ethernet chip while setting up SD
  pinMode(ETHERNET_CS_PIN, OUTPUT);
  digitalWrite(ETHERNET_CS_PIN, HIGH);
  
  // Initialize SD Card
  Serial.print(F("Initializing SD card..."));
  pinMode(SD_CS_PIN, OUTPUT);
  
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println(F("SD init failed!"));
    while (1) {
      delay(1000);
    }
  }
  Serial.println(F("OK"));
  
  // Create directory structure
  SD.mkdir("messages");
  SD.mkdir("files");
  SD.mkdir("news");
  SD.mkdir("notes");
  SD.mkdir("logs");
  SD.mkdir("weather");
  SD.mkdir("stocks");
  
  // Initialize default user
  if (!SD.exists("users.txt")) {
    File f = SD.open("users.txt", FILE_WRITE);
    if (f) {
      f.println("admin:admin123:1");
      f.close();
      Serial.println(F("Created default admin user"));
    }
  }
  
  // Create default news
  if (!SD.exists("news/news.txt")) {
    File f = SD.open("news/news.txt", FILE_WRITE);
    if (f) {
      f.println("=== ARDUINO TERMINAL OS v3.0 ===");
      f.println();
      f.println("WHAT'S NEW:");
      f.println("* Working Text Editor - Create and save notes");
      f.println("* Calculator with memory functions");
      f.println("* Weather information viewer");
      f.println("* Stock market data");
      f.println("* Games: Number Guessing");
      f.println("* Improved file management");
      f.println();
      f.println("QUICK START:");
      f.println("- Try the Text Editor to create notes");
      f.println("- Play the Number Guessing game");
      f.println("- Post messages on the board");
      f.println("- Check system stats");
      f.println();
      f.println("Have fun exploring!");
      f.close();
    }
  }
  
  // Start Ethernet with DHCP
  Serial.print(F("Obtaining IP via DHCP..."));
  if (Ethernet.begin(mac) == 0) {
    Serial.println(F("DHCP failed!"));
    IPAddress ip(192, 168, 1, 177);
    Ethernet.begin(mac, ip);
    Serial.print(F("Using static IP: "));
  } else {
    Serial.print(F("DHCP success! IP: "));
  }
  Serial.println(Ethernet.localIP());
  
  server.begin();
  Serial.println(F("Terminal OS ready!"));
  Serial.print(F("Free RAM: "));
  Serial.print(freeRam());
  Serial.println(F(" bytes"));
  
  // Initialize session
  session.loggedIn = false;
  session.isAdmin = false;
  session.currentMenu = MENU_MAIN;
  session.editorLines = 0;
  session.calcMemory = 0;
    session.waitingForContinue = false;  // ADD THIS
  session.returnToMenu = MENU_MAIN;    // ADD THIS
}

void loop() {
  Ethernet.maintain();
  
  if (!client || !client.connected()) {
    client = server.available();
    if (client) {
      Serial.println(F("New client connected"));
      sysStats.totalConnections++;
      session.loggedIn = false;
      session.isAdmin = false;
      session.currentMenu = MENU_MAIN;
      session.waitingForContinue = false;
      session.returnToMenu = MENU_MAIN;
      cmdIndex = 0;
      session.lastActivity = millis();
      
      showWelcomeBanner();
      showLoginPrompt();
    }
  }
  
  if (client && client.connected()) {
    if (millis() - session.lastActivity > TIMEOUT_MS) {
      client.println(F("\r\n\nSession timeout. Goodbye!"));
      client.stop();
      return;
    }
    
    while (client.available()) {
      char c = client.read();
      session.lastActivity = millis();
      
      if (c == 8 || c == 127) {
        if (cmdIndex > 0) {
          cmdIndex--;
          client.write(8);
          client.write(' ');
          client.write(8);
        }
      }
      else if (c == '\n' || c == '\r') {
        if (cmdIndex > 0) {
          cmdBuffer[cmdIndex] = '\0';
          client.println();
          
          // Clear any remaining \r or \n in buffer
          delay(10);
          while (client.available()) {
            char peek = client.peek();
            if (peek == '\r' || peek == '\n') {
              client.read(); // discard it
            } else {
              break;
            }
          }
          
          processInput();
          cmdIndex = 0;
        } else if (session.loggedIn) {
          // Empty input - just pressed Enter
          client.println();
          if (session.waitingForContinue) {
            session.waitingForContinue = false;
            // Return to the menu that was saved
            switch (session.returnToMenu) {
              case MENU_MAIN:
                showMainMenu();
                break;
              case MENU_FILES:
                showFileMenu();
                break;
              case MENU_EDITOR_MENU:
                showEditorMenu();
                break;
              case MENU_GAMES:
                showGamesMenu();
                break;
              case MENU_UTILITIES:
                showUtilitiesMenu();
                break;
              case MENU_SETTINGS:
                showSettingsMenu();
                break;
              case MENU_MESSAGES:
                showMessageMenu();
                break;
              default:
                showCurrentMenu();
            }
          } else {
            // Just re-show the prompt, don't process as a choice
            showPrompt();
          }
        }
      }
      else if (cmdIndex < MAX_CMD_LENGTH - 1 && c >= 32 && c <= 126) {
        cmdBuffer[cmdIndex++] = c;
        client.write(c);
      }
    }
  }
}

void showWelcomeBanner() {
  client.print(F("\033[2J\033[H"));
  client.print(CLR_BRIGHT_GREEN);
  client.println(F("⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣀⣄⣠⣀⡀⣀⣠⣤⣤⣤⣀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀        "));
  client.println(F("⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣄⢠⣠⣼⣿⣿⣿⣟⣿⣿⣿⣿⣿⣿⣿⣿⡿⠋⠀⠀⠀⢠⣤⣦⡄⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠰⢦⣄⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀      "));
  client.println(F("⠀⠀⠀⠀⠀⠀⠀⠀⣼⣿⣟⣾⣿⣽⣿⣿⣅⠈⠉⠻⣿⣿⣿⣿⣿⡿⠇⠀⠀⠀⠀⠀⠉⠀⠀⠀⠀⠀⢀⡶⠒⢉⡀⢠⣤⣶⣶⣿⣷⣆⣀⡀⠀⢲⣖⠒⠀⠀⠀⠀⠀⠀⠀    "));
  client.println(F("⢀⣤⣾⣶⣦⣤⣤⣶⣿⣿⣿⣿⣿⣿⣽⡿⠻⣷⣀⠀⢻⣿⣿⣿⡿⠟⠀⠀⠀⠀⠀⠀⣤⣶⣶⣤⣀⣀⣬⣷⣦⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣶⣦⣤⣦⣼⣀⠀ "));
  client.println(F("⠈⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⠛⠓⣿⣿⠟⠁⠘⣿⡟⠁⠀⠘⠛⠁⠀⠀⢠⣾⣿⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⠏⠙⠁"));
  client.println(F("⠀⠸⠟⠋⠀⠈⠙⣿⣿⣿⣿⣿⣿⣷⣦⡄⣿⣿⣿⣆⠀⠀⠀⠀⠀⠀⠀⠀⣼⣆⢘⣿⣯⣼⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡉⠉⢱⡿⠀⠀⠀⠀⠀  "));
  client.println(F("⠀⠀⠀⠀⠀⠀⠀⠘⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣟⡿⠦⠀⠀⠀⠀⠀⠀⠀⠙⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⡗⠀⠈⠀⠀⠀⠀⠀⠀   "));
  client.println(F("⠀⠀⠀⠀⠀⠀⠀⠀⢻⣿⣿⣿⣿⣿⣿⣿⣿⠋⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⢿⣿⣉⣿⡿⢿⢷⣾⣾⣿⣞⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠋⣠⠟⠀⠀⠀⠀⠀⠀⠀⠀    "));
  client.println(F("⠀⠀⠀⠀⠀⠀⠀⠀⠀⠹⣿⣿⣿⠿⠿⣿⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣀⣾⣿⣿⣷⣦⣶⣦⣼⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⠈⠛⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀    "));
  client.println(F("⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠉⠻⣿⣤⡖⠛⠶⠤⡀⠀⠀⠀⠀⠀⠀⠀⢰⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⠁⠙⣿⣿⠿⢻⣿⣿⡿⠋⢩⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀    "));
  client.println(F("⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠙⠧⣤⣦⣤⣄⡀⠀⠀⠀⠀⠀⠘⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡇⠀⠀⠀⠘⣧⠀⠈⣹⡻⠇⢀⣿⡆⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀     "));
  client.println(F("⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢠⣿⣿⣿⣿⣿⣤⣀⡀⠀⠀⠀⠀⠀⠀⠈⢽⣿⣿⣿⣿⣿⠋⠀⠀⠀⠀⠀⠀⠀⠀⠹⣷⣴⣿⣷⢲⣦⣤⡀⢀⡀⠀⠀⠀⠀⠀⠀     "));
  client.println(F("⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⢿⣿⣿⣿⣿⣿⣿⠟⠀⠀⠀⠀⠀⠀⠀⢸⣿⣿⣿⣿⣷⢀⡄⠀⠀⠀⠀⠀⠀⠀⠀⠈⠉⠂⠛⣆⣤⡜⣟⠋⠙⠂⠀⠀⠀⠀⠀     "));
  client.println(F("⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢹⣿⣿⣿⣿⠟⠀⠀⠀⠀⠀⠀⠀⠀⠘⣿⣿⣿⣿⠉⣿⠃⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣤⣾⣿⣿⣿⣿⣆⠀⠰⠄⠀⠉⠀⠀      "));
  client.println(F("⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣸⣿⣿⡿⠃⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢹⣿⡿⠃⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢻⣿⠿⠿⣿⣿⣿⠇⠀⠀⢀⠀⠀⠀       "));
  client.println(F("⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣿⡿⠛⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⢻⡇⠀⠀⢀⣼⠗⠀⠀        "));
  client.println(F("⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⣿⠃⣀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠙⠁⠀⠀⠀         "));
  client.println(F("⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠙⠒⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀           "));
  client.println(F("                                                                                     "));
  client.println(F("╔════════════════════════════════════════════════════════════════════════╗"));
  client.println(F("║                                                                        ║"));
  client.println(F("║              ╔═╗╦═╗╔╦╗╦ ╦╦╔╗╔╔═╗  ╔╦╗╔═╗╔═╗╔═╗  ╔═╗╔═╗                 ║"));
  client.println(F("║              ╠═╣╠╦╝ ║║║ ║║║║║║ ║  ║║║║╣ ║ ╦╠═╣  ║ ║╚═╗                 ║"));
  client.println(F("║              ╩ ╩╩╚══╩╝╚═╝╩╝╚╝╚═╝  ╩ ╩╚═╝╚═╝╩ ╩  ╚═╝╚═╝                 ║"));
  client.println(F("║                                                                        ║"));
  client.println(F("║                    Terminal Operating System v3.0                      ║"));
  client.println(F("║                         Authorized Users Only                          ║"));
  client.println(F("║                      System --- Copyright 1989                         ║"));
  client.println(F("╚════════════════════════════════════════════════════════════════════════╝"));
  client.print(CLR_RESET);
  client.println();
  
  client.print(CLR_YELLOW);
  client.print(F("System IP: "));
  client.print(CLR_WHITE);
  client.print(Ethernet.localIP());
  client.print(CLR_YELLOW);
  client.print(F("  |  Uptime: "));
  client.print(CLR_WHITE);
  printUptime((millis() - sysStats.bootTime) / 1000);
  client.print(CLR_YELLOW);
  client.print(F("  |  Connections: "));
  client.print(CLR_WHITE);
  client.println(sysStats.totalConnections);
  client.print(CLR_RESET);
  client.println();
}

void printUptime(unsigned long seconds) {
  unsigned long days = seconds / 86400;
  unsigned long hours = (seconds % 86400) / 3600;
  unsigned long mins = (seconds % 3600) / 60;
  
  if (days > 0) {
    client.print(days);
    client.print(F("d "));
  }
  client.print(hours);
  client.print(F("h "));
  client.print(mins);
  client.print(F("m"));
}

void showLoginPrompt() {
  client.print(CLR_BRIGHT_GREEN);
  client.print(F("Username: "));
  client.print(CLR_RESET);
}

void processInput() {
  sysStats.commandsExecuted++;
  
  if (!session.loggedIn) {
    handleLogin();
    return;
  }
  
  if (session.currentMenu == MENU_EDITOR) {
    handleEditorInput();
    return;
  }
  
  if (session.currentMenu == MENU_CALCULATOR) {
    handleCalculatorInput();
    return;
  }
  
  if (session.currentMenu == MENU_COMMAND) {
    handleCommandInput();
    return;
  }
  
  if (session.currentMenu == MENU_GAME_GUESS) {
    handleGuessGame();
    return;
  }
  
  if (session.currentMenu == MENU_EDITOR_MENU) {
    handleEditorMenuChoice(atoi(cmdBuffer));
    return;
  }
  
  handleMenuInput();
}

void handleLogin() {
  static bool usernameEntered = false;
  static char tempUser[MAX_USERNAME];
  
  if (!usernameEntered) {
    strncpy(tempUser, cmdBuffer, MAX_USERNAME - 1);
    tempUser[MAX_USERNAME - 1] = '\0';
    usernameEntered = true;
    client.print(CLR_BRIGHT_GREEN);
    client.print(F("Password: "));
    client.print(CLR_RESET);
  } else {
    if (verifyLogin(tempUser, cmdBuffer)) {
      session.loggedIn = true;
      strncpy(session.username, tempUser, MAX_USERNAME - 1);
      client.println();
      client.print(CLR_BRIGHT_GREEN);
      client.print(F("\r\n✓ Welcome, "));
      client.print(session.username);
      client.println(F("!"));
      client.print(CLR_RESET);
      delay(1000);
      showMainMenu();
    } else {
      client.println();
      client.print(CLR_RED);
      client.println(F("\r\n✗ Login failed!"));
      client.print(CLR_RESET);
      usernameEntered = false;
      delay(1500);
      showWelcomeBanner();
      showLoginPrompt();
    }
  }
}

bool verifyLogin(const char* username, const char* password) {
  File f = SD.open("users.txt", FILE_READ);
  if (!f) return false;
  
  char line[64];
  int idx = 0;
  
  while (f.available()) {
    char c = f.read();
    if (c == '\n' || c == '\r') {
      if (idx > 0) {
        line[idx] = '\0';
        char* user = strtok(line, ":");
        char* pass = strtok(NULL, ":");
        char* admin = strtok(NULL, ":");
        
        if (user && pass && strcmp(user, username) == 0 && strcmp(pass, password) == 0) {
          if (admin && atoi(admin) == 1) {
            session.isAdmin = true;
          }
          f.close();
          return true;
        }
        idx = 0;
      }
    } else if (idx < 63) {
      line[idx++] = c;
    }
  }
  
  f.close();
  return false;
}

void showMainMenu() {
  client.print(F("\033[2J\033[H"));
  session.currentMenu = MENU_MAIN;
  
  drawBox(CLR_RED, "MAIN MENU");
  
  client.println();
  client.print(CLR_YELLOW);
  client.print(F("  User: "));
  client.print(CLR_BRIGHT_GREEN);
  client.print(session.username);
  if (session.isAdmin) {
    client.print(CLR_BRIGHT_YELLOW);
    client.print(F(" [ADMIN]"));
  }
  client.print(CLR_YELLOW);
  client.print(F("  |  Uptime: "));
  client.print(CLR_WHITE);
  printUptime((millis() - sysStats.bootTime) / 1000);
  client.println();
  client.print(CLR_RESET);
  client.println();
  
    client.print(CLR_BRIGHT_GREEN);
  client.println(F("  ┌─────────────────────────────────────────────────────────┐"));
  client.println(F("  │                                                         │"));
  client.println(F("  │  [1] Message Board    - Post and read messages          │"));
  client.println(F("  │  [2] Text Editor      - Create/edit documents           │"));
  client.println(F("  │  [3] File Manager     - Browse files and notes          │"));
  client.println(F("  │  [4] Calculator       - Math calculations + memory      │"));
  client.println(F("  │  [5] Weather Info     - Current weather conditions      │"));
  client.println(F("  │  [6] Stock Market     - View stock information          │"));
  client.println(F("  │  [7] Games            - Play interactive games          │"));
  client.println(F("  │  [8] Utilities        - Tools and system info           │"));
  client.println(F("  │  [9] News Reader      - Read latest updates             │"));
  client.println(F("  │                                                         │"));
  client.println(F("  │  [0] Logout           - Exit the system                 │"));
  client.println(F("  │                                                         │"));
  client.println(F("  └─────────────────────────────────────────────────────────┘"));
  client.print(CLR_RESET);
  client.println();
  
  showPrompt();
}

void showCurrentMenu() {
  switch (session.currentMenu) {
    case MENU_MAIN:
      showMainMenu();
      break;
    case MENU_MESSAGES:
      showMessageMenu();
      break;
    case MENU_FILES:
      showFileMenu();
      break;
    case MENU_STATS:
      showStatsMenu();
      break;
    case MENU_SETTINGS:
      showSettingsMenu();
      break;
    case MENU_NEWS:
      showNewsReader();
      break;
    case MENU_UTILITIES:
      showUtilitiesMenu();
      break;
    case MENU_GAMES:
      showGamesMenu();
      break;
    default:
      showMainMenu();
  }
}

void handleMenuInput() {
  int choice = atoi(cmdBuffer);
  
  switch (session.currentMenu) {
    case MENU_MAIN:
      handleMainMenuChoice(choice);
      break;
    case MENU_MESSAGES:
      handleMessageMenuChoice(choice);
      break;
    case MENU_FILES:
      handleFileMenuChoice(choice);
      break;
    case MENU_STATS:
      if (choice == 0) showMainMenu();
      else showPrompt();
      break;
    case MENU_SETTINGS:
      handleSettingsChoice(choice);
      break;
    case MENU_NEWS:
      if (choice == 0) showMainMenu();
      else showPrompt();
      break;
    case MENU_WEATHER:
      if (choice == 0) showMainMenu();
      else showPrompt();
      break;
    case MENU_STOCKS:
      if (choice == 0) showMainMenu();
      else showPrompt();
      break;
    case MENU_UTILITIES:
      handleUtilitiesChoice(choice);
      break;
    case MENU_GAMES:
      handleGamesChoice(choice);
      break;
    default:
      showMainMenu();
  }
}

void handleMainMenuChoice(int choice) {
  switch (choice) {
    case 1:
      showMessageMenu();
      break;
    case 2:
      showEditorMenu();
      break;
    case 3:
      showFileMenu();
      break;
    case 4:
      startCalculator();
      break;
    case 5:
      showWeather();
      break;
    case 6:
      showStocks();
      break;
    case 7:
      showGamesMenu();
      break;
    case 8:
      showUtilitiesMenu();
      break;
    case 9:
      showNewsReader();
      break;
    case 0:
      logout();
      break;
    default:
      client.print(CLR_RED);
      client.println(F("Invalid choice."));
      client.print(CLR_RESET);
      showPrompt();
  }
}

void showEditorMenu() {
  client.print(F("\033[2J\033[H"));
  session.currentMenu = MENU_EDITOR_MENU;
  
  drawBox(CLR_GREEN, "TEXT EDITOR");
  
  client.println();
  client.print(CLR_BRIGHT_CYAN);
  client.println(F("  ┌─────────────────────────────────────────────────────────┐"));
  client.println(F("  │                                                         │"));
  client.println(F("  │  [1] Create New Note  - Start a new document            │"));
  client.println(F("  │  [2] Edit Note        - Edit existing note              │"));
  client.println(F("  │  [3] View All Notes   - List all saved notes            │"));
  client.println(F("  │                                                         │"));
  client.println(F("  │  [0] Back to Main Menu                                  │"));
  client.println(F("  │                                                         │"));
  client.println(F("  └─────────────────────────────────────────────────────────┘"));
  client.print(CLR_RESET);
  client.println();
  
  showPrompt();
}

void handleEditorMenuChoice(int choice) {
  switch (choice) {
    case 1:
      createNewNote();
      break;
    case 2:
      editExistingNote();
      break;
    case 3:
      listAllNotes();
      break;
    case 0:
      showMainMenu();
      break;
    default:
      client.println(F("Invalid choice."));
      showPrompt();
  }
}

void createNewNote() {
  client.println();
  client.print(CLR_BRIGHT_GREEN);
  client.print(F("Enter filename (e.g., mynote.txt): "));
  client.print(CLR_RESET);
  
  char filename[32];
  int idx = 0;
  unsigned long startTime = millis();
  
  while (millis() - startTime < 30000) {
    if (client.available()) {
      char c = client.read();
      if (c == '\n' || c == '\r') {
        filename[idx] = '\0';
        break;
      } else if (c == 8 || c == 127) {
        if (idx > 0) {
          idx--;
          client.write(8);
          client.write(' ');
          client.write(8);
        }
      } else if (idx < 31 && c >= 32) {
        filename[idx++] = c;
        client.write(c);
      }
    }
  }
  
  if (idx == 0) {
    client.println();
    client.println(F("Cancelled."));
    delay(1000);
    showEditorMenu();
    return;
  }
  
  strcpy(session.editorFilename, filename);
  session.editorLines = 0;
  startTextEditor();
}

void editExistingNote() {
  client.println();
  client.println(F("Available notes:"));
  
  File root = SD.open("notes");
  if (root) {
    int count = 0;
    while (true) {
      File entry = root.openNextFile();
      if (!entry) break;
      if (!entry.isDirectory()) {
        client.print(F("  - "));
        client.println(entry.name());
        count++;
      }
      entry.close();
    }
    root.close();
    
    if (count == 0) {
      client.println(F("  (no notes found)"));
      delay(2000);
      showEditorMenu();
      return;
    }
  }
  
  client.println();
  client.print(F("Enter filename to edit: "));
  
  char filename[32];
  int idx = 0;
  unsigned long startTime = millis();
  
  while (millis() - startTime < 30000) {
    if (client.available()) {
      char c = client.read();
      if (c == '\n' || c == '\r') {
        filename[idx] = '\0';
        break;
      } else if (c == 8 || c == 127) {
        if (idx > 0) {
          idx--;
          client.write(8);
          client.write(' ');
          client.write(8);
        }
      } else if (idx < 31 && c >= 32) {
        filename[idx++] = c;
        client.write(c);
      }
    }
  }
  
  if (idx == 0) {
    client.println();
    showEditorMenu();
    return;
  }
  
  strcpy(session.editorFilename, filename);
  loadFileToEditor();
  startTextEditor();
}

void loadFileToEditor() {
  char fullPath[40];
  sprintf(fullPath, "notes/%s", session.editorFilename);
  
  File f = SD.open(fullPath, FILE_READ);
  if (!f) {
    session.editorLines = 0;
    return;
  }
  
  session.editorLines = 0;
  char lineBuf[MAX_LINE_LENGTH];
  int lineIdx = 0;
  
  while (f.available() && session.editorLines < MAX_EDITOR_LINES) {
    char c = f.read();
    if (c == '\n' || c == '\r') {
      if (lineIdx > 0) {
        lineBuf[lineIdx] = '\0';
        strncpy(session.editorBuffer[session.editorLines], lineBuf, MAX_LINE_LENGTH - 1);
        session.editorBuffer[session.editorLines][MAX_LINE_LENGTH - 1] = '\0';
        session.editorLines++;
        lineIdx = 0;
      }
    } else if (lineIdx < MAX_LINE_LENGTH - 1) {
      lineBuf[lineIdx++] = c;
    }
  }
  
  f.close();
}

// STEP 4: Update listAllNotes() function
void listAllNotes() {
  client.print(F("\033[2J\033[H"));
  drawBox(CLR_GREEN, "ALL NOTES");
  client.println();
  
  File root = SD.open("notes");
  if (root) {
    int count = 0;
    while (true) {
      File entry = root.openNextFile();
      if (!entry) break;
      if (!entry.isDirectory()) {
        client.print(F("  ["));
        client.print(++count);
        client.print(F("] "));
        client.print(entry.name());
        client.print(F(" ("));
        client.print(entry.size());
        client.println(F(" bytes)"));
      }
      entry.close();
    }
    root.close();
    
    if (count == 0) {
      client.println(F("  No notes found."));
    }
  }
  
  client.println();
  client.println(F("Press Enter to continue..."));
  session.waitingForContinue = true;
  session.returnToMenu = MENU_EDITOR_MENU;  // Return to editor menu
}

void startTextEditor() {
  client.print(F("\033[2J\033[H"));
  session.currentMenu = MENU_EDITOR;
  
  drawBox(CLR_GREEN, "TEXT EDITOR");
  
  client.println();
  client.print(CLR_YELLOW);
  client.print(F("  Editing: "));
  client.print(session.editorFilename);
  client.print(F("  (Line "));
  client.print(session.editorLines + 1);
  client.print(F("/"));
  client.print(MAX_EDITOR_LINES);
  client.println(F(")"));
  client.print(CLR_RESET);
  client.println();
  client.println(F("  Commands: :w (save) | :q (quit) | :wq (save & quit) | :v (view)"));
  client.println(F("  ───────────────────────────────────────────────────────────"));
  client.println();
  
  showPrompt();
}

void handleEditorInput() {
  if (cmdBuffer[0] == ':') {
    if (strcmp(cmdBuffer, ":q") == 0) {
      session.editorLines = 0;
      showEditorMenu();
      return;
    } else if (strcmp(cmdBuffer, ":w") == 0) {
      saveEditorFile();
      client.println(F("Saved! Continue editing..."));
      showPrompt();
      return;
    } else if (strcmp(cmdBuffer, ":wq") == 0) {
      saveEditorFile();
      session.editorLines = 0;
      showEditorMenu();
      return;
    } else if (strcmp(cmdBuffer, ":v") == 0) {
      viewEditorContent();
      showPrompt();
      return;
    }
  }
  
  if (session.editorLines < MAX_EDITOR_LINES) {
    strncpy(session.editorBuffer[session.editorLines], cmdBuffer, MAX_LINE_LENGTH - 1);
    session.editorBuffer[session.editorLines][MAX_LINE_LENGTH - 1] = '\0';
    session.editorLines++;
    
    client.print(F("[Line "));
    client.print(session.editorLines + 1);
    client.print(F("/"));
    client.print(MAX_EDITOR_LINES);
    client.println(F("]"));
  } else {
    client.println(F("Buffer full! Save with :w"));
  }
  
  showPrompt();
}

void viewEditorContent() {
  client.println();
  client.println(F("─── Current Content ───"));
  for (int i = 0; i < session.editorLines; i++) {
    client.print(i + 1);
    client.print(F(": "));
    client.println(session.editorBuffer[i]);
  }
  client.println(F("───────────────────────"));
  client.println();
}

void saveEditorFile() {
  char fullPath[40];
  sprintf(fullPath, "notes/%s", session.editorFilename);
  
  if (SD.exists(fullPath)) {
    SD.remove(fullPath);
  }
  
  File f = SD.open(fullPath, FILE_WRITE);
  if (f) {
    for (int i = 0; i < session.editorLines; i++) {
      f.println(session.editorBuffer[i]);
    }
    f.close();
    sysStats.filesCreated++;
    client.print(CLR_BRIGHT_GREEN);
    client.print(F("✓ Saved: "));
    client.println(fullPath);
    client.print(CLR_RESET);
  } else {
    client.print(CLR_RED);
    client.println(F("✗ Error saving!"));
    client.print(CLR_RESET);
  }
}

void startCalculator() {
  client.print(F("\033[2J\033[H"));
  session.currentMenu = MENU_CALCULATOR;
  
  drawBox(CLR_YELLOW, "CALCULATOR");
  
  client.println();
  client.println(F("  Operations: + - * / (spaces required)"));
  client.println(F("  Memory: M (store) | MR (recall) | MC (clear)"));
  client.println(F("  Example: 5 + 3, 10 * 2, M, MR"));
  client.println(F("  Type 'exit' to return"));
  client.println(F("  ───────────────────────────────────────────────────────────"));
  client.println();
  client.print(F("  Memory: "));
  client.println(session.calcMemory, 2);
  client.println();
  
  showPrompt();
}

void handleCalculatorInput() {
  if (strcmp(cmdBuffer, "exit") == 0) {
    showMainMenu();
    return;
  }
  
  if (strcmp(cmdBuffer, "MC") == 0 || strcmp(cmdBuffer, "mc") == 0) {
    session.calcMemory = 0;
    client.println(F("Memory cleared"));
    client.println();
    showPrompt();
    return;
  }
  
  if (strcmp(cmdBuffer, "MR") == 0 || strcmp(cmdBuffer, "mr") == 0) {
    client.print(CLR_BRIGHT_GREEN);
    client.print(F("  Memory = "));
    client.println(session.calcMemory, 2);
    client.print(CLR_RESET);
    client.println();
    showPrompt();
    return;
  }
  
  float num1, num2, result;
  char op;
  
  int parsed = sscanf(cmdBuffer, "%f %c %f", &num1, &op, &num2);
  
  if (parsed == 3) {
    bool validOp = true;
    
    switch (op) {
      case '+':
        result = num1 + num2;
        break;
      case '-':
        result = num1 - num2;
        break;
      case '*':
      case 'x':
      case 'X':
        result = num1 * num2;
        break;
      case '/':
        if (num2 != 0) {
          result = num1 / num2;
        } else {
          client.println(F("Error: Division by zero!"));
          validOp = false;
        }
        break;
      default:
        validOp = false;
        client.println(F("Invalid operator"));
    }
    
    if (validOp) {
      client.print(CLR_BRIGHT_GREEN);
      client.print(F("  = "));
      client.println(result, 2);
      client.print(CLR_RESET);
      
      client.print(F("  Store in memory? (M/enter): "));
      
      char response[10];
      int idx = 0;
      unsigned long startTime = millis();
      
      while (millis() - startTime < 5000) {
        if (client.available()) {
          char c = client.read();
          if (c == '\n' || c == '\r') {
            response[idx] = '\0';
            break;
          } else if (idx < 9) {
            response[idx++] = c;
            client.write(c);
          }
        }
      }
      
      if (idx > 0 && (response[0] == 'M' || response[0] == 'm')) {
        session.calcMemory = result;
        client.println();
        client.println(F("Stored in memory!"));
      }
    }
  } else {
    client.println(F("Invalid format. Use: 5 + 3"));
  }
  
  client.println();
  showPrompt();
}

void showWeather() {
  client.print(F("\033[2J\033[H"));
  session.currentMenu = MENU_WEATHER;
  
  drawBox(CLR_CYAN, "WEATHER INFORMATION");
  
  client.println();
  client.print(CLR_BRIGHT_YELLOW);
  client.println(F("  Current Weather Conditions"));
  client.print(CLR_RESET);
  client.println(F("  ───────────────────────────────────────────────────────────"));
  client.println();
  
  client.println(F("  Location: San Francisco, CA"));
  client.println(F("  Temperature: 68°F (20°C)"));
  client.println(F("  Conditions: Partly Cloudy"));
  client.println(F("  Humidity: 65%"));
  client.println(F("  Wind: 12 mph NW"));
  client.println(F("  Pressure: 30.12 inHg"));
  client.println();
  client.print(CLR_YELLOW);
  client.println(F("  Note: Weather data is simulated. Connect to weather"));
  client.println(F("  API service for real-time data."));
  client.print(CLR_RESET);
  client.println();
  client.println(F("  [0] Back to Main Menu"));
  client.println();
  
  showPrompt();
}

void showStocks() {
  client.print(F("\033[2J\033[H"));
  session.currentMenu = MENU_STOCKS;
  
  drawBox(CLR_GREEN, "STOCK MARKET INFORMATION");
  
  client.println();
  client.print(CLR_BRIGHT_CYAN);
  client.println(F("  Market Summary"));
  client.print(CLR_RESET);
  client.println(F("  ───────────────────────────────────────────────────────────"));
  client.println();
  
  client.println(F("  AAPL - Apple Inc.           $178.50  (+2.3%)"));
  client.println(F("  MSFT - Microsoft Corp.      $412.75  (+1.8%)"));
  client.println(F("  GOOGL - Alphabet Inc.       $142.30  (-0.5%)"));
  client.println(F("  AMZN - Amazon.com Inc.      $178.90  (+3.1%)"));
  client.println(F("  TSLA - Tesla Inc.           $248.60  (+5.2%)"));
  client.println();
  client.println(F("  S&P 500:  5,234.18  (+0.8%)"));
  client.println(F("  NASDAQ:  16,745.30  (+1.2%)"));
  client.println(F("  DOW:     42,863.86  (+0.3%)"));
  client.println();
  client.print(CLR_YELLOW);
  client.println(F("  Note: Stock data is simulated. Connect to market"));
  client.println(F("  data API for real-time quotes."));
  client.print(CLR_RESET);
  client.println();
  client.println(F("  [0] Back to Main Menu"));
  client.println();
  
  showPrompt();
}

void showGamesMenu() {
  client.print(F("\033[2J\033[H"));
  session.currentMenu = MENU_GAMES;
  
  drawBox(CLR_MAGENTA, "GAMES");
  
  client.println();
  client.print(CLR_BRIGHT_CYAN);
  client.println(F("  ┌─────────────────────────────────────────────────────────┐"));
  client.println(F("  │                                                         │"));
  client.println(F("  │  [1] Number Guessing  - Guess the number (1-100)        │"));
  client.println(F("  │  [2] Math Quiz        - Test your math skills           │"));
  client.println(F("  │  [3] High Scores      - View game statistics            │"));
  client.println(F("  │                                                         │"));
  client.println(F("  │  [0] Back to Main Menu                                  │"));
  client.println(F("  │                                                         │"));
  client.println(F("  └─────────────────────────────────────────────────────────┘"));
  client.print(CLR_RESET);
  client.println();
  
  showPrompt();
}

void handleGamesChoice(int choice) {
  switch (choice) {
    case 1:
      startGuessGame();
      break;
    case 2:
      client.println(F("Math Quiz coming soon!"));
      delay(2000);
      showGamesMenu();
      break;
    case 3:
      showHighScores();
      break;
    case 0:
      showMainMenu();
      break;
    default:
      client.println(F("Invalid choice."));
      showPrompt();
  }
}

void startGuessGame() {
  client.print(F("\033[2J\033[H"));
  session.currentMenu = MENU_GAME_GUESS;
  
  drawBox(CLR_MAGENTA, "NUMBER GUESSING GAME");
  
  randomSeed(millis() + analogRead(A0));
  session.guessNumber = random(1, 101);
  session.guessAttempts = 0;
  
  client.println();
  client.println(F("  I'm thinking of a number between 1 and 100."));
  client.println(F("  Can you guess it?"));
  client.println();
  client.println(F("  Type 'quit' to exit"));
  client.println(F("  ───────────────────────────────────────────────────────────"));
  client.println();
  
  showPrompt();
}

void handleGuessGame() {
  if (strcmp(cmdBuffer, "quit") == 0) {
    showGamesMenu();
    return;
  }
  
  int guess = atoi(cmdBuffer);
  session.guessAttempts++;
  
  if (guess < 1 || guess > 100) {
    client.println(F("Please enter a number between 1 and 100."));
  } else if (guess < session.guessNumber) {
    client.print(CLR_YELLOW);
    client.println(F("Too low! Try again."));
    client.print(CLR_RESET);
  } else if (guess > session.guessNumber) {
    client.print(CLR_YELLOW);
    client.println(F("Too high! Try again."));
    client.print(CLR_RESET);
  } else {
    client.println();
    client.print(CLR_BRIGHT_GREEN);
    client.println(F("🎉 Correct! You guessed it!"));
    client.print(F("Attempts: "));
    client.println(session.guessAttempts);
    client.print(CLR_RESET);
    client.println();
    client.println(F("Press Enter to continue..."));
    
    saveGameScore(session.guessAttempts);
    
    session.currentMenu = MENU_GAMES;
    return;
  }
  
  client.println();
  showPrompt();
}

void saveGameScore(int attempts) {
  File f = SD.open("logs/scores.txt", FILE_WRITE);
  if (f) {
    f.print(session.username);
    f.print(":");
    f.print(attempts);
    f.print(":");
    f.println(millis() / 1000);
    f.close();
  }
}

// STEP 5: Update showHighScores() function
void showHighScores() {
  client.print(F("\033[2J\033[H"));
  drawBox(CLR_MAGENTA, "HIGH SCORES");
  
  client.println();
  client.println(F("  Number Guessing Game - Best Scores"));
  client.println(F("  ───────────────────────────────────────────────────────────"));
  client.println();
  
  File f = SD.open("logs/scores.txt", FILE_READ);
  if (f) {
    int count = 0;
    char line[64];
    int idx = 0;
    
    while (f.available() && count < 10) {
      char c = f.read();
      if (c == '\n' || c == '\r') {
        if (idx > 0) {
          line[idx] = '\0';
          char* user = strtok(line, ":");
          char* score = strtok(NULL, ":");
          
          if (user && score) {
            client.print(F("  "));
            client.print(user);
            client.print(F(" - "));
            client.print(score);
            client.println(F(" attempts"));
            count++;
          }
          idx = 0;
        }
      } else if (idx < 63) {
        line[idx++] = c;
      }
    }
    f.close();
    
    if (count == 0) {
      client.println(F("  No scores yet!"));
    }
  } else {
    client.println(F("  No scores recorded yet."));
  }
  
  client.println();
  client.println(F("Press Enter to continue..."));
  session.waitingForContinue = true;
  session.returnToMenu = MENU_GAMES;  // Return to games menu
}

void showUtilitiesMenu() {
  client.print(F("\033[2J\033[H"));
  session.currentMenu = MENU_UTILITIES;
  
  drawBox(CLR_BLUE, "UTILITIES");
  
  client.println();
  client.print(CLR_BRIGHT_CYAN);
  client.println(F("  ┌─────────────────────────────────────────────────────────┐"));
  client.println(F("  │                                                         │"));
  client.println(F("  │  [1] System Stats     - View system information         │"));
  client.println(F("  │  [2] Network Info     - Network configuration           │"));
  client.println(F("  │  [3] Settings         - User preferences                │"));
  client.println(F("  │  [4] Command Mode     - Advanced CLI                    │"));
  client.println(F("  │  [5] System Log       - View activity log               │"));
  client.println(F("  │                                                         │"));
  client.println(F("  │  [0] Back to Main Menu                                  │"));
  client.println(F("  │                                                         │"));
  client.println(F("  └─────────────────────────────────────────────────────────┘"));
  client.print(CLR_RESET);
  client.println();
  
  showPrompt();
}

void handleUtilitiesChoice(int choice) {
  switch (choice) {
    case 1:
      showStatsMenu();
      break;
    case 2:
      showNetworkInfo();
      break;
    case 3:
      showSettingsMenu();
      break;
    case 4:
      startCommandMode();
      break;
    case 5:
      showSystemLog();
      break;
    case 0:
      showMainMenu();
      break;
    default:
      client.println(F("Invalid choice."));
      showPrompt();
  }
}

// STEP 6: Update showSystemLog() function
void showSystemLog() {
  client.print(F("\033[2J\033[H"));
  drawBox(CLR_BLUE, "SYSTEM LOG");
  
  client.println();
  client.print(CLR_BRIGHT_CYAN);
  client.println(F("  Recent Activity:"));
  client.print(CLR_RESET);
  client.println(F("  ───────────────────────────────────────────────────────────"));
  client.println();
  
  client.print(F("  ["));
  printUptime((millis() - sysStats.bootTime) / 1000);
  client.print(F("] System started"));
  client.println();
  
  client.print(F("  ["));
  printUptime((millis() - sysStats.bootTime) / 1000);
  client.print(F("] User '"));
  client.print(session.username);
  client.println(F("' logged in"));
  
  client.print(F("  ["));
  printUptime((millis() - sysStats.bootTime) / 1000);
  client.print(F("] "));
  client.print(sysStats.commandsExecuted);
  client.println(F(" commands executed"));
  
  client.println();
  client.println(F("Press Enter to continue..."));
  session.waitingForContinue = true;
  session.returnToMenu = MENU_UTILITIES;  // Return to utilities menu
}

void showMessageMenu() {
  client.print(F("\033[2J\033[H"));
  session.currentMenu = MENU_MESSAGES;
  
  drawBox(CLR_MAGENTA, "MESSAGE BOARD");
  
  client.println();
  client.print(CLR_BRIGHT_CYAN);
  client.println(F("  ┌─────────────────────────────────────────────────────────┐"));
  client.println(F("  │                                                         │"));
  client.println(F("  │  [1] Read Messages    - View all messages               │"));
  client.println(F("  │  [2] Post Message     - Write a new message             │"));
  client.println(F("  │  [3] Message Count    - Show total messages             │"));
  client.println(F("  │                                                         │"));
  client.println(F("  │  [0] Back to Main Menu                                  │"));
  client.println(F("  │                                                         │"));
  client.println(F("  └─────────────────────────────────────────────────────────┘"));
  client.print(CLR_RESET);
  client.println();
  
  showPrompt();
}

void handleMessageMenuChoice(int choice) {
  switch (choice) {
    case 1:
      readMessages();
      break;
    case 2:
      postMessage();
      break;
    case 3:
      showMessageCount();
      break;
    case 0:
      showMainMenu();
      break;
    default:
      client.println(F("Invalid choice."));
      showPrompt();
  }
}

void showMessageCount() {
  File root = SD.open("messages");
  int count = 0;
  
  if (root) {
    while (true) {
      File entry = root.openNextFile();
      if (!entry) break;
      if (!entry.isDirectory()) count++;
      entry.close();
    }
    root.close();
  }
  
  client.println();
  client.print(CLR_BRIGHT_CYAN);
  client.print(F("Total messages: "));
  client.println(count);
  client.print(CLR_RESET);
  client.println();
  client.println(F("Press Enter to continue..."));
  session.waitingForContinue = true;
  session.returnToMenu = MENU_MESSAGES;  // Return to messages menu
}

// STEP 8: Update readMessages() function
void readMessages() {
  client.print(F("\033[2J\033[H"));
  drawBox(CLR_MAGENTA, "MESSAGE BOARD");
  client.println();
  
  File root = SD.open("messages");
  if (!root) {
    client.println(F("  No messages yet!"));
    client.println();
    client.println(F("Press Enter to continue..."));
    session.waitingForContinue = true;
    session.returnToMenu = MENU_MESSAGES;  // Return to messages menu
    return;
  }
  
  int count = 0;
  while (true) {
    File entry = root.openNextFile();
    if (!entry) break;
    
    if (!entry.isDirectory()) {
      client.print(CLR_BRIGHT_YELLOW);
      client.print(F("  ┌─[ Message #"));
      client.print(count++);
      client.println(F(" ]─────────────────────────────"));
      client.print(CLR_RESET);
      
      while (entry.available()) {
        client.print(F("  │ "));
        while (entry.available()) {
          char c = entry.read();
          if (c == '\n') {
            client.println();
            break;
          } else if (c != '\r') {
            client.write(c);
          }
        }
      }
      
      client.print(CLR_BRIGHT_YELLOW);
      client.println(F("  └────────────────────────────────────────────"));
      client.print(CLR_RESET);
      client.println();
    }
    entry.close();
  }
  
  root.close();
  
  if (count == 0) {
    client.println(F("  No messages yet."));
  }
  
  client.println();
  client.println(F("Press Enter to continue..."));
  session.waitingForContinue = true;
  session.returnToMenu = MENU_MESSAGES;  // Return to messages menu
}

void postMessage() {
  client.println();
  client.print(CLR_BRIGHT_GREEN);
  client.println(F("Enter your message (max 200 chars):"));
  client.print(CLR_RESET);
  client.print(F("> "));
  
  char message[201];
  int idx = 0;
  unsigned long startTime = millis();
  
  while (millis() - startTime < 60000) {
    if (client.available()) {
      char c = client.read();
      if (c == '\n' || c == '\r') {
        message[idx] = '\0';
        break;
      } else if (c == 8 || c == 127) {
        if (idx > 0) {
          idx--;
          client.write(8);
          client.write(' ');
          client.write(8);
        }
      } else if (idx < 200 && c >= 32) {
        message[idx++] = c;
        client.write(c);
      }
    }
  }
  
  if (idx == 0) {
    client.println();
    client.println(F("Cancelled."));
    delay(1000);
    showMessageMenu();
    return;
  }
  
  int msgNum = 0;
  char filename[32];
  
  while (msgNum < 1000) {
    sprintf(filename, "messages/msg%03d.txt", msgNum);
    if (!SD.exists(filename)) break;
    msgNum++;
  }
  
  File f = SD.open(filename, FILE_WRITE);
  if (f) {
    f.print("From: ");
    f.println(session.username);
    f.print("Time: ");
    printUptimeToFile(f, (millis() - sysStats.bootTime) / 1000);
    f.println();
    f.println("---");
    f.println(message);
    f.close();
    sysStats.messagesPosted++;
    
    client.println();
    client.print(CLR_BRIGHT_GREEN);
    client.println(F("✓ Message posted!"));
    client.print(CLR_RESET);
  } else {
    client.println();
    client.print(CLR_RED);
    client.println(F("✗ Error posting message."));
    client.print(CLR_RESET);
  }
  
  delay(2000);
  showMessageMenu();
}

void printUptimeToFile(File &f, unsigned long seconds) {
  unsigned long days = seconds / 86400;
  unsigned long hours = (seconds % 86400) / 3600;
  unsigned long mins = (seconds % 3600) / 60;
  
  if (days > 0) {
    f.print(days);
    f.print("d ");
  }
  f.print(hours);
  f.print("h ");
  f.print(mins);
  f.print("m");
}

void showFileMenu() {
  client.print(F("\033[2J\033[H"));
  session.currentMenu = MENU_FILES;
  
  drawBox(CLR_BLUE, "FILE MANAGER");
  
  client.println();
  client.print(CLR_BRIGHT_CYAN);
  client.println(F("  ┌─────────────────────────────────────────────────────────┐"));
  client.println(F("  │                                                         │"));
  client.println(F("  │  [1] List All Files   - Browse all directories          │"));
  client.println(F("  │  [2] View Note        - Read a saved note               │"));
  client.println(F("  │  [3] Delete File      - Remove a file (admin)           │"));
  client.println(F("  │                                                         │"));
  client.println(F("  │  [0] Back to Main Menu                                  │"));
  client.println(F("  │                                                         │"));
  client.println(F("  └─────────────────────────────────────────────────────────┘"));
  client.print(CLR_RESET);
  client.println();
  
  showPrompt();
}

void handleFileMenuChoice(int choice) {
  switch (choice) {
    case 1:
      listFiles();
      break;
    case 2:
      viewNote();
      break;
    case 3:
      if (session.isAdmin) {
        client.println(F("Delete feature coming soon!"));
        delay(2000);
        showFileMenu();
      } else {
        client.println(F("Permission denied."));
        delay(2000);
        showFileMenu();
      }
      break;
    case 0:
      showMainMenu();
      break;
    default:
      client.println(F("Invalid choice."));
      showPrompt();
  }
}

// STEP 9: Update viewNote() function
void viewNote() {
  client.println();
  client.println(F("Available notes:"));
  
  File root = SD.open("notes");
  if (root) {
    int count = 0;
    while (true) {
      File entry = root.openNextFile();
      if (!entry) break;
      if (!entry.isDirectory()) {
        client.print(F("  - "));
        client.println(entry.name());
        count++;
      }
      entry.close();
    }
    root.close();
    
    if (count == 0) {
      client.println(F("  (no notes found)"));
      delay(2000);
      showFileMenu();
      return;
    }
  }
  
  client.println();
  client.print(F("Enter filename to view: "));
  
  char filename[32];
  int idx = 0;
  unsigned long startTime = millis();
  
  while (millis() - startTime < 30000) {
    if (client.available()) {
      char c = client.read();
      if (c == '\n' || c == '\r') {
        filename[idx] = '\0';
        break;
      } else if (c == 8 || c == 127) {
        if (idx > 0) {
          idx--;
          client.write(8);
          client.write(' ');
          client.write(8);
        }
      } else if (idx < 31 && c >= 32) {
        filename[idx++] = c;
        client.write(c);
      }
    }
  }
  
  if (idx == 0) {
    client.println();
    showFileMenu();
    return;
  }
  
  client.println();
  client.println();
  
  char fullPath[40];
  sprintf(fullPath, "notes/%s", filename);
  
  File f = SD.open(fullPath, FILE_READ);
  if (!f) {
    client.println(F("File not found!"));
  } else {
    client.println(F("─── File Content ───"));
    while (f.available()) {
      char c = f.read();
      if (c != '\r') {
        client.write(c);
      }
    }
    f.close();
    client.println();
    client.println(F("────────────────────"));
  }
  
  client.println();
  client.println(F("Press Enter to continue..."));
  session.waitingForContinue = true;
  session.returnToMenu = MENU_FILES;  // Return to files menu
}

// STEP 10: Update listFiles() function
void listFiles() {
  client.print(F("\033[2J\033[H"));
  drawBox(CLR_BLUE, "FILE LISTING");
  client.println();
  
  const char* dirs[] = {"notes", "messages", "logs", "news"};
  
  for (int i = 0; i < 4; i++) {
    client.print(CLR_BRIGHT_YELLOW);
    client.print(F("  ["));
    client.print(dirs[i]);
    client.println(F("]"));
    client.print(CLR_RESET);
    
    File root = SD.open(dirs[i]);
    if (root) {
      int count = 0;
      while (true) {
        File entry = root.openNextFile();
        if (!entry) break;
        
        if (!entry.isDirectory()) {
          client.print(F("    "));
          client.print(entry.name());
          client.print(F(" ("));
          client.print(entry.size());
          client.println(F(" bytes)"));
          count++;
        }
        entry.close();
      }
      if (count == 0) {
        client.println(F("    (empty)"));
      }
      root.close();
    }
    client.println();
  }
  
  client.println(F("Press Enter to continue..."));
  session.waitingForContinue = true;
  session.returnToMenu = MENU_FILES;  // Return to files menu
}

void showStatsMenu() {
  client.print(F("\033[2J\033[H"));
  session.currentMenu = MENU_STATS;
  
  drawBox(CLR_MAGENTA, "SYSTEM STATISTICS");
  
  client.println();
  client.print(CLR_BRIGHT_CYAN);
  client.println(F("  ┌─[ SYSTEM INFORMATION ]────────────────────────────────────┐"));
  client.print(CLR_RESET);
  
  client.print(F("  │ Platform:        "));
  client.println(F("Arduino Mega 2560                        │"));
  
  client.print(F("  │ OS Version:      "));
  client.println(F("3.0 Enhanced                              │"));
  
  client.print(F("  │ IP Address:      "));
  client.print(Ethernet.localIP());
  int ipLen = String(Ethernet.localIP()).length();
  for (int i = 0; i < 40 - ipLen; i++) client.print(' ');
  client.println(F("│"));
  
  client.print(CLR_BRIGHT_CYAN);
  client.println(F("  ├─[ PERFORMANCE ]──────────────────────────────────────────┤"));
  client.print(CLR_RESET);
  
  unsigned long uptime = (millis() - sysStats.bootTime) / 1000;
  client.print(F("  │ Uptime:          "));
  printUptimeFormatted(uptime);
  client.println(F("│"));
  
  client.print(F("  │ Free RAM:        "));
  int ram = freeRam();
  client.print(ram);
  client.print(F(" bytes"));
  int ramLen = String(ram).length();
  for (int i = 0; i < 33 - ramLen; i++) client.print(' ');
  client.println(F("│"));
  
  client.print(CLR_BRIGHT_CYAN);
  client.println(F("  ├─[ STATISTICS ]───────────────────────────────────────────┤"));
  client.print(CLR_RESET);
  
  client.print(F("  │ Connections:     "));
  client.print(sysStats.totalConnections);
  int connLen = String(sysStats.totalConnections).length();
  for (int i = 0; i < 41 - connLen; i++) client.print(' ');
  client.println(F("│"));
  
  client.print(F("  │ Messages Posted: "));
  client.print(sysStats.messagesPosted);
  int msgLen = String(sysStats.messagesPosted).length();
  for (int i = 0; i < 41 - msgLen; i++) client.print(' ');
  client.println(F("│"));
  
  client.print(F("  │ Files Created:   "));
  client.print(sysStats.filesCreated);
  int fileLen = String(sysStats.filesCreated).length();
  for (int i = 0; i < 41 - fileLen; i++) client.print(' ');
  client.println(F("│"));
  
  client.print(F("  │ Commands Run:    "));
  client.print(sysStats.commandsExecuted);
  int cmdLen = String(sysStats.commandsExecuted).length();
  for (int i = 0; i < 41 - cmdLen; i++) client.print(' ');
  client.println(F("│"));
  
  client.print(CLR_BRIGHT_CYAN);
  client.println(F("  └───────────────────────────────────────────────────────────┘"));
  client.print(CLR_RESET);
  
  client.println();
  client.println(F("  [0] Back to Main Menu"));
  client.println();
  
  showPrompt();
}

void printUptimeFormatted(unsigned long seconds) {
  unsigned long days = seconds / 86400;
  unsigned long hours = (seconds % 86400) / 3600;
  unsigned long mins = (seconds % 3600) / 60;
  unsigned long secs = seconds % 60;
  
  char buffer[30];
  sprintf(buffer, "%lud %02luh %02lum %02lus", days, hours, mins, secs);
  client.print(buffer);
  
  int len = strlen(buffer);
  for (int i = 0; i < 37 - len; i++) client.print(' ');
}

void showSettingsMenu() {
  client.print(F("\033[2J\033[H"));
  session.currentMenu = MENU_SETTINGS;
  
  drawBox(CLR_YELLOW, "SETTINGS");
  
  client.println();
  client.print(CLR_BRIGHT_CYAN);
  client.println(F("  ┌─────────────────────────────────────────────────────────┐"));
  client.println(F("  │                                                         │"));
  client.println(F("  │  [1] User Management  - View/add users (admin)          │"));
  client.println(F("  │  [2] Change Password  - Update your password            │"));
  client.println(F("  │  [3] About System     - System information              │"));
  client.println(F("  │                                                         │"));
  client.println(F("  │  [0] Back             - Return to previous menu         │"));
  client.println(F("  │                                                         │"));
  client.println(F("  └─────────────────────────────────────────────────────────┘"));
  client.print(CLR_RESET);
  client.println();
  
  showPrompt();
}

void handleSettingsChoice(int choice) {
  switch (choice) {
    case 1:
      if (session.isAdmin) {
        showUserManagement();
      } else {
        client.println(F("Permission denied."));
        delay(2000);
        showSettingsMenu();
      }
      break;
    case 2:
      client.println(F("Password change coming soon!"));
      delay(2000);
      showSettingsMenu();
      break;
    case 3:
      showAbout();
      break;
    case 0:
      showUtilitiesMenu();
      break;
    default:
      client.println(F("Invalid choice."));
      showPrompt();
  }
}

// STEP 11: Update showAbout() function
void showAbout() {
  client.print(F("\033[2J\033[H"));
  drawBox(CLR_CYAN, "ABOUT THIS SYSTEM");
  
  client.println();
  client.println(F("  Arduino Mega Terminal OS v3.0"));
  client.println(F("  Enhanced BBS-Style Multi-User System"));
  client.println();
  client.println(F("  Platform: Arduino Mega 2560"));
  client.println(F("  Network:  Official Arduino Ethernet Shield"));
  client.println(F("  Storage:  SD Card"));
  client.println();
  client.println(F("  Features:"));
  client.println(F("  • DHCP/Static IP support"));
  client.println(F("  • Multi-user authentication"));
  client.println(F("  • Text editor with save/load"));
  client.println(F("  • Calculator with memory"));
  client.println(F("  • Message board system"));
  client.println(F("  • Weather & stock info"));
  client.println(F("  • Interactive games"));
  client.println(F("  • File management"));
  client.println(F("  • Real-time statistics"));
  client.println();
  client.println(F("Press Enter to continue..."));
  session.waitingForContinue = true;
  session.returnToMenu = MENU_SETTINGS;  // Return to settings menu
}

// STEP 12: Update showUserManagement() function
void showUserManagement() {
  client.print(F("\033[2J\033[H"));
  drawBox(CLR_RED, "USER MANAGEMENT");
  
  client.println();
  client.println(F("  Registered Users:"));
  client.println(F("  ───────────────────────────────────────────────────────────"));
  
  File f = SD.open("users.txt", FILE_READ);
  if (f) {
    char line[64];
    int idx = 0;
    
    while (f.available()) {
      char c = f.read();
      if (c == '\n' || c == '\r') {
        if (idx > 0) {
          line[idx] = '\0';
          char* user = strtok(line, ":");
          strtok(NULL, ":");
          char* admin = strtok(NULL, ":");
          
          client.print(F("    • "));
          client.print(user);
          if (admin && atoi(admin) == 1) {
            client.print(CLR_BRIGHT_YELLOW);
            client.print(F(" [ADMIN]"));
            client.print(CLR_RESET);
          }
          client.println();
          idx = 0;
        }
      } else if (idx < 63) {
        line[idx++] = c;
      }
    }
    f.close();
  }
  
  client.println();
  client.println(F("Press Enter to continue..."));
  session.waitingForContinue = true;
  session.returnToMenu = MENU_SETTINGS;  // Return to settings menu
}

// STEP 13: Update showNetworkInfo() function
void showNetworkInfo() {
  client.print(F("\033[2J\033[H"));
  drawBox(CLR_BLUE, "NETWORK INFORMATION");
  
  client.println();
  client.print(F("  IP Address:      "));
  client.println(Ethernet.localIP());
  
  client.print(F("  Subnet Mask:     "));
  client.println(Ethernet.subnetMask());
  
  client.print(F("  Gateway:         "));
  client.println(Ethernet.gatewayIP());
  
  client.print(F("  DNS Server:      "));
  client.println(Ethernet.dnsServerIP());
  
  client.print(F("  MAC Address:     "));
  for (int i = 0; i < 6; i++) {
    if (mac[i] < 16) client.print('0');
    client.print(mac[i], HEX);
    if (i < 5) client.print(':');
  }
  client.println();
  
  client.println();
  client.println(F("Press Enter to continue..."));
  session.waitingForContinue = true;
  session.returnToMenu = MENU_UTILITIES;  // Return to utilities menu
}

void showNewsReader() {
  client.print(F("\033[2J\033[H"));
  session.currentMenu = MENU_NEWS;
  
  drawBox(CLR_CYAN, "NEWS READER");
  
  client.println();
  
  File f = SD.open("news/news.txt", FILE_READ);
  if (f) {
    while (f.available()) {
      client.print(F("  "));
      char c = f.read();
      if (c == '\n') {
        client.println();
      } else if (c != '\r') {
        client.write(c);
      }
    }
    f.close();
  } else {
    client.println(F("  No news available."));
  }
  
  client.println();
  client.println();
  client.println(F("  [0] Back to Main Menu"));
  client.println();
  
  showPrompt();
}

void startCommandMode() {
  client.print(F("\033[2J\033[H"));
  session.currentMenu = MENU_COMMAND;
  
  drawBox(CLR_RED, "COMMAND MODE");
  
  client.println();
  client.print(CLR_YELLOW);
  client.println(F("  Advanced command-line interface"));
  client.println(F("  Type 'help' for commands or 'exit' to return"));
  client.print(CLR_RESET);
  client.println();
  
  showCommandPrompt();
}

void handleCommandInput() {
  if (strcmp(cmdBuffer, "exit") == 0) {
    showUtilitiesMenu();
    return;
  }
  
  char* cmd = strtok(cmdBuffer, " ");
  if (!cmd) {
    showCommandPrompt();
    return;
  }
  
  for (int i = 0; cmd[i]; i++) {
    cmd[i] = tolower(cmd[i]);
  }
  
  if (strcmp(cmd, "help") == 0) {
    client.println(F("Commands:"));
    client.println(F("  ls [path]   - List files"));
    client.println(F("  cat <file>  - Display file"));
    client.println(F("  echo <text> - Echo text"));
    client.println(F("  clear       - Clear screen"));
    client.println(F("  whoami      - Show username"));
    client.println(F("  uptime      - System uptime"));
    client.println(F("  free        - Show free RAM"));
    client.println(F("  date        - Show time"));
    client.println(F("  exit        - Return to menu"));
  } else if (strcmp(cmd, "clear") == 0) {
    client.print(F("\033[2J\033[H"));
  } else if (strcmp(cmd, "whoami") == 0) {
    client.println(session.username);
  } else if (strcmp(cmd, "uptime") == 0) {
    printUptime((millis() - sysStats.bootTime) / 1000);
    client.println();
  } else if (strcmp(cmd, "date") == 0) {
    client.print(F("System uptime: "));
    printUptime((millis() - sysStats.bootTime) / 1000);
    client.println();
  } else if (strcmp(cmd, "free") == 0) {
    client.print(F("Free RAM: "));
    client.print(freeRam());
    client.println(F(" bytes"));
  } else if (strcmp(cmd, "ls") == 0) {
    listFilesCommand();
  } else if (strcmp(cmd, "cat") == 0) {
    catCommand();
  } else if (strcmp(cmd, "echo") == 0) {
    char* text = strtok(NULL, "");
    if (text) client.println(text);
  } else {
    client.print(F("Unknown command: "));
    client.println(cmd);
  }
  
  client.println();
  showCommandPrompt();
}

void listFilesCommand() {
  char* path = strtok(NULL, " ");
  if (!path) path = "/";
  
  File root = SD.open(path);
  if (!root) {
    client.println(F("Cannot open directory"));
    return;
  }
  
  while (true) {
    File entry = root.openNextFile();
    if (!entry) break;
    
    if (entry.isDirectory()) {
      client.print(F("[DIR]  "));
    } else {
      client.print(F("[FILE] "));
    }
    client.print(entry.name());
    if (!entry.isDirectory()) {
      client.print(F("  ("));
      client.print(entry.size());
      client.print(F(" bytes)"));
    }
    client.println();
    entry.close();
  }
  root.close();
}

void catCommand() {
  char* filename = strtok(NULL, " ");
  if (!filename) {
    client.println(F("Usage: cat <filename>"));
    return;
  }
  
  File f = SD.open(filename, FILE_READ);
  if (!f) {
    client.print(F("Cannot open: "));
    client.println(filename);
    return;
  }
  
  while (f.available()) {
    char c = f.read();
    if (c != '\r') {
      client.write(c);
    }
  }
  client.println();
  f.close();
}

void showCommandPrompt() {
  client.print(CLR_BRIGHT_GREEN);
  client.print(session.username);
  client.print(CLR_CYAN);
  client.print(F("@ArduinoOS"));
  client.print(CLR_RESET);
  client.print(F(" $ "));
}

void showPrompt() {
  client.print(CLR_BRIGHT_GREEN);
  client.print(F("Choice"));
  client.print(CLR_RESET);
  client.print(F(" > "));
}

void drawBox(const char* color, const char* title) {
  client.print(color);
  client.print(F("  ╔"));
  for (int i = 0; i < 59; i++) client.print(F("═"));
  client.println(F("╗"));
  
  client.print(F("  ║ "));
  client.print(CLR_BRIGHT_WHITE);
  client.print(title);
  client.print(color);
  for (int i = 0; i < 57 - strlen(title); i++) client.print(F(" "));
  client.println(F("║"));
  
  client.print(F("  ╚"));
  for (int i = 0; i < 59; i++) client.print(F("═"));
  client.println(F("╝"));
  client.print(CLR_RESET);
}

int freeRam() {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

void logout() {
  client.println();
  client.print(CLR_BRIGHT_GREEN);
  client.println(F("  ╔═════════════════════════════════════════════════╗"));
  client.println(F("  ║    ╔═╗╦═╗╔╦╗╦ ╦╦╔╗╔╔═╗  ╔╦╗╔═╗╔═╗╔═╗  ╔═╗╔═╗    ║"));
  client.println(F("  ║    ╠═╣╠╦╝ ║║║ ║║║║║║ ║  ║║║║╣ ║ ╦╠═╣  ║ ║╚═╗    ║"));
  client.println(F("  ║    ╩ ╩╩╚══╩╝╚═╝╩╝╚╝╚═╝  ╩ ╩╚═╝╚═╝╩ ╩  ╚═╝╚═╝    ║"));
  client.println(F("  ║                   Logging out...                ║"));
  client.println(F("  ║      Thank you for using Arduino MEGA OS!       ║"));
  client.println(F("  ║         https://github.com/dialtone404/         ║"));
  client.println(F("  ╚═════════════════════════════════════════════════╝"));
  client.print(CLR_RESET);
  delay(2000);
  client.stop();
  session.loggedIn = false;
}