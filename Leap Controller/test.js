var Cylon = require('cylon');

// Initialize the robot
Cylon.robot({
  //create a connection for each piece of hardware
  connections: {
    //replace accessToken and deviceId with your own credentials
    voodoospark: {
      adaptor: 'voodoospark',
      accessToken: 'YOUR_ACCESS_TOKEN',
      deviceId: 'YOUR_DEVICE_ID',
      module: 'cylon-spark'
    },
    leapmotion: { adaptor: 'leapmotion' }
  },
  devices: {
    //link the motors and laser to the photon board 
    laser: { driver: 'led', pin: 'D2', connection: 'voodoospark' },
    servoX: { driver: 'servo', pin: 'D1', connection: 'voodoospark' },
    servoY: { driver: 'servo', pin: 'D0', connection: 'voodoospark' },
    leapmotion: { driver: 'leapmotion', connection: 'leapmotion'}

  },

  work: function(my) {
    every((.2).second(), function() {my.laser.turnOn()});
    var roll, pitch;
    my.leapmotion.on('frame', function(frame) {
    frame.hands.forEach(function(hand, index) { 
        roll = Math.min(Math.max(((hand.roll()*-1)+2.5)*30,10),170);
        pitch = Math.min(Math.max((hand.pitch()+1)*60,10),170);
        my.servoX.angle(roll);
        my.servoY.angle(pitch);    
      });    
    });
  }
}).start();