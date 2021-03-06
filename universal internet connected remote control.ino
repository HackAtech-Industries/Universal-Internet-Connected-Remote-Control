And here’s the code for the entire project (an explanation of the program code is given below(:
  #include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ESP8266WiFi.h>

#define LED_PIN 5
#define STORAGE_SIZE 5

const char* ssid = "YOUR_NETWORK_NAME";
const char* pass = "NETWORK_PASSWORD";

WiFiServer server(80);
IRsend sender(LED_PIN);

long codes[STORAGE_SIZE] = {0x00123,0x02,0xAB,0x0F,0x0};
bool stored[STORAGE_SIZE] = {true,false,false,false,false};
int protocols[STORAGE_SIZE] = {0,1,1,0,1};

// Function Prototypes
int containsValidIndex(String);
String findParameterValue(String, String);

void setup()
{
  Serial.begin(9600);

  while(!Serial)
	delay(50);

  sender.begin();

  Serial.println("");
  Serial.print("Connecting to ");
  Serial.print(ssid);

  WiFi.begin(ssid, pass);

  while(WiFi.status() != WL_CONNECTED)
  {
	Serial.print(".");
	delay(500);
  }

  Serial.println("Done!");

  Serial.println("Starting server...");
  server.begin();
 
  Serial.print("Server started with address ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
}

void loop()
{
  // Check for incoming connections
  WiFiClient c = server.available();

  if(c)
  {
	while(!c.available())
  	delay(10);

	// Read the first line of the HTTP request
	// It contains something similar to the following line:
	// METHOD /requested_url HTTP_VERSION
	// for example:
	// GET /send?value=0x0085&protocol=NEC HTTP/1.1
	// However, for the sake of simplicity this device only accepts
	// GET requests as they can be sent with any web browser.
	// Updating values this way is not the 'correct' way according to
	// the HTTP standard but it makes using the device easier.
    
	String request = c.readStringUntil('\r');
	c.flush();
 
	int error = 0;

	int value = request.indexOf("value=");
	int protocol = request.indexOf("protocol=");
	int index = request.indexOf("index=");
	int snd = request.indexOf("/send");
	int str = request.indexOf("/store");
	int del = request.indexOf("/delete");
    
	Serial.println(request);

	// The following few if/else-statements parse the incoming request
	// and then execute the action that the user requested.

	// The user requested the send page and did not include /store or /delete in their request
	if (snd != -1 && str == -1 && del == -1)
	{
  	// The user requested the send page and included a value for the index
  	if(index != -1)
  	{
    	// Check if the supplied index is valid (i.e. at least zero and less than STORAGE_SIZE)
    	// And check if the values array contains an entry at the requested position

    	// First, get the value of the parameter as a string
    	String index_string = findParameterValue(request, "index");
   	 
    	// Next, convert it to an integer and check whether the value is valid
    	int i = containsValidIndex(index_string);

    	if(i > -1 && stored[i])
    	{
      	// The parameter was valid. Repeat the stored value!
      	// Make sure to verify the bit length for your remotes!
      	// I used 32 and 14 in this program, but yours might vary
      	if(protocols[i] == 0)
        	sender.sendNEC(codes[i], 32);
      	else
        	sender.sendRC5(codes[i], 14);
     	 
      	Serial.print("Repeat the value stored at position ");
      	Serial.println(i);
    	}
  	}
  	// The user supplied the wrong parameters for this request.
  	else
  	{
    	error = 1;
    	Serial.print("Unknown request: ");
    	Serial.println(request);
  	}
	}
	// The user requested the /store page and the request didn't include
	// the /send or /delete page
	else if(str != -1 && snd == -1 && del == -1)
	{
  	// The user supplied the wrong parameters for this request.
  	// (Either of the three parameters is missing)
  	if(index == -1 || protocol == -1 || value == -1)
  	{
    	error = 1;
    	Serial.print("BAD REQUEST. Missing field: ");
    	Serial.println(request);
  	}
  	else
  	{
    	String p = findParameterValue(request, "protocol");
    	String v = findParameterValue(request, "value");
    	int i = containsValidIndex(findParameterValue(request, "index"));

    	// Check if the supplied index is valid
    	if(i > -1)
    	{
      	Serial.print("Store ");
      	Serial.print(v);
      	Serial.print(" for protocol ");
      	Serial.print(p);
      	Serial.print(" on position ");
      	Serial.println(i);
 
      	codes[i] = strtol(v.c_str(), NULL, 16);
      	protocols[i] = (p == "NEC") ? 0 : 1;
      	stored[i] = true;
    	}
  	}
	}
	// The user requested the /delete page and the request didn't include
	// the /send or /delete page
	else if(del != -1 && str == -1 && snd == -1)
	{
  	// The /delete page requires the index to work. It ignores the other parameters.
  	// So if the request didn't include the index, then answer with an error
  	if(index == -1)
  	{
    	error = 1;
    	Serial.print("BAD REQUEST. Missing field: ");
    	Serial.println(request);
  	}
  	else
  	{
    	int i = containsValidIndex(findParameterValue(request, "index"));

    	if(i > -1 && stored[i])
    	{
      	stored[i] = false;
     	 
      	Serial.print("Delete the value stored on position ");
      	Serial.println(i);
    	}
  	}
	}
   
	// Return the response
	// If no error occurred, send an HTML page that lists the stored codes
	// and that contains two forms for managing them (add new ones and delete existing codes).
	if(error == 0)
	{
              /* HTML code omitted! */
	}
	else
	{
              // Return a response header
  	  /* HTML error response omitted! */
	}
  }
}

int containsValidIndex(String index_string)
{
  // First check whether the string only contains digits
  for(int i = 0; i < index_string.length(); i++)
  {
	if(!isDigit(index_string.charAt(i)))
  	return -1;
  }

  // Then convert the string to an int and repeat the stored valud at that
  // position in the array (if the index is valid)

  int index_num = index_string.toInt();

  if(index_num >= 0 && index_num < STORAGE_SIZE)
	return index_num;
  else
	return -1;
}

String findParameterValue(String request, String param_name)
{
  int param_name_length = param_name.length()+1;
  // First, dispose of everything in the request that comes prior to the index parameter
  String tmp = request.substring(request.indexOf(param_name));

  // Then check whether the remaining part of the request (everything that comes after
  // the request=... part of the URL) contains more paramters (they are separated by
  // ampersand characters (&)).
  if(tmp.indexOf("&") != -1)
	// If there are more parameters, the value of the index sits between the
	// parameter name (index=) and the next ampersand (&).
	// For example: URL?index=5&other_param=24 (...)
	return tmp.substring(param_name_length, tmp.indexOf("&"));
  else
	// Otherwise, the value of interest comes right before the next
	// whitespace character like this:
	// URL?index=5 HTTP(...)
	return tmp.substring(param_name_length, tmp.indexOf(" "));
}
