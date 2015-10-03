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
const int SERVO_PIN = D0;
const int LEFT_HEADLIGHT = A0;
const int RIGHT_HEADLIGHT = A1;

// time between fetch attempts, in seconds.
const unsigned long fetch_interval_s = 30;

// max value for wait time of next train.
// any wait times longer than this will be rounded down.
const int MAX_WAIT_TIME = 30;

// populated by the callback name_handler below.
String my_device_name = "";

Servo gauge;
int current_angle = -1;
TCPClient client;
unsigned long last_fetch = 0;

// Uncomment this for lots of extra serial output.
//#define DEBUG_TO_SERIAL

// store the last time the "activate" button was pushed.
const int WAKEUP_PIN = D5;
unsigned long last_button_push = 0;
//const unsigned long BUTTON_ACTIVATION_TIME_MS = 30 * 60 * 1000;
const unsigned long BUTTON_ACTIVATION_TIME_MS = 5 * 60 * 1000;
const bool POWER_SAVE = false;
const int ACTIVE_INDICATOR_PIN = D7;

// Depending on the servo orientation, these may need swapping.
const int MIN_SERVO_POSITION = 180;
const int MAX_SERVO_POSITION = 0;
// These are milliseconds
const int MIN_SERVO_TIME = 20;
const int MAX_SERVO_TIME = 1000;

void setup(){
  Serial.begin(9600);

  pinMode(WAKEUP_PIN, INPUT_PULLUP);
  pinMode(ACTIVE_INDICATOR_PIN, OUTPUT);
  pinMode(LEFT_HEADLIGHT, OUTPUT);
  pinMode(RIGHT_HEADLIGHT, OUTPUT);

  for(int i=0;i<5;i++) {
      Serial.println("waiting... " + String(5 - i));
      delay(1000);
  }

  Spark.subscribe("spark/device/name", name_handler);
  Spark.publish("spark/device/name");

  // Allow for remote setting of angle.
  Spark.function("setAngle", setAngleCallback);
  Spark.variable("angle", &current_angle, INT);

  // Warm up with a full-range test with the servo.
  gauge.attach(SERVO_PIN);
  gauge.write(MAX_SERVO_POSITION);
  delay(MAX_SERVO_TIME);
  gauge.write(MIN_SERVO_POSITION);
  delay(MAX_SERVO_TIME);
  gauge.detach();

}

// Callback to set the device name, from the cloud.
void name_handler(const char *topic, const char *data) {
  Serial.println("received " + String(topic) + ": " + String(data));
  //this is stupid, but there's no operator=(String)
  my_device_name = "" + String(data);
}

// Cloud function, called to set angle.
// Consider also stopping the period angle updates once this is called.
int setAngleCallback(String str_angle){
  int angle = str_angle.toInt();
  return setGaugeAngle(angle);
}


void loop(){

  unsigned long current_time = millis();

  #ifdef DEBUG_TO_SERIAL
  Serial.println("current time: " + String(current_time));
  Serial.println("last fetch: " + String(last_fetch));
  Serial.println("last press: " + String(last_button_push));
  #endif

  // Check for button press
  if(digitalRead(WAKEUP_PIN) == LOW){
    delay(100);
    Serial.println("button pressed!");
    Spark.publish("button pressed!");
    last_button_push = current_time;
  }

  int time_since_push = current_time - last_button_push;
  if (time_since_push < BUTTON_ACTIVATION_TIME_MS &&
      last_button_push > 0){
    digitalWrite(ACTIVE_INDICATOR_PIN, HIGH);

    // Fade headlights in line with remaining activation time.
    int brightness = map(time_since_push, 0, BUTTON_ACTIVATION_TIME_MS, 200, 0);
    analogWrite(LEFT_HEADLIGHT, brightness);
    analogWrite(RIGHT_HEADLIGHT, brightness);

    if(millis() > (last_fetch + (fetch_interval_s * 1000)) &&
       !client.connected()){
      updateArrivalTime();
    }
  } else {
    // No work to do.
    // Disable indicator light.
    analogWrite(LEFT_HEADLIGHT, 0);
    analogWrite(RIGHT_HEADLIGHT, 0);
    digitalWrite(ACTIVE_INDICATOR_PIN, LOW);
    delay(100);
    return;
  }
}

void updateArrivalTime(){
    last_fetch = millis();
    Serial.println("attempting to get times.");
    int t = getNextArrivalTime();
    // publish the time we parsed, for viewing in the dashboard.
    Spark.publish("time-received", String(t), 60, PRIVATE);
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
      int angle = map(t,
                      0, MAX_WAIT_TIME,
                      MIN_SERVO_POSITION, MAX_SERVO_POSITION);
      Serial.println("angle: " + String(angle));
      //constrain(angle, 0, 180);
      setGaugeAngle(angle);
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
  int sleeptime = map(abs(current_angle - requested_angle),
                      0, 180,
                      MIN_SERVO_TIME, MAX_SERVO_TIME);
  if(current_angle < 0){
    sleeptime = MAX_SERVO_TIME;
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
  unsigned long timeout = 4000;
  bool error = false;
  while(client.connected() && !error){
    if(client.available()){
      char c = client.read();
      //Serial.print(c);
      buffer[bpos] = c;
      bpos++;
    }
    else {
      delay(5);
    }
    if( millis() > (read_start + timeout)){
      error = true;
      Serial.println("timeout");
    }
  }
  client.stop();
  buffer[bpos] = '\0';
  String full_response(buffer);
  #ifdef DEBUG_TO_SERIAL
  Serial.println(full_response);
  #endif
  int bodystart = full_response.indexOf("\r\n\r\n") + 4;
  if(error || bodystart < 0 || bodystart == full_response.length()){
    return -1;
  } else {
    return full_response.substring(bodystart, full_response.length()).toInt();
  }
}

