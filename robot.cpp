#include "robot.h"
#include <iostream>

simxFloat sensorAngle[8] = {PI/4,5/18*PI,PI/6,PI/18,-PI/18,-PI/6,-(5/18)*PI,-PI/4};
int start = 1;
int objectRight, objectLeft = 0;
int turnRight = 1; // 0 pra vira esquerda e 1 pra direita
int timeInCircle = 0;
Robot::Robot(int clientID, const char* name) {

    FILE *data =  fopen("gt.txt", "wt");
    if (data!=NULL)
        fclose(data);

    data =  fopen("sonar.txt", "wt");
    if (data!=NULL)
        fclose(data);

    this->clientID = clientID;
    simxGetObjectHandle(clientID, name, &handle, simx_opmode_blocking);


    /* Get handles of sensors and actuators */
    simxGetObjectHandle(clientID, "Pioneer_p3dx_leftWheel",&encoderHandle[0], simx_opmode_oneshot_wait);
    simxGetObjectHandle(clientID, "Pioneer_p3dx_rightWheel",&encoderHandle[1], simx_opmode_oneshot_wait);
    std::cout << encoderHandle[0] << std::endl;
    std::cout << encoderHandle[1] << std::endl;

    /* Get handles of sensors and actuators */
    simxGetObjectHandle(clientID, "Pioneer_p3dx_leftMotor",&motorHandle[0], simx_opmode_oneshot_wait);
    simxGetObjectHandle(clientID, "Pioneer_p3dx_rightMotor",&motorHandle[1], simx_opmode_oneshot_wait);
    std::cout << motorHandle[0] << std::endl;
    std::cout << motorHandle[1] << std::endl;

    simxChar sensorName[31];
    /* Connect to sonar sensors. Requires a handle per sensor. Sensor name: Pioneer_p3dx_ultrasonicSensorX, where
     * X is the sensor number, from 1 - 16 */
    for(int i = 0; i < 16; i++)
    {
        sprintf(sensorName,"%s%d","Pioneer_p3dx_ultrasonicSensor",i+1);
        if (simxGetObjectHandle(clientID, sensorName, &sonarHandle[i], simx_opmode_oneshot_wait) != simx_return_ok)
            std::cout << "Error on connecting to sensor " + i+1;
        else
        {
            std::cout << "Connected to sensor\n";
            /* Reads current value */
            simxReadProximitySensor(clientID,sonarHandle[i],NULL,NULL,NULL,NULL,simx_opmode_streaming);
        }
    }
    /* Get the robot current absolute position */
    simxGetObjectPosition(clientID,handle,-1,robotPosition,simx_opmode_streaming);
    simxGetObjectOrientation(clientID,handle,-1,robotOrientation,simx_opmode_streaming);
    initialPose[0]=robotPosition[0];
    initialPose[1]=robotPosition[1];
    initialPose[2]=robotOrientation[2];
    for(int i = 0; i < 3; i++)
    {
        robotLastPosition[i] = robotPosition[i];
    }
    /* Get the encoder data */
    /*simxGetJointPosition(clientID,motorHandle[0], &encoder[0],simx_opmode_streaming);  // left
    simxGetJointPosition(clientID,motorHandle[1], &encoder[1],simx_opmode_streaming);  // right
    std::cout << encoder[0] << std::endl;
    std::cout << encoder[1] << std::endl;*/

}

