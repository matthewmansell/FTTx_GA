//
//  main.c
//  A2_FFT_MM
//
//  Created by Matthew Mansell on 23/11/2017.
//  Copyright © 2017 Matthew Mansell. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include "header.h"

#define POPULATION_SIZE 100
#define GENERATIONS 100
#define TOURNAMENT_SIZE 5

static int noOfAreas = 10;
static int studyPeriod = 10;
static double rental = 2;
static double capex = 500;
static double opex = 200;
static double interest = 0.01;
static int maxRolloutPeriod = 10;

static int *households;
static long double *imitators;


//static int fitness[POPULATION_SIZE];


int loadFTT(char settingsFileName[], char areasFileName[]) {
    FILE *settingsFile, *areasFile;
    
    settingsFile = fopen(settingsFileName, "r");
    
    //Load settings into global variables
    if(settingsFile == NULL) {
        perror("Error when attempting to open the settings file.\n");
        return 0;
    } else {
        char *document; //Entire document
        fseek(settingsFile, 0, SEEK_END); //Go to end of the file
        long docLength = ftell(settingsFile); //Find the position (file size)
        fseek(settingsFile, 0, SEEK_SET); //Go back to start
        document = malloc(docLength+1); //Allocate document memory
        fread(document, docLength, 1, settingsFile); //Read document
        document[docLength] = '\0'; //Ensure document is terminated
        fclose(settingsFile); //Close the file
        
        int noa = 0, sp = 0, arc = 0, cpx = 0, opx = 0, ir = 0, mrp = 0;
        char *line, *sp1, *sp2;
        line = strtok_r(document, "\n", &sp1);
        while(line != NULL) {
            char *cmd;
            cmd = strtok_r(line, ",", &sp2);
            if(strcmp(cmd, "Number of areas") == 0) {
                sscanf(strtok_r(NULL, ",", &sp2), "%d", &noOfAreas);
                noa = 1;
            } else if(strcmp(cmd, "Study period") == 0) {
                sscanf(strtok_r(NULL, ",", &sp2), "%d", &studyPeriod);
                sp = 1;
            } else if(strcmp(cmd, "Annual rental charges") == 0) {
                sscanf(strtok_r(NULL, ",", &sp2), "%lf", &rental);
                arc = 1;
            } else if(strcmp(cmd, "CAPEX") == 0) {
                sscanf(strtok_r(NULL, ",", &sp2), "%lf", &capex);
                cpx = 1;
            } else if(strcmp(cmd, "OPEX") == 0) {
                sscanf(strtok_r(NULL, ",", &sp2), "%lf", &opex);
                opx = 1;
            } else if(strcmp(cmd, "Interest rate") == 0) {
                sscanf(strtok_r(NULL, ",", &sp2), "%lf", &interest);
                ir = 1;
            } else if(strcmp(cmd, "Maximum rollout period") == 0) {
                sscanf(strtok_r(NULL, ",", &sp2), "%d", &maxRolloutPeriod);
                mrp = 1;
            } else {
                printf("Cmd \"%s\" not recognised for the FFT settings.\n", cmd);
            }
            line = strtok_r(NULL, "\n", &sp1);
        }
        free(document); //Free allocated memory
        if(noa != 1 || sp != 1 || arc != 1 || cpx != 1 || opx != 1 || ir != 1 || mrp != 1) {
            return 0; //Not all required data has been set
        }
    }
    
    //Allocate memory for arrays now that we know the size
    households = (int*)malloc(noOfAreas*sizeof(int));
    imitators = (long double*)malloc(noOfAreas*sizeof(long double));
    if(households == NULL || imitators == NULL) {
        perror("malloc failed.\n");
    }
    
    areasFile = fopen(areasFileName, "r");
    //Load areas into households and imitator
    if(areasFile == NULL) {
        perror("Error when attempting to open the areas file.\n");
        return 0;
    } else {
        char *document; //Entire document
        fseek(areasFile, 0, SEEK_END); //Go to end of the file
        long docLength = ftell(settingsFile); //Find the position (file size)
        fseek(areasFile, 0, SEEK_SET); //Go back to start
        document = malloc(docLength+1); //Allocate document memory
        fread(document, docLength, 1, areasFile); //Read document
        document[docLength] = '\0'; //Ensure document is terminated
        fclose(areasFile); //Close the file
        
        char *line, *sp1, *sp2;
        int count = 0;
        line = strtok_r(document, "\n", &sp1);
        line = strtok_r(NULL, "\n", &sp1); //Ignore first line
        while(line != NULL) {
            sscanf(strtok_r(line, ",", &sp2), "%d", &households[count]);
            char *imitatorStr;
            imitatorStr = strtok_r(NULL, ",", &sp2);
            if(imitatorStr == NULL) {
                printf("Area %d has no imitator value specified.",count);
                return 0;
            } else { sscanf(imitatorStr, "%Lf", &imitators[count]); }
            count++;
            line = strtok_r(NULL, "\n", &sp1);
        }
        
        if(count != noOfAreas) { //Ensure we have the number the settings file stated
            printf("'Number of areas' (%d) and areas suppled (%d) do not match.\n", noOfAreas, count);
            return 0;
        }
        free(document); //Free allocated memory
    }
    return 1; //All tasks completed sucesfully
}

