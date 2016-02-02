//laser power control pin
int laser = D6;

void setup() {
  pinMode(laser,OUTPUT); // Our laser pin is output (lighting up the laser)
  
  // Here we are going to subscribe to a buttonFire event that calls myHandler when buttonFire is published
  Particle.subscribe("buttonFire", myHandler);
}
void loop() {
}
// Fire the laser for half a second 
void myHandler(const char *event,const char *data)
{
    digitalWrite(laser,HIGH);
    delay(500);
    digitalWrite(laser,LOW);
}