int Robot::frenteLivre() {
    return ((sonarReadings[3]==-1 || sonarReadings[3]>0.75) && (sonarReadings[4]==-1 || sonarReadings[4]>0.75));
}
void Robot::detectedPosition(simxFloat** position){
    int i =0;
    simxFloat posX,posY,angle,robotRay;
    robotRay = 0.21;
    angle = robotOrientation[2];
    //std::cout<<"positionRobot"<<i<<" values, x = "<<robotPosition[0]<<" y = "<<robotPosition[1]<<" gamma = "<<angle<<" bot ray"<<robotRay<<std::endl;
    for(i;i<8;i++){
        //std::cout<<"sensor"<<i<<" values, sonar = "<<sonarReadings[i]<<" sensorAngle = "<<sensorAngle[i]<<std::endl;
        if(sonarReadings[i]==-1){
            posX = -5000;
            posY = -5000;
        }else{
            posX = robotPosition[0]+(sonarReadings[i]+robotRay)*cos(angle+sensorAngle[i]);
            posY = robotPosition[1]+(sonarReadings[i]+robotRay)*sin(angle+sensorAngle[i]);
        }
        position[i][0] = posX;
        position[i][1] = posY;
        //std::cout<<"sensor"<<i<<" seen obstacle, x = "<<posX<<" y = "<<posY<<std::endl;
    }
}
void Robot::updateInfo() {

    /* Update sonars */
    for(int i = 0; i < NUM_SONARS; i++)
    {
        simxUChar state;       // sensor state
        simxFloat coord[3];    // detected point coordinates [only z matters]

        /* simx_opmode_streaming -> Non-blocking mode */

        /* read the proximity sensor
         * detectionState: pointer to the state of detection (0=nothing detected)
         * detectedPoint: pointer to the coordinates of the detected point (relative to the frame of reference of the sensor) */
        if (simxReadProximitySensor(clientID,sonarHandle[i],&state,coord,NULL,NULL,simx_opmode_buffer)==simx_return_ok)
        {
            if(state > 0)
                sonarReadings[i] = coord[2];
            else
                sonarReadings[i] = -1;
        }
    }
    for(int i = 0; i < 3; i++)
    {
        robotLastPosition[i] = robotPosition[i];
    }
    /*std::cout << robotPosition[0] << "\t";
    std::cout << robotPosition[1] << "\t";
    std::cout << robotPosition[2] << std::endl;*/

    /* Get the robot current position and orientation */
    simxGetObjectPosition(clientID,handle,-1,robotPosition,simx_opmode_streaming);
    simxGetObjectOrientation(clientID,handle,-1,robotOrientation,simx_opmode_streaming);
/*
    lastEncoder[0] = encoder[0];
    lastEncoder[1] = encoder[1];

    /* Get the encoder data *//*
    if (simxGetJointPosition(clientID,motorHandle[0], &encoder[0],simx_opmode_streaming) == simx_return_ok)
        std::cout << "ok left enconder"<< encoder[0] << std::endl;  // left
    if (simxGetJointPosition(clientID,motorHandle[1], &encoder[1],simx_opmode_streaming) == simx_return_ok)
        std::cout << "ok right enconder"<< encoder[1] << std::endl;  // right
*/


}
void Robot::update() {

    float vRight,vLeft;
    if (blockedFront()) {
        std::cout << "--> FRENTE BLOQUEADA"<< std::endl;
        vRight = 0.5;
        vLeft = -0.5;
    } else {
        std::cout << "--> FRENTE LIVRE"<< std::endl;
        vRight = 2;
        vLeft = 2;
    }

//    move(20,-10);
    move(vRight,vLeft);


//    simxSetJointTargetVelocity(clientID, motorHandle[0], velocity, simx_opmode_streaming);
//    simxSetJointTargetVelocity(clientID, motorHandle[1], velocity, simx_opmode_streaming);

    /* Update sonars */
    for(int i = 0; i < NUM_SONARS; i++)
    {
        simxUChar state;       // sensor state
        simxFloat coord[3];    // detected point coordinates [only z matters]

        /* simx_opmode_streaming -> Non-blocking mode */

        /* read the proximity sensor
         * detectionState: pointer to the state of detection (0=nothing detected)
         * detectedPoint: pointer to the coordinates of the detected point (relative to the frame of reference of the sensor) */
        if (simxReadProximitySensor(clientID,sonarHandle[i],&state,coord,NULL,NULL,simx_opmode_buffer)==simx_return_ok)
        {
            if(state > 0)
                sonarReadings[i] = coord[2];
            else
                sonarReadings[i] = -1;
        }
    }
    for(int i = 0; i < 3; i++)
    {
        robotLastPosition[i] = robotPosition[i];
    }
    std::cout << robotPosition[0] << "\t";
    std::cout << robotPosition[1] << "\t";
    std::cout << robotPosition[2] << std::endl;

    /* Get the robot current position and orientation */
    simxGetObjectPosition(clientID,handle,-1,robotPosition,simx_opmode_streaming);
    simxGetObjectOrientation(clientID,handle,-1,robotOrientation,simx_opmode_streaming);

    lastEncoder[0] = encoder[0];
    lastEncoder[1] = encoder[1];

    /* Get the encoder data */
    if (simxGetJointPosition(clientID,motorHandle[0], &encoder[0],simx_opmode_streaming) == simx_return_ok)
        std::cout << "ok left enconder"<< encoder[0] << std::endl;  // left
    if (simxGetJointPosition(clientID,motorHandle[1], &encoder[1],simx_opmode_streaming) == simx_return_ok)
        std::cout << "ok right enconder"<< encoder[1] << std::endl;  // right



}
int Robot::blockedFront() {
    return ((sonarReadings[2]!=-1 && sonarReadings[2]<0.3) || sonarReadings[3]!=-1 || sonarReadings[4]!=-1 || (sonarReadings[5]!=-1 && sonarReadings[5]<0.3));
}

