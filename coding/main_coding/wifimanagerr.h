  
// ====================== Konfigurasi Awal ======================
#define TRIGGER_PIN 0 // Tombol BOOT di ESP32

char botToken[120] = "YOUR_BOT_TOKEN";
char chatId[32]    = "YOUR_CHAT_ID";
unsigned long CONFIRM_TIMEOUT = 5000;  // default 5 detik


WiFiManager wifiManager;
bool shouldSaveConfig = false;

// ====================== Callback Save Config ======================
void saveConfigCallback() {
    Serial.println(F("Konfigurasi perlu disimpan."));
    shouldSaveConfig = true;
}

// ====================== Load Config dari SPIFFS ======================
void loadConfig_wifimanager() {
    Serial.println(F("Memuat konfigurasi..."));
    if (SPIFFS.begin(true)) {
        if (SPIFFS.exists("/config.json")) {
            File configFile = SPIFFS.open("/config.json", "r");
            if (configFile) {
                DynamicJsonDocument jsonBuffer(512);
                DeserializationError error = deserializeJson(jsonBuffer, configFile);
                if (!error) {
                    strlcpy(botToken, jsonBuffer["bot_token"] | "", sizeof(botToken));
                    strlcpy(chatId, jsonBuffer["chat_id"] | "", sizeof(chatId));
                    CONFIRM_TIMEOUT = jsonBuffer["confirm_timeout"] | 5000;
                }
                configFile.close();
            }
        }
    }
}

// ====================== Save Config ke SPIFFS ======================
void saveConfig_wifimanager() {
    Serial.println(F("Menyimpan konfigurasi..."));
    DynamicJsonDocument jsonBuffer(512);
    jsonBuffer["bot_token"] = botToken;
    jsonBuffer["chat_id"]   = chatId;
     jsonBuffer["confirm_timeout"] = CONFIRM_TIMEOUT;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
        Serial.println(F("Gagal menulis config.json"));
        return;
    }
    serializeJson(jsonBuffer, configFile);
    configFile.close();
    Serial.println(F("Konfigurasi tersimpan!"));
}

// ====================== Setup WiFiManager ======================
void initializeWiFiManager() {
    wifiManager.setSaveConfigCallback(saveConfigCallback);

    WiFiManagerParameter custom_bot_token("bot_token", "Bot Token", botToken, 120);
    WiFiManagerParameter custom_chat_id("chat_id", "Chat ID", chatId, 32);
    char confirm_timeout_str[10];
    snprintf(confirm_timeout_str, sizeof(confirm_timeout_str), "%lu", CONFIRM_TIMEOUT);
    WiFiManagerParameter custom_confirm_timeout("confirm_timeout", "Confirm Timeout (ms)", confirm_timeout_str, 10);
    

    wifiManager.addParameter(&custom_bot_token);
    wifiManager.addParameter(&custom_chat_id);
    wifiManager.addParameter(&custom_confirm_timeout);

    if (!wifiManager.autoConnect("IOT_Config", "12345678")) {
        Serial.println(F("Gagal terhubung ke WiFi."));
    }

    strlcpy(botToken, custom_bot_token.getValue(), sizeof(botToken));
    strlcpy(chatId, custom_chat_id.getValue(), sizeof(chatId));
    CONFIRM_TIMEOUT = strtoul(custom_confirm_timeout.getValue(), NULL, 10);

    if (shouldSaveConfig) {
        saveConfig_wifimanager();
    }
}

// ====================== Handler Web ======================


void resetWiFiSettings() {
    Serial.println(F("Reset WiFi Manager dan restart ESP32..."));
    wifiManager.resetSettings();
    ESP.restart();
}

// ====================== Inisialisasi ======================
void int_wifimanager() {
   

    loadConfig_wifimanager();
    initializeWiFiManager();

    Serial.println(F("Sistem siap."));
    Serial.println("Bot Token: " + String(botToken));
    Serial.println("Chat ID  : " + String(chatId));
    
    Serial.println("Timeout  : " + String(CONFIRM_TIMEOUT) + " ms");
        IPAddress ip = WiFi.localIP();
        Serial.print("Connected! IP: ");
        Serial.println(ip);

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("WiFi Connected!");
        lcd.setCursor(0, 1);
        lcd.print(ip);   // tampilkan IP di LCD
        delay(3000);

}
