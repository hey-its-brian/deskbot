// ---------------------------------------------------------------------------
//  WebApp.h - the control page and its small JSON API.
//
//    GET  /                    the page
//    GET  /api/status          everything the page shows
//    POST /api/cmd     c=...   any serial command (see docs/SERIAL.md)
//    POST /api/settings        form fields, validated, saved to flash
//    POST /api/notify  emotion=happy&hold=5   show a face for a while
// ---------------------------------------------------------------------------
#pragma once

namespace web {
void begin();
void loop();
}
