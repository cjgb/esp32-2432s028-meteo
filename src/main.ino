/////////////////////////////////////////////////////////////////
/*
@gilbellosta, 2023-04-09
Adapted from https://docs.lvgl.io/master/widgets/chart.html
*/
/////////////////////////////////////////////////////////////////

#include <stdlib.h>

#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include "lgfx_ESP32_2432S028.h"
#include <lvgl.h>

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include "config.h"


/* Change to your screen resolution */
static const uint32_t screenWidth  = 320;
static const uint32_t screenHeight = 240;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[ screenWidth * 10 ];

lv_obj_t *screenMain;

lv_obj_t *chart;
lv_chart_series_t * ser1;
lv_chart_series_t * ser2;

lv_obj_t *label1;
lv_obj_t *label2;
lv_obj_t *label3;

char *lab_1_text = "Label 1";
char *lab_2_text = "Label 2";
char *lab_3_text = "Label 3";

bool first_refresh;


// /* Display flushing */
void my_disp_flush( lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p )
{
   uint32_t w = ( area->x2 - area->x1 + 1 );
   uint32_t h = ( area->y2 - area->y1 + 1 );

   tft.startWrite();
   tft.setAddrWindow( area->x1, area->y1, w, h );
   tft.writePixels((lgfx::rgb565_t *)&color_p->full, w * h);
   tft.endWrite();

   lv_disp_flush_ready( disp );
}

void setup() {

    Serial.begin(115200);

    tft.begin();
    tft.setRotation(1);
    tft.setBrightness(32);
    // uint16_t calData[] = {3749, 3619, 3737, 207, 361, 3595, 267, 221};
    // tft.setTouchCalibrate(calData);

    lv_init();
    lv_disp_draw_buf_init( &draw_buf, buf, NULL, screenWidth * 10 );

    /*Initialize the display*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);

    /*Change the following line to your display resolution*/
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    screenMain = lv_obj_create(NULL);

    chart = lv_chart_create(screenMain);
    lv_obj_set_size(chart, 280, 200);
    lv_obj_set_pos(chart, 35, 5);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);   /*Show lines and points too*/
    lv_chart_set_point_count(chart, history_size);

    ser1 = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED),   LV_CHART_AXIS_PRIMARY_Y);
    ser2 = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_SECONDARY_Y);

    for (int i = 0; i < history_size; i++) {
        lv_chart_set_next_value(chart, ser1, 0);
        lv_chart_set_next_value(chart, ser2, 0);
    }

    lv_chart_refresh(chart);

    label1 = lv_label_create(screenMain);
    lv_label_set_text(label1, lab_1_text);
    lv_obj_set_style_text_align(label1, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_obj_set_size(label1, 150, 25);
    lv_obj_set_pos(label1, 5, 215);

    label2 = lv_label_create(screenMain);
    lv_label_set_text(label2, lab_2_text);
    lv_obj_set_style_text_align(label2, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    lv_obj_set_size(label2, 80, 25);
    lv_obj_set_pos(label2, 160, 215);

    label3 = lv_label_create(screenMain);
    lv_label_set_text(label3, lab_3_text);
    lv_obj_set_style_text_align(label3, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    lv_obj_set_size(label3, 75, 25);
    lv_obj_set_pos(label3, 240, 215);

    lv_scr_load(screenMain);

    Serial.println("Setup ended!");
    first_refresh = true;

}

void loop() {

    lv_timer_handler(); /* let the GUI do its work */

    if (first_refresh) first_refresh = false;
    else delay( 15 * 60 * 1000 );

    Serial.println("Loop start.");

    WiFi.begin(SSID, wifi_password);

    while (WiFi.status() != WL_CONNECTED) {

      delay(1000);
      Serial.println("Connecting to WiFi...");

    }

    Serial.println("Connected to WiFi");

    update_labels();
    update_chart();

    WiFi.disconnect();
}

String get_payload(String url) {

    HTTPClient http;

    http.begin(url);

    int httpCode = http.GET();
    Serial.println("http request code: " + httpCode);

    String payload = http.getString();
    Serial.println(payload);

    http.end();

    return payload;

}

void update_labels() {

    StaticJsonDocument<512> doc;
    DeserializationError error;

    String payload = get_payload(url1);
    error = deserializeJson(doc, payload);

    if (error) {

      Serial.print("deserializeJson() failed: ");
      Serial.println(error.f_str());

    } else {

      char tmp[10];
      JsonObject current_weather = doc["current_weather"];

      // data time
      const char *current_time = current_weather["time"];
      lv_label_set_text(label1, current_time);

      // temperature
      float current_temperature = current_weather["temperature"];
      dtostrf(current_temperature, 5, 2, tmp);
      strcat(tmp, "C");
      lv_label_set_text(label2, tmp);

      // weather code
      int wmo_code = current_weather["weathercode"];
      sprintf(tmp, "%d", wmo_code);
      lv_label_set_text(label3, tmp);

    }

}

void update_chart() {

    StaticJsonDocument<4096> doc;
    DeserializationError error;

    Serial.println("getting temperature predictions...");

    String payload = get_payload(url2);
    error = deserializeJson(doc, payload);

    if (error) {

      Serial.print("deserializeJson() failed: ");
      Serial.println(error.f_str());

    } else {

      JsonObject current_weather = doc["current_weather"];
      const char* current_time = current_weather["time"];

      JsonObject hourly = doc["hourly"];
      JsonArray hourly_time = hourly["time"];
      JsonArray hourly_temperature_2m = hourly["temperature_2m"];
      JsonArray hourly_weathercode = hourly["weathercode"];

      int start = 0;
      int index = 0;

      float max_temp = -100;
      float min_temp =  100;

      for (int i = 0; i < 48; i++){

          if (strcmp(hourly_time[i], current_time) == 0) {
              start = 1;
          }

          Serial.println(i);
          Serial.println(index);

          if ((start == 1) & (index < history_size)) {

            //Serial.println(index);

            ser1->y_points[index] = hourly_temperature_2m[i];
            ser2->y_points[index] = hourly_weathercode[i];

            float my_temp = hourly_temperature_2m[i];
            Serial.println(my_temp);

            if (max_temp < hourly_temperature_2m[i]) max_temp = hourly_temperature_2m[i];
            if (min_temp > hourly_temperature_2m[i]) min_temp = hourly_temperature_2m[i];

            index++;

          }
      }

      max_temp = ceil(max_temp);
      min_temp = floor(min_temp);
      int breaks = int(1 + (max_temp - min_temp) / 2);

      lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, min_temp - 1, max_temp + 1);
      lv_chart_set_axis_tick(chart, LV_CHART_AXIS_PRIMARY_Y, 10, 5, breaks, 2, true, 30);

      lv_chart_refresh(chart);

    }
}