int Robot::blockedRight() {
    turnRight = 1;
    return ((sonarReadings[6]!=-1 && sonarReadings[6]<0.3) || sonarReadings[7]!=-1 || sonarReadings[8]!=-1 || (sonarReadings[9]!=-1 && sonarReadings[9]<0.3));
}

int Robot::blockedLeft() {
    turnRight = 0;
    return ((sonarReadings[14]!=-1 && sonarReadings[14]<0.3) || sonarReadings[15]!=-1 || sonarReadings[0]!=-1 || (sonarReadings[1]!=-1 && sonarReadings[1]<0.3));
}

//int Robot::blockedLeft() {
//    return ((sonarReadings[15]!=-1 && sonarReadings[15]<0.75) || (sonarReadings[0]!=-1 && sonarReadings[0]<0.75));
//}

void Robot::moveForward() {
    timeInCircle = 0;
    float vRight,vLeft;
    if (blockedFront()) {
        std::cout << "--> FRENTE BLOQUEADA"<< std::endl;
        vRight = -1;
        vLeft = 1;
    } else {
        std::cout << "--> FRENTE LIVRE"<< std::endl;
        vRight = 5;
        vLeft = 5;
    }
    move(vLeft,vRight);
}

void Robot::moveInCircle() {
    float vRight,vLeft;
    if (blockedFront()) {
        std::cout << "--> CIRCULANDO FRENTE BLOQUEADA"<< std::endl;
        vRight = -2;
        vLeft = 2;
    } else {
        std::cout << "--> CIRCULANDO FRENTE LIVRE"<< std::endl;
        if (turnRight) {
            std::cout << "--> CIRCULANDO --> turnRight"<< std::endl;
            vRight = 1.5;
            vLeft = 3;
        } else {
            std::cout << "--> CIRCULANDO --> turn left!"<< std::endl;
            vRight = 3;
            vLeft = 1.5;
        }

        timeInCircle++;
    }
    move(vLeft,vRight);
}


void Robot::updatePosition() {
    objectRight = blockedRight();
    objectLeft = blockedLeft();

    if (start) {
        std::cout << "--> TO PROCURANDOO A PAREDE..."<< std::endl;
        moveForward();
        if (objectRight || objectLeft)
            start = 0;
    } else {
        if (objectRight || objectLeft) {
            std::cout << "--> Paredinha aqui do lado..."<< std::endl;
            moveForward();
        } else {
            std::cout << "--> CADE A PAREDE??"<< std::endl;
            moveInCircle();
            if (timeInCircle > 2000)
                start = 1;
        }
    }

}

void Robot::writeGT() {
    /* write data to file */
    /* file format: robotPosition[x] robotPosition[y] robotPosition[z] robotLastPosition[x] robotLastPosition[y] robotLastPosition[z]
     *              encoder[0] encoder[1] lastEncoder[0] lastEncoder[1] */
    FILE *data =  fopen("gt.txt", "at");
    if (data!=NULL)
    {
        for (int i=0; i<3; ++i)
            fprintf(data, "%.2f\t",robotPosition[i]);
        for (int i=0; i<3; ++i)
            fprintf(data, "%.2f\t",robotLastPosition[i]);
        for (int i=0; i<3; ++i)
            fprintf(data, "%.2f\t",robotOrientation[i]);
        for (int i=0; i<2; ++i)
            fprintf(data, "%.2f\t",encoder[i]);
        for (int i=0; i<2; ++i)
            fprintf(data, "%.2f\t",lastEncoder[i]);
        fprintf(data, "\n");
        fflush(data);
        fclose(data);
      }
      else
        std::cout << "Unable to open file";
}

void Robot::writeSonars() {
    /* write data to file */
    /* file format: robotPosition[x] robotPosition[y] robotPosition[z] robotLastPosition[x] robotLastPosition[y] robotLastPosition[z]
     *              encoder[0] encoder[1] lastEncoder[0] lastEncoder[1] */
    FILE *data =  fopen("sonar.txt", "at");
    if (data!=NULL)
    {
        if (data!=NULL)
        {
            for (int i=0; i<NUM_SONARS; ++i)
                fprintf(data, "%.2f\t",sonarReadings[i]);
            fprintf(data, "\n");
            fflush(data);
            fclose(data);
        }
    }
}

void Robot::printPosition() {
  simxFloat position[3];
  //simxGetObjectPosition(_clientID, _handle, -1, position, simx_opmode_streaming);
  //std::cout << "[" << position[0] << ", " << position[1] << ", " << position[2] << "]" << std::endl;
}

void Robot::move(float vLeft, float vRight) {
    simxSetJointTargetVelocity(clientID, motorHandle[0], vLeft, simx_opmode_streaming);
    simxSetJointTargetVelocity(clientID, motorHandle[1], vRight, simx_opmode_streaming);
}


