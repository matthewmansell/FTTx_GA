//
//  main.c
//  A2_Optimal_FTTx_Rollout_MM
//  Task B
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

#define POPULATION_SIZE 3000                //
#define GENERATIONS 200                     //
#define TOURNAMENT_SIZE 5                   //Tournament size
#define MUTATION_CHANCE 5                   //Mutation % chance
#define MUTATION_POINTS 2                   //The number of points changed per mutation
#define RESULTS_FILE "results.txt"          //Results file name

static int noOfAreas = 0;                   //
static int studyPeriod = 0;                 //
static double rental = 0;                   //
static double capex = 0;                    //
static double opex = 0;                     //
static double interest = 0;                 //
static int maxRolloutPeriod = 0;            //
static double y1MaxSpend = 100000;          //Max spending for y1 developments

static int *households;                     //Store for loaded households
static long double *imitators;              //Store for loaded imitators

static int *population[POPULATION_SIZE];    //The main population
static int *newPopulation[POPULATION_SIZE]; //The holder for generating a new population

// ############################################################################
// #################### ----------- FILE READER ---------- ####################
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
        document = malloc(docLength+1); //Allocate document memory/buffer
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
        document = malloc(docLength+1); //Allocate document memory/buffer
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

// ############################################################################
// #################### ----------- FILE WRITER ---------- ####################
void writeLog(char* msg) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t); //Get system time
    char date[19];
    strftime(date, 19, "%x_%X\t\0", tm); //Format time code
    char log[120] = {'\0'}; //Ensure string it closed
    strcat(log, date); //Concat date to log
    strcat(log, msg); //Concat message
    FILE *resultsFile = fopen(RESULTS_FILE, "a"); //Open the log file, if it doesnt exist it will be created.
    fputs(log, resultsFile);
    fclose(resultsFile);
}


// ####################################################################################
// #################### ----------- FTTx NPV CALCULATOR ---------- ####################
/* Runs the given FTT rollout plan agains the model.
 * Return The final NPV for this plan.
 * Param origHouseholds An array containing
 */
