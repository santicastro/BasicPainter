#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SoftwareSerial.h>

SoftwareSerial grbl(10, 11); // RX, TX

double segmentLength = 20; // mm
char lineBuffer[100];

double paddingLeft = 10;
double paddingRight = 10;
double paddingTop = 10;

double height = 600;
double width = 1200;

typedef struct {
  double x;
  double y;
} Point;

Point M1 = { -paddingLeft, height + paddingTop };
Point M2 = { width + paddingRight, height + paddingTop };

Point currentPos;

Point toPolar(Point cartesian) {
  Point value = {
    module(getVector(M1, cartesian)),
    module(getVector(M2, cartesian))
  };
  /*
    Serial.print("  Cartesian: ");
    Serial.print(cartesian.x);
    Serial.print(",");
    Serial.println(cartesian.y);
    Serial.print("  Polar: ");
    Serial.print(value.x);
    Serial.print(",");
    Serial.println(value.y);
  */
  return value;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("Setup finished");
  currentPos = {(double)0.0, (double)0.0};


  sendToGrbl("G90"); // absolute coordinates
  sendToGrbl("G21");
  generateTranslateGcode("G92", M1.x, M1.y, 0.0); // set zero
  sendToGrbl(lineBuffer);

  char *line = (char *)malloc(100);
  strcpy(line, "G1 X123.34 Y256.78 F5000");
  processLine(line);

  Serial.println("-----");
  strcpy(line, "G1 X0 Y600 F1000");
  processLine(line);
}

double module(Point p) {
  return sqrt(p.x * p.x + p.y * p.y);
}

Point getVector(Point p1, Point p2) {
  return {p2.x - p1.x, p2.y - p1.y};
}

double distance(Point p1, Point p2) {
  Point p = getVector(p1, p2);
  return module(p);
}
void generateTranslateGcode(const char *op, double x, double y, double f) {
  sprintf(lineBuffer, "%s X", op);
  dtostrf(x, 4, 2, &lineBuffer[strlen(lineBuffer)]);
  strcpy(&lineBuffer[strlen(lineBuffer)], " Y");
  dtostrf(y, 4, 2, &lineBuffer[strlen(lineBuffer)]);
  if (f) {
    strcpy(&lineBuffer[strlen(lineBuffer)], " F");
    dtostrf(f, 4, 2, &lineBuffer[strlen(lineBuffer)]);
  }
}

void waitForOk() {
  // read the receive buffer (if anything to read)
  char c, lastc;
  while (true) {
    if (grbl.available()) {
    c = grbl.read();
      if (lastc == 'o' && c == 'k') {
        Serial.println("GRBL <- OK");
      }
      lastc = c;
    }
    delay(3);
  }
}

void sendToGrbl(const char *gcode) {
  Serial.print("GRBL -> ");
  Serial.println(gcode);

  // vaciar buffer
  while (grbl.available()) {
    grbl.read();
  }
  grbl.println(gcode);
  //waitForOk();
}

void processLine(char *line) {
  double X;
  double Y;
  double F = 100;
  char *lineCopy = strdup(line);
  char *op = strtok(lineCopy, " ");
  if ((strcmp(op, "G1") == 0) || (strcmp(op, "G0") == 0)) {
    char *param;
    while (param = strtok(NULL, " ")) {
      //        Serial.print("Param: ");
      //        Serial.print(param);
      //        Serial.println(".");
      if (param[0] == 'X') {
        //sscanf(&param[1], "%lf", &X);
        X = atof(&param[1]);
      } else if (param[0] == 'Y') {
        //sscanf(&param[1], "%lf", &Y);
        Y = atof(&param[1]);
      } else if (param[0] == 'F') {
        //sscanf(&param[1], "%lf", &F);
        F = atof(&param[1]);
      } else {
        Serial.print("Unknown param ");
        Serial.println(param);
      }
    }

    Point targetPos = {X, Y};
    Point vector = getVector(currentPos, targetPos);
    double v_mod = module(vector);

    double segments = v_mod / segmentLength;

    double deltaX = vector.x / segments;
    double deltaY = vector.y / segments;
    Point nextPos;
    Point nextPosPolar;
    for (int i = 1; i < segments; i++) {
      nextPos.x = currentPos.x + deltaX * i;
      nextPos.y = currentPos.y + deltaY * i;
      nextPosPolar = toPolar(nextPos);
      generateTranslateGcode(op, nextPosPolar.x, nextPosPolar.y, F);
      sendToGrbl(lineBuffer);
    }
    nextPosPolar = toPolar(targetPos);
    generateTranslateGcode(op, nextPosPolar.x, nextPosPolar.y, F);
    sendToGrbl(lineBuffer);

    currentPos.x = targetPos.x;
    currentPos.y = targetPos.y;

  } else {
    sendToGrbl(line);
  }
  free(lineCopy);
}

void loop() {

  delay(500);
  return (0);
}
