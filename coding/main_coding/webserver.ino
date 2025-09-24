

//const char* ssid = "GAZEL";     // ganti dengan SSID router
//const char* password = "1234qwer";   // ganti dengan password router


AsyncWebServer server(80);

void int_webserver() {

    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS Mount Failed");
                lcd.setCursor(0, 1);
        lcd.print("SPIFFS Error");
        return;
    }

// Konek ke router (station mode)
//    WiFi.mode(WIFI_STA);
//    WiFi.begin(ssid, password);

//    lcd.clear();
//    lcd.setCursor(0, 0);
//    lcd.print("Connecting WiFi");
//
//    int retry = 0;
//    while (WiFi.status() != WL_CONNECTED && retry < 30) { // max 30 x ~15 detik
//        delay(500);
//        Serial.print(".");
//        lcd.setCursor(0, 1);
//        lcd.print("Retry: ");
//        lcd.print(retry);
//        retry++;
//    }
//
//    if (WiFi.status() == WL_CONNECTED) {
//        IPAddress ip = WiFi.localIP();
//        Serial.print("Connected! IP: ");
//        Serial.println(ip);
//
//        lcd.clear();
//        lcd.setCursor(0, 0);
//        lcd.print("WiFi Connected!");
//        lcd.setCursor(0, 1);
//        lcd.print(ip);   // tampilkan IP di LCD
//    } else {
//        Serial.println("WiFi Failed!");
//        lcd.clear();
//        lcd.setCursor(0, 0);
//        lcd.print("WiFi Failed!");
//        lcd.setCursor(0, 1);
//        lcd.print("Check Router");
//        return; // stop kalau gagal konek
//    }

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/index.html", "text/html");
    });

  server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request){
      if (request->hasParam("name", true) && request->hasParam("kelas", true) && request->hasParam("uid", true)) {
          String name = request->getParam("name", true)->value();
          String kelas = request->getParam("kelas", true)->value();
          String uid = request->getParam("uid", true)->value();
          saveData(name, kelas, uid);
          request->send(200, "text/plain", "Data saved");
      } else {
          request->send(400, "text/plain", "Missing parameters");
      }
  });


    server.on("/resetwifi", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "Resetting WiFi...");
        wifiManager.resetSettings();
        ESP.restart();
    });

    server.on("/getdata", HTTP_GET, [](AsyncWebServerRequest *request){
        int offset = 0;
        int limit = 5; // Default limit (bisa diubah sesuai kebutuhan)
    
        if (request->hasParam("offset")) {
            offset = request->getParam("offset")->value().toInt();
        }
    
        if (request->hasParam("limit")) {
            limit = request->getParam("limit")->value().toInt();
        }
    
        String data = getData(offset, limit);
        request->send(200, "application/json", data);
    });

server.on("/edit", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasParam("name", true) && request->hasParam("kelas", true) && request->hasParam("uid", true)) {
        String name = request->getParam("name", true)->value();
        String kelas = request->getParam("kelas", true)->value();
        String uid = request->getParam("uid", true)->value();
        editData(name, kelas, uid);
        request->send(200, "text/plain", "Data updated");
    } else {
        request->send(400, "text/plain", "Missing parameters");
    }
});


    server.on("/delete", HTTP_POST, [](AsyncWebServerRequest *request){
        if (request->hasParam("name", true)) {
            String name = request->getParam("name", true)->value();
            deleteData(name);
            request->send(200, "text/plain", "Data deleted");
        } else {
            request->send(400, "text/plain", "Missing parameters");
        }
    });
    server.on("/scanrfid", HTTP_GET, [](AsyncWebServerRequest *request){
    String rfidUID = getRFIDUID(); // Fungsi untuk membaca UID dari RFID
    request->send(200, "text/plain", rfidUID); // Kirim UID ke client
});


    server.begin();
}



// ==== SAVE DATA ====
void saveData(String name, String kelas, String uid) {
    File file = SPIFFS.open("/data.txt", "a");
    if (file) {
        file.println(name + "," + kelas + "," + uid + ",0"); // default status = 0
        file.close();
    }
}


// ==== GET DATA ====
String getData(int offset, int limit) {
    String result = "[";
    File file = SPIFFS.open("/data.txt", "r");
    int currentLine = 0;
    int dataCount = 0;
    if (file) {
        while (file.available()) {
            String line = file.readStringUntil('\n');
            line.trim();
            int c1 = line.indexOf(',');
            int c2 = line.indexOf(',', c1 + 1);
            int c3 = line.indexOf(',', c2 + 1);
            if (c1 == -1 || c2 == -1 || c3 == -1) continue;

            String name   = line.substring(0, c1);
            String kelas  = line.substring(c1 + 1, c2);
            String uid    = line.substring(c2 + 1, c3);
            String status = line.substring(c3 + 1);

            if (currentLine >= offset && dataCount < limit) {
                result += "{\"name\":\"" + name +
                          "\", \"kelas\":\"" + kelas +
                          "\", \"uid\":\"" + uid +
                          "\", \"status\":\"" + status + "\"},";
                dataCount++;
            }
            currentLine++;
        }
        file.close();
    }
    if (result.length() > 1) {
        result = result.substring(0, result.length() - 1);
    }
    result += "]";
    return result;
}


// ==== EDIT DATA ====
void editData(String name, String kelas, String uid) {
    File file = SPIFFS.open("/data.txt", "r");
    String data = "";
    if (file) {
        while (file.available()) {
            String line = file.readStringUntil('\n');
            line.trim();
            
            int c1 = line.indexOf(',');
            if (c1 == -1) continue;
            String oldName = line.substring(0, c1);

            if (oldName == name) {
                int c2 = line.indexOf(',', c1 + 1);
                int c3 = line.indexOf(',', c2 + 1);
                String status = "0";
                if (c3 != -1) {
                    status = line.substring(c3 + 1);
                }
                line = name + "," + kelas + "," + uid + "," + status; // update data
            }
            data += line + "\n";
        }
        file.close();
    }

    file = SPIFFS.open("/data.txt", "w");
    if (file) {
        file.print(data);
        file.close();
    }
}




// ==== DELETE DATA (hapus berdasarkan name) ====
void deleteData(String name) {
    File file = SPIFFS.open("/data.txt", "r");
    String data = "";
    if (file) {
        while (file.available()) {
            String line = file.readStringUntil('\n');
            line.trim();
            int c1 = line.indexOf(',');
            if (c1 == -1) continue;
            String oldName = line.substring(0, c1);
            if (oldName != name) {
                data += line + "\n"; 
            }
        }
        file.close();
    }

    file = SPIFFS.open("/data.txt", "w");
    if (file) {
        file.print(data);
        file.close();
    }
}
