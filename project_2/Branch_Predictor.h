#ifndef __BRANCH_PREDICTOR_HH__
#define __BRANCH_PREDICTOR_HH__

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include "Instruction.h"

// saturating counter
typedef struct Sat_Counter
{
    unsigned counter_bits;
    uint64_t max_val;
    uint64_t counter;
} Sat_Counter;

typedef struct Perceptrons
{
    int64_t *weights;
    unsigned num_perceptrons;
} Perceptrons;

typedef struct Branch_Predictor
{
    unsigned global_predictor_size;
    unsigned global_history_mask;
    Sat_Counter *global_counters;
    uint64_t global_history;

    unsigned threshold;
    unsigned perceptron_size;
    unsigned perceptron_mask;
    Perceptrons *perceptrons;
} Branch_Predictor;

// Initialization function
Branch_Predictor *initBranchPredictor();

// Counter functions
void initSatCounter(Sat_Counter *sat_counter, unsigned counter_bits);
void incrementCounter(Sat_Counter *sat_counter);
void decrementCounter(Sat_Counter *sat_counter);

// Branch predictor functions
void initPerceptron(Perceptrons *perceptrons, unsigned counter_bits);
bool predict(Branch_Predictor *branch_predictor, Instruction *instr);
int64_t computePerceptron(Perceptrons *perceptrons, Sat_Counter *sat_counter);
void train(Perceptrons *perceptrons, unsigned threshold, Sat_Counter *sat_counter, bool is_taken, int64_t y);

unsigned getIndex(uint64_t branch_addr, unsigned index_mask);
bool getPrediction(Sat_Counter *sat_counter);

// Utility
int checkPowerofTwo(unsigned x);

#endif








