// Generated with clasptree
// To use this file, define HTTPD_CONTENT_IMPLEMENTATION in exactly one translation unit (.c/.cpp file) before including this header.
#ifndef HTTPD_CONTENT_H
#define HTTPD_CONTENT_H

#include <stdint.h>
#include <stddef.h>

#define HTTPD_RESPONSE_HANDLER_COUNT 2
typedef struct { const char* path; const char* path_encoded; void (* handler) (void* arg); } httpd_response_handler_t;
extern httpd_response_handler_t httpd_response_handlers[HTTPD_RESPONSE_HANDLER_COUNT];
#ifdef __cplusplus
extern "C" {
#endif

// .index.clasp
void httpd_content_index_clasp(void* resp_arg);

#ifdef __cplusplus
}
#endif

#endif // HTTPD_CONTENT_H

#ifdef HTTPD_CONTENT_IMPLEMENTATION

httpd_response_handler_t httpd_response_handlers[2] = {
    { "/", "/", httpd_content_index_clasp },
    { "/index.clasp", "/index.clasp", httpd_content_index_clasp }
};

void httpd_content_index_clasp(void* resp_arg) {
     httpd_send_block("HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nTrans"
        "fer-Encoding: chunked\r\n\r\n", 87,  resp_arg);
    
    
        char ssid[65];
        char pass[129];
        ssid[0]='\0';
        pass[0]='\0';
        config_get_value("wifi",0,ssid,sizeof(ssid)-1);
        config_get_value("wifi",1,pass,sizeof(pass)-1);
    
     httpd_send_block("11e\r\n<!DOCTYPE html>\n<html>\n    <head>\n        <meta name=\"vie"
        "wport\" content=\"width=device-width, initial-scale=1.0\" />\n        <title>Configu"
        "re Clock</title>\n    </head>\n    <body>\n        <form method=\"GET\" action=\".\">\n "
        "           <label>SSID:</label><input type=\"text\" name=\"ssid\" value=\"\r\n", 293,  resp_arg);
     httpd_send_expr(ssid,  resp_arg);
     httpd_send_block("51\r\n\"><br>\n            <label>Pass:</label><input type=\"passwo"
        "rd\" name=\"pass\" value=\"\r\n", 87,  resp_arg);
     httpd_send_expr(pass,  resp_arg);
     httpd_send_block("210f\r\n\"><br>\n            <label>Timezone:</label><select name="
        "\"tzoffset\">\n                <option value=\"0\">(UTC) Coordinated Universal Time</"
        "option>\n                <option value=\"-43200\">(UTC-12:00) International Date Li"
        "ne West</option>\n                <option value=\"-39600\">(UTC-11:00) Coordinated "
        "Universal Time-11</option>\n                <option value=\"-36000\">(UTC-10:00) Ha"
        "waii</option>\n                <option value=\"-28800\">(UTC-09:00) Alaska</option>"
        "\n                <option value=\"-25200\">(UTC-08:00) Baja California</option>\n   "
        "             <option value=\"-25200\">(UTC-07:00) Pacific Daylight Time (US & Cana"
        "da)</option>\n                <option value=\"-28800\">(UTC-08:00) Pacific Standard"
        " Time (US & Canada)</option>\n                <option value=\"-25200\">(UTC-07:00) "
        "Arizona</option>\n                <option value=\"-21600\">(UTC-07:00) Chihuahua, L"
        "a Paz, Mazatlan</option>\n                <option value=\"-21600\">(UTC-07:00) Moun"
        "tain Time (US & Canada)</option>\n                <option value=\"-21600\">(UTC-06:"
        "00) Central America</option>\n                <option value=\"-18000\">(UTC-06:00) "
        "Central Time (US & Canada)</option>\n                <option value=\"-18000\">(UTC-"
        "06:00) Guadalajara, Mexico City, Monterrey</option>\n                <option valu"
        "e=\"-21600\">(UTC-06:00) Saskatchewan</option>\n                <option value=\"-180"
        "00\">(UTC-05:00) Bogota, Lima, Quito</option>\n                <option value=\"-180"
        "00\">(UTC-05:00) Eastern Time (US & Canada)</option>\n                <option valu"
        "e=\"-14400\">(UTC-04:00) Eastern Daylight Time (US & Canada)</option>\n            "
        "    <option value=\"-18000\">(UTC-05:00) Indiana (East)</option>\n                <"
        "option value=\"-14400\">(UTC-04:30) Caracas</option>\n                <option value"
        "=\"-14400\">(UTC-04:00) Asuncion</option>\n                <option value=\"-10800\">("
        "UTC-04:00) Atlantic Time (Canada)</option>\n                <option value=\"-14400"
        "\">(UTC-04:00) Cuiaba</option>\n                <option value=\"-14400\">(UTC-04:00)"
        " Georgetown, La Paz, Manaus, San Juan</option>\n                <option value=\"-1"
        "4400\">(UTC-04:00) Santiago</option>\n                <option value=\"-7200\">(UTC-0"
        "3:30) Newfoundland</option>\n                <option value=\"-10800\">(UTC-03:00) B"
        "rasilia</option>\n                <option value=\"-10800\">(UTC-03:00) Buenos Aires"
        "</option>\n                <option value=\"-10800\">(UTC-03:00) Cayenne, Fortaleza<"
        "/option>\n                <option value=\"-10800\">(UTC-03:00) Greenland</option>\n "
        "               <option value=\"-10800\">(UTC-03:00) Montevideo</option>\n          "
        "      <option value=\"-10800\">(UTC-03:00) Salvador</option>\n                <opti"
        "on value=\"-7200\">(UTC-02:00) Coordinated Universal Time-02</option>\n            "
        "    <option value=\"-3600\">(UTC-02:00) Mid-Atlantic - Old</option>\n              "
        "  <option value=\"0\">(UTC-01:00) Azores</option>\n                <option value=\"-"
        "3600\">(UTC-01:00) Cape Verde Is.</option>\n                <option value=\"3600\">("
        "UTC) Casablanca</option>\n                <option value=\"0\">(UTC) Edinburgh, Lond"
        "on</option>\n                <option value=\"3600\">(UTC+01:00) Edinburgh, London</"
        "option>\n                <option value=\"3600\">(UTC) Dublin, Lisbon</option>\n     "
        "           <option value=\"0\">(UTC) Monrovia, Reykjavik</option>\n                "
        "<option value=\"7200\">(UTC+01:00) Amsterdam, Berlin, Bern, Rome, Stockholm, Vienn"
        "a</option>\n                <option value=\"7200\">(UTC+01:00) Belgrade, Bratislava"
        ", Budapest, Ljubljana, Prague</option>\n                <option value=\"7200\">(UTC"
        "+01:00) Brussels, Copenhagen, Madrid, Paris</option>\n                <option val"
        "ue=\"7200\">(UTC+01:00) Sarajevo, Skopje, Warsaw, Zagreb</option>\n                "
        "<option value=\"3600\">(UTC+01:00) West Central Africa</option>\n                <o"
        "ption value=\"3600\">(UTC+01:00) Windhoek</option>\n                <option value=\""
        "10800\">(UTC+02:00) Athens, Bucharest</option>\n                <option value=\"108"
        "00\">(UTC+02:00) Beirut</option>\n                <option value=\"7200\">(UTC+02:00)"
        " Cairo</option>\n                <option value=\"10800\">(UTC+02:00) Damascus</opti"
        "on>\n                <option value=\"10800\">(UTC+02:00) E. Europe</option>\n       "
        "         <option value=\"7200\">(UTC+02:00) Harare, Pretoria</option>\n            "
        "    <option value=\"10800\">(UTC+02:00) Helsinki, Kyiv, Riga, Sofia, Tallinn, Viln"
        "ius</option>\n                <option value=\"10800\">(UTC+03:00) Istanbul</option>"
        "\n                <option value=\"10800\">(UTC+02:00) Jerusalem</option>\n          "
        "      <option value=\"7200\">(UTC+02:00) Tripoli</option>\n                <option "
        "value=\"10800\">(UTC+03:00) Amman</option>\n                <option value=\"10800\">("
        "UTC+03:00) Baghdad</option>\n                <option value=\"10800\">(UTC+02:00) Ka"
        "liningrad</option>\n                <option value=\"10800\">(UTC+03:00) Kuwait, Riy"
        "adh</option>\n                <option value=\"10800\">(UTC+03:00) Nairobi</option>\n"
        "                <option value=\"10800\">(UTC+03:00) Moscow, St. Petersburg, Volgog"
        "rad, Minsk</option>\n                <option value=\"14400\">(UTC+04:00) Samara, Ul"
        "yanovsk, Saratov</option>\n                <option value=\"14400\">(UTC+03:30) Tehr"
        "an</option>\n                <option value=\"14400\">(UTC+04:00) Abu Dhabi, Muscat<"
        "/option>\n                <option value=\"18000\">(UTC+04:00) Baku</option>\n       "
        "         <option value=\"14400\">(UTC+04:00) Port Louis</option>\n                <"
        "option value=\"14400\">(UTC+04:00) Tbilisi</option>\n                <option value="
        "\"14400\">(UTC+04:00) Yerevan</option>\n                <option value=\"14400\">(UTC+"
        "04:30) Kabul</option>\n                <option value=\"18000\">(UTC+05:00) Ashgabat"
        ", Tashkent</option>\n                <option value=\"18000\">(UTC+05:00) Yekaterinb"
        "urg</option>\n                <option value=\"18000\">(UTC+05:00) Islamabad, Karach"
        "i</option>\n                <option value=\"18000\">(UTC+05:30) Chennai, Kolkata, M"
        "umbai, New Delhi</option>\n                <option value=\"18000\">(UTC+05:30) Sri "
        "Jayawardenepura</option>\n                <option value=\"18000\">(UTC+05:45) Kathm"
        "andu</option>\n                <option value=\"21600\">(UTC+06:00) Nur-Sultan (Asta"
        "na)</option>\n                <option value=\"21600\">(UTC+06:00) Dhaka</option>\n  "
        "              <option value=\"21600\">(UTC+06:30) Yangon (Rangoon)</option>\n      "
        "          <option value=\"25200\">(UTC+07:00) Bangkok, Hanoi, Jakarta</option>\n   "
        "             <option value=\"25200\">(UTC+07:00) Novosibirsk</option>\n            "
        "    <option value=\"28800\">(UTC+08:00) Beijing, Chongqing, Hong Kong, Urumqi</opt"
        "ion>\n                <option value=\"28800\">(UTC+08:00) Krasnoyarsk</option>\n    "
        "            <option value=\"28800\">(UTC+08:00) Kuala Lumpur, Singapore</option>\n "
        "               <option value=\"28800\">(UTC+08:00) Perth</option>\n                "
        "<option value=\"28800\">(UTC+08:00) Taipei</option>\n                <option value="
        "\"28800\">(UTC+08:00) Ulaanbaatar</option>\n                <option value=\"28800\">("
        "UTC+08:00) Irkutsk</option>\n                <option value=\"32400\">(UTC+09:00) Os"
        "aka, Sapporo, Tokyo</option>\n                <option value=\"32400\">(UTC+09:00) S"
        "eoul</option>\n                <option value=\"32400\">(UTC+09:30) Adelaide</option"
        ">\n                <option value=\"32400\">(UTC+09:30) Darwin</option>\n            "
        "    <option value=\"36000\">(UTC+10:00) Brisbane</option>\n                <option "
        "value=\"36000\">(UTC+10:00) Canberra, Melbourne, Sydney</option>\n                <"
        "option value=\"36000\">(UTC+10:00) Guam, Port Moresby</option>\n                <op"
        "tion value=\"36000\">(UTC+10:00) Hobart</option>\n                <option value=\"32"
        "400\">(UTC+09:00) Yakutsk</option>\n                <option value=\"39600\">(UTC+11:"
        "00) Solomon Is., New Caledonia</option>\n                <option value=\"36000\">(U"
        "TC+10:00) Vladivostok</option>\n                <option value=\"39600\">(UTC+11:00)"
        " Sakhalin</option>\n                <option value=\"43200\">(UTC+12:00) Auckland, W"
        "ellington</option>\n                <option value=\"43200\">(UTC+12:00) Coordinated"
        " Universal Time+12</option>\n                <option value=\"43200\">(UTC+12:00) Fi"
        "ji</option>\n                <option value=\"43200\">(UTC+12:00) Magadan</option>\n "
        "               <option value=\"46800\">(UTC+12:00) Petropavlovsk-Kamchatsky - Old<"
        "/option>\n                <option value=\"46800\">(UTC+13:00) Nuku'alofa</option>\n "
        "               <option value=\"46800\">(UTC+13:00) Samoa</option>\n            </se"
        "lect><br>\n            <input type=\"checkbox\" name=\"military\"><label>24 hour time"
        "</label><br>\n            <input type=\"submit\">     \n        </form>\n    </body>\n"
        "</html>\r\n", 8471,  resp_arg);
     httpd_send_block("0\r\n\r\n", 5,  resp_arg);
       free(resp_arg);
    

}
#endif // HTTPD_CONTENT_IMPLEMENTATION

