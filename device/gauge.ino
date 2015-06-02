/* Remote-control a servo for showing time until next train arrival.
 * 
 */

const int MAXTIMES = 2;
Servo gauge;
int set_angle = 0;
int known_times[MAXTIMES] = { 0, 10};

void setup(){
  Spark.publish("Starting up!");

  gauge.attach(D0);

  Spark.function("gauge", setGaugeAngle);
  Spark.variable("angle", &set_angle, INT);

}

void loop(){

  // Show times by converting to angle. 30m in 90d = 3d/m
  // loop backwards, pausing to allow for visibility of all known times.
  for(int i=MAXTIMES-1;i>=0;i--){
    setGaugeAngle(String(known_times[i] * 3));
    delay (2000/i);
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
    if(times.indexOf(',', startpos) > 0){
      int t = times.substring(startpos, times.indexOf(',', startpos)).toInt();
      known_times[i] = t;
    }
  }
  return 1;
  
}
