// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <debug/dag_log.h>
#include <webBrowserHelper/webBrowser.h>


namespace webbrowser
{

IBrowser *get_helper() { return nullptr; }

void WebBrowserHelper::init(const Configuration &cfg)
{
  debug("[BRWS] %s: embedded browser is not supported", __FUNCTION__);
  return;
}

bool WebBrowserHelper::createBrowserWindow() { return false; }
void WebBrowserHelper::update(uint16_t, uint16_t) {}
void WebBrowserHelper::destroyBrowserWindow() {}
void WebBrowserHelper::shutdown() {}

bool WebBrowserHelper::browserExists() { return false; }
bool WebBrowserHelper::canEnable() { return false; }

bool WebBrowserHelper::hasTexture() { return false; }
TEXTUREID WebBrowserHelper::getTexture() { return BAD_TEXTUREID; }
d3d::SamplerHandle WebBrowserHelper::getSampler() const { return d3d::INVALID_SAMPLER_HANDLE; }

void WebBrowserHelper::go(const char *url) {}
void WebBrowserHelper::goBack() {}
void WebBrowserHelper::reload() {}

const char *WebBrowserHelper::getUrl() { return nullptr; }
const char *WebBrowserHelper::getTitle() { return nullptr; }

} // namespace webbrowser