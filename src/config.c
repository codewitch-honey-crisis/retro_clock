#ifdef ESP_PLATFORM
#include "config.h"
#include <string.h>
#include <stdio.h>

bool config_get_value(const char* key, int index, char* out_data, size_t out_data_length) {
    char buf[256];
    strcpy(buf,"/spiffs/");
    strcat(buf,key);
    strcat(buf,".txt");
    FILE* f = fopen(buf,"r");
    if(f==NULL) {
       return false;
    }
    if(out_data!=NULL && out_data_length>0) {
        while(index-->0) {
            fgets(buf,sizeof(buf),f);
        }
        fgets(out_data,out_data_length,f);
        char* sn = strchr(out_data, '\n');
        if (sn != NULL) *sn = '\0';
        sn = strchr(out_data, '\r');
        if (sn != NULL) *sn = '\0';
    }
    fclose(f);
    return true;
}
bool config_clear_values(const char* key) {
    char buf[256];
    strcpy(buf,"/spiffs/");
    strcat(buf,key);
    strcat(buf,".txt");
    
    remove(buf);
    return true;
}
bool config_add_value(const char* key, const char* value) {
    char buf[256];
    strcpy(buf,"/spiffs/");
    strcat(buf,key);
    strcat(buf,".txt");
    FILE* f = fopen(buf,"a");
    if(f==NULL) {
        return false;
    }
    fputs(value,f);
    fputc('\n',f);
    fclose(f);
    return true;
}
#endif