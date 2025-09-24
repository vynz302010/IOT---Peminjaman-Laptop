#include <WiFiManager.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2); // I2C LCD address 0x27
#include "wifimanagerr.h"
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

#include <MFRC522.h>
//#include <WiFi.h>


//#define BOT_TOKEN "5591950768:AAFXhbTBAVZMMarvQhL-Lz8FyOQAMLsQCMA"   // Token bot dari @BotFather
//#define CHAT_ID   "1567178914"       // Chat ID admin

WiFiClientSecure secured_client;
UniversalTelegramBot* bot;  // pakai pointer



// Pin Definitions
#define RST_PIN         4          // RST pin for RFID
#define SS_PIN          5          // SDA pin for RFID (default for SPI)
#define BUZZER_PIN      32         // Pin to control solenoid via MOSFET
#define RFID_TIMEOUT    10000      // Timeout in milliseconds for RFID attempts
unsigned long lastCheck = 0;
const int checkInterval = 2000;

// Tambahan variabel global
bool waitingConfirm = false;
String waitingUID = "";
unsigned long confirmStartTime = 0;
//const unsigned long CONFIRM_TIMEOUT = 5000; // 5 detik

// RFID and User Data
MFRC522 rfid(SS_PIN, RST_PIN);     // Create MFRC522 instance



// Variables
bool doorUnlocked = false;
unsigned long lastAttemptTime = 0;
int invalidAttempts = 0;

void setup() {
  // Initialize serial communication
  Serial.begin(115200);

  SPI.begin();                     // Init SPI bus
  rfid.PCD_Init();                 // Init MFRC522
  pinMode(BUZZER_PIN, OUTPUT);     // MOSFET pin to control solenoid
  digitalWrite(BUZZER_PIN, HIGH);   // Ensure solenoid is locked

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");
  int_wifimanager();
  secured_client.setInsecure(); // skip SSL cert check
bot = new UniversalTelegramBot(botToken, secured_client);
  lcd.setCursor(0, 1);
  lcd.print("Ready");
  delay(1000);
      lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Pinjaman Laptop");
    lcd.setCursor(2, 1);
    lcd.print("Tap Your Card");
    int_webserver();
    secured_client.setInsecure(); // supaya HTTPS Telegram bisa jalan
  // Sinkronisasi waktu (buat getDateTime)
  configTime(7 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  delay(3000);
  showIdleScreen();
}

void loop() {
  // Check if RFID card is present
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    String cardID = getRFIDUID();
    Serial.println(cardID);
    lcd.clear();
    
    processCard(cardID);  // <<-- pakai fungsi baru


    // Halt detection for a short period to avoid multiple triggers
    delay(500);
  }

if (waitingConfirm) {
  if (millis() - confirmStartTime > CONFIRM_TIMEOUT) {
    // Timeout → gagal konfirmasi
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Pengembalian");
    lcd.setCursor(0, 1);
    lcd.print("Gagal!");
    beep();
    sendTelegram("❌ Pengembalian UID " + waitingUID + " gagal (timeout)");
    delay(3000);
    showIdleScreen();

    waitingConfirm = false;
    waitingUID = "";
  }
}



  // cek pesan telegram
  if (millis() - lastCheck > checkInterval) {
    int numNewMessages = bot->getUpdates(bot->last_message_received + 1);
    while(numNewMessages) {
      for (int i=0; i<numNewMessages; i++) {
        String text = bot->messages[i].text;
        handleTelegramMessage(text);
      }
      numNewMessages = bot->getUpdates(bot->last_message_received + 1);
    }
    lastCheck = millis();
  }


}

// Function to retrieve RFID UID as a string
String getRFIDUID() {
  String uidString = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    uidString += "0x" + String(rfid.uid.uidByte[i], HEX);
  }
  rfid.PICC_HaltA();  // Halt PICC to prevent further reading
  return uidString;
}

int checkValidRFID(String cardID) {
    File file = SPIFFS.open("/data.txt", "r");
    if (!file) {
        Serial.println("Failed to open data file");
        return -1;
    }

    String line;
    int index = 0;
    while (file.available()) {
        line = file.readStringUntil('\n');
        line.trim();

        int commaIndex = line.indexOf(',');
        if (commaIndex == -1) continue; // skip kalau baris rusak

        String name = line.substring(0, commaIndex);
        String uid  = line.substring(commaIndex + 1);

        if (cardID == uid) {
            Serial.println("UID valid: " + cardID + " (" + name + ")");
            file.close();
            return index;
        }
        index++;
    }

    file.close();
    return -1; // ga ketemu
}





// Function to handle invalid attempts
void invalidAttempt() {
  lcd.setCursor(2, 0);
  lcd.print("RFID Invalid");
  lcd.setCursor(3, 1);
  lcd.print("Try Again");

  invalidAttempts++;
  lastAttemptTime = millis();
}

// Function to center the text on the LCD
String centerText(String text, int width) {
  int padding = (width - text.length()) / 2;
  String paddedText = "";
  for (int i = 0; i < padding; i++) {
    paddedText += " ";
  }
  paddedText += text;
  return paddedText;
}

void processCard(String uid) {
    String name, kelas;
    int status;

    if (!getUserData(uid, name, kelas, status)) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("RFID Invalid");
        Serial.println("[RFID] UID tidak dikenal: " + uid);
        return;
    }

    if (status == 0) {
        // PINJAM
        updateStatus(uid, 1);
        String msg = name + " (" + kelas + ") pinjam laptop " + getDateTime();
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Laptop dipinjam");
        lcd.setCursor(0, 1);
        lcd.print(name);
        beep();
        sendTelegram(msg);
        delay(3000);
        showIdleScreen();
    }
    else if (status == 1) {
        // KEMBALIKAN → tunggu konfirmasi admin
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Konfirmasi Return");
        lcd.setCursor(0, 1);
        lcd.print(name);

        beep();
        String msg = name + " (" + kelas + ") minta kembalikan laptop.\nTekan tombol di bawah untuk ACC.";
        String keyboard = "[[{\"text\":\"✅ Accept\",\"callback_data\":\"ACCEPT_" + uid + "\"}]]";
        
        bot->sendMessageWithInlineKeyboard(chatId, msg, "", keyboard);

        waitingConfirm = true;
        waitingUID = uid;
        confirmStartTime = millis();
    }
}

