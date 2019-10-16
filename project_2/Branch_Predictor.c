#include "Branch_Predictor.h"

const unsigned instShiftAmt = 2; // Number of bits to shift a PC by

// You can play around with these settings.
const unsigned globalPredictorSize = 8192; 
const unsigned globalCounterBits = 32;                 // Best if from 12-62
const unsigned perceptronSize = 8192;  

Branch_Predictor *initBranchPredictor()
{
    Branch_Predictor *branch_predictor = (Branch_Predictor *)malloc(sizeof(Branch_Predictor));
    // GLOBAL HISTORY TABLE
    assert(checkPowerofTwo(globalPredictorSize));
    branch_predictor->global_predictor_size = globalPredictorSize;
    branch_predictor->global_history_mask = globalPredictorSize - 1;
    branch_predictor->global_history = 0; // global history register
    // Initialize global counters
    branch_predictor->global_counters = (Sat_Counter *)malloc(globalPredictorSize * sizeof(Sat_Counter));
    for (int i = 0; i < globalPredictorSize; i++)
        initSatCounter(&(branch_predictor->global_counters[i]), globalCounterBits);

    // PERCEPTRON
    assert(checkPowerofTwo(perceptronSize));
    branch_predictor->perceptron_size = perceptronSize;
    branch_predictor->threshold = 1.93 * globalCounterBits + 14; // This is best set at 1.93*history_length+14
    unsigned perceptronBits = 1 + floor(log2(branch_predictor->threshold));
    branch_predictor->perceptron_mask = perceptronSize - 1;
    branch_predictor->perceptrons = (Perceptrons *)malloc(perceptronSize * sizeof(Perceptrons));
    for(int i = 0; i < perceptronSize; i++)
	initPerceptron(&(branch_predictor->perceptrons[i]), globalCounterBits);
    
    return branch_predictor;
}

// Branch Predictor functions
bool predict(Branch_Predictor *branch_predictor, Instruction *instr)
{
    uint64_t branch_address = instr->PC;
    // Step one, get global prediction.
    unsigned global_predictor_idx = (branch_predictor->global_history & branch_predictor->global_history_mask) ^ (branch_address & branch_predictor->global_history_mask);
    
    // Step five, update counters
    if (instr->taken)
        incrementCounter(&(branch_predictor->global_counters[global_predictor_idx]));
    else
        decrementCounter(&(branch_predictor->global_counters[global_predictor_idx]));

    // Step six, update global history register
    branch_predictor->global_history = (branch_predictor->global_history << 1) | instr->taken;
    
    unsigned perceptron_idx = branch_predictor->perceptron_mask & branch_address;
    int64_t y = computePerceptron(&(branch_predictor->perceptrons[perceptron_idx]), &(branch_predictor->global_counters[global_predictor_idx]));
    train(&(branch_predictor->perceptrons[perceptron_idx]), branch_predictor->threshold, &(branch_predictor->global_counters[global_predictor_idx]), instr->taken, y);
    bool prediction = (y > 0);
    return prediction == instr->taken;
}

void initPerceptron(Perceptrons *perceptrons, unsigned counter_bits)
{
    perceptrons->weights = (int64_t *)malloc(counter_bits * sizeof(int64_t));
    for(int i = 0; i <= counter_bits; i++)
	perceptrons->weights[i] = 1;    
    perceptrons->num_perceptrons = counter_bits;
}

int64_t computePerceptron(Perceptrons *perceptrons, Sat_Counter *sat_counter)
{
    int64_t y = perceptrons->weights[0];
    for(int i = 1; i <= sat_counter->counter_bits; i++)
    {
	int8_t temp = (sat_counter->counter & (1 << i));
	int8_t x = 1;
	if(temp < 0)
	    x = -1;
	y += x * perceptrons->weights[i];
    }
    return y;
}

void train(Perceptrons *perceptrons, unsigned threshold, Sat_Counter *global, bool is_taken, int64_t y)
{
    if((y < 0) == is_taken || (y > 0) == is_taken || y <= threshold)
	for(int i = 0; i <= global->counter_bits; i++)
	{
	    int8_t temp = (global->counter & (1 << i));
	    int8_t x = 1;
	    int8_t t = 1;
	    if(temp < 0)
		x = -1;
	    if(is_taken == false)
		t = -1;
	    perceptrons->weights[i] = perceptrons->weights[i] + (t * x);
	}
}

// sat counter functions
inline void initSatCounter(Sat_Counter *sat_counter, unsigned counter_bits)
{
    sat_counter->counter_bits = counter_bits;
    sat_counter->counter = 0;
    sat_counter->max_val = (1 << counter_bits) - 1;
}

inline void incrementCounter(Sat_Counter *sat_counter)
{
    if (sat_counter->counter < sat_counter->max_val)
    {
        ++sat_counter->counter;
    }
}

inline void decrementCounter(Sat_Counter *sat_counter)
{
    if (sat_counter->counter > 0) 
    {
        --sat_counter->counter;
    }
}

int checkPowerofTwo(unsigned x)
{
    //checks whether a number is zero or not
    if (x == 0)
    {
        return 0;
    }

    //true till x is not equal to 1
    while( x != 1)
    {
        //checks whether a number is divisible by 2
        if(x % 2 != 0)
        {
            return 0;
        }
        x /= 2;
    }
    return 1;
}
