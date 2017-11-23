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

#define POPULATION_SIZE 100

static int noOfAreas = 3;
static int period = 10;
static double rental = 2;
static double capex = 500;
static double opex = 200;
static double interest = 0.01;
static int households[3] = {100, 100, 1000};
static double imitator[3] = {0.2, 0.5, 0.2};

//static int fitness[POPULATION_SIZE];

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

void evaluate(int population[POPULATION_SIZE][noOfAreas], double *fitness) {
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


int main(int argc, const char * argv[]) {
    srand((unsigned) time(NULL)); // Initialise rand seed based on system time
    int population[POPULATION_SIZE][noOfAreas];
    initialise(population);
    
    for(int i = 0; i < noOfAreas; i++) {
        printf("%d\n",population[0][i]);
    }
    
    
    double fitness[POPULATION_SIZE];
    evaluate(population, fitness);
    printf("%f\n",fitness[0]);
    
    
    
    int plan[3] = {0, 2, 1};
    double result = model(plan);
    printf("%f\n",result);
    return 0;
}
