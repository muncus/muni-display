/* Control a servo for showing time until next train arrival.
 *
 */

// Server address/port.
//146.148.61.247
const IPAddress SERVER_IP(146,148,61,247);
const char* SERVER_ADDR = "muni.marcdougherty.com";
const int SERVER_PORT = 4567;

/* from http://docs.particle.io/core/firmware/:
 * on the Core, Servo can be connected to A0, A1, A4, A5, A6, A7, D0, and D1.
 * on the Photon, Servo can be connected to A4, A5, WKP, RX, TX, D0, D1, D2, D3
 */
const int SERVO_PIN = A7;

// time between fetch attempts, in seconds.
const int fetch_interval_s = 30;

// max value for wait time of next train.
// any wait times longer than this will be rounded down.
const int MAX_WAIT_TIME = 30;

// populated by the callback name_handler below.
String my_device_name = "";

Servo gauge;
int current_angle = -1;
TCPClient client;
unsigned long last_fetch = 0;

// store the last time the "activate" button was pushed.
const int WAKEUP_PIN = A6;
unsigned long last_button_push = 0;
const int BUTTON_ACTIVATION_TIME_MS = 60 * 60 * 1000;
const bool REQUIRE_BUTTON_PRESS = true;

void setup(){
  Serial.begin(9600);

  pinMode(WAKEUP_PIN, INPUT_PULLUP);

  for(int i=0;i<5;i++) {
      Serial.println("waiting... " + String(5 - i));
      delay(1000);
  }

  Spark.subscribe("spark/device/name", name_handler);
  Spark.publish("spark/device/name");

  // Allow for remote setting of angle.
  Spark.function("setAngle", setAngle);
  Spark.variable("angle", &current_angle, INT);

  // Warm up with a full-range test with the servo.
  gauge.attach(SERVO_PIN);
  gauge.write(0);
  delay(1000);
  gauge.write(180);
  delay(1000);
  gauge.detach();

}

// Callback to set the device name, from the cloud.
void name_handler(const char *topic, const char *data) {
  Serial.println("received " + String(topic) + ": " + String(data));
  //this is stupid, but there's no operator=(String)
  my_device_name = "" + String(data);
}

// Consider also stopping the period angle updates once this is called.
int setAngle(String str_angle){
  int angle = str_angle.toInt();
  return setGaugeAngle(angle);
}


void loop(){

  // Check for button press
  if(digitalRead(WAKEUP_PIN) == LOW){
    delay(100);
    last_button_push = millis();
  }

  // Exit loop early if we are not active.
  if(REQUIRE_BUTTON_PRESS && 
      ((millis() - last_button_push) > BUTTON_ACTIVATION_TIME_MS)){
    // No work to do.
    return;
  }

  if(millis() > (last_fetch + (fetch_interval_s * 1000)) && !client.connected()){
    last_fetch = millis();
    Serial.println("attempting to get times.");
    int t = getNextArrivalTime();
    //delay(1000);
    if(t<0){
      Serial.println("An error occurred while fetching times.");
    } else {
      Serial.println("Got: " + String(t));
      // If the servo goes the other way, try this instead:
      // int angle = map(t, 0, MAX_WAIT_TIME, 0, 180);
      // Round down, to avoid overflow.
      if(t > MAX_WAIT_TIME){
        t = MAX_WAIT_TIME;
      }
      int angle = map(t, 0, MAX_WAIT_TIME, 180, 0);
      Serial.println("angle: " + String(angle));
      //constrain(angle, 0, 180);
      setGaugeAngle(angle);
    }
  }
}

int setGaugeAngle(int angle) {
  gauge.attach(SERVO_PIN);
  int requested_angle = angle;
  if(requested_angle > 180 || requested_angle < 0){
    return -1;
  }
  gauge.write(requested_angle);
  // make the sleep relative to how much distance the servo needs to travel.
  // somewhere between 20ms and 1000ms.
  // if current_angle is unknown (as on the initial run), give it all the time.
  int sleeptime = map(abs(current_angle - requested_angle), 0, 180, 20, 1000);
  if(current_angle < 0){
    sleeptime = 1000;
  }
  delay(sleeptime);
  current_angle = requested_angle;
  gauge.detach();
  return 1;
};

// Fetch next train time from the server.
// Requires that the response body be just the number, nothing else.
int getNextArrivalTime(){
  char buffer[500];
  int bpos = 0;

  if(!client.connect(SERVER_ADDR, SERVER_PORT)){
    Serial.println("connect failure");
    return -1;
  }
  client.println("GET /times/" + my_device_name + " HTTP/1.0");
  client.println("Host: " + String(SERVER_ADDR));
  client.println("Connection: close");
  client.println();
  delay(200);

  unsigned long read_start = millis();
  unsigned long timeout = 2000;
  bool error = false;
  while(client.connected() && !error){
    if(client.available()){
      char c = client.read();
      //Serial.print(c);
      buffer[bpos] = c;
      bpos++;
    }
    if( millis() > (read_start + timeout)){
      error = true;
      Serial.println("timeout");
    }
  }
  client.stop();
  buffer[bpos] = '\0';
  String full_response(buffer);
  Serial.println(full_response);
  int bodystart = full_response.indexOf("\r\n\r\n") + 4;
  if(error || bodystart < 0 || bodystart == full_response.length()){
    return -1;
  } else {
    return full_response.substring(bodystart, full_response.length()).toInt();
  }
}

