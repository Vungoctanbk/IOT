#include <WiFi.h>
#include <PubSubClient.h>
#include <string.h>
#include <ThingsBoard.h>



#define MQTT_SERVER "192.168.1.14" //Địa chỉ IP của máy tính 
#define MQTT_PORT 1883 //Mặc định

#define MQTT_TEMP_TOPIC1 "home/temp"
#define MQTT_TEMP_TOPIC2 "outdoor/temp" 
#define MQTT_DKOUTLET_TOPIC "home/outlet" //o cam
#define MQTT_DKPUMP_TOPIC "outdoor/pump" //o cam
#define MQTT_AUTO_TOPIC "home/auto" 

const char* ssid = "Icebear"; // Setup wifi và passs
const char* password = "ngaysinhcuaban";
const char* PARAM_INPUT_1 = "output";
const char* PARAM_INPUT_2 = "state";

int auto1=0; 
int batcongtac=0;
float temperatureC,temperatureC1; //Biến đo nhiệt
float setupnhiet = 25; //Nhiệt độ cài đặt bật điều hòa

int i = 0;
int k=0;


#define TOKEN               "TNuYOzrCXRK464UUnKFM"
#define THINGSBOARD_SERVER  "demo.thingsboard.io"

WiFiClient wifiClient; //Setup Client PubSub
PubSubClient client(wifiClient);

// Initialize ThingsBoard client
WiFiClient espClient;
// Initialize ThingsBoard instance
ThingsBoard tb(espClient);


//setup WiFi
void setup_wifi() {
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


//setup Broker
void connect_to_broker() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "Gateway";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe(MQTT_TEMP_TOPIC1);
      client.subscribe(MQTT_TEMP_TOPIC2);
      client.subscribe(MQTT_DKOUTLET_TOPIC);
      client.subscribe(MQTT_DKPUMP_TOPIC);
      client.subscribe(MQTT_AUTO_TOPIC);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 2 seconds");
      delay(1000);
    }
  }
}

//callback từ broker nếu topic thay đổi 
void callback(char* topic, byte *payload, unsigned int length) {
  String status;
  Serial.println("-------new message from broker-----");
  Serial.print("topic: ");
  Serial.println(topic);
  Serial.print("message: ");
  Serial.write(payload, length);
  Serial.println();
  for(int i = 0; i<length; i++)
  {
    status += (char)payload[i];
  }
  Serial.println(status);
  if(String(topic) == MQTT_TEMP_TOPIC1)
  {
    temperatureC = String(status).toFloat();
  } 
  if(String(topic) == MQTT_TEMP_TOPIC2)
  {
    temperatureC1 = String(status).toFloat();
  }
    if (!tb.connected()) {
    // Connect to the ThingsBoard
    Serial.print("Connecting to: ");
    Serial.print(THINGSBOARD_SERVER);
    Serial.print(" with token ");
    Serial.println(TOKEN);
    while(!tb.connect(THINGSBOARD_SERVER, TOKEN)) {
      Serial.println("Failed to connect");
      delay(2000);
      k++;
      if(k==3){
        auto1=1;
        break;
      }
    }
  }

  Serial.println("Sending data...");
  tb.sendTelemetryFloat("temperature",temperatureC );
  tb.sendTelemetryFloat("temperature1",temperatureC1 );
   
  }

bool subscribed = false;

RPC_Response ts1(const RPC_Data &data)
{
  Serial.println("Received the set switch method 4!");
  char params[10];
  serializeJson(data, params);
  //Serial.println(params);
  String _params = params;
  if (_params == "true") {
    Serial.println("Toggle Switch - 1 => On");
    client.publish(MQTT_DKOUTLET_TOPIC, "ON");

  }
  else  if (_params == "false")  {
    Serial.println("Toggle Switch - 1 => Off");
    client.publish(MQTT_DKOUTLET_TOPIC, "OFF");
  }
}
const size_t callbacks_size = 1;
RPC_Callback callbacks[callbacks_size] = {
  { "getValue_1",         ts1 }   // enter the name of your switch variable inside the string

};



void setup() {
// put your setup code here, to run once:
  Serial.begin(115200);
  
  setup_wifi();
 
 
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);
  connect_to_broker();
  
}

void loop() {
// put your main code here, to run repeatedly:
  if (!client.connected()) { //Chưa kết nối broker được thì kết nối lại 
    connect_to_broker();}
  client.loop();
  if(auto1==1) //Nếu ở chế độ manual
  {
    if(temperatureC>setupnhiet)
    {
     client.publish(MQTT_DKOUTLET_TOPIC,"ON");
    }
    else
    {
      client.publish(MQTT_DKOUTLET_TOPIC,"OFF");
    }
  }
  if (!subscribed) {
    Serial.println("Subscribing for RPC...");

    // Perform a subscription. All consequent data processing will happen in
    // processTemperatureChange() and processSwitchChange() functions,
    // as denoted by callbacks[] array.
    if (!tb.RPC_Subscribe(callbacks, callbacks_size)) {
      Serial.println("Failed to subscribe for RPC");
      return;
    }

    Serial.println("Subscribe done");
    subscribed = true;
  }
  tb.loop();
//hàm điều khiển 
 //get the last data of the fields
    delay(1000);
}
