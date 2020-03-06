
void prettyBytes(uint32_t bytes, String &output) {

    const char *suffixes[7] = {"B", "KB", "MB", "GB", "TB", "PB", "EB"};
    uint8_t s = 0;
    double count = bytes;

    while (count >= 1024 && s < 7) {
        s++;
        count /= 1024;
    }
    if (count - floor(count) == 0.0) {
        output = String((int) count) + suffixes[s];

    } else {
        output = String(round(count * 10.0) / 10.0, 1) + suffixes[s];
    };
}

uint32_t getUptimeSecs() {
    static uint32_t uptime = 0;
    static uint32_t previousMillis = 0;
    uint32_t now = millis();

    uptime += (now - previousMillis) / 1000UL;
    previousMillis = now;
    return uptime;
}

void getUptimeDhms(char *output, size_t max_len) {
    uint32 d, h, m, s;
    uint32_t sec = getUptimeSecs();

    d = sec / 86400;
    sec = sec % 86400;
    h = sec / 3600;
    sec = sec % 3600;
    m = sec / 60;
    s = sec % 60;

    snprintf(output, max_len, "%dd %02d:%02d:%02d", d, h, m, s);
}

