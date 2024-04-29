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

float* getMinMaxAvgFromArray(float a[1000], int count){
    float pocet = (float)count;
    
    float soucet = 0;
    float min = a[0];
    float max = a[0];

    for(int i = 0; i<pocet; i++)
    {
        float actual = a[i];
        if(actual<min){
            min = actual;
        }
        if(actual>max){
            max = actual;
        }
        soucet+=actual;
    }
    float* resp = malloc(3 * sizeof(float));
    resp[0] = min;
    resp[1] = max;
    resp[2] = soucet/pocet;
    return resp;
}

int main(){

    FILE *file;
    char *filename = "./measurements.txt";
    char buffer[1024];
    char mesto[100]; // Proměnná pro uložení názvu města
    char teplota[100]; // Proměnná pro uložení teploty
    CityTemperature* Array = malloc(1 * sizeof(CityTemperature));
    
    int count = 0;
    //loading
    file = fopen(filename, "r");
    while(fgets(buffer, sizeof(buffer), file) != NULL){
        char *token = strtok(buffer, ";\n");
        if (token != NULL) {
            //printf("%s\n", token);
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
        //printf("Mesto: %s, Teplota: %f, real: %s\n", Array[count].city, Array[count].temperature, teplota);
        count++;
        Array = realloc(Array, (count + 1) * sizeof(CityTemperature));
    }
    fclose(file);
    //printf("%d", count);
    for(int i = 0; i<= count; i++){
        char city[100];
        strncpy(city, Array[i].city, sizeof(city));
        int numsCount = 0;
        float nums[1000];
        int d;
        for(d = 0; d<=count; d++){
            //printf("%s\n",city);

            if(strcmp(Array[d].city, city) == 0){
                //printf("%f\n", Array[d].temperature);
                nums[numsCount] = Array[d].temperature;
                numsCount++;
            }
        }
        float* vys = getMinMaxAvgFromArray(nums, numsCount);

        printf("Mesto: %s, prumer: %f, min: %f, max:%f\n", city, vys[2],vys[0],vys[1]);
    }
    return 0;
}