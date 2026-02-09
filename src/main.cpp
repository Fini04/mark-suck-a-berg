const int DEBUG = true;

#include <Arduino.h>
#include <esp_types.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <esp_bt.h>
#include "soc/rtc_cntl_reg.h"

#include "engine/engine.hpp"
#include "axisconfigs.hpp"
#include "SystemSate.hpp"
#include "dashboard.hpp"

/*
   |-----------------------------------------------------------------|
   | Hiermit entschuldige ich mich schon einmal im Voraus bei allen, |
   | die dieses ungetüm an Programcode lesen müssen.                 |
   |-----------------------------------------------------------------|
*/

#pragma region INTERRUPTS
volatile unsigned long lastInterruptTimeXHome = 0;
void IRAM_ATTR engine1ValidHome()
{
  unsigned long now = millis();
  if (now - lastInterruptTimeXHome > 200)
  { // 100 ms
    if (digitalRead(axisX.config.pinInputHome) == LOW)
    {
      axisX.validHome = true;
      //axisX.steps = 0; // hard-stop if home pressed during a step
      logLine("X axis home pressed");
    }
    lastInterruptTimeXHome = now;
  }
}

volatile unsigned long lastInterruptTimeXStep = 0;
void IRAM_ATTR engine1StepMade()
{
  unsigned long now = millis();
  if (now - lastInterruptTimeXStep > 200)
  { // 100 ms
    if (axisX.steps != 0)
    {
      axisX.steps--;
    }
    lastInterruptTimeXStep = now;
  }
}

volatile unsigned long lastInterruptTimeYHome = 0;
void IRAM_ATTR engine2ValidHome()
{
  unsigned long now = millis();
  if (now - lastInterruptTimeYHome > 200)
  { // 100 ms
    if (digitalRead(axisY.config.pinInputHome) == LOW)
    {
      axisY.validHome = true;
      //axisY.steps = 0; // hard-stop if home pressed during a step
      logLine("Y axis home pressed");
    }
    lastInterruptTimeYHome = now;
  }
}

volatile unsigned long lastInterruptTimeYStep = 0;
void IRAM_ATTR engine2StepMade()
{
  unsigned long now = millis();
  if (now - lastInterruptTimeYStep > 100)
  { // 100 ms
    if (axisY.steps != 0)
    {
      axisY.steps--;
    }
    lastInterruptTimeYStep = now;
  }
}
#pragma endregion

SystemState systemState;
void logLine(const String &line);
static void setupServer();
void setup()
{
  systemState = SystemState::Stopped;

#pragma region POWER_SAVING
  WiFi.mode(WIFI_AP);
  btStop();
  esp_bt_controller_disable();

#pragma endregion

#pragma region PIN_INITIALISATION
  pinMode(axisX.config.pinOutForeward, OUTPUT);
  pinMode(axisX.config.pinOutBackward, OUTPUT);
  pinMode(axisY.config.pinOutForeward, OUTPUT);
  pinMode(axisY.config.pinOutBackward, OUTPUT);

  pinMode(axisX.config.pinInputHome, INPUT);
  pinMode(axisX.config.pinInputRotationFeedback, INPUT);
  pinMode(axisY.config.pinInputHome, INPUT);
  pinMode(axisY.config.pinInputRotationFeedback, INPUT);
  attachInterrupt(
      digitalPinToInterrupt(axisX.config.pinInputHome),
      engine1ValidHome,
      FALLING // LOW → gedrückt (wegen Pullup)
  );
  attachInterrupt(
      digitalPinToInterrupt(axisX.config.pinInputRotationFeedback),
      engine1StepMade,
      FALLING // HIGH → LOW
  );
  attachInterrupt(
      digitalPinToInterrupt(axisY.config.pinInputHome),
      engine2ValidHome,
      FALLING // LOW → gedrückt (wegen Pullup)
  );
  attachInterrupt(
      digitalPinToInterrupt(axisY.config.pinInputRotationFeedback),
      engine2StepMade,
      FALLING // HIGH → LOW
  );
#pragma endregion

  Serial.begin(9600);

  if (DEBUG)
  {
    logLine("System starting...");
  }

#pragma region WIFI_DASHBOARD
  const char *apSsid = "ESP-Dashboard";
  const char *apPass = "esp12345";
  WiFi.softAP(apSsid, apPass);
  IPAddress ip = WiFi.softAPIP();
  if (DEBUG)
  {
    Serial.print("Dashboard AP IP: ");
    Serial.println(ip);
  }
#pragma endregion

  setupServer();
}

AsyncWebServer server(80);
volatile bool runRequested = false;
volatile bool stopRequested = false;
volatile bool homingRequested = false;
volatile bool pauseRequested = false;
String logBuffer;
const size_t LOG_MAX = 2000;

void logLine(const String &line)
{
  String stamped = "[" + String(millis()) + " ms] " + line;
  Serial.println(stamped);
  logBuffer += stamped;
  logBuffer += "\n";
  if (logBuffer.length() > LOG_MAX)
  {
    logBuffer.remove(0, logBuffer.length() - LOG_MAX);
  }
}

static String statusJson()
{
  String json = "{";
  json += "\"running\":" + String(systemState == SystemState::Running ? "true" : "false");
  json += ",\"paused\":" + String(systemState == SystemState::Ready ? "true" : "false");
  json += ",\"stopped\":" + String(systemState == SystemState::Stopped ? "true" : "false");
  json += ",\"homing\":" + String(systemState == SystemState::Homing ? "true" : "false");
  json += "}";
  return json;
}