bool getUserData(String uid, String &name, String &kelas, int &status) {
    File file = SPIFFS.open("/data.txt", "r");
    if (!file) return false;

    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();

        int c1 = line.indexOf(',');
        int c2 = line.indexOf(',', c1 + 1);
        int c3 = line.indexOf(',', c2 + 1);
        if (c1 == -1 || c2 == -1 || c3 == -1) continue;

        name  = line.substring(0, c1);
        kelas = line.substring(c1 + 1, c2);
        String fileUid = line.substring(c2 + 1, c3);
        status = line.substring(c3 + 1).toInt();

        if (uid == fileUid) {
            file.close();
            return true;
        }
    }
    file.close();
    return false;
}

void updateStatus(String uid, int newStatus) {
    File file = SPIFFS.open("/data.txt", "r");
    String newData = "";
    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();

        int c1 = line.indexOf(',');
        int c2 = line.indexOf(',', c1 + 1);
        int c3 = line.indexOf(',', c2 + 1);
        if (c1 == -1 || c2 == -1 || c3 == -1) continue;

        String name  = line.substring(0, c1);
        String kelas = line.substring(c1 + 1, c2);
        String fileUid = line.substring(c2 + 1, c3);
        String status = line.substring(c3 + 1);

        if (fileUid == uid) {
            newData += name + "," + kelas + "," + uid + "," + String(newStatus) + "\n";
        } else {
            newData += line + "\n";
        }
    }
    file.close();

    file = SPIFFS.open("/data.txt", "w");
    if (file) {
        file.print(newData);
        file.close();
    }
}

void handleTelegramMessage(String text) {
    Serial.println("[Telegram] Pesan diterima: " + text);

    if (text == "/start") {
        String helpMsg = "📌 Daftar Perintah:\n";
        helpMsg += "/cek - Lihat siapa saja yang sedang pinjam\n";
        helpMsg += "/accept - Konfirmasi pengembalian aktif\n";
        sendTelegram(helpMsg);
    }
    else if (text == "/cek") {
        File file = SPIFFS.open("/data.txt", "r");
        if (!file) {
            sendTelegram("Gagal membaca data.txt");
            return;
        }

        String borrowers = "📋 Daftar Peminjam:\n";
        bool ada = false;

        while (file.available()) {
            String line = file.readStringUntil('\n');
            line.trim();
            int c1 = line.indexOf(',');
            int c2 = line.indexOf(',', c1 + 1);
            int c3 = line.indexOf(',', c2 + 1);
            if (c1 == -1 || c2 == -1 || c3 == -1) continue;

            String name  = line.substring(0, c1);
            String kelas = line.substring(c1 + 1, c2);
            String uid   = line.substring(c2 + 1, c3);
            int status   = line.substring(c3 + 1).toInt();

            if (status == 1) {
                borrowers += "- " + name + " (" + kelas + ")\n";
                ada = true;
            }
        }
        file.close();

        if (!ada) {
            borrowers = "✅ Tidak ada yang sedang pinjam.";
        }
        sendTelegram(borrowers);
    }
    else if (text == "/accept") {
        if (waitingConfirm && waitingUID != "") {
            updateStatus(waitingUID, 0);
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Pengembalian");
            lcd.setCursor(0, 1);
            lcd.print("accepted");
            beep();
            sendTelegram("✅ Pengembalian UID " + waitingUID + " diterima admin");

            delay(3000);
            showIdleScreen();

            waitingConfirm = false;
            waitingUID = "";
        } else {
            sendTelegram("❌ Tidak ada permintaan konfirmasi aktif.");
        }
    }
    else if (text.startsWith("ACCEPT_")) {
        // tetap bisa handle tombol inline
        String uid = text.substring(7);
        if (waitingConfirm && uid == waitingUID) {
            updateStatus(uid, 0);
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Pengembalian");
            lcd.setCursor(0, 1);
            lcd.print("accepted");
            beep();
            sendTelegram("✅ Pengembalian UID " + uid + " diterima admin");

            delay(3000);
            showIdleScreen();

            waitingConfirm = false;
            waitingUID = "";
        } else {
            sendTelegram("❌ Tidak ada permintaan konfirmasi aktif.");
        }
    }
}


void sendTelegram(String message) {
  bool success = bot->sendMessage(chatId, message, "");
  if (success) {
      Serial.println("[Telegram] Pesan berhasil dikirim");
  } else {
      Serial.println("[Telegram] GAGAL kirim pesan!");
  }
}

String getDateTime() {
  time_t now = time(nullptr);
  struct tm* p_tm = localtime(&now);

  char buffer[30];
  sprintf(buffer, "%02d/%02d %02d:%02d",
          p_tm->tm_mday, p_tm->tm_mon + 1,
          p_tm->tm_hour, p_tm->tm_min);
  return String(buffer);
}

void beep() {
  digitalWrite(BUZZER_PIN, LOW);
  delay(100);                   // bunyi sebentar
  digitalWrite(BUZZER_PIN, HIGH);
}
void showIdleScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Welcome to Return");
  lcd.setCursor(0, 1);
  lcd.print("  Tap Your Card");
}
