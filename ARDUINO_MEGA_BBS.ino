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
 * - Home Assistant Support
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
  MENU_UTILITIES,
  MENU_HOME_ASSISTANT,
  MENU_HA_SETUP,
  MENU_HA_MANAGE_LIGHTS,
  MENU_HA_MANAGE_SENSORS,
  MENU_SECURE_MAIL,
  MENU_MAIL_COMPOSE,
  MENU_THEME_SELECTOR      // ADD THIS LINE
};
// Theme definitions
enum ThemeID {
  THEME_DEFAULT,      // Original green/cyan theme
  THEME_DOS,          // Classic DOS green on black
  THEME_AMBER,        // Amber monitor
  THEME_IBM,          // IBM blue screen
  THEME_C64,          // Commodore 64
  THEME_MATRIX,       // Matrix/Hacker green
  THEME_WINDOWS       // Windows 95 gray/blue
};

struct Theme {
  const char* name;
  const char* primary;      // Main text color
  const char* secondary;    // Secondary text color
  const char* accent;       // Accent/highlight color
  const char* success;      // Success messages
  const char* error;        // Error messages
  const char* warning;      // Warning messages
  const char* prompt;       // Prompt color
  const char* title;        // Title/header color
};

// Theme definitions
const Theme themes[] = {
  // THEME_DEFAULT - Original colorful theme
  {
    "Default (Colorful)",
    "\033[97m",   // primary: bright white
    "\033[96m",   // secondary: bright cyan
    "\033[93m",   // accent: bright yellow
    "\033[92m",   // success: bright green
    "\033[91m",   // error: bright red
    "\033[93m",   // warning: bright yellow
    "\033[92m",   // prompt: bright green
    "\033[97m"    // title: bright white
  },
  // THEME_DOS - Classic DOS green
  {
    "Classic DOS",
    "\033[92m",   // primary: bright green
    "\033[32m",   // secondary: green
    "\033[97m",   // accent: bright white
    "\033[92m",   // success: bright green
    "\033[91m",   // error: bright red
    "\033[93m",   // warning: bright yellow
    "\033[92m",   // prompt: bright green
    "\033[97m"    // title: bright white
  },
  // THEME_AMBER - Amber monitor
  {
    "Amber Monitor",
    "\033[33m",   // primary: yellow (amber)
    "\033[93m",   // secondary: bright yellow
    "\033[97m",   // accent: bright white
    "\033[93m",   // success: bright yellow
    "\033[91m",   // error: bright red
    "\033[33m",   // warning: yellow
    "\033[93m",   // prompt: bright yellow
    "\033[97m"    // title: bright white
  },
  // THEME_IBM - IBM Blue Screen
  {
    "IBM Blue Screen",
    "\033[97m",   // primary: bright white
    "\033[96m",   // secondary: bright cyan
    "\033[93m",   // accent: bright yellow
    "\033[97m",   // success: bright white
    "\033[91m",   // error: bright red
    "\033[93m",   // warning: bright yellow
    "\033[96m",   // prompt: bright cyan
    "\033[97m"    // title: bright white
  },
  // THEME_C64 - Commodore 64
  {
    "Commodore 64",
    "\033[96m",   // primary: bright cyan (light blue)
    "\033[36m",   // secondary: cyan
    "\033[95m",   // accent: bright magenta (purple)
    "\033[96m",   // success: bright cyan
    "\033[91m",   // error: bright red
    "\033[95m",   // warning: bright magenta
    "\033[96m",   // prompt: bright cyan
    "\033[97m"    // title: bright white
  },
  // THEME_MATRIX - Matrix/Hacker
  {
    "Matrix/Hacker",
    "\033[92m",   // primary: bright green
    "\033[32m",   // secondary: green
    "\033[97m",   // accent: bright white
    "\033[92m",   // success: bright green
    "\033[91m",   // error: bright red
    "\033[92m",   // warning: bright green
    "\033[92m",   // prompt: bright green
    "\033[92m"    // title: bright green
  },
  // THEME_WINDOWS - Windows 95
  {
    "Windows 95",
    "\033[37m",   // primary: white (gray)
    "\033[97m",   // secondary: bright white
    "\033[34m",   // accent: blue
    "\033[92m",   // success: bright green
    "\033[91m",   // error: bright red
    "\033[93m",   // warning: bright yellow
    "\033[34m",   // prompt: blue
    "\033[97m"    // title: bright white
  }
};

// Current theme (default to 0)
int currentTheme = 0;

// Theme-aware color getters - these return colors based on current theme
#define GET_PRIMARY()   themes[currentTheme].primary
#define GET_SECONDARY() themes[currentTheme].secondary
#define GET_ACCENT()    themes[currentTheme].accent
#define GET_SUCCESS()   themes[currentTheme].success
#define GET_ERROR()     themes[currentTheme].error
#define GET_WARNING()   themes[currentTheme].warning
#define GET_PROMPT()    themes[currentTheme].prompt
#define GET_TITLE()     themes[currentTheme].title

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
  bool waitingForContinue;
  MenuState returnToMenu;
  uint32_t userEncryptionKey;  // ADD THIS
} session;

// Home Assistant Configuration
struct HAConfig {
  char server[40];
  int port;
  char token[256];
  bool configured;
} haConfig;

struct HALight {
  char entityId[50];
  char displayName[30];
};

struct HASensor {
  char entityId[50];
  char displayName[30];
  char unit[10];  // Unit of measurement (°F, %, hPa, etc.)
};

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

