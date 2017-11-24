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

typedef int bool;
#define true bool 1
#define false bool 0

#define POPULATION_SIZE 100
#define GENERATIONS 100

static int noOfAreas = 3;
static int period = 10;
static double rental = 2;
static double capex = 500;
static double opex = 200;
static double interest = 0.01;
static int households[3] = {100, 100, 1000};
static double imitator[3] = {0.2, 0.5, 0.2};

//static int fitness[POPULATION_SIZE];

void loadFTT() {
    FILE *settingsFile, *areasFile;
    
    char path1[200];
    char path2[200];
    getcwd(path1, sizeof(path1));
    getcwd(path2, sizeof(path2));
    
    for(int i = 0; i < sizeof(path1); i++) {
        printf("%c", path1[i]);
    }
    
    strcat(path1, "/assignment2a.csv");
    strcat(path2, "/assignment2b.csv");
    
    
    
    settingsFile = fopen(path1, "r");
    areasFile = fopen(path2, "r");
    
    if(settingsFile == NULL) {
        perror("Error when attempting to open the file.\n");
        //exit(EXIT_FAILURE);
    } else {
        printf("Content:\n");
        
        char ch;
        while((ch = fgetc(settingsFile)) != EOF) {
            printf("%c",ch);
        }
        fclose(settingsFile);
        
    }
    
    
    //Load settings into global variables
    
    //Load areas into households and imitator
}

double model(int plan[]) {
    int totalCustomers = 0, totalDevelopments = 0;
    
    double NPV = 0, expenditure = 0, income = 0;
    for(int year = 1; year <= period; year++) { //For each year
        double yearCAPEX = 0, yearOPEX = totalDevelopments * opex, cashFlow = 0;
        for(int area = 0; area < sizeof(*plan); area++) { //For each area
            if(plan[area] < year && plan[area] != 0) { //Customers for this year
                int newCustomers = households[area]*imitator[area];
                totalCustomers += newCustomers;
                households[area]-= newCustomers;
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

void evaluate(int population[POPULATION_SIZE][noOfAreas], double fitness[noOfAreas]) {
    for(int individual = 0; individual < POPULATION_SIZE; individual++) {
        fitness[individual] = model(population[individual]);
    }
}

void initialise(int population[POPULATION_SIZE][noOfAreas]) {
    for(int individual = 0; individual < POPULATION_SIZE; individual++) {
        for(int area = 0; area < noOfAreas; area++) {
            population[individual][area] = rand() % period;
        }
    }
}

/* Performs a crossover mutation on the 2 parents supplied,
 * sets the content of the 2 supplied children.
 */
void crossover(int parent1[noOfAreas], int parent2[noOfAreas], int child1[noOfAreas], int child2[noOfAreas]) {
    for(int area = 0; area < noOfAreas; area++) {
        child1[area] = parent2[area];
        child2[area] = parent1[area];
    }
}

void mutation(int parent[noOfAreas], int child[noOfAreas]) {
    
}

int tournamentSelect(double fitness[noOfAreas]) {
    return 0;
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
 *
 */
void run(bool print) {
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
    for(int run = 0; run < runs; run++) {
        //run(false);
    }
}

int main(int argc, const char * argv[]) {
    srand((unsigned) time(NULL)); // Initialise rand seed based on system time
    int population[POPULATION_SIZE][noOfAreas];
    initialise(population);
    
    for(int i = 0; i < noOfAreas; i++) {
        printf("%d\n",population[0][i]);
    }
    printIndividual(population[0]);
    
    double fitness[POPULATION_SIZE];
    evaluate(population, fitness);
    printf("%f\n",fitness[0]);
    
    int newPopulation[POPULATION_SIZE][noOfAreas];
    generatePopulation(population, newPopulation, fitness);
    
    for(int i = 0; i < noOfAreas; i++) {
        printf("%d\n",newPopulation[0][i]);
    }
    printIndividual(newPopulation[0]);
    
    
    int plan[3] = {0, 2, 1};
    double result = model(plan);
    printf("%f\n",result);
    
    loadFTT();
    
    return 0;
}
