#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- HARDVÉROVÁ KONFIGURÁCIA ---
#define MQ3_PIN A0
#define ADC_RESOLUTION 1023.0 
#define VOLTAGE_REF 5.0       
#define RL 10.0               // Väčšina modulov (ako na foto) má záťažový odpor 10kOhm (SMD kód 103)
#define WARMUP_TIME_SEC 60    // Bezpečný čas na stabilizáciu NTC vrstvy
#define CLEAN_AIR_RATIO 60.0  // KLÚČOVÝ FAKTOR: Rs/R0 v čistom vzduchu podľa datasheetu

// --- GLOBÁLNE PREMENNÉ ---
float R0 = 1.0; 
float adcBaseline = 0.0;
bool isReady = false;
unsigned long startTime;

void setup() {
  Serial.begin(115200); 
  
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    while (true);
  }
  
  pinMode(MQ3_PIN, INPUT);
  display.clearDisplay();
  display.setTextColor(WHITE);
  
  startTime = millis();
}

void loop() {
  if (!isReady) {
    handleWarmup();
  } else {
    handleMeasurementCorrected();
  }
}

float getFastADC() {
  long sum = 0;
  for(int i = 0; i < 10; i++) { 
    sum += analogRead(MQ3_PIN);
    delayMicroseconds(500);    
  }
  return sum / 10.0;
}

float calculateRs(float rawADC) {
  float sensorVoltage = (rawADC / ADC_RESOLUTION) * VOLTAGE_REF;
  if (sensorVoltage <= 0.1) return 99999.0; 
  return RL * ((VOLTAGE_REF - sensorVoltage) / sensorVoltage);
}

void handleWarmup() {
  unsigned long elapsedSeconds = (millis() - startTime) / 1000;
  
  if (elapsedSeconds <= WARMUP_TIME_SEC) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(10, 5);
    display.print("    KALIBRACIA");
    
    display.setCursor(50, 22);
    display.setTextSize(2);
    display.print(WARMUP_TIME_SEC - elapsedSeconds);
    
    int barWidth = map(elapsedSeconds, 0, WARMUP_TIME_SEC, 0, 108);
    display.drawRect(9, 45, 110, 12, WHITE);
    display.fillRect(10, 46, barWidth, 10, WHITE);
    display.display();
  } 
  else {
    // KALIBRÁCIA: Odmeriame odpor v čistom vzduchu
    adcBaseline = getFastADC();
    float Rs_clean = calculateRs(adcBaseline);
    
    // Vypočítame R0 pre alkoholový model (Rs v čistom vzduchu vydelíme faktorom 60)
    R0 = Rs_clean / CLEAN_AIR_RATIO; 
    
    isReady = true;
  }
}

void handleMeasurementCorrected() {
  float rawADC = getFastADC();
  float Rs = calculateRs(rawADC);
  
  // Výpočet pomeru voči nakalibrovanému R0
  float ratio = Rs / R0;
  
  // Korektný prepočet z log-log grafu pre alkohol
  float mgL = 0.44 * pow(ratio, -1.45); 
  
  // SOFTVÉROVÁ NULA: Ak je surové ADC blízko alebo pod kalibračnou hodnotou, je to čistý vzduch
  if (rawADC <= (adcBaseline + 10)) {
    mgL = 0.0;
  }
  
  float promile = mgL * 2.1;

  // Debug výpis do PC
  Serial.print("ADC: "); Serial.print(rawADC, 0);
  Serial.print(" | Baseline: "); Serial.print(adcBaseline, 0);
  Serial.print(" | Ratio: "); Serial.print(ratio, 2);
  Serial.print(" | Promile: "); Serial.println(promile, 2);

  // Vykreslenie na OLED
  display.clearDisplay();
  
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Surove ADC: ");
  display.print(rawADC, 0);
  
  display.setCursor(0, 18);
  display.setTextSize(3);
  if (promile > 5.0) display.print("MAX"); // Ochrana rozsahu
  else display.print(promile, 2);
  
  display.setTextSize(1);
  display.print(" %.");
  
  display.setCursor(0, 45);
  display.print("Dych: ");
  display.print(mgL, 2);
  display.print(" mg/L");
  
  // Status Bar kopíruje reálny rozsah od baseline (nula) po plné nasýtenie (ADC 900)
  int liveBar = map((int)rawADC, (int)adcBaseline, 900, 0, 128);
  if(liveBar < 0) liveBar = 0;
  if(liveBar > 128) liveBar = 128;
  display.fillRect(0, 58, liveBar, 6, WHITE);

  display.display();
}
