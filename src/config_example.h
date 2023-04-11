/**
 * @gilbellosta, 2023-04-11
 * Configuration parameters
 */


char * SSID = "asdf";
char * wifi_password = "asdfasdf";

// API urls
char * url1 = "https://api.open-meteo.com/v1/forecast?latitude=40.416775&longitude=-3.703790&current_weather=true&timezone=Europe%2FBerlin";
char * url2 = "https://api.open-meteo.com/v1/forecast?latitude=40.416775&longitude=-3.703790&current_weather=true&hourly=temperature_2m,weathercode&forecast_days=2&timezone=Europe%2FBerlin";

// number of periods/hours to keep in the charts
const uint16_t history_size = 24;