double model(int *plan) {
    //Make a copy of the households array that we are able to edit
    int copyHouseholds[noOfAreas];
    for(int i = 0; i < noOfAreas; i++) copyHouseholds[i] = households[i];
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

// ##############################################################################################
// #################### ----------- EVALUATION/FITNESS GENERATION ---------- ####################
/* Generates fitness values using data for the given population.
 * Param population The population to evaluate.
 * Param fitness The fitness array to populate.
 */
void evaluate(int **subjectPop, double *fitnessArray) {
    for(int individual = 0; individual < POPULATION_SIZE; individual++) {
        fitnessArray[individual] = model(subjectPop[individual]); //Run model for this area
    }
}

// ###############################################################################
// #################### ----------- INITIALISATION ---------- ####################
/* Initialises, with random values, the given population.
 */
void initialise() {
    for(int individual = 0; individual < POPULATION_SIZE; individual++) {
        double y1Spend = 0;
        for(int area = 0; area < noOfAreas; area++) {
            if(area < 10) population[individual][area] = 0; //Dont want to build in first 10 cities
            else {
                int canAdd = 0, value = 0;
                do { //Find value which we can add
                    value = rand() % (maxRolloutPeriod+1);
                    if(value == 1) { //Additional checks for y1
                        if(y1Spend <= (y1MaxSpend-capex)) canAdd = 1;
                    } else canAdd = 1;
                } while(canAdd != 1); //If we cant add this value, re-random
                if(value == 1) y1Spend += capex; //Add to out y1 spending if we added a 1
                population[individual][area] = value;
            }
        }
    }
}

// ##########################################################################
// #################### ----------- CROSSOVER ---------- ####################
/* Performs a crossover on the 2 given parents, using my 'random point swap with rollback'
 * method to ensure that parent meets the criteria.
 * Param
 */
void crossover(int *parent1, int *parent2, int *child1, int *child2) {
    double c1Y1Spend = 0, c2Y1Spend = 0; //Values for y1 development spendings
    for(int area = 0; area < noOfAreas; area++) { //Copy entire parent
        child1[area] = parent1[area];
        if(child1[area] == 1) c1Y1Spend += capex;
        child2[area] = parent2[area];
        if(child2[area] == 1) c2Y1Spend += capex;
    }
    int crossovers = 1 + rand() % (noOfAreas-1); //Least no. of crossovers
    int changes[crossovers];
    for(int i = 0; i < crossovers; i++) { //Perform required no. of crossovers
        int cop = rand() % noOfAreas; //Point to swap
        changes[i] = cop; //Remember the point we changed
        //Calculate new Y1 spend for after swap
        if(child1[cop] == 1 && child2[cop] != 1) { c1Y1Spend -= capex; c2Y1Spend += capex; }
        if(child1[cop] != 1 && child2[cop] == 1) { c1Y1Spend += capex; c2Y1Spend -= capex; }
        //Swap values
        int valStore = child1[cop]; //Store value so its not lost
        child1[cop] = child2[cop];
        child2[cop] = valStore;
    }
    int undoPoint = crossovers-1; //Select last change
    while(c1Y1Spend > y1MaxSpend || c2Y1Spend > y1MaxSpend) {
        int cop = changes[undoPoint]; //Point to swap back
        //Calculate new Y1 spend for after swap
        if(child1[cop] == 1 && child2[cop] != 1) { c1Y1Spend -= capex; c2Y1Spend += capex; }
        if(child1[cop] != 1 && child2[cop] == 1) { c1Y1Spend += capex; c2Y1Spend -= capex; }
        //Swap values
        int valStore = child1[cop]; //Store value so its not lost
        child1[cop] = child2[cop];
        child2[cop] = valStore;
        undoPoint--;
    }
}

// #########################################################################
// #################### ----------- MUTATION ---------- ####################
/* Performs a two point mutation on the parent supplied,
 sets the content
 */
void mutation(int *parent, int *child) {
    double y1Spend = 0;
    for(int i = 0; i < noOfAreas; i++) { //Copy all data
        child[i] = parent[i];
        if(child[i] == 1) y1Spend += capex; // Count y1 builds
    }
    for(int i = 0; i < MUTATION_POINTS; i++) {
        int canBuildY1 = 0, canAdd = 0, value = 0;
        if(y1Spend <= (y1MaxSpend-capex)) canBuildY1 = 1; //Can we introduce any 1s?
        do { //Ensure that our new random value meets the criteria
            value = rand() % (maxRolloutPeriod+1);
            if(value == 1 && canBuildY1 == 1) {
                canAdd = 1;
                y1Spend += capex; //Adding a 1 so increase capex for next mutation calc
            } else if(value != 1) canAdd = 1;
        } while(canAdd != 1);
        int area = 10 + rand() % (noOfAreas-10); //Select random index above 10
        child[area] = value; //Apply mutation
    }
}

// ##########################################################################
// #################### ----------- SELECTION ---------- ####################
/* Returns an index from the given array.
 */
int tournamentSelect(double *fitness) {
    int best = rand() % POPULATION_SIZE; // Select first individual
    for(int i = 0; i < TOURNAMENT_SIZE-1; i++) { // For the rest of the tournament
        int individual = rand() % POPULATION_SIZE;
        if(fitness[individual] > fitness[best]) best = individual; // Set the current best
    }
    return best;
}

// #######################################################################################
// #################### ----------- SELECT BEST INDIVIDUAL ---------- ####################
/* Finds and returns the index of the best individual from the givne fitness array.
 */
int selectBest(double *fitness) {
    int best = 0;
    for(int i = 1; i < POPULATION_SIZE; i++) {
        if(fitness[i] > fitness[best]) best = i;
    }
    return best;
}

// ####################################################################################
// #################### ----------- GENERATE POPULATION ---------- ####################
/* Generates a new population using the provided parent population,
 * writing the new population to the supplied child/new population,
 * using the provided fitness array for judgement.
 */
void generatePopulation(double *fitness) {
    int best = selectBest(fitness); //Best individual from previous
    for(int area = 0; area < noOfAreas; area++) { //Copy best individual (elitism)
        newPopulation[0][area] = population[best][area];
    }
    for(int individual = 1; individual < POPULATION_SIZE; individual++) { //Generate the rest of population
        if(rand() % 100 < MUTATION_CHANCE || individual > POPULATION_SIZE-2) { //Choice of mutation/crossover
            mutation(population[tournamentSelect(fitness)], newPopulation[individual]);
        } else {
            crossover(population[tournamentSelect(fitness)], population[tournamentSelect(fitness)], newPopulation[individual], newPopulation[individual+1]);
            individual++; //Additional increment
        }
    }
}

// ################################################################################
// #################### ----------- POPULATION COPY ---------- ####################
/* Copies the given source population into the given destination population.
 * (Used for moving the new population in to the main population)
 */
void copyPopulation(int *srcPopulation[POPULATION_SIZE], int *destPopulation[POPULATION_SIZE]) {
    for(int individual = 0; individual < POPULATION_SIZE; individual++) {
        for(int area = 0; area < noOfAreas; area++) {
            destPopulation[individual][area] = srcPopulation[individual][area];
        }
    }
}

// ##############################################################################################
// #################### ----------- PRINT INDIVIDUAL ROLLOUT PLAN ---------- ####################
void printIndividual(int *individual) {
    for(int area = 0; area < noOfAreas; area++) printf("%d,", individual[area]);
    printf("\n");
}

// ##############################################################################
// #################### ----------- RESULT PRINTS ---------- ####################
void fitnessStats(double *fitness, char* results) {
    double best = 0, worst = fitness[0], mean = 0;
    for(int i = 0; i < POPULATION_SIZE; i++) {
        mean += fitness[i];
        if(fitness[i] > best) { best = fitness[i]; }
        if(fitness[i] < worst) { worst = fitness[i]; }
    }
    mean = mean / POPULATION_SIZE;
    sprintf(results + strlen(results), "%.2f :\t%.2f :\t%.2f\n", worst, mean, best);
}

// ######################################################################################
// #################### ----------- EVOLUTIONARY LOOP/RUN ---------- ####################
/* Main evolutionary loop
 * param int Print generational stats
 */
double run(int print, char* results, int *bestIndividual) {
    double fitness[POPULATION_SIZE]; //Fitness buffer
    initialise(); //Initial population 0
    evaluate(population, fitness); //Evaluate initial population
    if(print == 1) { char stats[100] = {'\0'}; fitnessStats(fitness, stats); printf("%d: %s", 0, stats);}
    for(int generation = 1; generation < GENERATIONS; generation++) {
        generatePopulation(fitness);
        evaluate(newPopulation, fitness);
        if(print == 1) { char stats[100] = {'\0'}; fitnessStats(fitness, stats); printf("%d: %s", generation, stats); }
        copyPopulation(newPopulation, population);
    }
    int best = selectBest(fitness);
    if(print == 1) { printf("Best: "); printIndividual(population[best]); }
    fitnessStats(fitness, results); //Write to given results array
    for(int i = 0; i < noOfAreas; i++) bestIndividual[i] = population[best][i]; //Copy of best
    return fitness[best];
}

// ##############################################################################
// #################### ----------- MULTIPLE RUNS ---------- ####################
void runFor(int runs) {
    writeLog("RUN\tWorst:\t\tMean:\t\tBest:\n\0");
    double results[runs], avg = 0, min = 0, max = 0, sd = 0;
    int bestIndividual[noOfAreas];
    for(int runi = 0; runi < runs; runi++) {
        char print[100]; sprintf(print, "Run %d:\t", runi+1); //Statement to print
        double result; //Local result store
        int bestIndividualRun[noOfAreas];
        result = run(1, print, bestIndividualRun);
        writeLog(print); //Print statement
        results[runi] = result;
        avg += result/runs;
        if(result < min || runi == 0) min = result;
        if(result > max) {
            max = result;
            for(int i = 0; i < noOfAreas; i++) bestIndividual[i] = bestIndividualRun[i];
        }
    }
    int no0 = 0, no1 = 0, no2 = 0, no3 = 0, no4 = 0;
    for(int i = 0; i < noOfAreas; i++) {
        if(bestIndividual[i] == 0) no0++;
        if(bestIndividual[i] == 1) no1++;
        if(bestIndividual[i] == 2) no2++;
        if(bestIndividual[i] == 3) no3++;
        if(bestIndividual[i] == 4) no4++;
    }
    printf("Best of all runs: \n "); printIndividual(bestIndividual);
    printf("0s:%d\t1s:%d\t2s:%d\t3s:%d\t4s:%d\n", no0, no1, no2, no3, no4);
    for(int i = 0; i < runs; i++) { //Calculate standard deviation
        double deviation = pow(results[i] - avg, 2);
        sd += deviation;
    } sd = sqrt(sd/runs);
    char print[100];
    sprintf(print, "Avg:\t%.2f\n", avg);
    writeLog(print);
    sprintf(print, "SD:\t%.2f\n", sd);
    writeLog(print);
    sprintf(print, "Max:\t%.2f\n", max);
    writeLog(print);
    sprintf(print, "Min:\t%.2f\n\n", min);
    writeLog(print);
}

// ##############################################################################
// #################### ----------- MAIN ---------- ####################
int main(int argc, const char * argv[]) {
    srand((unsigned) time(NULL)); // Initialise rand seed based on system time
    printf("----- OPTIMAL FTTx ROLLOUT GA -----\n");
    printf("Matthew Mansell (mcm36)\n");
    char settingsFile[100], areasFile[100];
    strcpy(settingsFile, argv[1]); strcpy(areasFile, argv[2]);
    if(loadFTT(settingsFile, areasFile) != 1) {
        printf("FTTx GA cannot run without the correct data.\nExiting...\n");
        return 0; //Exit
    }
    printf("FFT data loaded sucesfully.\n");
    //Allocate memory for populations
    for(int i = 0; i < POPULATION_SIZE; i++) {
        population[i] = malloc(noOfAreas * sizeof(int));
        newPopulation[i] = malloc(noOfAreas * sizeof(int));
    }
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
            runFor(10);
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
    //Free allocated memory
    free(households);
    free(imitators);
    free(population); //FIX THIS!!!!
    free(newPopulation); //FIX THIS!!!!
    return 0;
}