/* Runs the given FTT rollout plan agains the model.
 * Return The final NPV for this plan.
 * Param origHouseholds An array containing
 */
double model(int plan[]) {
    //Make a copy of the households array that we are able to edit
    int copyHouseholds[noOfAreas];
    for(int i = 0; i < noOfAreas; i++) {
        copyHouseholds[i] = households[i];
    }
    int totalCustomers = 0, totalDevelopments = 0;
    double NPV = 0, expenditure = 0, income = 0;
    for(int year = 1; year <= studyPeriod; year++) { //For each year
        double yearCAPEX = 0, yearOPEX = totalDevelopments * opex, cashFlow = 0;
        for(int area = 0; area < noOfAreas; area++) { //For each area
            if(plan[area] < year && plan[area] != 0) { //Customers for this year
                int newCustomers = copyHouseholds[area]*imitators[area];
                totalCustomers += newCustomers;
                copyHouseholds[area]-= newCustomers;
            }
            if(plan[area] == year) { //New developments
                yearCAPEX += capex;
                totalDevelopments++;
            }
        }
        expenditure += yearOPEX + yearCAPEX;
        income += totalCustomers * rental;
        cashFlow = income - expenditure;
        NPV += cashFlow/pow((1+interest),year);
    }
    return NPV;
}

/* Generates fitness values using data for the given population.
 * Param population The population to evaluate.
 * Param fitness The fitness array to populate.
 */
void evaluate(int population[POPULATION_SIZE][noOfAreas], double fitnessArray[noOfAreas]) {
    for(int individual = 0; individual < POPULATION_SIZE; individual++) {
        fitnessArray[individual] = model(population[individual]);
    }
}

/* Initialises, with random values, the given population.
 */
void initialise(int population[POPULATION_SIZE][noOfAreas]) {
    for(int individual = 0; individual < POPULATION_SIZE; individual++) {
        for(int area = 0; area < noOfAreas; area++) {
            population[individual][area] = rand() % studyPeriod;
        }
    }
}

/* Performs a single point crossover on the 2 parents supplied,
 * sets the content of the 2 supplied children.
 */
void crossover(int parent1[noOfAreas], int parent2[noOfAreas], int child1[noOfAreas], int child2[noOfAreas]) {
    int cop = rand() % noOfAreas-1; //Crossover point selection
    for(int area = 0; area < cop; area++) { //Up to cop: copy from parent
        child1[area] = parent1[area];
        child2[area] = parent2[area];
    }
    for(int area = cop; area < noOfAreas; area++) { //After cop: copy from alternate parent
        child1[area] = parent2[area];
        child2[area] = parent1[area];
    }
}

/* Performs a single point mutation on the parent supplied,
 sets the content
 */
void mutation(int parent[noOfAreas], int child[noOfAreas]) {
    copyArray(parent, child); //Copy the individual
    int area = rand() % noOfAreas; //Select random index
    parent[area] = rand() % studyPeriod; //Replace with random
}

/* Returns an index from the given array.
 */
int tournamentSelect(double fitness[noOfAreas]) {
    int best = rand() % POPULATION_SIZE; // Select first individual
    for(int i = 0; i < TOURNAMENT_SIZE-1; i++) { // For the rest of the tournament
        int individual = rand() % POPULATION_SIZE;
        if(fitness[individual] > fitness[best]) {
            best = individual; // Set the current best
        }
    }
    return best;
}

/* Generates a new population using the provided parent population,
 * writing the new population to the supplied child/new population,
 * using the provided fitness array for judgement.
 */
void generatePopulation(int parentPopulation[POPULATION_SIZE][noOfAreas], int newPopulation[POPULATION_SIZE][noOfAreas], double fitness[POPULATION_SIZE]) {
    for(int individual = 0; individual < POPULATION_SIZE; individual++) {
        crossover(parentPopulation[tournamentSelect(fitness)], parentPopulation[tournamentSelect(fitness)], newPopulation[individual], newPopulation[individual+1]);
        individual++;
    }
}

