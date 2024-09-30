// Pripojenie LED a Buzzu
const int smokePin = A0;    // Pin pre snímač dymu (analógový vstup)
const int ledPin = 3;       // Pin pre LED (digitálny výstup)
const int buzzerPin = 4;    // Pin pre Buzzer (digitálny výstup)

// Hodnota prahu (threshold) pre detekciu dymu (nastav podľa kalibrácie snímača)
int smokeThreshold = 400;    // Prispôsob túto hodnotu podľa testov (napr. 400 pre MQ-2)

void setup() {
  Serial.begin(9600);        // Inicializácia sériovej komunikácie
  pinMode(ledPin, OUTPUT);   // Nastavenie LED ako výstup
  pinMode(buzzerPin, OUTPUT);// Nastavenie Buzzer ako výstup
}

void loop() {
  int smokeValue = analogRead(smokePin);  // Čítanie hodnoty zo snímača dymu

  Serial.println(smokeValue);            // Vytlačí hodnotu do sériového monitora

  // Ak je hodnota vyššia ako nastavený prah (dym detekovaný)
  if (smokeValue > smokeThreshold) {
    digitalWrite(ledPin, HIGH);          // Rozsvieti LED
    digitalWrite(buzzerPin, HIGH);       // Spustí Buzzer
    Serial.println("Smoke detected!");   // Tlačí hlášku do sériového monitora
  } else {
    digitalWrite(ledPin, LOW);           // Vypne LED
    digitalWrite(buzzerPin, LOW);        // Vypne Buzzer
  }

  delay(500);                            // Malé oneskorenie, aby sa nevypínalo/nezapínalo príliš rýchlo
}
