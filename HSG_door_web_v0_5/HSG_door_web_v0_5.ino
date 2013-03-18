#include "SPI.h"
#include "Ethernet.h"
#include "WebServer.h"

static uint8_t mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
static uint8_t ip[] = { 192, 168, 1, 210 };

int IRpin = 2;

int greenLed = 7;
int redLed = 8;
int triggerPin = 9;
int thresholdLimit = 5;
int threshold = 0;

int doorPin = 2;
boolean doorShouldOpen = false;
int doorLoopCount = 0;

P(Page_header) = "<!DOCTYPE html><html><head><title>HSG Door Web</title><meta name=\"viewport\" content=\"width=device-width\"><meta name=\"apple-mobile-web-app-capable\" content=\"yes\"><link rel=\"apple-touch-icon\" href=\"http://dl.dropbox.com/u/3361521/hackerspace/icon.png\" /><script src=\"//ajax.googleapis.com/ajax/libs/jquery/1.8.1/jquery.min.js\"></script></head><body>";
P(Title_msg) = "<h1>HackerspaceSG Door Web</h1>";
P(CSS) = "<style type=\"text/css\">body{font-family:Helvetica, Arial, \"San-Serif\"} a{display:block;padding:10px; margin:0px auto 0px auto; width:150px; text-align:center; text-decoration:none; font-weight:bold; font-size:15px; color:white; background-color:#3CAD34; border-radius: 22px; -moz-border-radius: 22px; -webkit-border-radius: 22px; border: 6px solid #AAC46C;}h2{display:block; background-color: red; color:white; font-weight:bold; font-size:25px;}</style>";
P(Page_footer) = "</body></html>";
P(REST_success) = "{\"status\":\"OPEN\"}";

#define PREFIX ""
WebServer webserver(PREFIX, 80);

float getDistance()
{
  float volts = analogRead(IRpin)*0.0048828125;
  return 65*pow(volts, -1.10);
}

void checkSensor()
{
  if (doorShouldOpen) return;
  float distance = getDistance();
  if (distance < 70)
  {
    threshold++;
    if (threshold > thresholdLimit)
    {
      openDoor();
    }
  }
  else
  {
    threshold = 0;
  }
}

void isButtonTriggered()
{
  if (digitalRead(triggerPin) == HIGH)
  {
    openDoor();
  }
}

void openDoor()
{
  doorShouldOpen = true;
}

void doorOpenInterrupt()
{
  if (doorShouldOpen)
  {
    if (doorLoopCount == 0)
    {
      digitalWrite(doorPin, HIGH);
      digitalWrite(redLed, LOW);
      digitalWrite(greenLed, HIGH);
    }
    doorLoopCount++;
    if (doorLoopCount == 5000)
    {
      digitalWrite(doorPin, LOW);
      digitalWrite(redLed, HIGH);
      digitalWrite(greenLed, LOW);
      doorShouldOpen = false;
      doorLoopCount = 0;
    }
  }
}

boolean shouldAllowAccess(WebServer &server)
{
  if (server.checkCredentials("xxxxxxxxxxxxxxxxxxxxxx"))
  {
    return true;
  }
  else
  {
    server.httpUnauthorized();
  }
  return false;
}

void homeCmd(WebServer &server, WebServer::ConnectionType type, char *, bool)
{
  if (shouldAllowAccess(server))
  {
    server.httpSuccess();
    if (type != WebServer::HEAD)
    {
      P(Open_link) = "<p><a href=\"/open.html\" id=\"go_open\">Open Door</a></p><h2 style=\"display:none;\" id=\"status_box\">Door Open!</h2>";
      P(JS_block) = "<script type=\"text/javascript\"> $(window).load(function(){  $('#go_open').click(function(e){  e.preventDefault();$.getJSON('/open.json', function(data){ if(data.status == 'OPEN'){  $('#status_box').slideDown().delay(2000).slideUp(); }  });  });   });</script>";
      server.printP(Page_header);
      server.printP(CSS);
      server.printP(Title_msg);
      server.printP(Open_link);
      server.printP(JS_block);
      server.printP(Page_footer);
    }
  }
}

void openCmd(WebServer &server, WebServer::ConnectionType type, char *, bool)
{
  if (shouldAllowAccess(server))
  {
    server.httpSuccess();
    if (type != WebServer::HEAD)
    {
      P(Refresh_header) = "<meta http-equiv=\"refresh\" content=\"3;url=/index.html\">";
      P(Success_message) = "<h2>Door Open!</h2>";
      server.printP(Page_header);
      server.printP(CSS);
      server.printP(Refresh_header);
      server.printP(Title_msg);
      server.printP(Success_message);
      server.printP(Page_footer);
      openDoor();
    }
  }
}

void restOpenCmd(WebServer &server, WebServer::ConnectionType type, char *, bool)
{
  if (shouldAllowAccess(server))
  {
    server.httpSuccess("application/json");
    if (type != WebServer::HEAD)
    {
      server.printP(REST_success);
      openDoor();
    }
  }
}

void setup()
{
  pinMode(redLed, OUTPUT);
  pinMode(greenLed, OUTPUT);
  pinMode(triggerPin, INPUT);
  pinMode(doorPin, OUTPUT);
  digitalWrite(redLed, HIGH);
  digitalWrite(greenLed, LOW);

  Ethernet.begin(mac, ip);
  webserver.setDefaultCommand(&homeCmd);
  webserver.addCommand("index.html", &homeCmd);
  webserver.addCommand("open.html", &openCmd);
  webserver.addCommand("open.json", &restOpenCmd);
  webserver.begin();
}

void loop()
{
  char buff[64];
  int len = 64;
  webserver.processConnection(buff, &len);
  isButtonTriggered();
  checkSensor();
  doorOpenInterrupt();
}
