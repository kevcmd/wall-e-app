#include "robot.h"

int addr = 0x40;
unsigned int setServo[_PCA9685_CHANS] = {0};
unsigned int offVals[_PCA9685_CHANS] = {0};

// Default POS - Min POS - Max POS
int controls[9][3] = {
    {100, 25, 175}, // Head
    {90, 25, 120},  // Neck
    {95, 50, 95},   // Left Eye
    {95, 65, 95},   // Right Eye
    {180, 1, 180},  // Left Track
    {120, 1, 180},  // Left Arm
    {1, 1, 180},    // Right Track
    {55, 1, 180},   // Right Arm
    {130, 30, 170}, // Bottom Neck
};

void robot_init()
{
    wiringPiSetup();
    serial_port = serialOpen("/dev/ttyAMA0", 9600);
    sleep(1);

    fd = initHardware(1, addr, 50);

    sleep(2);

    /*  for(int i = 0; i < 9; i++)
      {
          //servos[i].defPos = controls[i][0];
          servos[i].pos = controls[i][0];
          servos[i].min = controls[i][1];
          servos[i].max = controls[i][2];
          servos[i].p = 0.0000001;
          servos[i].d = 0.0000001;
      }
  */
    posX.pos = controls[0][0];
    posX.p = 0.0065;
    posX.d = 0.032;
    posX.prevError = 0;
    posX.min = controls[0][1];
    posX.max = controls[0][2];

    posY.pos = controls[1][0];
    posY.p = 0.004;
    posY.d = 0.008;
    posY.prevError = 0;
    posY.min = controls[1][1];
    posY.max = controls[1][2];

    tilt.pos = controls[8][0];
    tilt.p = 0.006;
    tilt.d = 0.008;
    tilt.prevError = 0;
    tilt.min = controls[8][1];
    tilt.max = controls[8][2];

    // initPOS();
}

int initHardware(int adpt, int addr, int freq)
{
    int afd = PCA9685_openI2C(adpt, addr);
    PCA9685_initPWM(afd, addr, freq);
    return afd;
}

void delay(int mili)
{
    usleep(mili * 1000);
}

void sendData(int angle, int distance, int ctrlId)
{
    char strData[14];
    sprintf(strData, "%03d %02d", angle, ctrlId);
    // sprintf(strData, "%03d %02d %02d", angle, distance, ctrlId);

    serialPuts(serial_port, strData);
}

void closeSerial()
{
    serialClose(serial_port);
}

uint16_t pulseWidth(int channel, float degree)
{
    if (degree < controls[channel][1])
        degree = controls[channel][1];
    else if (degree > controls[channel][2])
        degree = controls[channel][2];
    // controls[channel][2] = degree;
    return round(degree * (512 - 102) / 180 + 102);
}
void setServoAngle()
{
    

    //unsigned int onVals[_PCA9685_CHANS] = {0};
    //unsigned int offVals[_PCA9685_CHANS] = {0};

    //onVals[channel] = round(pulseWidth);

    PCA9685_setPWMVals(fd, addr, offVals, setServo);
    setServo[_PCA9685_CHANS] = 0;
}

void initPOS()
{
    for (int i = 0; i <= 8; i++)
    {
        //setServoAngle(i, controls[i][0]);
        delay(250);
    }
    delay(250);
    // eyeCalibration();
}

void eyeCalibration()
{
    int delayAmt = 500;
    if (servos[3].pos != controls[3][2])
        //setServoAngle(3, controls[3][2]);
    if (servos[2].pos != controls[2][2])
        //setServoAngle(2, controls[2][2]);

    if (servos[0].pos != 90)
    {
        //setServoAngle(0, 90);
        delay(1000);
    }

    //setServoAngle(2, controls[2][1]); // Left down
    delay(delayAmt);
    //setServoAngle(3, controls[3][1]); // Right down
    delay(delayAmt);
    //setServoAngle(2, controls[2][2]); // Left up
    delay(delayAmt);
    //setServoAngle(3, controls[3][2]); // Down up
    delay(delayAmt);
    //setServoAngle(3, controls[3][1]); // Left and Right Down
    //setServoAngle(2, controls[2][1]);
    delay(delayAmt + 200);
    //setServoAngle(3, controls[3][2]); // Left and Right Up
    //setServoAngle(2, controls[2][2]);
}

void updatePD(Servo *newPos, int error)
{
    if (newPos->prevError != 0)
    {
        newPos->pos += (error * newPos->p + (error - newPos->prevError) * newPos->d);

        if (newPos->pos > newPos->max)
            newPos->pos = newPos->max;
        else if (newPos->pos < newPos->min)
            newPos->pos = newPos->min;
    }
    newPos->prevError = error;
}

void updateHead(int x, int y, int area)
{
    if (x != 0)
    {
        updatePD(&posX, x - CAM_WIDTH/2);
        setServo[0] = pulseWidth(0, posX.pos);
        if(posY.pos > 100 && tilt.pos > 90)
        {
            if(posX.pos >= 130)
                setServo[0] = pulseWidth(0, 130);
            if(posX.pos <= 70)
                setServo[0] = pulseWidth(0, 70);
        }
        //setServoAngle();
    }
    
    if (y != 0)
    {
        updatePD(&posY, y - CAM_HEIGHT/2);
        setServo[1] = pulseWidth(1, posY.pos);
        //setServoAngle(1, posY.pos);
        int tilt_error = posY.pos - ((posY.min + posY.max) / 2);
        updatePD(&tilt, tilt_error);
        setServo[8] = pulseWidth(8, tilt.pos);
    }
    
    setServoAngle();
}

void updateCoords(objectCoord *obj)
{
    if (obj->area != 0)
    {
        updateHead(obj->x, obj->y, obj->area);

        if (obj->x == CAM_WIDTH/2 && obj->y == CAM_HEIGHT/2)
            obj->area = 0;
        else
        {
            if (obj->x > CAM_WIDTH/2)
                obj->x = constrain(obj->x -= 10, CAM_WIDTH/2, CAM_WIDTH);//-= 1;
            else if (obj->x < CAM_WIDTH/2)
                obj->x = constrain(obj->x += 15, 0, CAM_WIDTH/2);

            if (obj->y > CAM_HEIGHT/2)
                obj->y = constrain(obj->y -= 10, CAM_HEIGHT/2, CAM_HEIGHT);
            else if (obj->y < CAM_HEIGHT/2)
                obj->y = constrain(obj->y += 10, 0, CAM_HEIGHT/2);
        }
    }
    else
        setServo[_PCA9685_CHANS] = 0;
}

Point convert_angle(int angle) {
    Point result;

    double radians = (angle * 3.14159) / 180.0;

    result.x = mapRange(-CAM_WIDTH,CAM_WIDTH,1,CAM_WIDTH,CAM_WIDTH * cos(radians));
    result.y = mapRange(-CAM_HEIGHT,CAM_HEIGHT,1,CAM_HEIGHT,CAM_HEIGHT * sin(radians));

    return result;
}