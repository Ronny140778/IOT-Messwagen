/*
 * DataView.h
 *
 *  Created on: 20.01.2023
 *      Author: Ronny
 */

#ifndef DATAVIEW_H_
#define DATAVIEW_H_

#include <WiFiNINA.h>

// value if no Button pressed
#define NO_BUTTON "-1"

class DataView {
public:
	String handleClient(WiFiServer server);
	void setTitle(String _title);
	void setRefreshInterval(int _interval);
	void addField(String label, String id);
	void addField(String label, String id, String initValue);
	void addField(String label, String id, String initValue, String unit);
	void setCurrentData(String id, String value);
	void addButton(String label, String id);


private:
	void sendHTML(WiFiClient client, String contentType, String values);
	void sendSite(WiFiClient client);
	void sendData(WiFiClient client);
	String generateDataViewHTML();
	String generateJsFunction();

	String title="Daten";
	int refreshInterval=500;

	String request="";

	struct dataField{
		String id;
		String label;
		String curValue;
		String unit;
		dataField *next = NULL;
	};

	struct buttonField{
		String id;
		String label;
		buttonField *next = NULL;
	};

	dataField* firstDataField = NULL;
	dataField* lastDataField = NULL;
	buttonField* firstButtonField = NULL;
	buttonField* lastButtonField = NULL;


	String html = F(
		"<!DOCTYPE html>"
		"<html lang=\"de\">"//
		 "<head>"//
		 	 "<meta name = \"dataView\" content = \"width = device-width, initial-scale = 1.0\">	 <meta charset=\"UTF-8\">" //
			 "<title>%s</title>" //
			 "<style>"
			 "body {font-family: Arial, Helvetica, sans-serif}"//
//			 "p{font-size:3.5em}"
//			 "font-family: Arial, Helvetica, sans-serif;"//
			 "input{"//
				"width:200px;"//
				"height:35px;"//
				"font-size:20px;"//
				"text-align: center;"//
				"border-radius: 12px;"//
				"background-color: blue;"//
				"color: white;"//
				"margin: 20px;"//
				"}"//
				"table"//
				"{"//
				"border-spacing: 10px;"//
				"margin-left: auto;"//
				"margin-right: auto;"//
				"font-size: 3.0em;"//
				"}"//
				"td:nth-child(2) {"//
				"width:300px;"//
			    "min-width:300px;"//
//				"width: 45%;"//
				"}"//
				"</style>" //
		 "</head>" //
		 "<body>"
			"%s"
			"<script>"
				"%s"
			"</script>"
			"</body></html>");

};

#endif /* DATAVIEW_H_ */