// Function declarations
void showMainMenu();
void showCurrentMenu();
void showPrompt();
void drawBox(const char* color, const char* title);
int freeRam();
void printUptime(unsigned long seconds);
void printUptimeFormatted(unsigned long seconds);
void printUptimeToFile(File &f, unsigned long seconds);
// Secure Mail functions
void showSecureMailMenu();
void handleSecureMailChoice(int choice);
void showInbox();
void composeMail();
void handleComposeMailInput();
void showSentMail();
void viewMessage();
void deleteMessage();
uint32_t generateHardwareKey();
void encryptMessage(char* message, uint32_t key);
void decryptMessage(char* message, uint32_t key);
int countUnreadMail();
void markMessageRead(const char* filename);
bool isMessageRead(const char* filename);
void regenerateEncryptionKey();
void changePassword();
void changeUsername(); // optional
void showThemeMenu();
void handleThemeChoice(int choice);
void applyTheme(int themeId);
void loadTheme();
void saveTheme();
void previewTheme(int themeId);

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
  SD.mkdir("mail");      // ADD THIS
  
  // Initialize default user
  if (!SD.exists("users.txt")) {
    File f = SD.open("users.txt", FILE_WRITE);
    if (f) {
      f.println("admin:admin123:1");
      f.close();
      Serial.println(F("Created default admin user"));
    }
  }
  // Create HA directory
  SD.mkdir("ha");
  
  // Load HA configuration
  loadHAConfig();
  
  loadTheme();

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
  // Initialize session
  session.loggedIn = false;
  session.isAdmin = false;
  session.currentMenu = MENU_MAIN;
  session.editorLines = 0;
  session.calcMemory = 0;
  session.waitingForContinue = false;
  session.returnToMenu = MENU_MAIN;
  session.userEncryptionKey = 0;  // ADD THIS
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
              case MENU_HOME_ASSISTANT:
                showHomeAssistantMenu();
                break;
              case MENU_SECURE_MAIL:
                showSecureMailMenu();
                break;
              default:
                showCurrentMenu();
            }
          } else {
            // Just re-show the prompt, don't process as a choice
           
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
  client.print(F("\033[3J\033[2J\033[H"));
  client.print(GET_SUCCESS());
  client.println(F("   ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣀⣄⣠⣀⡀⣀⣠⣤⣤⣤⣀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀        "));
  client.println(F("   ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣄⢠⣠⣼⣿⣿⣿⣟⣿⣿⣿⣿⣿⣿⣿⣿⡿⠋⠀⠀⠀⢠⣤⣦⡄⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠰⢦⣄⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀      "));
  client.println(F("   ⠀⠀⠀⠀⠀⠀⠀⠀⣼⣿⣟⣾⣿⣽⣿⣿⣅⠈⠉⠻⣿⣿⣿⣿⣿⡿⠇⠀⠀⠀⠀⠀⠉⠀⠀⠀⠀⠀⢀⡶⠒⢉⡀⢠⣤⣶⣶⣿⣷⣆⣀⡀⠀⢲⣖⠒⠀⠀⠀⠀⠀⠀⠀    "));
  client.println(F("   ⢀⣤⣾⣶⣦⣤⣤⣶⣿⣿⣿⣿⣿⣿⣽⡿⠻⣷⣀⠀⢻⣿⣿⣿⡿⠟⠀⠀⠀⠀⠀⠀⣤⣶⣶⣤⣀⣀⣬⣷⣦⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣶⣦⣤⣦⣼⣀⠀ "));
  client.println(F("   ⠈⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⠛⠓⣿⣿⠟⠁⠘⣿⡟⠁⠀⠘⠛⠁⠀⠀⢠⣾⣿⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⠏⠙⠁"));
  client.println(F("   ⠀⠸⠟⠋⠀⠈⠙⣿⣿⣿⣿⣿⣿⣷⣦⡄⣿⣿⣿⣆⠀⠀⠀⠀⠀⠀⠀⠀⣼⣆⢘⣿⣯⣼⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡉⠉⢱⡿⠀⠀⠀⠀⠀  "));
  client.println(F("   ⠀⠀⠀⠀⠀⠀⠀⠘⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣟⡿⠦⠀⠀⠀⠀⠀⠀⠀⠙⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⡗⠀⠈⠀⠀⠀⠀⠀⠀   "));
  client.println(F("   ⠀⠀⠀⠀⠀⠀⠀⠀⢻⣿⣿⣿⣿⣿⣿⣿⣿⠋⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⢿⣿⣉⣿⡿⢿⢷⣾⣾⣿⣞⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠋⣠⠟⠀⠀⠀⠀⠀⠀⠀⠀    "));
  client.println(F("   ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠹⣿⣿⣿⠿⠿⣿⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣀⣾⣿⣿⣷⣦⣶⣦⣼⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⠈⠛⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀    "));
  client.println(F("   ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠉⠻⣿⣤⡖⠛⠶⠤⡀⠀⠀⠀⠀⠀⠀⠀⢰⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⠁⠙⣿⣿⠿⢻⣿⣿⡿⠋⢩⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀    "));
  client.println(F("   ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠙⠧⣤⣦⣤⣄⡀⠀⠀⠀⠀⠀⠘⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡇⠀⠀⠀⠘⣧⠀⠈⣹⡻⠇⢀⣿⡆⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀     "));
  client.println(F("   ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢠⣿⣿⣿⣿⣿⣤⣀⡀⠀⠀⠀⠀⠀⠀⠈⢽⣿⣿⣿⣿⣿⠋⠀⠀⠀⠀⠀⠀⠀⠀⠹⣷⣴⣿⣷⢲⣦⣤⡀⢀⡀⠀⠀⠀⠀⠀⠀     "));
  client.println(F("   ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⢿⣿⣿⣿⣿⣿⣿⠟⠀⠀⠀⠀⠀⠀⠀⢸⣿⣿⣿⣿⣷⢀⡄⠀⠀⠀⠀⠀⠀⠀⠀⠈⠉⠂⠛⣆⣤⡜⣟⠋⠙⠂⠀⠀⠀⠀⠀     "));
  client.println(F("   ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢹⣿⣿⣿⣿⠟⠀⠀⠀⠀⠀⠀⠀⠀⠘⣿⣿⣿⣿⠉⣿⠃⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣤⣾⣿⣿⣿⣿⣆⠀⠰⠄⠀⠉⠀⠀      "));
  client.println(F("   ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣸⣿⣿⡿⠃⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢹⣿⡿⠃⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢻⣿⠿⠿⣿⣿⣿⠇⠀⠀⢀⠀⠀⠀       "));
  client.println(F("   ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣿⡿⠛⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⢻⡇⠀⠀⢀⣼⠗⠀⠀        "));
  client.println(F("   ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⣿⠃⣀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠙⠁⠀⠀⠀         "));
  client.println(F("   ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠙⠒⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀           "));
  client.println(F("                                                                                        "));
  client.println(F("╔════════════════════════════════════════════════════════════════════════╗"));
  client.println(F("║                                                                        ║"));
  client.println(F("║              ╔═╗╦═╗╔╦╗╦ ╦╦╔╗╔╔═╗  ╔╦╗╔═╗╔═╗╔═╗  ╔═╗╔═╗                 ║"));
  client.println(F("║              ╠═╣╠╦╝ ║║║ ║║║║║║ ║  ║║║║╣ ║ ╦╠═╣  ║ ║╚═╗                 ║"));
  client.println(F("║              ╩ ╩╩╚══╩╝╚═╝╩╝╚╝╚═╝  ╩ ╩╚═╝╚═╝╩ ╩  ╚═╝╚═╝                 ║"));
  client.println(F("║                                                                        ║"));
  client.println(F("║                    Terminal Operating System v3.0                      ║"));
  client.println(F("║                         Authorized Users Only                          ║"));
  client.println(F("║                    https://github.com/dialtone404/                     ║"));
  client.println(F("╚════════════════════════════════════════════════════════════════════════╝"));
  client.print(CLR_RESET);
  client.println();
  
  client.print(GET_WARNING());
  client.print(F("System IP: "));
  client.print(GET_PRIMARY());
  client.print(Ethernet.localIP());
  client.print(GET_WARNING());
  client.print(F("  |  Uptime: "));
  client.print(GET_PRIMARY());
  printUptime((millis() - sysStats.bootTime) / 1000);
  client.print(GET_WARNING());
  client.print(F("  |  Connections: "));
  client.print(GET_PRIMARY());
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
  client.print(GET_SUCCESS());
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
    client.print(GET_SUCCESS());
    client.print(F("Password: "));
    client.print(CLR_RESET);
  } else {
    if (verifyLogin(tempUser, cmdBuffer)) {
      session.loggedIn = true;
      strncpy(session.username, tempUser, MAX_USERNAME - 1);
      client.println();
      client.print(GET_SUCCESS());
      client.print(F("\r\n✓ Welcome, "));
      client.print(session.username);
      client.println(F("!"));
      client.print(CLR_RESET);
      delay(1000);
      showMainMenu();
    } else {
      client.println();
      client.print(GET_ERROR());
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
          
          // ADD THIS BLOCK:
          // Generate hardware encryption key for this user
          session.userEncryptionKey = generateHardwareKey();
          
          // Create user mail directories
          char inboxPath[40];
          char sentPath[40];
          sprintf(inboxPath, "mail/%s/inbox", username);
          sprintf(sentPath, "mail/%s/sent", username);
          SD.mkdir(inboxPath);
          SD.mkdir(sentPath);
          // END ADD
          
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
  client.print(F("\033[3J\033[2J\033[H"));
  session.currentMenu = MENU_MAIN;
  
  drawBox(GET_ERROR(), "MAIN MENU");
  
  client.println();
  client.print(GET_WARNING());
  client.print(F("  User: "));
  client.print(GET_SUCCESS());
  client.print(session.username);
  if (session.isAdmin) {
    client.print(GET_WARNING());
    client.print(F(" [ADMIN]"));
  }
  client.print(GET_WARNING());
  client.print(F("  |  Uptime: "));
  client.print(GET_PRIMARY());
  printUptime((millis() - sysStats.bootTime) / 1000);
  client.println();
  client.print(CLR_RESET);
  client.println();
  
  client.print(GET_SUCCESS());
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
   // ADD THESE LINES:
  int unreadCount = countUnreadMail();
  if (unreadCount > 0) {
    client.print(GET_WARNING());
    client.print(F("  │  [M] Secure Mail      - Private messages ("));
    client.print(unreadCount);
    client.print(F(" new)"));
    int spaces = 14 - String(unreadCount).length();
    for(int i = 0; i < spaces; i++) client.print(F(" "));
    client.println(F("│"));
    client.print(GET_SUCCESS());
  } else {
    client.println(F("  │  [M] Secure Mail      - Private messages               │"));
  }
  // END ADD
  if (haConfig.configured) {
    client.print(GET_SECONDARY());
    client.println(F("  │  [H] Home Control     - Control Home Assistant          │"));
    client.print(GET_SUCCESS());
  } else {
    client.print(GET_WARNING());
    client.println(F("  │  [H] Home Control     - Setup required                  │"));
    client.print(GET_SUCCESS());
  }
  
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

void handleMainMenuChoice(int choice) {
  // Check for 'H' or 'h' command first
  if (cmdBuffer[0] == 'h' || cmdBuffer[0] == 'H') {
    if (haConfig.configured) {
      showHomeAssistantMenu();
    } else {
      showHASetupMenu();
    }
    return;
  }
  
  // ADD THIS BLOCK:
  if (cmdBuffer[0] == 'm' || cmdBuffer[0] == 'M') {
    showSecureMailMenu();
    return;
  }
  // END ADD

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
      client.print(GET_ERROR());
      client.println(F("Invalid choice."));
      client.print(CLR_RESET);
      showPrompt();
  }
}

void showEditorMenu() {
  client.print(F("\033[3J\033[2J\033[H"));
  session.currentMenu = MENU_EDITOR_MENU;
  
  drawBox(GET_PRIMARY(), "TEXT EDITOR");
  
  client.println();
  client.print(GET_SECONDARY());
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

void loadHAConfig() {
  haConfig.configured = false;
  
  File f = SD.open("ha/config.txt", FILE_READ);
  if (!f) {
    Serial.println(F("No HA config found"));
    return;
  }
  
  char line[300];
  int idx = 0;
  
  while (f.available()) {
    char c = f.read();
    if (c == '\n' || c == '\r') {
      if (idx > 0) {
        line[idx] = '\0';
        
        // Parse line: key=value
        char* key = strtok(line, "=");
        char* value = strtok(NULL, "=");
        
        if (key && value) {
          if (strcmp(key, "server") == 0) {
            strncpy(haConfig.server, value, 39);
            haConfig.server[39] = '\0';
          } else if (strcmp(key, "port") == 0) {
            haConfig.port = atoi(value);
          } else if (strcmp(key, "token") == 0) {
            strncpy(haConfig.token, value, 255);
            haConfig.token[255] = '\0';
          }
        }
        idx = 0;
      }
    } else if (idx < 299) {
      line[idx++] = c;
    }
  }
  
  f.close();
  
  // Check if we have all required fields
  if (strlen(haConfig.server) > 0 && haConfig.port > 0 && strlen(haConfig.token) > 0) {
    haConfig.configured = true;
    Serial.println(F("HA config loaded"));
  }
}

void saveHAConfig() {
  if (SD.exists("ha/config.txt")) {
    SD.remove("ha/config.txt");
  }
  
  File f = SD.open("ha/config.txt", FILE_WRITE);
  if (f) {
    f.print("server=");
    f.println(haConfig.server);
    f.print("port=");
    f.println(haConfig.port);
    f.print("token=");
    f.println(haConfig.token);
    f.close();
    
    client.print(GET_SUCCESS());
    client.println(F("✓ Configuration saved!"));
    client.print(CLR_RESET);
  } else {
    client.print(GET_ERROR());
    client.println(F("✗ Error saving configuration!"));
    client.print(CLR_RESET);
  }
}

void addLight(const char* entityId, const char* displayName) {
  File f = SD.open("ha/lights.txt", FILE_WRITE);
  if (f) {
    f.print(entityId);
    f.print(":");
    f.println(displayName);
    f.close();
    return;
  }
}

int countLights() {
  File f = SD.open("ha/lights.txt", FILE_READ);
  if (!f) return 0;
  
  int count = 0;
  char line[100];
  int idx = 0;
  
  while (f.available()) {
    char c = f.read();
    if (c == '\n' || c == '\r') {
      if (idx > 0) {
        count++;
        idx = 0;
      }
    } else if (idx < 99) {
      line[idx++] = c;
    }
  }
  
  f.close();
  return count;
}

bool getLight(int index, HALight& light) {
  File f = SD.open("ha/lights.txt", FILE_READ);
  if (!f) return false;
  
  int currentIndex = 0;
  char line[100];
  int idx = 0;
  bool found = false;
  
  while (f.available()) {
    char c = f.read();
    if (c == '\n' || c == '\r') {
      if (idx > 0) {
        if (currentIndex == index) {
          line[idx] = '\0';
          char* entity = strtok(line, ":");
          char* name = strtok(NULL, ":");
          
          if (entity && name) {
            strncpy(light.entityId, entity, 49);
            light.entityId[49] = '\0';
            strncpy(light.displayName, name, 29);
            light.displayName[29] = '\0';
            found = true;
          }
          break;
        }
        currentIndex++;
        idx = 0;
      }
    } else if (idx < 99) {
      line[idx++] = c;
    }
  }
  
  f.close();
  return found;
}

void deleteLight(int index) {
  // Read all lights except the one to delete
  File fRead = SD.open("ha/lights.txt", FILE_READ);
  if (!fRead) return;
  
  File fTemp = SD.open("ha/temp.txt", FILE_WRITE);
  if (!fTemp) {
    fRead.close();
    return;
  }
  
  int currentIndex = 0;
  char line[100];
  int idx = 0;
  
  while (fRead.available()) {
    char c = fRead.read();
    if (c == '\n' || c == '\r') {
      if (idx > 0) {
        if (currentIndex != index) {
          line[idx] = '\0';
          fTemp.println(line);
        }
        currentIndex++;
        idx = 0;
      }
    } else if (idx < 99) {
      line[idx++] = c;
    }
  }
  
  fRead.close();
  fTemp.close();
  
  SD.remove("ha/lights.txt");
  // Note: SD library doesn't have rename, so we need to copy
  File fTempRead = SD.open("ha/temp.txt", FILE_READ);
  File fNew = SD.open("ha/lights.txt", FILE_WRITE);
  
  if (fTempRead && fNew) {
    while (fTempRead.available()) {
      fNew.write(fTempRead.read());
    }
    fTempRead.close();
    fNew.close();
  }
  
  SD.remove("ha/temp.txt");
}

void showHASetupMenu() {
  client.print(F("\033[3J\033[2J\033[H"));
  session.currentMenu = MENU_HA_SETUP;
  
  drawBox(GET_ACCENT(), "HOME ASSISTANT SETUP");
  
  client.println();
  
  if (haConfig.configured) {
    client.print(GET_SUCCESS());
    client.println(F("  ✓ Home Assistant is configured"));
    client.print(CLR_RESET);
    client.println();
    client.print(F("  Server: "));
    client.println(haConfig.server);
    client.print(F("  Port: "));
    client.println(haConfig.port);
    client.print(F("  Token: "));
    // Show only first and last 4 characters of token
    if (strlen(haConfig.token) > 8) {
      for (int i = 0; i < 4; i++) client.print(haConfig.token[i]);
      client.print(F("..."));
      int len = strlen(haConfig.token);
      for (int i = len - 4; i < len; i++) client.print(haConfig.token[i]);
    } else {
      client.print(F("****"));
    }
    client.println();
  } else {
    client.print(GET_WARNING());
    client.println(F("  ⚠ Home Assistant not configured"));
    client.print(CLR_RESET);
  }
  
  client.println();
  client.print(GET_SECONDARY());
  client.println(F("  ┌─────────────────────────────────────────────────────────┐"));
  client.println(F("  │                                                         │"));
  client.println(F("  │  [1] Configure Server - Set HA IP and port              │"));
  client.println(F("  │  [2] Set API Token    - Enter long-lived token          │"));
  client.println(F("  │  [3] Test Connection  - Verify settings work            │"));
  client.println(F("  │  [4] Manage Lights    - Add/remove/list lights          │"));
  client.println(F("  │  [5] Manage Sensors   - Add/remove/list sensors         │"));
  client.println(F("  │  [6] Reset Config     - Clear all settings              │"));
  if (haConfig.configured) {
    client.println(F("  │  [C] Control Lights   - Go to control menu             │"));
    client.println(F("  │                                                        │"));
  }
  client.println(F("  │  [0] Back to Main Menu                                  │"));
  client.println(F("  │                                                         │"));
  client.println(F("  └─────────────────────────────────────────────────────────┘"));
  client.print(CLR_RESET);
  client.println();
  
  showPrompt();
}

void handleHASetupChoice(int choice) {
  // Check for 'C' command
  if (cmdBuffer[0] == 'c' || cmdBuffer[0] == 'C') {
    if (haConfig.configured) {
      showHomeAssistantMenu();
    } else {
      client.println(F("Please configure Home Assistant first."));
      delay(2000);
      showHASetupMenu();
    }
    return;
  }
  
  switch (choice) {
    case 1:
      configureHAServer();
      break;
    case 2:
      configureHAToken();
      break;
    case 3:
      testHAConnection();
      break;
    case 4:
      showManageLightsMenu();
      break;
    case 5:
      showManageSensorsMenu();
     break;
    case 6:  // This was case 5 before
      resetHAConfig();
    break;
    case 0:
      showMainMenu();
      break;
    default:
      client.println(F("Invalid choice."));
      showPrompt();
  }
}

void configureHAServer() {
  client.println();
  client.print(GET_SUCCESS());
  client.print(F("Enter Home Assistant IP (e.g., 192.168.1.100): "));
  client.print(CLR_RESET);
  
  char server[40];
  int idx = 0;
  unsigned long startTime = millis();
  
  while (millis() - startTime < 60000) {
    if (client.available()) {
      char c = client.read();
      if (c == '\n' || c == '\r') {
        server[idx] = '\0';
        break;
      } else if (c == 8 || c == 127) {
        if (idx > 0) {
          idx--;
          client.write(8);
          client.write(' ');
          client.write(8);
        }
      } else if (idx < 39 && c >= 32) {
        server[idx++] = c;
        client.write(c);
      }
    }
  }
  
  if (idx == 0) {
    client.println();
    client.println(F("Cancelled."));
    delay(1500);
    showHASetupMenu();
    return;
  }
  
  client.println();
  client.print(F("Enter port (default 8123): "));
  
  char portStr[10];
  idx = 0;
  startTime = millis();
  
  while (millis() - startTime < 30000) {
    if (client.available()) {
      char c = client.read();
      if (c == '\n' || c == '\r') {
        portStr[idx] = '\0';
        break;
      } else if (c == 8 || c == 127) {
        if (idx > 0) {
          idx--;
          client.write(8);
          client.write(' ');
          client.write(8);
        }
      } else if (idx < 9 && c >= '0' && c <= '9') {
        portStr[idx++] = c;
        client.write(c);
      }
    }
  }
  
  int port = (idx > 0) ? atoi(portStr) : 8123;
  
  // Save configuration
  strncpy(haConfig.server, server, 39);
  haConfig.server[39] = '\0';
  haConfig.port = port;
  
  client.println();
  client.print(GET_SUCCESS());
  client.print(F("✓ Server set to: "));
  client.print(haConfig.server);
  client.print(F(":"));
  client.println(haConfig.port);
  client.print(CLR_RESET);
  
  // Check if we're fully configured now
  if (strlen(haConfig.token) > 0) {
    haConfig.configured = true;
    saveHAConfig();
  }
  
  delay(2000);
  showHASetupMenu();
}

void configureHAToken() {
  client.println();
  client.print(GET_SUCCESS());
  client.println(F("Enter your Home Assistant Long-Lived Access Token:"));
  client.println(F("(Go to Profile > Security > Long-Lived Access Tokens)"));
  client.print(CLR_RESET);
  client.print(F("> "));
  
  char token[256];
  int idx = 0;
  unsigned long startTime = millis();
  
  while (millis() - startTime < 120000) {  // 2 minute timeout for token entry
    if (client.available()) {
      char c = client.read();
      if (c == '\n' || c == '\r') {
        token[idx] = '\0';
        break;
      } else if (c == 8 || c == 127) {
        if (idx > 0) {
          idx--;
          client.write(8);
          client.write(' ');
          client.write(8);
        }
      } else if (idx < 255 && c >= 32) {
        token[idx++] = c;
        client.write('*');  // Show asterisks for security
      }
    }
  }
  
  if (idx == 0) {
    client.println();
    client.println(F("Cancelled."));
    delay(1500);
    showHASetupMenu();
    return;
  }
  
  // Save token
  strncpy(haConfig.token, token, 255);
  haConfig.token[255] = '\0';
  
  client.println();
  client.print(GET_SUCCESS());
  client.println(F("✓ Token saved!"));
  client.print(CLR_RESET);
  
  // Check if we're fully configured now
  if (strlen(haConfig.server) > 0 && haConfig.port > 0) {
    haConfig.configured = true;
    saveHAConfig();
  }
  
  delay(2000);
  showHASetupMenu();
}

void testHAConnection() {
  client.println();
  client.println(F("Testing connection to Home Assistant..."));
  client.println();
  
  if (!haConfig.configured) {
    client.print(GET_ERROR());
    client.println(F("✗ Configuration incomplete!"));
    client.print(CLR_RESET);
    delay(2000);
    showHASetupMenu();
    return;
  }
  
  EthernetClient haClient;
  
  client.print(F("Connecting to "));
  client.print(haConfig.server);
  client.print(F(":"));
  client.print(haConfig.port);
  client.println(F("..."));
  
  if (haClient.connect(haConfig.server, haConfig.port)) {
    client.print(GET_SUCCESS());
    client.println(F("✓ Connected!"));
    client.print(CLR_RESET);
    
    // Try to get API status
    haClient.println(F("GET /api/ HTTP/1.1"));
    haClient.print(F("Host: "));
    haClient.print(haConfig.server);
    haClient.print(F(":"));
    haClient.println(haConfig.port);
    haClient.print(F("Authorization: Bearer "));
    haClient.println(haConfig.token);
    haClient.println(F("Connection: close"));
    haClient.println();
    
    // Wait for response
    unsigned long timeout = millis();
    while (haClient.connected() && millis() - timeout < 5000) {
      if (haClient.available()) break;
      delay(10);
    }
    
    bool success = false;
    while (haClient.available()) {
      String line = haClient.readStringUntil('\n');
      if (line.indexOf("200 OK") >= 0) {
        success = true;
      }
      if (line.indexOf("message") >= 0 && line.indexOf("API running") >= 0) {
        success = true;
      }
    }
    
    haClient.stop();
    
    if (success) {
      client.print(GET_SUCCESS());
      client.println(F("✓ Home Assistant API responding correctly!"));
      client.print(CLR_RESET);
    } else {
      client.print(GET_WARNING());
      client.println(F("⚠ Connected but API response unclear."));
      client.println(F("  Check your token permissions."));
      client.print(CLR_RESET);
    }
  } else {
    client.print(GET_ERROR());
    client.println(F("✗ Connection failed!"));
    client.println(F("  Check IP address and port."));
    client.print(CLR_RESET);
  }
  
  client.println();
  client.println(F("Press Enter to continue..."));
  session.waitingForContinue = true;
  session.returnToMenu = MENU_HA_SETUP;
}

void resetHAConfig() {
  client.println();
  client.print(GET_ERROR());
  client.print(F("Reset all Home Assistant configuration? (y/n): "));
  client.print(CLR_RESET);
  
  char response[10];
  int idx = 0;
  unsigned long startTime = millis();
  
  while (millis() - startTime < 10000) {
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
  
  client.println();
  
  if (idx > 0 && (response[0] == 'y' || response[0] == 'Y')) {
    SD.remove("ha/config.txt");
    SD.remove("ha/lights.txt");
	SD.remove("ha/sensors.txt");  // ADD THIS LINE
    
    haConfig.server[0] = '\0';
    haConfig.port = 0;
    haConfig.token[0] = '\0';
    haConfig.configured = false;
    
    client.print(GET_SUCCESS());
    client.println(F("✓ Configuration reset!"));
    client.print(CLR_RESET);
  } else {
    client.println(F("Cancelled."));
  }
  
  delay(2000);
  showHASetupMenu();
}

void showManageLightsMenu() {
  client.print(F("\033[3J\033[2J\033[H"));
  session.currentMenu = MENU_HA_MANAGE_LIGHTS;
  
  drawBox(GET_ACCENT(), "MANAGE LIGHTS");
  
  client.println();
  
  int lightCount = countLights();
  
  if (lightCount > 0) {
    client.print(GET_WARNING());
    client.println(F("  Configured Lights:"));
    client.print(CLR_RESET);
    client.println(F("  ───────────────────────────────────────────────────────────"));
    
    for (int i = 0; i < lightCount; i++) {
      HALight light;
      if (getLight(i, light)) {
        client.print(F("  ["));
        client.print(i + 1);
        client.print(F("] "));
        client.print(light.displayName);
        client.print(F(" ("));
        client.print(light.entityId);
        client.println(F(")"));
      }
    }
    client.println();
  } else {
    client.print(GET_WARNING());
    client.println(F("  No lights configured yet."));
    client.print(CLR_RESET);
    client.println();
  }
  
  client.print(GET_SECONDARY());
  client.println(F("  ┌─────────────────────────────────────────────────────────┐"));
  client.println(F("  │                                                         │"));
  client.println(F("  │  [1] Add Light        - Add a new light entity          │"));
  client.println(F("  │  [2] Remove Light     - Delete a light from list        │"));
  client.println(F("  │  [3] List All Lights  - Show all configured lights      │"));
  client.println(F("  │                                                         │"));
  client.println(F("  │  [0] Back             - Return to setup menu            │"));
  client.println(F("  │                                                         │"));
  client.println(F("  └─────────────────────────────────────────────────────────┘"));
  client.print(CLR_RESET);
  client.println();
  
  showPrompt();
}

void handleManageLightsChoice(int choice) {
  switch (choice) {
    case 1:
      addLightInteractive();
      break;
    case 2:
      removeLightInteractive();
      break;
    case 3:
      listAllLights();
      break;
    case 0:
      showHASetupMenu();
      break;
    default:
      client.println(F("Invalid choice."));
      showPrompt();
  }
}

void addLightInteractive() {
  client.println();
  client.print(GET_SUCCESS());
  client.print(F("Enter entity ID (e.g., light.living_room): "));
  client.print(CLR_RESET);
  
  char entityId[50];
  int idx = 0;
  unsigned long startTime = millis();
  bool gotInput = false;
  
  while (millis() - startTime < 60000) {
    if (client.available()) {
      char c = client.read();
      if (c == '\n' || c == '\r') {
        // Skip any extra newline characters
        delay(10);
        while (client.available()) {
          char peek = client.peek();
          if (peek == '\r' || peek == '\n') {
            client.read();
          } else {
            break;
          }
        }
        if (gotInput) {
          entityId[idx] = '\0';
          break;
        }
      } else if (c == 8 || c == 127) {
        if (idx > 0) {
          idx--;
          gotInput = (idx > 0);
          client.write(8);
          client.write(' ');
          client.write(8);
        }
      } else if (idx < 49 && c >= 32) {
        entityId[idx++] = c;
        gotInput = true;
        client.write(c);
      }
    }
  }
  
  if (!gotInput) {
    client.println();
    client.println(F("Cancelled."));
    delay(1500);
    showManageLightsMenu();
    return;
  }
  
  client.println();
  client.print(F("Enter display name (e.g., Living Room): "));
  
  char displayName[30];
  idx = 0;
  startTime = millis();
  gotInput = false;
  
  while (millis() - startTime < 60000) {
    if (client.available()) {
      char c = client.read();
      if (c == '\n' || c == '\r') {
        // Skip any extra newline characters
        delay(10);
        while (client.available()) {
          char peek = client.peek();
          if (peek == '\r' || peek == '\n') {
            client.read();
          } else {
            break;
          }
        }
        if (gotInput) {
          displayName[idx] = '\0';
          break;
        }
      } else if (c == 8 || c == 127) {
        if (idx > 0) {
          idx--;
          gotInput = (idx > 0);
          client.write(8);
          client.write(' ');
          client.write(8);
        }
      } else if (idx < 29 && c >= 32) {
        displayName[idx++] = c;
        gotInput = true;
        client.write(c);
      }
    }
  }
  
  if (!gotInput) {
    client.println();
    client.println(F("Cancelled."));
    delay(1500);
    showManageLightsMenu();
    return;
  }
  
  addLight(entityId, displayName);
  
  client.println();
  client.print(GET_SUCCESS());
  client.print(F("✓ Light added: "));
  client.print(displayName);
  client.print(F(" ("));
  client.print(entityId);
  client.println(F(")"));
  client.print(CLR_RESET);
  
  delay(2000);
  showManageLightsMenu();
}

void removeLightInteractive() {
  int lightCount = countLights();
  
  if (lightCount == 0) {
    client.println();
    client.println(F("No lights to remove."));
    delay(2000);
    showManageLightsMenu();
    return;
  }
  
  client.println();
  client.println(F("Which light to remove?"));
  
  for (int i = 0; i < lightCount; i++) {
    HALight light;
    if (getLight(i, light)) {
      client.print(F("  ["));
      client.print(i + 1);
      client.print(F("] "));
      client.println(light.displayName);
    }
  }
  
  client.println();
  client.print(F("Enter number (1-"));
  client.print(lightCount);
  client.print(F("): "));
  char numStr[5];
int idx = 0;
unsigned long startTime = millis();
while (millis() - startTime < 30000) {
if (client.available()) {
char c = client.read();
if (c == '\n' || c == '\r') {
numStr[idx] = '\0';
break;
} else if (c == 8 || c == 127) {
if (idx > 0) {
idx--;
client.write(8);
client.write(' ');
client.write(8);
}
} else if (idx < 4 && c >= '0' && c <= '9') {
numStr[idx++] = c;
client.write(c);
}
}
}
if (idx == 0) {
client.println();
showManageLightsMenu();
return;
}
int num = atoi(numStr);
if (num >= 1 && num <= lightCount) {
HALight light;
if (getLight(num - 1, light)) {
deleteLight(num - 1);
client.println();
client.print(GET_SUCCESS());
client.print(F("✓ Removed: "));
client.println(light.displayName);
client.print(CLR_RESET);
}
} else {
client.println();
client.println(F("Invalid number."));
}
delay(2000);
showManageLightsMenu();
}
void listAllLights() {
client.print(F("\033[3J\033[2J\033[H"));
drawBox(GET_ACCENT(), "ALL CONFIGURED LIGHTS");
client.println();
int lightCount = countLights();
if (lightCount == 0) {
client.println(F("  No lights configured."));
} else {
for (int i = 0; i < lightCount; i++) {
HALight light;
if (getLight(i, light)) {
client.print(GET_WARNING());
client.print(F("  ┌─[ "));
client.print(light.displayName);
client.println(F(" ]"));
client.print(CLR_RESET);
client.print(F("  │ Entity ID: "));
client.println(light.entityId);
client.print(GET_WARNING());
client.println(F("  └────────────────────────────────────────────"));
client.print(CLR_RESET);
client.println();
}
}
}
client.println(F("Press Enter to continue..."));
session.waitingForContinue = true;
session.returnToMenu = MENU_HA_MANAGE_LIGHTS;
}

void addSensor(const char* entityId, const char* displayName, const char* unit) {
  File f = SD.open("ha/sensors.txt", FILE_WRITE);
  if (f) {
    f.print(entityId);
    f.print(":");
    f.print(displayName);
    f.print(":");
    f.println(unit);
    f.close();
    return;
  }
}

int countSensors() {
  File f = SD.open("ha/sensors.txt", FILE_READ);
  if (!f) return 0;
  
  int count = 0;
  char line[100];
  int idx = 0;
  
  while (f.available()) {
    char c = f.read();
    if (c == '\n' || c == '\r') {
      if (idx > 0) {
        count++;
        idx = 0;
      }
    } else if (idx < 99) {
      line[idx++] = c;
    }
  }
  
  f.close();
  return count;
}

bool getSensor(int index, HASensor& sensor) {
  File f = SD.open("ha/sensors.txt", FILE_READ);
  if (!f) return false;
  
  int currentIndex = 0;
  char line[100];
  int idx = 0;
  bool found = false;
  
  while (f.available()) {
    char c = f.read();
    if (c == '\n' || c == '\r') {
      if (idx > 0) {
        if (currentIndex == index) {
          line[idx] = '\0';
          char* entity = strtok(line, ":");
          char* name = strtok(NULL, ":");
          char* unit = strtok(NULL, ":");
          
          if (entity && name) {
            strncpy(sensor.entityId, entity, 49);
            sensor.entityId[49] = '\0';
            strncpy(sensor.displayName, name, 29);
            sensor.displayName[29] = '\0';
            if (unit) {
              strncpy(sensor.unit, unit, 9);
              sensor.unit[9] = '\0';
            } else {
              sensor.unit[0] = '\0';
            }
            found = true;
          }
          break;
        }
        currentIndex++;
        idx = 0;
      }
    } else if (idx < 99) {
      line[idx++] = c;
    }
  }
  
  f.close();
  return found;
}

void deleteSensor(int index) {
  File fRead = SD.open("ha/sensors.txt", FILE_READ);
  if (!fRead) return;
  
  File fTemp = SD.open("ha/temp.txt", FILE_WRITE);
  if (!fTemp) {
    fRead.close();
    return;
  }
  
  int currentIndex = 0;
  char line[100];
  int idx = 0;
  
  while (fRead.available()) {
    char c = fRead.read();
    if (c == '\n' || c == '\r') {
      if (idx > 0) {
        if (currentIndex != index) {
          line[idx] = '\0';
          fTemp.println(line);
        }
        currentIndex++;
        idx = 0;
      }
    } else if (idx < 99) {
      line[idx++] = c;
    }
  }
  
  fRead.close();
  fTemp.close();
  
  SD.remove("ha/sensors.txt");
  File fTempRead = SD.open("ha/temp.txt", FILE_READ);
  File fNew = SD.open("ha/sensors.txt", FILE_WRITE);
  
  if (fTempRead && fNew) {
    while (fTempRead.available()) {
      fNew.write(fTempRead.read());
    }
    fTempRead.close();
    fNew.close();
  }
  
  SD.remove("ha/temp.txt");
}

void showManageSensorsMenu() {
  client.print(F("\033[3J\033[2J\033[H"));
  session.currentMenu = MENU_HA_MANAGE_SENSORS;
  
  drawBox(GET_PRIMARY(), "MANAGE SENSORS");
  
  client.println();
  
  int sensorCount = countSensors();
  
  if (sensorCount > 0) {
    client.print(GET_WARNING());
    client.println(F("  Configured Sensors:"));
    client.print(CLR_RESET);
    client.println(F("  ───────────────────────────────────────────────────────────"));
    
    for (int i = 0; i < sensorCount; i++) {
      HASensor sensor;
      if (getSensor(i, sensor)) {
        client.print(F("  ["));
        client.print(i + 1);
        client.print(F("] "));
        client.print(sensor.displayName);
        client.print(F(" ("));
        client.print(sensor.entityId);
        client.print(F(") ["));
        client.print(sensor.unit);
        client.println(F("]"));
      }
    }
    client.println();
  } else {
    client.print(GET_WARNING());
    client.println(F("  No sensors configured yet."));
    client.print(CLR_RESET);
    client.println();
  }
  
  client.print(GET_SECONDARY());
  client.println(F("  ┌─────────────────────────────────────────────────────────┐"));
  client.println(F("  │                                                         │"));
  client.println(F("  │  [1] Add Sensor       - Add a new sensor entity         │"));
  client.println(F("  │  [2] Remove Sensor    - Delete a sensor from list       │"));
  client.println(F("  │  [3] List All Sensors - Show all configured sensors     │"));
  client.println(F("  │                                                         │"));
  client.println(F("  │  [0] Back             - Return to setup menu            │"));
  client.println(F("  │                                                         │"));
  client.println(F("  └─────────────────────────────────────────────────────────┘"));
  client.print(CLR_RESET);
  client.println();
  
  showPrompt();
}

void handleManageSensorsChoice(int choice) {
  switch (choice) {
    case 1:
      addSensorInteractive();
      break;
    case 2:
      removeSensorInteractive();
      break;
    case 3:
      listAllSensors();
      break;
    case 0:
      showHASetupMenu();
      break;
    default:
      client.println(F("Invalid choice."));
      showPrompt();
  }
}

void addSensorInteractive() {
  client.println();
  client.print(GET_SUCCESS());
  client.print(F("Enter entity ID (e.g., sensor.temperature): "));
  client.print(CLR_RESET);
  
  char entityId[50];
  int idx = 0;
  unsigned long startTime = millis();
  bool gotInput = false;
  
  while (millis() - startTime < 60000) {
    if (client.available()) {
      char c = client.read();
      if (c == '\n' || c == '\r') {
        delay(10);
        while (client.available()) {
          char peek = client.peek();
          if (peek == '\r' || peek == '\n') {
            client.read();
          } else {
            break;
          }
        }
        if (gotInput) {
          entityId[idx] = '\0';
          break;
        }
      } else if (c == 8 || c == 127) {
        if (idx > 0) {
          idx--;
          gotInput = (idx > 0);
          client.write(8);
          client.write(' ');
          client.write(8);
        }
      } else if (idx < 49 && c >= 32) {
        entityId[idx++] = c;
        gotInput = true;
        client.write(c);
      }
    }
  }
  
  if (!gotInput) {
    client.println();
    client.println(F("Cancelled."));
    delay(1500);
    showManageSensorsMenu();
    return;
  }
  
  client.println();
  client.print(F("Enter display name (e.g., Living Room Temp): "));
  
  char displayName[30];
  idx = 0;
  startTime = millis();
  gotInput = false;
  
  while (millis() - startTime < 60000) {
    if (client.available()) {
      char c = client.read();
      if (c == '\n' || c == '\r') {
        delay(10);
        while (client.available()) {
          char peek = client.peek();
          if (peek == '\r' || peek == '\n') {
            client.read();
          } else {
            break;
          }
        }
        if (gotInput) {
          displayName[idx] = '\0';
          break;
        }
      } else if (c == 8 || c == 127) {
        if (idx > 0) {
          idx--;
          gotInput = (idx > 0);
          client.write(8);
          client.write(' ');
          client.write(8);
        }
      } else if (idx < 29 && c >= 32) {
        displayName[idx++] = c;
        gotInput = true;
        client.write(c);
      }
    }
  }
  
  if (!gotInput) {
    client.println();
    client.println(F("Cancelled."));
    delay(1500);
    showManageSensorsMenu();
    return;
  }
  
  client.println();
  client.print(F("Enter unit (e.g., °F, %, hPa): "));
  
  char unit[10];
  idx = 0;
  startTime = millis();
  gotInput = false;
  
  while (millis() - startTime < 30000) {
    if (client.available()) {
      char c = client.read();
      if (c == '\n' || c == '\r') {
        delay(10);
        while (client.available()) {
          char peek = client.peek();
          if (peek == '\r' || peek == '\n') {
            client.read();
          } else {
            break;
          }
        }
        if (gotInput) {
          unit[idx] = '\0';
          break;
        } else {
          unit[0] = '\0';
          break;
        }
      } else if (c == 8 || c == 127) {
        if (idx > 0) {
          idx--;
          gotInput = (idx > 0);
          client.write(8);
          client.write(' ');
          client.write(8);
        }
      } else if (idx < 9 && c >= 32) {
        unit[idx++] = c;
        gotInput = true;
        client.write(c);
      }
    }
  }
  
  addSensor(entityId, displayName, unit);
  
  client.println();
  client.print(GET_SUCCESS());
  client.print(F("✓ Sensor added: "));
  client.print(displayName);
  client.print(F(" ("));
  client.print(entityId);
  client.println(F(")"));
  client.print(CLR_RESET);
  
  delay(2000);
  showManageSensorsMenu();
}

void removeSensorInteractive() {
  int sensorCount = countSensors();
  
  if (sensorCount == 0) {
    client.println();
    client.println(F("No sensors to remove."));
    delay(2000);
    showManageSensorsMenu();
    return;
  }
  
  client.println();
  client.println(F("Which sensor to remove?"));
  
  for (int i = 0; i < sensorCount; i++) {
    HASensor sensor;
    if (getSensor(i, sensor)) {
      client.print(F("  ["));
      client.print(i + 1);
      client.print(F("] "));
      client.println(sensor.displayName);
    }
  }
  
  client.println();
  client.print(F("Enter number (1-"));
  client.print(sensorCount);
  client.print(F("): "));
  
  char numStr[5];
  int idx = 0;
  unsigned long startTime = millis();
  
  while (millis() - startTime < 30000) {
    if (client.available()) {
      char c = client.read();
      if (c == '\n' || c == '\r') {
        numStr[idx] = '\0';
        break;
      } else if (c == 8 || c == 127) {
        if (idx > 0) {
          idx--;
          client.write(8);
          client.write(' ');
          client.write(8);
        }
      } else if (idx < 4 && c >= '0' && c <= '9') {
        numStr[idx++] = c;
        client.write(c);
      }
    }
  }
  
  if (idx == 0) {
    client.println();
    showManageSensorsMenu();
    return;
  }
  
  int num = atoi(numStr);
  
  if (num >= 1 && num <= sensorCount) {
    HASensor sensor;
    if (getSensor(num - 1, sensor)) {
      deleteSensor(num - 1);
      client.println();
      client.print(GET_SUCCESS());
      client.print(F("✓ Removed: "));
      client.println(sensor.displayName);
      client.print(CLR_RESET);
    }
  } else {
    client.println();
    client.println(F("Invalid number."));
  }
  
  delay(2000);
  showManageSensorsMenu();
}

void listAllSensors() {
  client.print(F("\033[3J\033[2J\033[H"));
  drawBox(GET_PRIMARY(), "ALL CONFIGURED SENSORS");
  client.println();
  
  int sensorCount = countSensors();
  
  if (sensorCount == 0) {
    client.println(F("  No sensors configured."));
  } else {
    for (int i = 0; i < sensorCount; i++) {
      HASensor sensor;
      if (getSensor(i, sensor)) {
        client.print(GET_WARNING());
        client.print(F("  ┌─[ "));
        client.print(sensor.displayName);
        client.println(F(" ]"));
        client.print(CLR_RESET);
        client.print(F("  │ Entity ID: "));
        client.println(sensor.entityId);
        client.print(F("  │ Unit: "));
        if (strlen(sensor.unit) > 0) {
          client.println(sensor.unit);
        } else {
          client.println(F("(none)"));
        }
        client.print(GET_WARNING());
        client.println(F("  └────────────────────────────────────────────"));
        client.print(CLR_RESET);
        client.println();
      }
    }
  }
  
  client.println(F("Press Enter to continue..."));
  session.waitingForContinue = true;
  session.returnToMenu = MENU_HA_MANAGE_SENSORS;
}

String getSensorValue(const char* entityId) {
  EthernetClient haClient;
  
  if (haClient.connect(haConfig.server, haConfig.port)) {
    haClient.print(F("GET /api/states/"));
    haClient.print(entityId);
    haClient.println(F(" HTTP/1.1"));
    haClient.print(F("Host: "));
    haClient.print(haConfig.server);
    haClient.print(F(":"));
    haClient.println(haConfig.port);
    haClient.print(F("Authorization: Bearer "));
    haClient.println(haConfig.token);
    haClient.println(F("Connection: close"));
    haClient.println();
    
    unsigned long timeout = millis();
    while (haClient.connected() && millis() - timeout < 5000) {
      if (haClient.available()) break;
      delay(10);
    }
    
    String response = "";
    bool inBody = false;
    
    while (haClient.available()) {
      String line = haClient.readStringUntil('\n');
      
      if (line == "\r" || line.length() == 0) {
        inBody = true;
        continue;
      }
      
      if (inBody) {
        response += line;
      }
    }
    
    haClient.stop();
    
    if (response.length() > 0) {
      int statePos = response.indexOf("\"state\":");
      if (statePos >= 0) {
        int quoteStart = response.indexOf("\"", statePos + 8);
        int quoteEnd = response.indexOf("\"", quoteStart + 1);
        
        if (quoteStart >= 0 && quoteEnd >= 0) {
          return response.substring(quoteStart + 1, quoteEnd);
        }
      }
    }
  }
  
  return "";
}


void createNewNote() {
  client.println();
  client.print(GET_SUCCESS());
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

void showHomeAssistantMenu() {
  client.print(F("\033[3J\033[2J\033[H"));
  session.currentMenu = MENU_HOME_ASSISTANT;
  
  drawBox(GET_ACCENT(), "HOME ASSISTANT CONTROL");
  
  client.println();
  
  int lightCount = countLights();
  
  if (lightCount == 0) {
    client.print(GET_WARNING());
    client.println(F("  No lights configured!"));
    client.println(F("  Go to Setup > Manage Lights to add lights."));
    client.print(CLR_RESET);
    client.println();
    client.println(F("  [S] Setup Menu"));
    client.println(F("  [0] Back to Main Menu"));
    client.println();
    showPrompt();
    return;
  }
  
  client.print(GET_SECONDARY());
  client.println(F("  ┌─────────────────────────────────────────────────────────┐"));
  client.println(F("  │                                                         │"));
  
  // Show each configured light
  for (int i = 0; i < lightCount && i < 8; i++) {  // Max 8 lights for display
    HALight light;
    if (getLight(i, light)) {
      client.print(F("  │  ["));
      client.print(i + 1);
      client.print(F("] "));
      client.print(light.displayName);
      
      // Pad with spaces
      int padding = 48 - strlen(light.displayName);
      for (int p = 0; p < padding; p++) client.print(F(" "));
      
      client.println(F("│"));
    }
  }
  
  client.println(F("  │                                                         │"));
  client.println(F("  │  [A] Toggle All       - Turn all lights on/off          │"));
  client.println(F("  │  [C] Check Status     - View current light states       │"));
  client.println(F("  │  [S] Setup            - Manage configuration            │"));
  client.println(F("  │                                                         │"));
  client.println(F("  │  [0] Back to Main Menu                                  │"));
  client.println(F("  │                                                         │"));
  client.println(F("  └─────────────────────────────────────────────────────────┘"));
  client.print(CLR_RESET);
  client.println();
  
  showPrompt();
}

void handleHomeAssistantChoice(int choice) {
  // Check for letter commands
  if (cmdBuffer[0] == 'a' || cmdBuffer[0] == 'A') {
    toggleAllLights();
    return;
  }
  if (cmdBuffer[0] == 'c' || cmdBuffer[0] == 'C') {
    checkAllLightsStatus();
    return;
  }
  if (cmdBuffer[0] == 's' || cmdBuffer[0] == 'S') {
    showHASetupMenu();
    return;
  }
  
  // Number choices for individual lights
  if (choice >= 1 && choice <= countLights()) {
    HALight light;
    if (getLight(choice - 1, light)) {
      toggleLight(light.entityId, light.displayName);
    }
  } else if (choice == 0) {
    showMainMenu();
  } else {
    client.println(F("Invalid choice."));
    showPrompt();
  }
}

void toggleLight(const char* entityId, const char* displayName) {
  client.println();
  client.print(F("Toggling "));
  client.print(displayName);
  client.println(F("..."));
  
  EthernetClient haClient;
  
  if (haClient.connect(haConfig.server, haConfig.port)) {
    // Build the HTTP POST request
    String postData = String("{\"entity_id\":\"") + entityId + "\"}";
    
    haClient.println(F("POST /api/services/light/toggle HTTP/1.1"));
    haClient.print(F("Host: "));
    haClient.print(haConfig.server);
    haClient.print(F(":"));
    haClient.println(haConfig.port);
    haClient.print(F("Authorization: Bearer "));
    haClient.println(haConfig.token);
    haClient.println(F("Content-Type: application/json"));
    haClient.print(F("Content-Length: "));
    haClient.println(postData.length());
    haClient.println(F("Connection: close"));
    haClient.println();
    haClient.println(postData);
    
    // Wait for response
    unsigned long timeout = millis();
    while (haClient.connected() && millis() - timeout < 5000) {
      if (haClient.available()) break;
      delay(10);
    }
    
    bool success = false;
    while (haClient.available()) {
      String line = haClient.readStringUntil('\n');
      if (line.indexOf("200 OK") >= 0) {
        success = true;
      }
    }
    
    haClient.stop();
    
    if (success) {
      client.print(GET_SUCCESS());
      client.print(F("✓ "));
      client.print(displayName);
      client.println(F(" toggled successfully!"));
      client.print(CLR_RESET);
    } else {
      client.print(GET_ERROR());
      client.println(F("✗ Failed to toggle light"));
      client.print(CLR_RESET);
    }
  } else {
    client.print(GET_ERROR());
    client.println(F("✗ Cannot connect to Home Assistant"));
    client.print(CLR_RESET);
  }
  
  delay(2000);
  showHomeAssistantMenu();
}

void toggleAllLights() {
  client.println();
  client.println(F("Toggling all lights..."));
  client.println();
  
  int lightCount = countLights();
  int successCount = 0;
  
  for (int i = 0; i < lightCount; i++) {
    HALight light;
    if (getLight(i, light)) {
      client.print(F("  "));
      client.print(light.displayName);
      client.print(F("... "));
      
      EthernetClient haClient;
      if (haClient.connect(haConfig.server, haConfig.port)) {
        String postData = String("{\"entity_id\":\"") + light.entityId + "\"}";
        
        haClient.println(F("POST /api/services/light/toggle HTTP/1.1"));
        haClient.print(F("Host: "));
        haClient.print(haConfig.server);
        haClient.print(F(":"));
        haClient.println(haConfig.port);
        haClient.print(F("Authorization: Bearer "));
        haClient.println(haConfig.token);
        haClient.println(F("Content-Type: application/json"));
        haClient.print(F("Content-Length: "));
        haClient.println(postData.length());
        haClient.println(F("Connection: close"));
        haClient.println();
        haClient.println(postData);
        
        delay(300);
        
        bool success = false;
        while (haClient.available()) {
          String line = haClient.readStringUntil('\n');
          if (line.indexOf("200 OK") >= 0) {
            success = true;
          }
        }
        
        haClient.stop();
        
        if (success) {
          client.print(GET_SUCCESS());
          client.println(F("✓"));
          client.print(CLR_RESET);
          successCount++;
        } else {
          client.print(GET_ERROR());
          client.println(F("✗"));
          client.print(CLR_RESET);
        }
      } else {
        client.print(GET_ERROR());
        client.println(F("✗"));
        client.print(CLR_RESET);
      }
      
      delay(200);
    }
  }
  
  client.println();
  client.print(GET_SUCCESS());
  client.print(F("✓ Toggled "));
  client.print(successCount);
  client.print(F(" of "));
  client.print(lightCount);
  client.println(F(" lights"));
  client.print(CLR_RESET);
  
  delay(3000);
  showHomeAssistantMenu();
}

void checkAllLightsStatus() {
  client.print(F("\033[3J\033[2J\033[H"));
  drawBox(GET_ACCENT(), "LIGHT STATUS");
  client.println();
  
  client.print(GET_WARNING());
  client.println(F("  Fetching status from Home Assistant..."));
  client.print(CLR_RESET);
  client.println();
  
  int lightCount = countLights();
  
  for (int i = 0; i < lightCount; i++) {
    HALight light;
    if (getLight(i, light)) {
      getLightStatus(light.entityId, light.displayName);
      delay(300);
    }
  }
  
  client.println();
  client.println(F("Press Enter to continue..."));
  session.waitingForContinue = true;
  session.returnToMenu = MENU_HOME_ASSISTANT;
}

void getLightStatus(const char* entityId, const char* displayName) {
  EthernetClient haClient;
  
  client.print(F("  "));
  client.print(displayName);
  client.print(F(": "));
  
  if (haClient.connect(haConfig.server, haConfig.port)) {
    // Build the HTTP GET request for entity state
    haClient.print(F("GET /api/states/"));
    haClient.print(entityId);
    haClient.println(F(" HTTP/1.1"));
    haClient.print(F("Host: "));
    haClient.print(haConfig.server);
    haClient.print(F(":"));
    haClient.println(haConfig.port);
    haClient.print(F("Authorization: Bearer "));
    haClient.println(haConfig.token);
    haClient.println(F("Connection: close"));
    haClient.println();
    
    // Wait for response
    unsigned long timeout = millis();
    while (haClient.connected() && millis() - timeout < 5000) {
      if (haClient.available()) break;
      delay(10);
    }
    
    // Read response - look for the state
    String response = "";
    bool inBody = false;
    
    while (haClient.available()) {
      String line = haClient.readStringUntil('\n');
      
      // Check if we've reached the body (empty line after headers)
      if (line == "\r" || line.length() == 0) {
        inBody = true;
        continue;
      }
      
      if (inBody) {
        response += line;
      }
    }
    
    haClient.stop();
    
    // Parse the JSON response for state
    if (response.length() > 0) {
      int statePos = response.indexOf("\"state\":");
      if (statePos >= 0) {
        int quoteStart = response.indexOf("\"", statePos + 8);
        int quoteEnd = response.indexOf("\"", quoteStart + 1);
        
        if (quoteStart >= 0 && quoteEnd >= 0) {
          String state = response.substring(quoteStart + 1, quoteEnd);
          
          if (state == "on") {
            client.print(GET_SUCCESS());
            client.println(F("ON"));
            client.print(CLR_RESET);
          } else if (state == "off") {
            client.print(GET_WARNING());
            client.println(F("OFF"));
            client.print(CLR_RESET);
          } else {
            client.println(state);
          }
          
          return;
        }
      }
    }
    
    client.print(GET_WARNING());
    client.println(F("Unknown"));
    client.print(CLR_RESET);
    
  } else {
    client.print(GET_ERROR());
    client.println(F("Connection failed"));
    client.print(CLR_RESET);
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
    case MENU_THEME_SELECTOR:  // ADD THIS CASE
      handleThemeChoice(choice);
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
    case MENU_HOME_ASSISTANT:
      handleHomeAssistantChoice(choice);
      break;
    case MENU_HA_SETUP:
      handleHASetupChoice(choice);
      break;
    case MENU_HA_MANAGE_LIGHTS:
      handleManageLightsChoice(choice);
      break;
    case MENU_HA_MANAGE_SENSORS:
      handleManageSensorsChoice(choice);
      break;
    case MENU_SECURE_MAIL:
      handleSecureMailChoice(choice);
      break;
    case MENU_MAIL_COMPOSE:
      handleComposeMailInput();
      return;
    default:
      showMainMenu();
  }
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
  client.print(F("\033[3J\033[2J\033[H"));
  drawBox(GET_PRIMARY(), "ALL NOTES");
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
  client.print(F("\033[3J\033[2J\033[H"));
  session.currentMenu = MENU_EDITOR;
  
  drawBox(GET_PRIMARY(), "TEXT EDITOR");
  
  client.println();
  client.print(GET_WARNING());
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
    client.print(GET_SUCCESS());
    client.print(F("✓ Saved: "));
    client.println(fullPath);
    client.print(CLR_RESET);
  } else {
    client.print(GET_ERROR());
    client.println(F("✗ Error saving!"));
    client.print(CLR_RESET);
  }
}

void startCalculator() {
  client.print(F("\033[3J\033[2J\033[H"));
  session.currentMenu = MENU_CALCULATOR;
  
  drawBox(GET_WARNING(), "CALCULATOR");
  
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
    client.print(GET_SUCCESS());
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
      client.print(GET_SUCCESS());
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
  client.print(F("\033[3J\033[2J\033[H"));
  session.currentMenu = MENU_WEATHER;
  
  drawBox(GET_ACCENT(), "WEATHER INFORMATION");
  
  client.println();
  
  int sensorCount = countSensors();
  
  if (!haConfig.configured || sensorCount == 0) {
    client.print(GET_WARNING());
    if (!haConfig.configured) {
      client.println(F("  ⚠ Home Assistant not configured"));
    } else {
      client.println(F("  ⚠ No sensors configured"));
      client.println(F("  Go to Home Assistant Setup > Manage Sensors"));
    }
    client.println(F("  Showing simulated data..."));
    client.print(CLR_RESET);
    client.println();
    client.println(F("  ───────────────────────────────────────────────────────────"));
    client.println();
    client.println(F("  Location: San Francisco, CA"));
    client.println(F("  Temperature: 68°F (20°C)"));
    client.println(F("  Conditions: Partly Cloudy"));
    client.println(F("  Humidity: 65%"));
    client.println(F("  Wind: 12 mph NW"));
    client.println(F("  Pressure: 30.12 inHg"));
  } else {
    client.print(GET_WARNING());
    client.println(F("  Fetching sensor data from Home Assistant..."));
    client.print(CLR_RESET);
    client.println(F("  ───────────────────────────────────────────────────────────"));
    client.println();
    
    for (int i = 0; i < sensorCount; i++) {
      HASensor sensor;
      if (getSensor(i, sensor)) {
        client.print(F("  "));
        client.print(sensor.displayName);
        client.print(F(": "));
        
        String value = getSensorValue(sensor.entityId);
        
        if (value.length() > 0) {
          client.print(GET_SUCCESS());
          client.print(value);
          if (strlen(sensor.unit) > 0) {
            client.print(F(" "));
            client.print(sensor.unit);
          }
          client.print(CLR_RESET);
        } else {
          client.print(GET_ERROR());
          client.print(F("unavailable"));
          client.print(CLR_RESET);
        }
        client.println();
        
        delay(200);
      }
    }
  }
  
  client.println();
  client.println(F("  [0] Back to Main Menu"));
  client.println();
  
  showPrompt();
}

void showStocks() {
  client.print(F("\033[3J\033[2J\033[H"));
  session.currentMenu = MENU_STOCKS;
  
  drawBox(GET_PRIMARY(), "STOCK MARKET INFORMATION");
  
  client.println();
  client.print(GET_SECONDARY());
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
  client.print(GET_WARNING());
  client.println(F("  Note: Stock data is simulated. Connect to market"));
  client.println(F("  data API for real-time quotes."));
  client.print(CLR_RESET);
  client.println();
  client.println(F("  [0] Back to Main Menu"));
  client.println();
  
  showPrompt();
}

void showGamesMenu() {
  client.print(F("\033[3J\033[2J\033[H"));
  session.currentMenu = MENU_GAMES;
  
  drawBox(GET_ACCENT(), "GAMES");
  
  client.println();
  client.print(GET_SECONDARY());
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
  client.print(F("\033[3J\033[2J\033[H"));
  session.currentMenu = MENU_GAME_GUESS;
  
  drawBox(GET_ACCENT(), "NUMBER GUESSING GAME");
  
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
    client.print(GET_WARNING());
    client.println(F("Too low! Try again."));
    client.print(CLR_RESET);
  } else if (guess > session.guessNumber) {
    client.print(GET_WARNING());
    client.println(F("Too high! Try again."));
    client.print(CLR_RESET);
  } else {
    client.println();
    client.print(GET_SUCCESS());
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
  client.print(F("\033[3J\033[2J\033[H"));
  drawBox(GET_ACCENT(), "HIGH SCORES");
  
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
  client.print(F("\033[3J\033[2J\033[H"));
  session.currentMenu = MENU_UTILITIES;
  
  drawBox(GET_SECONDARY(), "UTILITIES");
  
  client.println();
  client.print(GET_SECONDARY());
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
  client.print(F("\033[3J\033[2J\033[H"));
  drawBox(GET_SECONDARY(), "SYSTEM LOG");
  
  client.println();
  client.print(GET_SECONDARY());
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
  client.print(F("\033[3J\033[2J\033[H"));
  session.currentMenu = MENU_MESSAGES;
  
  drawBox(GET_ACCENT(), "MESSAGE BOARD");
  
  client.println();
  client.print(GET_SECONDARY());
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
  client.print(GET_SECONDARY());
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
  client.print(F("\033[3J\033[2J\033[H"));
  drawBox(GET_ACCENT(), "MESSAGE BOARD");
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
      client.print(GET_WARNING());
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
      
      client.print(GET_WARNING());
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
  client.print(GET_SUCCESS());
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
    client.print(GET_SUCCESS());
    client.println(F("✓ Message posted!"));
    client.print(CLR_RESET);
  } else {
    client.println();
    client.print(GET_ERROR());
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
  client.print(F("\033[3J\033[2J\033[H"));
  session.currentMenu = MENU_FILES;
  
  drawBox(GET_SECONDARY(), "FILE MANAGER");
  
  client.println();
  client.print(GET_SECONDARY());
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
  client.print(F("\033[3J\033[2J\033[H"));
  drawBox(GET_SECONDARY(), "FILE LISTING");
  client.println();
  
  const char* dirs[] = {"notes", "messages", "logs", "news"};
  
  for (int i = 0; i < 4; i++) {
    client.print(GET_WARNING());
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
  client.print(F("\033[3J\033[2J\033[H"));
  session.currentMenu = MENU_STATS;
  
  drawBox(GET_ACCENT(), "SYSTEM STATISTICS");
  
  client.println();
  client.print(GET_SECONDARY());
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
  
  client.print(GET_SECONDARY());
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
  
  client.print(GET_SECONDARY());
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
  
  client.print(GET_SECONDARY());
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
  client.print(F("\033[3J\033[2J\033[H"));
  session.currentMenu = MENU_SETTINGS;
  
  const Theme& t = themes[currentTheme];
  
  client.print(t.accent);
  client.println(F("  ╔═════════════════════════════════════════════════════════╗"));
  client.println(F("  │                                                         │"));
  client.println(F("  │  [1] User Management  - View/add users (admin)          │"));
  client.println(F("  │  [2] Change Password  - Update your password            │"));
  client.println(F("  │  [3] Theme Selector   - Change color scheme             │"));  // ADD THIS
  client.println(F("  │  [4] About System     - System information              │"));  // UPDATE NUMBER
  client.println(F("  │                                                         │"));
  client.println(F("  │  [0] Back             - Return to previous menu         │"));
  client.println(F("  │                                                         │"));
  client.println(F("  └─────────────────────────────────────────────────────────┘"));
  client.print(CLR_RESET);
  
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
      changePassword();
      break;
    case 3:
      session.currentMenu = MENU_THEME_SELECTOR;  // SET MENU STATE
      showThemeMenu();
      break;
    case 4:
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
  client.print(F("\033[3J\033[2J\033[H"));
  drawBox(GET_ACCENT(), "ABOUT THIS SYSTEM");
  
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
  client.print(F("\033[3J\033[2J\033[H"));
  drawBox(GET_ERROR(), "USER MANAGEMENT");
  
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
            client.print(GET_WARNING());
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
  client.print(F("\033[3J\033[2J\033[H"));
  drawBox(GET_SECONDARY(), "NETWORK INFORMATION");
  
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
  client.print(F("\033[3J\033[2J\033[H"));
  session.currentMenu = MENU_NEWS;
  
  drawBox(GET_ACCENT(), "NEWS READER");
  
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
  client.print(F("\033[3J\033[2J\033[H"));
  session.currentMenu = MENU_COMMAND;
  
  drawBox(GET_ERROR(), "COMMAND MODE");
  
  client.println();
  client.print(GET_WARNING());
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
    client.print(F("\033[3J\033[2J\033[H"));
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
  client.print(GET_SUCCESS());
  client.print(session.username);
  client.print(GET_ACCENT());
  client.print(F("@ArduinoOS"));
  client.print(CLR_RESET);
  client.print(F(" $ "));
}

void showPrompt() {
  client.print(GET_SUCCESS());
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
  client.print(GET_TITLE());
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

// ==================== SECURE MAIL SYSTEM ====================

uint32_t generateHardwareKey() {
  uint32_t seed = 0;
  
  // Read analog noise from unconnected pins (A0-A3 are usually safe)
  for(int i = 0; i < 4; i++) {
    seed ^= (analogRead(A0 + i) << (i * 8));
    delay(5);
  }
  
  // Mix with MAC address for board uniqueness
  for(int i = 0; i < 6; i++) {
    seed ^= ((uint32_t)mac[i] << ((i % 4) * 8));
  }
  
  // Add username hash for user uniqueness
  for(int i = 0; session.username[i] != '\0'; i++) {
    seed ^= ((uint32_t)session.username[i] << ((i % 4) * 8));
  }
  
  // Mix with current millis for additional entropy
  seed ^= millis();
  
  return seed;
}

void encryptMessage(char* message, uint32_t key) {
  int len = strlen(message);
  for(int i = 0; i < len; i++) {
    // XOR with rotating key bytes
    message[i] ^= (key >> ((i % 4) * 8)) & 0xFF;
  }
}

void decryptMessage(char* message, uint32_t key) {
  // XOR encryption is symmetric, so decrypt is same as encrypt
  encryptMessage(message, key);
}

int countUnreadMail() {
  char inboxPath[40];
  sprintf(inboxPath, "mail/%s/inbox", session.username);
  
  File inbox = SD.open(inboxPath);
  if (!inbox) return 0;
  
  int count = 0;
  while (true) {
    File entry = inbox.openNextFile();
    if (!entry) break;
    
    if (!entry.isDirectory() && !isMessageRead(entry.name())) {
      count++;
    }
    entry.close();
  }
  inbox.close();
  
  return count;
}

bool isMessageRead(const char* filename) {
  // Check if .read marker file exists
  char readMarker[60];
  sprintf(readMarker, "mail/%s/inbox/%s.read", session.username, filename);
  return SD.exists(readMarker);
}

void markMessageRead(const char* filename) {
  char readMarker[60];
  sprintf(readMarker, "mail/%s/inbox/%s.read", session.username, filename);
  
  File marker = SD.open(readMarker, FILE_WRITE);
  if (marker) {
    marker.println("read");
    marker.close();
  }
}

void showSecureMailMenu() {
  client.print(F("\033[3J\033[2J\033[H"));
  session.currentMenu = MENU_SECURE_MAIL;
  
  drawBox(GET_ACCENT(), "SECURE MAIL SYSTEM");
  
  client.println();
  
  int unreadCount = countUnreadMail();
  
  client.print(GET_SECONDARY());
  client.print(F("  User: "));
  client.print(GET_TITLE());
  client.print(session.username);
  client.print(GET_SECONDARY());
  client.print(F("  |  Encryption: "));
  client.print(GET_SUCCESS());
  client.println(F("ACTIVE"));
  
  if (unreadCount > 0) {
    client.print(GET_WARNING());
    client.print(F("  You have "));
    client.print(unreadCount);
    client.println(F(" unread message(s)!"));
  }
  
  client.print(CLR_RESET);
  client.println();
  
  client.print(GET_SECONDARY());
  client.println(F("  ┌─────────────────────────────────────────────────────────┐"));
  client.println(F("  │                                                         │"));
  client.println(F("  │  [1] Inbox            - Read incoming mail              │"));
  client.println(F("  │  [2] Compose Message  - Send encrypted mail             │"));
  client.println(F("  │  [3] Sent Messages    - View sent mail                  │"));
  client.println(F("  │  [4] Encryption Key   - View/regenerate key             │"));
  client.println(F("  │                                                         │"));
  client.println(F("  │  [0] Back to Main Menu                                  │"));
  client.println(F("  │                                                         │"));
  client.println(F("  └─────────────────────────────────────────────────────────┘"));
  client.print(CLR_RESET);
  client.println();
  
  client.print(GET_WARNING());
  client.println(F("  ⚠ Hardware-encrypted messages can only be read on this"));
  client.println(F("    Arduino Mega 2560 platform by the intended recipient."));
  client.print(CLR_RESET);
  client.println();
  
  showPrompt();
}

void handleSecureMailChoice(int choice) {
  switch (choice) {
    case 1:
      showInbox();
      break;
    case 2:
      composeMail();
      break;
    case 3:
      showSentMail();
      break;
    case 4:
      regenerateEncryptionKey();
      break;
    case 0:
      showMainMenu();
      break;
    default:
      client.println(F("Invalid choice."));
      showPrompt();
  }
}

void showInbox() {
  client.print(F("\033[3J\033[2J\033[H"));
  drawBox(GET_ACCENT(), "INBOX");
  client.println();
  
  char inboxPath[40];
  sprintf(inboxPath, "mail/%s/inbox", session.username);
  
  File inbox = SD.open(inboxPath);
  if (!inbox) {
    client.println(F("  No messages."));
    client.println();
    client.println(F("Press Enter to continue..."));
    session.waitingForContinue = true;
    session.returnToMenu = MENU_SECURE_MAIL;
    return;
  }
  
  // Count actual messages first
  int totalMessages = 0;
  while (true) {
    File entry = inbox.openNextFile();
    if (!entry) break;
    
    if (!entry.isDirectory() && strstr(entry.name(), ".read") == NULL) {
      totalMessages++;
    }
    entry.close();
  }
  inbox.close();
  
  if (totalMessages == 0) {
    client.println(F("  Inbox is empty."));
    client.println();
    client.println(F("Press Enter to continue..."));
    session.waitingForContinue = true;
    session.returnToMenu = MENU_SECURE_MAIL;
    return;
  }
  
  // Reopen and display messages
  inbox = SD.open(inboxPath);
  int msgCount = 0;
  
  while (true) {
    File entry = inbox.openNextFile();
    if (!entry) break;
    
    if (!entry.isDirectory() && strstr(entry.name(), ".read") == NULL) {
      bool unread = !isMessageRead(entry.name());
      
      if (unread) {
        client.print(GET_WARNING());
        client.print(F("  [NEW] "));
      } else {
        client.print(F("  [ ✓ ] "));
      }
      
      client.print(GET_TITLE());
      client.print(F("["));
      client.print(msgCount + 1);
      client.print(F("] "));
      
      // Read first line (from:)
      char line[80];
      int idx = 0;
      bool foundFrom = false;
      
      while (entry.available() && idx < 79) {
        char c = entry.read();
        if (c == '\n' || c == '\r') {
          line[idx] = '\0';
          if (strncmp(line, "From:", 5) == 0) {
            foundFrom = true;
            break;
          }
          idx = 0;
        } else {
          line[idx++] = c;
        }
      }
      
      if (foundFrom) {
        client.print(line);
      } else {
        client.print(entry.name());
      }
      
      client.print(CLR_RESET);
      client.println();
      
      msgCount++;
    }
    entry.close();
  }
  inbox.close();
  
  client.println();
  client.print(F("  Enter message number to read, or press Enter to return: "));
  
  char numStr[5];
  int idx = 0;
  unsigned long startTime = millis();
  bool gotInput = false;
  
  while (millis() - startTime < 30000) {
    if (client.available()) {
      char c = client.read();
      if (c == '\n' || c == '\r') {
        // Clear any additional newlines
        delay(10);
        while (client.available()) {
          char peek = client.peek();
          if (peek == '\r' || peek == '\n') {
            client.read();
          } else {
            break;
          }
        }
        
        if (gotInput) {
          numStr[idx] = '\0';
          break;
        } else {
          // User just pressed Enter without typing a number
          showSecureMailMenu();
          return;
        }
      } else if (c == 8 || c == 127) {
        if (idx > 0) {
          idx--;
          gotInput = (idx > 0);
          client.write(8);
          client.write(' ');
          client.write(8);
        }
      } else if (idx < 4 && c >= '0' && c <= '9') {
        numStr[idx++] = c;
        gotInput = true;
        client.write(c);
      }
    }
  }
  
  if (!gotInput) {
    showSecureMailMenu();
    return;
  }
  
  int msgNum = atoi(numStr);
  
  if (msgNum < 1 || msgNum > msgCount) {
    client.println();
    client.println(F("Invalid message number."));
    delay(1500);
    showInbox();
    return;
  }
  
  // Find and display the message
  inbox = SD.open(inboxPath);
  int currentMsg = 0;
  bool found = false;
  
  while (true) {
    File entry = inbox.openNextFile();
    if (!entry) break;
    
    if (!entry.isDirectory() && strstr(entry.name(), ".read") == NULL) {
      currentMsg++;
      
      if (currentMsg == msgNum) {
        client.println();
        client.println();
        client.print(GET_SECONDARY());
        client.println(F("  ═══════════════════════════════════════════════════════════"));
        client.print(CLR_RESET);
        client.print(F("  "));
        
        // Read and decrypt message
        char encryptedMsg[400];
        int msgIdx = 0;
        bool inBody = false;
        int dashCount = 0;
        
        while (entry.available()) {
          char c = entry.read();
          
          // Look for message body (after "---")
          if (!inBody) {
            if (c == '-') {
              dashCount++;
              if (dashCount == 3) {
                // Skip to next line
                while (entry.available() && entry.peek() != '\n') entry.read();
                if (entry.available()) entry.read(); // consume \n
                inBody = true;
                dashCount = 0;
                continue;
              }
            } else {
              dashCount = 0;
            }
            // Display header
            if (c == '\n') {
              client.println();
              client.print(F("  "));
            } else if (c != '\r') {
              client.write(c);
            }
          } else {
            // Collect encrypted body
            if (msgIdx < 399) {
              encryptedMsg[msgIdx++] = c;
            }
          }
        }
        encryptedMsg[msgIdx] = '\0';
        
        // Decrypt
        decryptMessage(encryptedMsg, session.userEncryptionKey);
        
        client.println();
        client.print(GET_SECONDARY());
        client.println(F("  ───────────────────────────────────────────────────────────"));
        client.print(CLR_RESET);
        client.print(F("  "));
        
        // Display decrypted message
        for (int i = 0; i < msgIdx; i++) {
          if (encryptedMsg[i] == '\n') {
            client.println();
            client.print(F("  "));
          } else if (encryptedMsg[i] != '\r') {
            client.write(encryptedMsg[i]);
          }
        }
        
        client.println();
        client.print(GET_SECONDARY());
        client.println(F("  ═══════════════════════════════════════════════════════════"));
        client.print(CLR_RESET);
        
        // Mark as read
        markMessageRead(entry.name());
        
        found = true;
        entry.close();
        break;
      }
    }
    entry.close();
  }
  inbox.close();
  
  if (!found) {
    client.println();
    client.println(F("Message not found."));
    delay(1500);
    showInbox();
    return;
  }
  
  client.println();
  client.println(F("Press Enter to continue..."));
  session.waitingForContinue = true;
  session.returnToMenu = MENU_SECURE_MAIL;
}

void composeMail() {
  client.print(F("\033[3J\033[2J\033[H"));
  session.currentMenu = MENU_MAIL_COMPOSE;
  
  drawBox(GET_PRIMARY(), "COMPOSE SECURE MAIL");
  
  client.println();
  
  // Show available users
  client.print(GET_WARNING());
  client.println(F("  Available Recipients:"));
  client.print(CLR_RESET);
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
          
          if (user && strcmp(user, session.username) != 0) {
            client.print(F("    • "));
            client.println(user);
          }
          idx = 0;
        }
      } else if (idx < 63) {
        line[idx++] = c;
      }
    }
    f.close();
  }
  
  client.println();
  client.print(GET_SUCCESS());
  client.print(F("  To: "));
  client.print(CLR_RESET);
  
  char recipient[MAX_USERNAME];
  int recipIdx = 0;
  unsigned long startTime = millis();
  
  while (millis() - startTime < 60000) {
    if (client.available()) {
      char c = client.read();
      if (c == '\n' || c == '\r') {
        recipient[recipIdx] = '\0';
        break;
      } else if (c == 8 || c == 127) {
        if (recipIdx > 0) {
          recipIdx--;
          client.write(8);
          client.write(' ');
          client.write(8);
        }
      } else if (recipIdx < MAX_USERNAME - 1 && c >= 32) {
        recipient[recipIdx++] = c;
        client.write(c);
      }
    }
  }
  
  if (recipIdx == 0) {
    client.println();
    client.println(F("Cancelled."));
    delay(1500);
    showSecureMailMenu();
    return;
  }
  
  // Verify recipient exists
  bool recipientExists = false;
  f = SD.open("users.txt", FILE_READ);
  if (f) {
    char line[64];
    int idx = 0;
    
    while (f.available()) {
      char c = f.read();
      if (c == '\n' || c == '\r') {
        if (idx > 0) {
          line[idx] = '\0';
          char* user = strtok(line, ":");
          
          if (user && strcmp(user, recipient) == 0) {
            recipientExists = true;
            break;
          }
          idx = 0;
        }
      } else if (idx < 63) {
        line[idx++] = c;
      }
    }
    f.close();
  }
  
  if (!recipientExists) {
    client.println();
    client.print(GET_ERROR());
    client.println(F("✗ User not found!"));
    client.print(CLR_RESET);
    delay(2000);
    showSecureMailMenu();
    return;
  }
  
  client.println();
  client.print(F("  Subject: "));
  
  char subject[60];
  int subjIdx = 0;
  startTime = millis();
  
  while (millis() - startTime < 60000) {
    if (client.available()) {
      char c = client.read();
      if (c == '\n' || c == '\r') {
        subject[subjIdx] = '\0';
        break;
      } else if (c == 8 || c == 127) {
        if (subjIdx > 0) {
          subjIdx--;
          client.write(8);
          client.write(' ');
          client.write(8);
        }
      } else if (subjIdx < 59 && c >= 32) {
        subject[subjIdx++] = c;
        client.write(c);
      }
    }
  }
  
  client.println();
  client.println();
  client.print(GET_WARNING());
  client.println(F("  Enter your message (max 300 chars):"));
  client.print(CLR_RESET);
  client.print(F("  > "));
  
  char message[301];
  int msgIdx = 0;
  startTime = millis();
  
  while (millis() - startTime < 120000) {
    if (client.available()) {
      char c = client.read();
      if (c == '\n' || c == '\r') {
        message[msgIdx] = '\0';
        break;
      } else if (c == 8 || c == 127) {
        if (msgIdx > 0) {
          msgIdx--;
          client.write(8);
          client.write(' ');
          client.write(8);
        }
      } else if (msgIdx < 300 && c >= 32) {
        message[msgIdx++] = c;
        client.write(c);
      }
    }
  }
  
  if (msgIdx == 0) {
    client.println();
    client.println(F("Cancelled."));
    delay(1500);
    showSecureMailMenu();
    return;
  }
  
  // Encrypt the message
  encryptMessage(message, session.userEncryptionKey);
  
  // Generate unique filename
  int mailNum = 0;
  char filename[60];
  
  while (mailNum < 1000) {
    sprintf(filename, "mail/%s/inbox/msg%03d.txt", recipient, mailNum);
    if (!SD.exists(filename)) break;
    mailNum++;
  }
  
  // Save to recipient's inbox
  File mailFile = SD.open(filename, FILE_WRITE);
  if (mailFile) {
    mailFile.print("From: ");
    mailFile.println(session.username);
    mailFile.print("Subject: ");
    mailFile.println(subject);
    mailFile.print("Time: ");
    printUptimeToFile(mailFile, (millis() - sysStats.bootTime) / 1000);
    mailFile.println();
    mailFile.println("---");
    mailFile.println(message);
    mailFile.close();
    
    // Save copy to sent folder
    sprintf(filename, "mail/%s/sent/sent%03d.txt", session.username, mailNum);
    mailFile = SD.open(filename, FILE_WRITE);
    if (mailFile) {
      mailFile.print("To: ");
      mailFile.println(recipient);
      mailFile.print("Subject: ");
      mailFile.println(subject);
      mailFile.print("Time: ");
      printUptimeToFile(mailFile, (millis() - sysStats.bootTime) / 1000);
      mailFile.println();
      mailFile.println("---");
      
      // Decrypt before saving to sent (so sender can read it)
      decryptMessage(message, session.userEncryptionKey);
      mailFile.println(message);
      mailFile.close();
    }
    
    client.println();
    client.println();
    client.print(GET_SUCCESS());
    client.println(F("  ✓ Encrypted message sent successfully!"));
    client.print(CLR_RESET);
  } else {
    client.println();
    client.print(GET_ERROR());
    client.println(F("  ✗ Error sending message!"));
    client.print(CLR_RESET);
  }
  
  delay(2500);
  showSecureMailMenu();
}

void handleComposeMailInput() {
  // This is handled directly in composeMail()
  showSecureMailMenu();
}

void showSentMail() {
  client.print(F("\033[3J\033[2J\033[H"));
  drawBox(GET_SECONDARY(), "SENT MESSAGES");
  client.println();
  
  char sentPath[40];
  sprintf(sentPath, "mail/%s/sent", session.username);
  
  File sentBox = SD.open(sentPath);
  if (!sentBox) {
    client.println(F("  No sent messages."));
    client.println();
    client.println(F("Press Enter to continue..."));
    session.waitingForContinue = true;
    session.returnToMenu = MENU_SECURE_MAIL;
    return;
  }
  
  int count = 0;
  while (true) {
    File entry = sentBox.openNextFile();
    if (!entry) break;
    
    if (!entry.isDirectory()) {
      client.print(GET_WARNING());
      client.print(F("  ┌─[ Message #"));
      client.print(count++);
      client.println(F(" ]─────────────────────────────"));
      client.print(CLR_RESET);
      
      bool startOfLine = true;
      while (entry.available()) {
        char c = entry.read();
        if (c == '\n') {
          client.println();
          startOfLine = true;
        } else if (c != '\r') {
          if (startOfLine) {
            client.print(F("  │ "));
            startOfLine = false;
          }
          client.write(c);
        }
      }
      
      client.print(GET_WARNING());
      client.println(F("  └────────────────────────────────────────────"));
      client.print(CLR_RESET);
      client.println();
    }
    entry.close();
  }
  sentBox.close();
  
  if (count == 0) {
    client.println(F("  No sent messages."));
  }
  
  client.println();
  client.println(F("Press Enter to continue..."));
  session.waitingForContinue = true;
  session.returnToMenu = MENU_SECURE_MAIL;
}

void regenerateEncryptionKey() {
  client.print(F("\033[3J\033[2J\033[H"));
  drawBox(GET_WARNING(), "ENCRYPTION KEY");
  
  client.println();
  client.print(GET_SECONDARY());
  client.println(F("  Hardware-Generated Encryption Key"));
  client.print(CLR_RESET);
  client.println(F("  ───────────────────────────────────────────────────────────"));
  client.println();
  
  client.print(F("  Current Key: "));
  client.print(GET_SUCCESS());
  client.print(F("0x"));
  client.println(session.userEncryptionKey, HEX);
  client.print(CLR_RESET);
  client.println();
  
  client.print(GET_WARNING());
  client.println(F("  This key is generated from:"));
  client.println(F("  • Arduino Mega 2560 analog pin noise"));
  client.println(F("  • Ethernet shield MAC address"));
client.println(F("  • Your username hash"));
client.println(F("  • System timing entropy"));
client.println();
client.println(F("  This combination makes your messages readable ONLY on"));
client.println(F("  this specific Arduino Mega hardware platform."));
client.print(CLR_RESET);
client.println();
client.print(GET_ERROR());
client.print(F("  Regenerate key? (y/n): "));
client.print(CLR_RESET);
char response[10];
int idx = 0;
unsigned long startTime = millis();
while (millis() - startTime < 10000) {
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
client.println();
if (idx > 0 && (response[0] == 'y' || response[0] == 'Y')) {
uint32_t oldKey = session.userEncryptionKey;
session.userEncryptionKey = generateHardwareKey();
client.println();
client.print(GET_SUCCESS());
client.print(F("  ✓ New key generated: 0x"));
client.println(session.userEncryptionKey, HEX);
client.print(CLR_RESET);
client.println();
client.print(GET_ERROR());
client.println(F("  ⚠ WARNING: Old messages may not decrypt correctly!"));
client.print(CLR_RESET);
} else {
client.println(F("  Cancelled."));
}
client.println();
client.println(F("Press Enter to continue..."));
session.waitingForContinue = true;
session.returnToMenu = MENU_SECURE_MAIL;
}

// ==================== END SECURE MAIL SYSTEM ====================

void changePassword() {
  client.print(F("\033[3J\033[2J\033[H"));
  drawBox(GET_WARNING(), "CHANGE PASSWORD");
  
  client.println();
  client.print(GET_SUCCESS());
  client.print(F("Enter current password: "));
  client.print(CLR_RESET);
  
  char currentPass[MAX_PASSWORD];
  int idx = 0;
  unsigned long startTime = millis();
  
  while (millis() - startTime < 30000) {
    if (client.available()) {
      char c = client.read();
      if (c == '\n' || c == '\r') {
        currentPass[idx] = '\0';
        break;
      } else if (c == 8 || c == 127) {
        if (idx > 0) {
          idx--;
          client.write(8);
          client.write(' ');
          client.write(8);
        }
      } else if (idx < MAX_PASSWORD - 1 && c >= 32) {
        currentPass[idx++] = c;
        client.write('*');
      }
    }
  }
  
  // ADD THIS: Clear any remaining characters in the buffer
  delay(50);
  while (client.available()) {
    client.read();
  }
  
  // Verify current password
  if (!verifyPassword(session.username, currentPass)) {
    client.println();
    client.print(GET_ERROR());
    client.println(F("\n✗ Incorrect current password!"));
    client.print(CLR_RESET);
    delay(2000);
    showSettingsMenu();
    return;
  }
  
  client.println();
  client.print(F("Enter new password: "));
  
  char newPass[MAX_PASSWORD];
  idx = 0;
  startTime = millis();
  
  while (millis() - startTime < 30000) {
    if (client.available()) {
      char c = client.read();
      if (c == '\n' || c == '\r') {
        newPass[idx] = '\0';
        break;
      } else if (c == 8 || c == 127) {
        if (idx > 0) {
          idx--;
          client.write(8);
          client.write(' ');
          client.write(8);
        }
      } else if (idx < MAX_PASSWORD - 1 && c >= 32) {
        newPass[idx++] = c;
        client.write('*');
      }
    }
  }
  
  // ADD THIS: Clear buffer again
  delay(50);
  while (client.available()) {
    client.read();
  }
  
  if (idx < 6) {
    client.println();
    client.print(GET_ERROR());
    client.println(F("\n✗ Password must be at least 6 characters!"));
    client.print(CLR_RESET);
    delay(2000);
    showSettingsMenu();
    return;
  }
  
  client.println();
  client.print(F("Confirm new password: "));
  
  char confirmPass[MAX_PASSWORD];
  idx = 0;
  startTime = millis();
  
  while (millis() - startTime < 30000) {
    if (client.available()) {
      char c = client.read();
      if (c == '\n' || c == '\r') {
        confirmPass[idx] = '\0';
        break;
      } else if (c == 8 || c == 127) {
        if (idx > 0) {
          idx--;
          client.write(8);
          client.write(' ');
          client.write(8);
        }
      } else if (idx < MAX_PASSWORD - 1 && c >= 32) {
        confirmPass[idx++] = c;
        client.write('*');
      }
    }
  }
  
  // ADD THIS: Final buffer clear
  delay(50);
  while (client.available()) {
    client.read();
  }
  
  if (strcmp(newPass, confirmPass) != 0) {
    client.println();
    client.print(GET_ERROR());
    client.println(F("\n✗ Passwords do not match!"));
    client.print(CLR_RESET);
    delay(2000);
    showSettingsMenu();
    return;
  }
  
  // Update password in users.txt
  if (updateUserPassword(session.username, newPass)) {
    client.println();
    client.print(GET_SUCCESS());
    client.println(F("\n✓ Password changed successfully!"));
    client.print(CLR_RESET);
  } else {
    client.println();
    client.print(GET_ERROR());
    client.println(F("\n✗ Error updating password!"));
    client.print(CLR_RESET);
  }
  
  delay(2000);
  showSettingsMenu();
}

bool verifyPassword(const char* username, const char* password) {
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
        
        if (user && pass && strcmp(user, username) == 0 && strcmp(pass, password) == 0) {
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

bool updateUserPassword(const char* username, const char* newPassword) {
  // Read all users into memory
  File fRead = SD.open("users.txt", FILE_READ);
  if (!fRead) return false;
  
  File fTemp = SD.open("temp.txt", FILE_WRITE);
  if (!fTemp) {
    fRead.close();
    return false;
  }
  
  char line[64];
  int idx = 0;
  
  while (fRead.available()) {
    char c = fRead.read();
    if (c == '\n' || c == '\r') {
      if (idx > 0) {
        line[idx] = '\0';
        char lineCopy[64];
        strcpy(lineCopy, line);
        
        char* user = strtok(line, ":");
        strtok(NULL, ":");
        char* admin = strtok(NULL, ":");
        
        if (user && strcmp(user, username) == 0) {
          // Write updated line
          fTemp.print(user);
          fTemp.print(":");
          fTemp.print(newPassword);
          fTemp.print(":");
          fTemp.println(admin ? admin : "0");
        } else {
          // Write original line
          fTemp.println(lineCopy);
        }
        idx = 0;
      }
    } else if (idx < 63) {
      line[idx++] = c;
    }
  }
  
  fRead.close();
  fTemp.close();
  
  // Replace original file
  SD.remove("users.txt");
  File fTempRead = SD.open("temp.txt", FILE_READ);
  File fNew = SD.open("users.txt", FILE_WRITE);
  
  if (fTempRead && fNew) {
    while (fTempRead.available()) {
      fNew.write(fTempRead.read());
    }
    fTempRead.close();
    fNew.close();
  }
  
  SD.remove("temp.txt");
  return true;
}

void loadTheme() {
  File f = SD.open("theme.txt", FILE_READ);
  if (f) {
    char line[32];
    int idx = 0;
    
    while (f.available() && idx < 31) {
      char c = f.read();
      if (c == '\n' || c == '\r') {
        line[idx] = '\0';
        break;
      }
      line[idx++] = c;
    }
    f.close();
    
    if (idx > 0) {
      currentTheme = atoi(line);
      // Validate theme ID
      if (currentTheme < 0 || currentTheme > 6) {
        currentTheme = 0;
      }
      Serial.print(F("Loaded theme: "));
      Serial.println(currentTheme);
    }
  }
}

void saveTheme() {
  if (SD.exists("theme.txt")) {
    SD.remove("theme.txt");
  }
  
  File f = SD.open("theme.txt", FILE_WRITE);
  if (f) {
    f.println(currentTheme);
    f.close();
  }
}

void applyTheme(int themeId) {
  currentTheme = themeId;
  saveTheme();
}

void previewTheme(int themeId) {
  const Theme& t = themes[themeId];
  
  client.println();
  client.print(t.primary);
  client.print(F("  Primary text: "));
  client.print(t.name);
  client.print(CLR_RESET);
  client.println();
  
  client.print(t.secondary);
  client.println(F("  Secondary text example"));
  client.print(CLR_RESET);
  
  client.print(t.accent);
  client.println(F("  Accent text example"));
  client.print(CLR_RESET);
  
  client.print(t.success);
  client.println(F("  ✓ Success message example"));
  client.print(CLR_RESET);
  
  client.print(t.error);
  client.println(F("  ✗ Error message example"));
  client.print(CLR_RESET);
  
  client.print(t.warning);
  client.println(F("  ⚠ Warning message example"));
  client.print(CLR_RESET);
  
  // Draw a sample box
  client.println();
  client.print(t.accent);
  client.println(F("  ┌─────────────────────────────────────┐"));
  client.print(F("  │ "));
  client.print(t.title);
  client.print(F("Sample Box with Title"));
  client.print(t.accent);
  client.println(F("           │"));
  client.println(F("  ├─────────────────────────────────────┤"));
  client.print(F("  │ "));
  client.print(t.primary);
  client.print(F("This is how content looks"));
  client.print(t.accent);
  client.println(F("        │"));
  client.print(F("  │ "));
  client.print(t.secondary);
  client.print(F("With secondary text mixed in"));
  client.print(t.accent);
  client.println(F("      │"));
  client.println(F("  └─────────────────────────────────────┘"));
  client.print(CLR_RESET);
  client.println();
}

void showThemeMenu() {
  client.print(F("\033[3J\033[2J\033[H"));
  
  const Theme& t = themes[currentTheme];
  
  // Use current theme colors for the menu
  client.print(t.accent);
  client.println(F("  ╔═══════════════════════════════════════════════════════════╗"));
  client.print(F("  ║ "));
  client.print(t.title);
  client.print(F("THEME SELECTOR"));
  client.print(t.accent);
  client.println(F("                                           ║"));
  client.println(F("  ╚═══════════════════════════════════════════════════════════╝"));
  client.print(CLR_RESET);
  
  client.println();
  client.print(t.secondary);
  client.print(F("  Current theme: "));
  client.print(t.success);
  client.println(themes[currentTheme].name);
  client.print(CLR_RESET);
  client.println();
  
  client.print(t.primary);
  client.println(F("  Available Themes:"));
  client.println(F("  ─────────────────────────────────────────────────────────"));
  client.print(CLR_RESET);
  client.println();
  
  // List all themes
  for (int i = 0; i < 7; i++) {
    if (i == currentTheme) {
      client.print(t.success);
      client.print(F("  ► ["));
    } else {
      client.print(t.primary);
      client.print(F("    ["));
    }
    client.print(i + 1);
    client.print(F("] "));
    client.print(themes[i].name);
    if (i == currentTheme) {
      client.print(F(" (Active)"));
    }
    client.print(CLR_RESET);
    client.println();
  }
  
  client.println();
  client.print(t.accent);
  client.println(F("  ┌─────────────────────────────────────────────────────────┐"));
  client.println(F("  │                                                         │"));
  client.print(t.primary);
  client.println(F("  │  [1-7] Select Theme   - Choose and preview             │"));
  client.println(F("  │  [P] Preview Current  - See current theme details      │"));
  client.println(F("  │                                                         │"));
  client.println(F("  │  [0] Back             - Return to settings             │"));
  client.println(F("  │                                                         │"));
  client.print(t.accent);
  client.println(F("  └─────────────────────────────────────────────────────────┘"));
  client.print(CLR_RESET);
  client.println();
  
  client.print(t.prompt);
  client.print(F("Choice"));
  client.print(CLR_RESET);
  client.print(F(" > "));
}

void handleThemeChoice(int choice) {
  const Theme& t = themes[currentTheme];
  
  // Check for 'P' command
  if (cmdBuffer[0] == 'p' || cmdBuffer[0] == 'P') {
    client.print(F("\033[3J\033[2J\033[H"));
    client.print(t.accent);
    client.println(F("  ╔═══════════════════════════════════════════════════════════╗"));
    client.print(F("  ║ "));
    client.print(t.title);
    client.print(F("THEME PREVIEW"));
    client.print(t.accent);
    client.println(F("                                            ║"));
    client.println(F("  ╚═══════════════════════════════════════════════════════════╝"));
    client.print(CLR_RESET);
    
    previewTheme(currentTheme);
    
    client.println();
    client.println(F("Press Enter to continue..."));
    session.waitingForContinue = true;
    session.returnToMenu = MENU_SETTINGS;
    return;
  }
  
  if (choice >= 1 && choice <= 7) {
    int newTheme = choice - 1;
    
    client.print(F("\033[3J\033[2J\033[H"));
    client.print(themes[newTheme].accent);
    client.println(F("  ╔═══════════════════════════════════════════════════════════╗"));
    client.print(F("  ║ "));
    client.print(themes[newTheme].title);
    client.print(F("PREVIEW: "));
    client.print(themes[newTheme].name);
    client.print(themes[newTheme].accent);
    
    // Pad the title
    int padding = 45 - strlen(themes[newTheme].name);
    for (int i = 0; i < padding; i++) client.print(F(" "));
    
    client.println(F("║"));
    client.println(F("  ╚═══════════════════════════════════════════════════════════╝"));
    client.print(CLR_RESET);
    
    previewTheme(newTheme);
    
    client.println();
    client.print(themes[newTheme].secondary);
    client.print(F("Apply this theme? (y/n): "));
    client.print(CLR_RESET);
    
    char response[10];
    int idx = 0;
    unsigned long startTime = millis();
    
    while (millis() - startTime < 10000) {
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
    
    client.println();
    
    if (idx > 0 && (response[0] == 'y' || response[0] == 'Y')) {
      applyTheme(newTheme);
      client.print(themes[newTheme].success);
      client.print(F("✓ Theme changed to: "));
      client.println(themes[newTheme].name);
      client.print(CLR_RESET);
      delay(2000);
    } else {
      client.print(t.warning);
      client.println(F("Theme not changed."));
      client.print(CLR_RESET);
      delay(1500);
    }
    
    showThemeMenu();
  } else if (choice == 0) {
    showSettingsMenu();
  } else {
    client.print(t.error);
    client.println(F("Invalid choice."));
    client.print(CLR_RESET);
    delay(1000);
    showThemeMenu();
  }
}

void logout() {
  client.println();
  client.print(GET_SUCCESS());
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
