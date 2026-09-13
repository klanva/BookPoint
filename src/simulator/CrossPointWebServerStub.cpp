#include "network/CrossPointWebServer.h"
#include "activities/network/CrossPointWebServerActivity.h"

CrossPointWebServer::CrossPointWebServer() = default;
CrossPointWebServer::~CrossPointWebServer() = default;

void CrossPointWebServer::begin() { running = true; }
void CrossPointWebServer::stop() { running = false; }
void CrossPointWebServer::handleClient() {}

CrossPointWebServer::WsUploadStatus CrossPointWebServer::getWsUploadStatus() const {
  return {};
}

void CrossPointWebServerActivity::onEnter() {}
void CrossPointWebServerActivity::onExit() {}
void CrossPointWebServerActivity::loop() {}
void CrossPointWebServerActivity::render(RenderLock&&) {}