static void setupServer()
{
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/html", dashboardHtml); });
  server.on("/run", HTTP_POST, [](AsyncWebServerRequest *request)
            {
    logLine("Run requested");
    if (!(systemState == SystemState::Running || systemState == SystemState::Homing)) {
      runRequested = true;
      request->send(202, "text/plain", "scheduled");
      return;
    }
    request->send(409, "text/plain", "busy"); });
  server.on("/home", HTTP_POST, [](AsyncWebServerRequest *request)
            {
    logLine("home requested");
    if (systemState != SystemState::Homing) {
      homingRequested = true;
      request->send(202, "text/plain", "scheduled");
      return;
    }
    request->send(409, "text/plain", "busy"); });
  server.on("/stop", HTTP_POST, [](AsyncWebServerRequest *request)
            {
    logLine("Stop requested");
    stopRequested = true;
    request->send(202, "text/plain", "stopping");
    return; });
  server.on("/pause", HTTP_POST, [](AsyncWebServerRequest *request)
            {
    logLine("Pause requested");
    pauseRequested = true;
    request->send(202, "text/plain", "pauseing");
    return; });
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "application/json", statusJson()); });
  server.on("/logs", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", logBuffer); });
  server.begin();
}

const int LINES = 2;
const int X_STEPS = 7;
const int Y_STEPS = 28;

int cycle = 0;
int stateInCycle = 0;
bool success = 0;
static void runPattern()
{
  if (cycle < LINES)
  {
    switch (stateInCycle)
    {
    case 0:
    engineStep(&axisY, FOREWARD, &success);
    if (success)
      {
        axisX.steps = X_STEPS;
        logLine("cycle " + String(cycle) + ": step X");
        stateInCycle = 1;
      }
      break;
    case 1:
      engineStep(&axisX, FOREWARD, &success);
      if (success)
      {
        axisY.validHome = false;
        logLine("cycle " + String(cycle) + ": find home Y");
        stateInCycle = 2;
      }
      break;
    case 2:
      engineFindHome(&axisY);
      if (axisY.validHome)
      {
        axisX.steps = X_STEPS;
        logLine("cycle " + String(cycle) + ": step X");
        stateInCycle = 3;
      }
      break;
    case 3:
      engineStep(&axisX, FOREWARD, &success);
      if (success)
      {
        axisY.steps = Y_STEPS;
        stateInCycle = 0;
        logLine("cycle last: step Y");
        cycle++;
      }
      break;

    default:
      systemState = SystemState::Stopped;
      break;
    }
  }
  else if (cycle = LINES)
  {
    switch (stateInCycle)
    {
    case 0:
      engineStep(&axisY, FOREWARD, &success);
      if (success)
      {
        axisX.steps = X_STEPS - 5;
        logLine("cycle last: step X");
        stateInCycle = 1;
      }
      break;
    case 1:
      engineStep(&axisX, FOREWARD, &success);
      if (success)
      {
        logLine("cycle last: find home Y");
        axisY.validHome = false;
        axisY.steps = 0;
        stateInCycle = 2;
      }
      break;
      case 2:
      engineFindHome(&axisY);
      if (axisY.validHome)
      {
        axisY.steps = 2;
        logLine("cycle last: step Y");
        stateInCycle = 3;
      }
      break;
    case 3:
      engineStep(&axisY, FOREWARD, &success);
      if (success)
      {
        stateInCycle = 0;
        cycle = 0;
        systemState = SystemState::Stopped;
      }
      break;

    default:
      systemState = SystemState::Stopped;
      break;
    }
  }
}

static void homing()
{
  engineFindHome(&axisX);
  engineFindHome(&axisY);
}

static void stop()
{
  engineRunning(&axisX, false);
  engineRunning(&axisY, false);
}

void clearFlaggs()
{
  runRequested = false;
  homingRequested = false;
  stopRequested = false;
  pauseRequested = false;
}

void loop()
{
  switch (systemState)
  {
  case SystemState::Stopped:
    stop();
    axisX.steps = 0;
    axisY.steps = 0;
    axisX.validHome = false;
    axisY.validHome = false;
    cycle = 0;
    stateInCycle = 0;
    
    if (homingRequested)
    {
      clearFlaggs();
      systemState = SystemState::Homing;
      logLine("homing");
    }
    else if (runRequested)
    {
      clearFlaggs();
      systemState = SystemState::Homing;
      logLine("homing before run");
    }
    break;
    
    case SystemState::Homing:
    if (stopRequested)
    {
      systemState = SystemState::Stopped;
      clearFlaggs();
      logLine("stopping");
      return;
    }
    homing();

    if (axisX.validHome && axisY.validHome)
    {
      systemState = SystemState::Ready;
      cycle = 0;
      logLine("ready");
      logLine("cycle " + String(cycle) + ": step Y");
      axisY.steps = Y_STEPS;
      clearFlaggs();
    }
    break;

  case SystemState::Running:
  if (stopRequested)
  {
    systemState = SystemState::Stopped;
    clearFlaggs();
    logLine("stopping");
    return;
  }
  runPattern();
  
    if (pauseRequested)
    {
      systemState = SystemState::Ready;
      clearFlaggs();
      logLine("pausing");
    }
    break;

  case SystemState::Ready:
  if (stopRequested)
  {
    systemState = SystemState::Stopped;
    clearFlaggs();
    logLine("stopping");
    return;
  }
  stop();
  if (runRequested)
  {
      systemState = SystemState::Running;
      clearFlaggs();
      logLine("running");
    }
    else if (homingRequested)
    {
      systemState = SystemState::Homing;
      clearFlaggs();
      logLine("homing");
    }
    break;

  default:
    systemState = SystemState::Stopped;
    break;
  }
}