void copyPopulation(int srcPopulation[POPULATION_SIZE][noOfAreas], int destPopulation[POPULATION_SIZE][noOfAreas]) {
    for(int individual = 0; individual < POPULATION_SIZE; individual++) {
        for(int area = 0; area < noOfAreas; area++) {
            destPopulation[individual][area] = srcPopulation[individual][area];
        }
    }
}

void printIndividual(int individual[noOfAreas]) {
    for(int area = 0; area < noOfAreas; area++) {
        printf("%d,", individual[area]);
    }
    printf("\n");
}

/* Main Evolutionary loop
 * param _Bool Print generational stats
 */
void run(int print) {
    int population[POPULATION_SIZE][noOfAreas];
    initialise(population);
    double fitness[POPULATION_SIZE];
    evaluate(population, fitness);
    for(int generation = 0; generation < GENERATIONS; generation++) {
        int newPopulation[POPULATION_SIZE][noOfAreas];
        generatePopulation(population, newPopulation, fitness);
        evaluate(newPopulation, fitness);
        copyPopulation(newPopulation, population);
    }
}

void runFor(int runs) {
    for(int runi = 0; runi < runs; runi++) {
        run(1);
    }
}

int main(int argc, const char * argv[]) {
    printf("----- OPTIMAL FFTx ROLLOUT GA -----\n");
    printf("Matthew Mansell (mcm36)\n");
    
    
    char settingsFile[100], areasFile[100];
    strcpy(settingsFile, argv[1]); strcpy(areasFile, argv[2]);
    if(loadFTT(settingsFile, areasFile) != 1) {
        printf("FFT GA cannot run without the correct data.\nExiting...\n");
        return 0; //Exit
    }
    printf("FFT data loaded sucesfully.\n");
    
    //printf("Number of areas: %d\n", noOfAreas);
    //printf("Study period: %d\n", studyPeriod);
    //printf("Annual rental charges: %.2f\n", rental);
    //printf("CAPEX: %.2f\n", capex);
    //printf("OPEX: %.2f\n", opex);
    //printf("Interest rate: %.1f\n", interest);
    //printf("Maximum rollout period: %d\n", maxRolloutPeriod);
    
    //for(int i = 0; i < noOfAreas; i++) {
    //    printf("%d | %.9Lf\n", households[i], imitators[i]);
    //}
    
    printf("Type 'help' for a list of commands\n");
    
    int wantToQuit = 0;
    while(wantToQuit == 0) {
        char input[100];
        fgets(input, 100, stdin);
        strtok(input, "\n");
        
        int handled = 0;
        
        if(strcmp(input, "quit") == 0) {
            handled = 1;
            wantToQuit = 1;
            printf("Exiting...\n");
        } else if(strcmp(input, "help") == 0) {
            handled = 1;
            printf("----- HELP -----\n");
            printf("help: commands\n");
            printf("quit: exit the applicaiton\n");
            printf("print loaded data: shows the data loaded from the data files\n");
            printf("run: runs the GA\n");
        } else if(strcmp(input, "run") == 0) {
            handled = 1;
        } else if(strcmp(input, "print loaded data") == 0) {
            handled = 1;
            printf("----- INFO -----\n");
            printf("Number of areas: %d\n", noOfAreas);
            printf("Study period: %d\n", studyPeriod);
            printf("Annual rental charges: %.2f\n", rental);
            printf("CAPEX: %.2f\n", capex);
            printf("OPEX: %.2f\n", opex);
            printf("Interest rate: %.1f\n", interest);
            printf("Maximum rollout period: %d\n", maxRolloutPeriod);
            printf("----- AREA DATA -----\n");
            printf("area: households | imitator\n");
            for(int i = 0; i < noOfAreas; i++) {
                printf("%d: %d | %.9Lf\n", i, households[i], imitators[i]);
            }
        }
        
        if(handled == 0) {
            printf("Command not understood. Try 'help'\n");
        }
    }
    
    
    
    
    
    //srand((unsigned) time(NULL)); // Initialise rand seed based on system time
    //int population[POPULATION_SIZE][noOfAreas];
    //initialise(population);
    
    //for(int i = 0; i < noOfAreas; i++) {
    //    printf("%d\n",population[0][i]);
    //}
    //printIndividual(population[0]);
    
    //double fitness[POPULATION_SIZE];
    //evaluate(population, fitness);
    //printf("%f\n",fitness[0]);
    
    //int newPopulation[POPULATION_SIZE][noOfAreas];
    //generatePopulation(population, newPopulation, fitness);
    
    //for(int i = 0; i < noOfAreas; i++) {
    //    printf("%d\n",newPopulation[0][i]);
    //}
    //printIndividual(newPopulation[0]);
    
    //int plan[3] = {0, 2, 1};
    //double result = model(plan);
    //printf("%f\n",result);
    
    free(households);
    free(imitators);
    
    return 0;
}
