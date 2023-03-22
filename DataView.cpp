#include "DataView.h"
#include <WiFiNINA.h>

String DataView::handleClient(WiFiServer server) {

	WiFiClient client = server.available();   // listen for incoming clients
	String returnButton = NO_BUTTON;
	if (client) {
		String currentLine = "";
		while (client.connected()) {
			if (client.available()) {
				char c = client.read();
				if (c == '\n') {
					if (currentLine.length() == 0) {
						if (request.startsWith("GET /data")) {
							sendData(client);
							request = "";
							break;
						} else if (request.startsWith("GET /btnclick?id=")) {
							int startIndex =
									String("GET /btnclick?id=").length();
							int endIndex = request.length()
									- String(" HTTP/1.1").length();
							returnButton = request.substring(startIndex,
									endIndex);
							request = "";
							break;
						} else if (request.startsWith("GET /")) {
							sendSite(client);
							request = "";
							break;
						}
					} else {
						if (currentLine.startsWith("GET ")) {
							request = currentLine;
						}
						currentLine = "";
					}
				} else if (c != '\r') { // if you got anything else but a carriage return character,
					currentLine += c; // add it to the end of the currentLine
				}
			}
		}
		// close the connection:
		client.stop();
	}
	return returnButton;
}

void DataView::sendSite(WiFiClient client) {
	String dataViewHtml = generateDataViewHTML();
	String jsFunction = generateJsFunction();
	int bufferSize = dataViewHtml.length() + jsFunction.length()
			+ title.length() + sizeof(refreshInterval) + 1000;
	char buffer[bufferSize];
	sprintf(buffer, html.c_str(), title.c_str(), dataViewHtml.c_str(),
			jsFunction.c_str());
	sendHTML(client, "text/html", buffer);
}

void DataView::sendData(WiFiClient client) {
	String data = "[";
	dataField *curField = firstDataField;
	while (curField != NULL) {
		data += "\"" + curField->curValue + "\",";
		curField = curField->next;
	}
	data = data.substring(0, data.length() - 1);
	data += "]";
	sendHTML(client, "application/json", data);
}

void DataView::sendHTML(WiFiClient client, String contentType, String value) {
	client.println("HTTP/1.1 200 OK");
	client.println("Content-type:" + contentType);
//	client.println("Connection: keep-alive");
	client.println();
	client.print(value);
	// The HTTP response ends with another blank line:
	client.println();
}

String DataView::generateDataViewHTML() {
	String viewHtml = "";
	viewHtml += "<table>";
	viewHtml += " <tbody>";
	dataField *curDataField = firstDataField;
	while (curDataField != NULL) {
		viewHtml += "<tr>";
		viewHtml += "<td div align=\"left\">" + curDataField->label + "</td>";
		viewHtml += "<td div align=\"right\"><span id=\"" + curDataField->id
				+ "\">0</span></td>";
		viewHtml += "<td div align=\"left\">" + curDataField->unit + "</td>";
		viewHtml += "</tr>";
		curDataField = curDataField->next;
	}
	if (firstButtonField != NULL) {
		viewHtml += "<tr><td colspan=\"3\" div align=\"center\">";
		buttonField *curButtonField = firstButtonField;
		while (curButtonField != NULL) {
			viewHtml += "<input type=\"button\" id=\"" + curButtonField->id
					+ "\" onclick=\"btnPressed(" + curButtonField->id
					+ ")\" value=\"" + curButtonField->label + "\">";
			curButtonField = curButtonField->next;
		}
		viewHtml += "</td></tr>";
	}
	viewHtml += " </tbody>";
	viewHtml += "</table>";
	return viewHtml;
}

String DataView::generateJsFunction() {
	int counter = 0;
	String jsFunction = "";
	if (firstDataField != NULL) {
		dataField *curField = firstDataField;
		jsFunction += "setInterval(refreshData, " + (String) refreshInterval
				+ ");";
		jsFunction += "refreshData();";
		jsFunction += "function refreshData(){";
		jsFunction += "fetch('/data')";
		jsFunction += ".then( function(response){";
		jsFunction += "return response.json();})";
		jsFunction += ".then(function(dataArray){";
		while (curField != NULL) {
			jsFunction += "document.getElementById(\"" + curField->id
					+ "\").innerHTML=dataArray[" + counter + "];";
			counter++;
			curField = curField->next;
		}
		jsFunction += "});}";
	}
	if (firstButtonField != NULL) {
		jsFunction += "function btnPressed(btnId){";
		jsFunction += "fetch('/btnclick?id='+ btnId)";
		jsFunction += ".then( function(response){";
		jsFunction += "return response.text();";
		jsFunction += "});}";
	}
	return jsFunction;
}

void DataView::addField(String label, String id) {
	addField(label, id, "");
}
void DataView::addField(String label, String id, String initValue) {
	addField(label, id, initValue, "");
}

void DataView::addField(String label, String id, String initValue,
		String unit) {
	dataField *newField = new dataField;
	newField->id = id;
	newField->label = label;
	newField->curValue = initValue;
	newField->unit = unit;
	if (firstDataField == NULL) {
		firstDataField = lastDataField = newField;
	} else {
		lastDataField->next = newField;
		lastDataField = newField;
	}
}

void DataView::addButton(String label, String id) {
	buttonField *newField = new buttonField;
	newField->id = id;
	newField->label = label;
	if (firstButtonField == NULL) {
		firstButtonField = lastButtonField = newField;
	} else {
		lastButtonField->next = newField;
		lastButtonField = newField;
	}
}

void DataView::setCurrentData(String id, String value) {
	dataField *curField = firstDataField;
	while (curField != NULL) {
		if (curField->id.equals(id)) {
			curField->curValue = value;
		}
		curField = curField->next;
	}
}

void DataView::setTitle(String _title) {
	title = _title;
}

void DataView::setRefreshInterval(int _interval) {
	refreshInterval = _interval;
}