void Robot::changeCoordinateToOrigin(float *localFrame, float *transformedFrame) {

}

//Acelerômetro
//simxReadForceSensor(clientID,Acelerometro,NULL,Forca,NULL,simx_opmode_streaming);
//simxGetObjectFloatParameter(clientID,MassaAcelerometro,3005,MassaSensor,simx_opmode_streaming);
//for(int i=0;i<3;i++)
//  Acel[i]=Forca[i]/MassaSensor[0];
/***************************************************************
 * This method rotates the robot to a global coordinate system
 ***************************************************************/
/*void Robot::changeCoordinatesToOrigin(float* datax, float* datay, float theta0)
{
    //The best estimate of the robot angle is simply theta = thetaold + deltatheta.
    //X and Y coordinates must be rotated based on this angle.

    //Floats that represent the robot start position, whith respect to the origin
    float x0,y0; //,theta0;
    float RoLoc, ThetaLoc, RoRot, ThetaRot, RoTrans, XRot, YRot, XTrans, YTrans;

    // Get original positions of robots
    x0 = startPosition[0];
    y0 = startPosition[1];
    //theta0 = startDirectionZ.ToDouble();
    //theta0 = theta0 - *deltatheta;

    //Calculate point in polar coordinates
    RoLoc = sqrt(pow(*datax,2)+pow(*datay,2));
    if ( (*datax==0) && (*datay==0)) {
        ThetaLoc = 0;
    }else if ((*datay == 0) && (*datax>0)) {
        ThetaLoc = 0;
    } else if ((*datay==0) && (*datax<0)) {
        ThetaLoc = 3.1415;
    } else if ((*datax==0) && (*datay>0)) {
        ThetaLoc = 3.1415/2;
    } else if ((*datax==0) && (*datay<0)) {
        ThetaLoc = -3.1415/2;
    } else {
        ThetaLoc = atan(*datax/ *datay);
        // There are many angles with the same atan.
        // Some corrections must be made.
        if ((*datax<0) && (*datay<0)) {
                ThetaLoc = ThetaLoc + 3.1415;
        }
        if ((*datax>0) && (*datay<0)) {
                ThetaLoc = ThetaLoc + 3.1415;
        }
    }


    //Calculate Rotation of this reference system
    ThetaRot = ThetaLoc+theta0;
    RoRot = RoLoc;
    XRot = RoRot*sin(ThetaRot);
    YRot = RoRot*cos(ThetaRot);

    //Claculate Translation of this reference system
    XTrans = XRot + x0;
    YTrans = YRot + y0;

    *datax = XTrans;
    *datay = YTrans;
}
*/
/*
void Robot::rotate(float xPGlobal, float yPGlobal, float zPGlobal, float xDGlobal, float yDGlobal, float zDGlobal)
{
    // Floats that represent the robot start position
    float xP,yP,zP,xD,yD,zD;

    // R1: robot slave (must be rotated)
    xP = startPosition[0];
    yP = startPosition[1];
    zP = startPosition[2];

    xD = startOrientation[0];
    yD = startOrientation[1];
    zD = startOrientation[2];

    // R = [x0, y0, z0, pitch0, roll0, yaw0];
    float deltax = xP - xPGlobal;
    float deltay = yP - yPGlobal;
    float deltaz = zP - zPGlobal;

    // Pitch - y-axis
    float deltapitch = yD - yDGlobal;
    // Roll - x-axis
    float deltaroll = xD - xDGlobal;
    // Yaw - z-axis
    float deltayaw = zD - zDGlobal;

    // R1: new coordinates
    xP = (xP*cos(deltapitch)+(yP*sin(deltaroll)+zP*cos(deltaroll))*sin(deltapitch))*cos(deltayaw)-(yP*cos(deltaroll)-zP*sin(deltaroll))*sin(deltayaw);
    yP = (xP*cos(deltapitch)+(yP*sin(deltaroll)+zP*cos(deltaroll))*sin(deltapitch))*sin(deltayaw)+(yP*cos(deltaroll)-zP*sin(deltaroll))*cos(deltayaw);
    zP = -xP*sin(deltapitch)+(yP*sin(deltaroll)+zP*cos(deltaroll))*cos(deltapitch);

    xD = deltaroll;
    yD = deltapitch;
    zD = deltayaw;

    // Checar aqui a transformação para String
    startPosition[0] = xP;
    startPosition[1] = yP;
    startPosition[2] = zP;

  //  startDirectionX = xD;
  //  startDirectionY = yD;
  //  startDirectionZ = zD;
}*/
