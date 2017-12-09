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

#define POPULATION_SIZE 1000
#define GENERATIONS 1000
#define TOURNAMENT_SIZE 5
#define MUTATION_CHANCE 5

static int noOfAreas = 3;
static int studyPeriod = 10;
static double rental = 2;
static double capex = 500;
static double opex = 200;
static double interest = 0.01;
static int maxRolloutPeriod = 10;

static int *households;
static long double *imitators;

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
double model(int *plan) {
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
void evaluate(int population[][noOfAreas], double *fitnessArray) {
    for(int individual = 0; individual < POPULATION_SIZE; individual++) {
        fitnessArray[individual] = model(population[individual]);
    }
}

/* Initialises, with random values, the given population.
 */
void initialise(int population[][noOfAreas]) {
    for(int individual = 0; individual < POPULATION_SIZE; individual++) {
        for(int area = 0; area < noOfAreas; area++) {
            population[individual][area] = rand() % (maxRolloutPeriod+1);
        }
    }
}

/* Performs a single point crossover on the 2 parents supplied,
 * sets the content of the 2 supplied children.
 */
void crossover(int *parent1, int *parent2, int *child1, int *child2) {
    int cop = rand() % noOfAreas-1; //Crossover point selection
    //printf("######## COP %d\n",cop);
    for(int area = 0; area < cop; area++) { //Up to cop: copy from parent
        //printf("p1 %p\n",&parent1[area]);
        //printf("p2 %p\n",&parent2[area]);
        //printf("c1 %p\n",&child1[area]);
        //printf("c2 %p\n",&child2[area]);
        child1[area] = parent1[area];
        child2[area] = parent2[area];
        //printf("Orig %d\n",area);
    }
    for(int area = cop; area < noOfAreas; area++) { //After cop: copy from alternate parent
        //printf("p1 %p\n",&parent1[area]);
        //printf("p2 %p\n",&parent2[area]);
        //printf("c1 %p\n",&child1[area]);
        //printf("c2 %p\n",&child2[area]);
        child1[area] = parent2[area];
        child2[area] = parent1[area];
        //printf("Alt %d\n",area);
    }
}

/* Performs a single point mutation on the parent supplied,
 sets the content
 */
void mutation(int *parent, int *child) {
    for(int i = 0; i < noOfAreas; i++) { //Copy all data
        child[i] = parent[i];
    }
    int area = rand() % noOfAreas; //Select random index
    child[area] = rand() % (maxRolloutPeriod+1); //Replace with random
}

/* Returns an index from the given array.
 */
int tournamentSelect(double *fitness) {
    int best = rand() % POPULATION_SIZE; // Select first individual
    for(int i = 0; i < TOURNAMENT_SIZE-1; i++) { // For the rest of the tournament
        int individual = rand() % POPULATION_SIZE;
        if(fitness[individual] > fitness[best]) {
            best = individual; // Set the current best
        }
    }
    return best;
}

int selectBest(double *fitness) {
    int best = 0;
    for(int i = 1; i < POPULATION_SIZE; i++) {
        if(fitness[i] > fitness[best]) {
            best = i;
        }
    }
    return best;
}

/* Generates a new population using the provided parent population,
 * writing the new population to the supplied child/new population,
 * using the provided fitness array for judgement.
 */
void generatePopulation(int parentPopulation[][noOfAreas], int newPopulation[][noOfAreas], double *fitness) {
    //Elitism
    for(int area = 0; area < noOfAreas; area++) {
        newPopulation[0][area] = parentPopulation[selectBest(fitness)][area];
    }
    
    for(int individual = 1; individual < POPULATION_SIZE; individual++) {
        if(rand() % 100 < MUTATION_CHANCE || individual == POPULATION_SIZE-2) {
            mutation(parentPopulation[tournamentSelect(fitness)], newPopulation[individual]);
        } else {
            crossover(parentPopulation[tournamentSelect(fitness)], parentPopulation[tournamentSelect(fitness)], newPopulation[individual], newPopulation[individual+1]);
            individual++; //Additional increment
        }
    }
}

void copyPopulation(int srcPopulation[][noOfAreas], int destPopulation[][noOfAreas]) {
    for(int individual = 0; individual < POPULATION_SIZE; individual++) {
        for(int area = 0; area < noOfAreas; area++) {
            destPopulation[individual][area] = srcPopulation[individual][area];
        }
    }
}

void printIndividual(int *individual) {
    for(int area = 0; area < noOfAreas; area++) {
        printf("%d,", individual[area]);
    }
    printf("\n");
}

void printFitnessData(double *fitness) {
    double best = 0, worst = fitness[0], mean = 0;
    for(int i = 0; i < POPULATION_SIZE; i++) {
        mean += fitness[i];
        if(fitness[i] > best) { best = fitness[i]; }
        if(fitness[i] < worst) { worst = fitness[i]; }
    }
    mean = mean / POPULATION_SIZE;
    printf("%.2f : %.2f : %.2f\n", worst, mean, best);
}

/* Main evolutionary loop
 * param int Print generational stats
 */
void run(int print) {
    int population[POPULATION_SIZE][noOfAreas];
    initialise(population);
    double fitness[POPULATION_SIZE];
    evaluate(population, fitness);
    printf("%d: ", 0);
    printFitnessData(fitness);
    //printf("Best: ");
    //printIndividual(population[selectBest(fitness)]);
    for(int generation = 1; generation < GENERATIONS; generation++) {
        int newPopulation[POPULATION_SIZE][noOfAreas];
        generatePopulation(population, newPopulation, fitness);
        evaluate(newPopulation, fitness);
        printf("%d: ", generation);
        printFitnessData(fitness);
        copyPopulation(newPopulation, population);
    }
    printf("Best: ");
    printIndividual(population[selectBest(fitness)]);
}

void runFor(int runs) {
    for(int runi = 0; runi < runs; runi++) {
        run(1);
    }
}

int main(int argc, const char * argv[]) {
    srand((unsigned) time(NULL)); // Initialise rand seed based on system time
    
    //TEST MODEL
    //households = (int*)malloc(noOfAreas*sizeof(int));
    //imitators = (long double*)malloc(noOfAreas*sizeof(long double));
    //households[0] = 100;
    //households[1] = 100;
    //households[2] = 1000;
    //imitators[0] = 0.2;
    //imitators[1] = 0.5;
    //imitators[2] = 0.2;
    //int plan[noOfAreas];
    //plan[0] = 0;
    //plan[1] = 2;
    //plan[2] = 1;
    //printf("%.2f\n", model(plan));
    
    printf("----- OPTIMAL FFTx ROLLOUT GA -----\n");
    printf("Matthew Mansell (mcm36)\n");
    
    char settingsFile[100], areasFile[100];
    strcpy(settingsFile, argv[1]); strcpy(areasFile, argv[2]);
    if(loadFTT(settingsFile, areasFile) != 1) {
        printf("FTTx GA cannot run without the correct data.\nExiting...\n");
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
    
    int test[noOfAreas];
    for(int i = 0; i < noOfAreas; i++) {
        test[i] = 1;
    }
    printf("%.2f\n", model(test));
    
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
            run(1);
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
    
    
    
    
    
    srand((unsigned) time(NULL)); // Initialise rand seed based on system time
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

