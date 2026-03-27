//
// UPnP Event Log helper
//

#ifndef UPNP_EVENT_LOG_H
#define UPNP_EVENT_LOG_H

#ifdef ENABLE_UPNP

#include <string>

struct _IXML_Document;
using IXML_Document = _IXML_Document;

std::string FormatUPnPEventLog(const std::string& sidUtf8, int eventKey, IXML_Document* changedVariables);

#endif // ENABLE_UPNP

#endif // UPNP_EVENT_LOG_H
