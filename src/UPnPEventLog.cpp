#include "config.h"

#ifdef ENABLE_UPNP

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include "UPnPEventLog.h"

#include <upnp/ixml.h>

#include <sstream>

#include "UPnPCompatibility.h"

namespace
{
	std::string SafeDomString(const DOMString value)
	{
		return value ? std::string(value) : std::string();
	}

	std::string SafeNodeName(IXML_Node* node)
	{
		return node ? SafeDomString(ixmlNode_getNodeName(node)) : std::string("<unknown>");
	}
}

std::string FormatUPnPEventLog(const std::string& sidUtf8, int eventKey, IXML_Document* changedVariables)
{
	std::ostringstream oss;
	oss << "UPNP_EVENT_RECEIVED:\n    SID: " << sidUtf8 << "\n    Key: " << eventKey << "\n    Property list:";
	if (changedVariables == nullptr) {
		oss << "\n    <null document>";
		return oss.str();
	}
	IXML_Node* current = ixmlNode_getFirstChild(reinterpret_cast<IXML_Node*>(changedVariables));
	while (current && ixmlNode_getNodeType(current) != eELEMENT_NODE) {
		current = ixmlNode_getNextSibling(current);
	}
	IXML_Element* root = reinterpret_cast<IXML_Element*>(current);
	if (root == nullptr) {
		oss << "\n    Empty property list.";
		return oss.str();
	}
	bool anyProperty = false;
	for (IXML_Node* property = ixmlNode_getFirstChild(reinterpret_cast<IXML_Node*>(root)); property != nullptr; property = ixmlNode_getNextSibling(property)) {
		if (ixmlNode_getNodeType(property) != eELEMENT_NODE) {
			continue;
		}
		IXML_Node* variableNode = ixmlNode_getFirstChild(property);
		while (variableNode && ixmlNode_getNodeType(variableNode) != eELEMENT_NODE) {
			variableNode = ixmlNode_getNextSibling(variableNode);
		}
		std::string tag = SafeNodeName(variableNode ? variableNode : property);
		std::string value;
		if (variableNode) {
			IXML_Node* textNode = ixmlNode_getFirstChild(variableNode);
			if (textNode && ixmlNode_getNodeType(textNode) == eTEXT_NODE) {
				value = SafeDomString(ixmlNode_getNodeValue(textNode));
			}
		}
		if (value.empty()) {
			value = "<empty>";
		}
		oss << "\n        " << tag << "='" << value << "'";
		anyProperty = true;
	}
	if (!anyProperty) {
		oss << "\n    Empty property list.";
	}
	return oss.str();
}

#endif // ENABLE_UPNP
