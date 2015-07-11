/* Remote-control a servo for showing time until next train arrival.
 * 
 */


Servo gauge;
int current_angle = 0;
TCPClient client;
unsigned long last_fetch = 0;
int fetch_interval_s = 30;

//146.148.61.247
const IPAddress SERVER_IP(146,148,61,247);
const int SERVER_PORT = 4567;

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
  //this is stupid, but there's no operator=(String)
  my_device_name = "" + String(data);
}

void loop(){

  if(millis() > (last_fetch + (fetch_interval_s * 1000)) && !client.connected()){
    last_fetch = millis();
    Serial.println("attempting to get times.");
    Serial.println("httpprint1");
    int t = httpprint1();
    //delay(1000);
    //TODO: reenable this code when i'm able to read the http response body.
    if(t<0){
      Serial.println("An error occurred while fetching times.");
    } else {
      Serial.println("Got: " + String(t));
      int angle = map(t, 0, 30, 0, 180);
      Serial.println("angle: " + String(angle));
      //constrain(angle, 0, 180);
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


// stripped-down approach. 
int httpprint1(){
  char buffer[500];
  int bpos = 0;

  if(!client.connect(SERVER_IP, SERVER_PORT)){
    Serial.println("connect failure");
    return -1;
  }
  client.println("GET /times/" + my_device_name + " HTTP/1.0");
  client.println("Host: example.com");
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
  if(bodystart < 0){
    return bodystart;
  } else {
    return full_response.substring(bodystart, full_response.length()).toInt();
  }
}

int httpprint2(){
  unsigned long start_time = millis();
  unsigned long timeout = 2000;
  bool error = false;
  if(!client.connect(SERVER_IP, SERVER_PORT)){
    Serial.println("connect failure");
    return -1;
  }
  client.write("GET /times/sendtoserial HTTP/1.0\r\n");
  client.write("Host: example.com\r\n");
  client.write("Connection: close\r\n");
  client.write("\r\n");
  delay(200);

  do{
    int bytes = client.available();
    if(bytes){
      Serial.print("Bytes available: ");
      Serial.println(bytes);
    }
    if(millis() >= (start_time + timeout)){
      Serial.println("timeout");
      error = true;
    }
    while(client.available()){
      char c = client.read();
      Serial.print(c);
    }
    delay(300);
  } while(client.connected() && !error );
  client.stop();
  return -1;
};
// Fetch the next arrival from the server.
int justprintit(){
  if(client.connect(SERVER_IP, SERVER_PORT)){
    client.println("GET /times/" + my_device_name + " HTTP/1.0");
    //client.println("Accept: text/plain");
    //client.println("Connection: close");
    client.println();
    client.println();
    //client.flush();
    delay(400);

    //char buffer[500];
    //memset(&buffer[0], 0, sizeof(buffer));
              
    boolean error = false;
    int buf_pos = 0;
    unsigned long timeout_ms = 5000;
    unsigned long start_time = millis();

    do{
      int bytes = client.available();
      if(bytes){
        Serial.print("Bytes available: ");
        Serial.println(bytes);
      }

      if(millis() >= (start_time + timeout_ms)){
        Serial.println("timeout");
        error = true;
        //break;
      }

      while(client.available()){
        /*
        if(buf_pos == sizeof(buffer)-1){
          Serial.println("buffer full.");
          error = true;
          break;
        }
        */
        char c = client.read();
        //buffer[buf_pos] = c;
        if( c == -1){
          Serial.println("error");
          error = true;
          break;
        }
        Serial.print(c);
      }
      delay(300);
    } while(client.connected() && !error );
    //buffer[++buf_pos] = '\0';
    //Serial.println("Buffer:");
    //Serial.print(String(buffer).substring(0,buf_pos));
    //client.stop();
  } else {
    Serial.println("Connection failed!");
  }
  client.stop();
  return -1;
}
