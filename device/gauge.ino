/* Remote-control a servo for showing time until next train arrival.
 * 
 */


const int MAXTIMES = 2;
Servo gauge;
int set_angle = 0;
int known_times[MAXTIMES] = { 0, 10};
TCPClient client;
unsigned long last_fetch = 0;
int fetch_interval_s = 30;

String my_device_name = "";

void setup(){
  Serial.begin(9600);

  for(int i=0;i<5;i++) {
      Serial.println("waiting... " + String(5 - i));
      delay(1000);
  }

  Spark.subscribe("spark/device/name", name_handler);
  Spark.publish("spark/device/name");

  /*
  gauge.attach(D0);
  gauge.write(0);
  delay(500);
  gauge.detach();
  */

  //Spark.function("gauge", setGaugeAngle);
  //Spark.function("times", setTimes);
  //Spark.variable("angle", &set_angle, INT);

}

// Callback to set the device name, from the cloud.
void name_handler(const char *topic, const char *data) {
  Serial.println("received " + String(topic) + ": " + String(data));
  //FIXME: this is stupid, but there's no operator=(String)
  my_device_name = "" + String(data);
}

void loop(){

  // Show times by converting to angle.
  // loop backwards, pausing to allow for visibility of all known times.
  /*
  for(int i=MAXTIMES-1;i>=0;i--){
    int angle = map(known_times[i], 0, 30, 0, 180);
    constrain(angle, 0, 180);
    setGaugeAngle(angle);
    delay (500);
  }
  */
  //Serial.println(my_device_name);
  /*
  delay(2000);
  Serial.println("ANGLES");
  for( int i=0;i <= 180;i+=10){
    gauge.attach(D0);
    Serial.println("setting: " + String(i));
    gauge.write(i);
    delay(700);
    gauge.detach();
    delay(1000);
  }
  */
  if(millis() > last_fetch + (fetch_interval_s * 1000) && !client.connected()){
    last_fetch = millis();
    Serial.println("attempting to get times...");
    Serial.println("Got: " + getTimes());
  }
}

int setGaugeAngle(String angle) {
  int requested_angle = angle.toInt();
  if(requested_angle > 180 || requested_angle < 0){
    return -1;
  }
  set_angle = requested_angle;
  gauge.write(requested_angle);
  return 1;
};

// Assume times are comma-separated.
int setTimes(String times) {
  int startpos = 0;
  for(int i=0;i<MAXTIMES;i++){
    int limit = (times.indexOf(',', startpos) || times.length()-1);
    // substring is not inclusive.
    int t = times.substring(startpos, limit).toInt();
    Serial.println("Found time:" + String(t));
    known_times[i] = t;
    startpos = limit+1;
  }
  return 1;
}

// Fetch the list of times, and return it.
String getTimes(){
  char data[50];
  //146.148.61.247
  IPAddress svr(146,148,61,247);
  //byte svr[] = { 146, 148, 61, 247 };
  //IPAddress svr(192,168,2,101);
  //byte svr[] = { 192, 168, 2, 101 };
  //client.connect(svr, 4567);
  //client.connect("device.muniminutes-hrd.appspot.com", 80);

  if(client.connect(svr, 4567)){
    //client.println("Host: device.muniminutes-hrd.appspot.com");
    //Serial.println("GET /times/" + my_device_name + " HTTP/1.0");
    //NB: the \r\n ending of println() appear to make sinatra sad.
    client.print("GET /times/" + my_device_name + " HTTP/1.0\n");
    //client.println("Accept: text/plain");
    //XXX: consider using a String and setCharAt() to do the parsing here.
    //FIXME: this seems to make the core go unresponsive, and it needed a factory reset :/
    // Figure out what happened, and fix it. - maybe doing too much network, no time for cloud?
    client.print("\n\n");
    delay(300);
    int i = 0;
    char buffer[100];
    //String datastring;
    //datastring.reserve(50);
    char data[50];

    while(client.available() && i<100){
      buffer[i] = client.read();
      //Serial.print(String(buffer[i]));
      if(buffer[i] == '\n'){
        //line end, check for empty.
        Serial.println("NEWLINE");
        // 13 == \r
        //Serial.println("Previous char: " + String(buffer[i-1], DEC));
        if(i < 4){
          //empty line.
          Serial.println("FOUND EMPTY");
          int j = 0;
          while(client.available() && j<50){
            char c = client.read();
            data[j] = c;
            //datastring.setCharAt(j, c);
            j++;
          }
          Serial.println("DATA: " + String(data));
          //Serial.println("DATASTRING; " + datastring);
          return String(data);
        }
        //Serial.println("RESET");
        i = 0; //reset for next line.
      }
      i++;
      if (i >= 100){
        Serial.println("buffer full.");
        i = 0;
      }
    }
    //client.flush();
    client.stop();
  }
  else {
    Serial.println("Connection failed!");
    return String("");
  }
  //client.flush();
  //client.stop();
  //return String("");
}
