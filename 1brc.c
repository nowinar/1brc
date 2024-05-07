#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
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
    //time measuring
    struct timespec start, end;
    long long elapsed_time; // čas v milisekundách

    // Zaznamenává startovní čas
    clock_gettime(CLOCK_MONOTONIC, &start);

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
    char **cities = malloc(1 * sizeof(char *));
    int citiesCount = 0;
    //printf("%d", count);
    //,,logical" error - its stuck before end, because there is a lot of stuff that it went already through
    for(int i = 0; i<= count; i++){
        char city[100];
        strncpy(city, Array[i].city, sizeof(city));
        //printf(" original %s", Array[i].city);
        //till here cities are allright
        //printf("%s start\n", Array[i].city);
        int numsCount = 0;
        float* nums = malloc(1 * sizeof(float));
        int d;
        //printf("%s\n", city);
        bool ShouldFind = false;
        //was currentcity checked?
        for(int j = 0; j<citiesCount; j++){
            if(strcmp(city, cities[j]) == 0){
                //printf("%s", city);
                ShouldFind = true;
            }
        }
        //do stuff
        if(!ShouldFind){
            //printf("%s start\n%s\n", Array[i].city, city);
            for(d = 0; d<=count; d++){
                //printf("%s\n",city);
                if(strcmp(Array[d].city, city) == 0){
                    //printf("%f\n", Array[d].temperature);
                    nums[numsCount] = Array[d].temperature;
                    numsCount++;
                    nums = realloc(nums, (numsCount * sizeof(float))+1);
                }
            }
            //printf("cisla: %d",numsCount);
            float* vys = getMinMaxAvgFromArray(nums, numsCount);

            printf("Mesto: %s, prumer: %f, min: %f, max:%f\n", city, vys[2],vys[0],vys[1]);
            cities = realloc(cities, (citiesCount+1) * sizeof(char *));
            cities[citiesCount] = strdup(city);
            citiesCount++;
            printf("%d",citiesCount);
            }
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    // Výpočet uplynulého času v milisekundách
    elapsed_time = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000;
    printf("\n%lldms\n", elapsed_time);
    return 0;
}