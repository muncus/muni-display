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

}

// Callback to set the device name, from the cloud.
void name_handler(const char *topic, const char *data) {
  Serial.println("received " + String(topic) + ": " + String(data));
  //FIXME: this is stupid, but there's no operator=(String)
  my_device_name = "" + String(data);
}

void loop(){

  if(millis() > last_fetch + (fetch_interval_s * 1000) && !client.connected()){
    last_fetch = millis();
    Serial.println("attempting to get times...");
    int t = getTime();
    if(t<0){
      Serial.println("An error occurred while fetching times.");
    } else {
      Serial.println("Got: " + String(t));
      int angle = map(t, 0, 30, 0, 180);
      Serial.println("angle: " + String(angle));
      constrain(angle, 0, 180);
      setGaugeAngle(angle);
    }
  }
}

int setGaugeAngle(int angle) {
  gauge.attach(D0);
  int requested_angle = angle;
  if(requested_angle > 180 || requested_angle < 0){
    return -1;
  }
  //set_angle = requested_angle;
  gauge.write(requested_angle);
  delay(700);
  gauge.detach();
  return 1;
};


// Fetch the list of times, and return it.
int getTime(){
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
    char data_buffer[50];

    while(client.available() && i<100){
      buffer[i] = client.read();
      //Serial.print(String(buffer[i]));
      if(buffer[i] == '\n'){
        //line end, check for empty.
        //Serial.println("NEWLINE");
        // 13 == \r
        //Serial.println("Previous char: " + String(buffer[i-1], DEC));
        if(i < 3){
          //empty line.
          //Serial.println("FOUND EMPTY");
          int j = 0;
          while(client.available() && j<50){
            char c = client.read();
            Serial.println(c);
            data_buffer[j] = c;
            j++;
          }
          //Serial.println("DATA: " + String(data_buffer));
          client.flush();
          client.stop();
          return String(data_buffer).toInt();
        }
        else {
          i = 0; //reset for next line.
        }
      }
      i++;
    }
  }
  else {
    Serial.println("Connection failed!");
  }
  client.flush();
  client.stop();
  return -1;
}
