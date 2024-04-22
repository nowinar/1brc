#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char city[100];
    float temperature;
    float min;
    float max;
    float avg;
} CityTemperature;

float getMin(float a, float b){
    if(a<b){
        return a;
    }else{
        return b;
    }
}
float getMax(float a, float b){
    if(a<b){
        return b;
    }else{
        return a;
    }
}

int main(){

    FILE *file;
    char *filename = "../measurements.txt";
    char buffer[1024];
    char mesto[100]; // Proměnná pro uložení názvu města
    char teplota[100]; // Proměnná pro uložení teploty
    CityTemperature Array[1000];
    int count = 0;
    file = fopen(filename, "r");
    while(fgets(buffer, sizeof(buffer), file) != NULL){
        char *token = strtok(buffer, ";\n");
        if (token != NULL) {
            strncpy(mesto, token, sizeof(mesto)); // Uložení prvního tokenu do proměnné mesto
            mesto[sizeof(mesto) - 1] = '\0';
            
            strncpy(Array[count].city, mesto, sizeof(Array[count].city)); // Kopírování města do struktury
            Array[count].city[sizeof(Array[count].city) - 1] = '\0'; // Zajištění ukončení řetězce

            token = strtok(NULL, ";\n");
        }
        if (token != NULL) {
            strncpy(teplota, token, sizeof(teplota)); // Uložení druhého tokenu do proměnné teplota
        }
        Array[count].temperature = atof(teplota);
        printf("Mesto: %s, Teplota: %f, real: %s\n", Array[count].city, Array[count].temperature, teplota);
        count++;
    }
    for(int i = 0; i<= count; i++){
        char city[100];
        strncpy(city, Array[i].city, sizeof(city));
        for(int d = 0; d<=count; d++){
            if(Array[d].city == city){
                
            }
        }
    }
    fclose(file);
    return 0;
}