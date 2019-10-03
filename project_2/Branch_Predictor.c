#include "Branch_Predictor.h"

const unsigned instShiftAmt = 2; // Number of bits to shift a PC by

// You can play around with these settings.
const unsigned localPredictorSize = 2048;
const unsigned localCounterBits = 2;
const unsigned localHistoryTableSize = 2048; 
const unsigned globalPredictorSize = 8192; // This needs to match the size of global_history or PC whichever is bigger 
const unsigned globalCounterBits = 2;
const unsigned choicePredictorSize = 8192; // Keep this the same as globalPredictorSize.
const unsigned choiceCounterBits = 2;

Branch_Predictor *initBranchPredictor()
{
    Branch_Predictor *branch_predictor = (Branch_Predictor *)malloc(sizeof(Branch_Predictor));

    #ifdef TWO_BIT_LOCAL
    branch_predictor->local_predictor_sets = localPredictorSize;
    assert(checkPowerofTwo(branch_predictor->local_predictor_sets));

    branch_predictor->index_mask = branch_predictor->local_predictor_sets - 1;

    // Initialize sat counters
    branch_predictor->local_counters =
        (Sat_Counter *)malloc(branch_predictor->local_predictor_sets * sizeof(Sat_Counter));

    for (int i = 0; i < branch_predictor->local_predictor_sets; i++)
        initSatCounter(&(branch_predictor->local_counters[i]), localCounterBits);
    #endif

    #ifdef TOURNAMENT
    assert(checkPowerofTwo(localPredictorSize));
    assert(checkPowerofTwo(localHistoryTableSize));
    assert(checkPowerofTwo(globalPredictorSize));
    assert(checkPowerofTwo(choicePredictorSize));
    assert(globalPredictorSize == choicePredictorSize);

    branch_predictor->local_predictor_size = localPredictorSize;
    branch_predictor->local_history_table_size = localHistoryTableSize;
    branch_predictor->global_predictor_size = globalPredictorSize;
    branch_predictor->choice_predictor_size = choicePredictorSize;
   
    // Initialize local counters 
    branch_predictor->local_counters =
        (Sat_Counter *)malloc(localPredictorSize * sizeof(Sat_Counter));

    for (int i = 0; i < localPredictorSize; i++)
        initSatCounter(&(branch_predictor->local_counters[i]), localCounterBits);

    branch_predictor->local_predictor_mask = localPredictorSize - 1;

    // Initialize local history table
    branch_predictor->local_history_table = 
        (unsigned *)malloc(localHistoryTableSize * sizeof(unsigned));

    for (int i = 0; i < localHistoryTableSize; i++)
        branch_predictor->local_history_table[i] = 0;

    branch_predictor->local_history_table_mask = localHistoryTableSize - 1;

    // Initialize global counters
    branch_predictor->global_counters = 
        (Sat_Counter *)malloc(globalPredictorSize * sizeof(Sat_Counter));

    for (int i = 0; i < globalPredictorSize; i++)
        initSatCounter(&(branch_predictor->global_counters[i]), globalCounterBits);

    branch_predictor->global_history_mask = globalPredictorSize - 1;

    // Initialize choice counters
    branch_predictor->choice_counters = 
        (Sat_Counter *)malloc(choicePredictorSize * sizeof(Sat_Counter));

    for (int i = 0; i < choicePredictorSize; i++)
        initSatCounter(&(branch_predictor->choice_counters[i]), choiceCounterBits);

    branch_predictor->choice_history_mask = choicePredictorSize - 1;

    // global history register
    branch_predictor->global_history = 0;

    // We assume choice predictor size is always equal to global predictor size.
    branch_predictor->history_register_mask = choicePredictorSize - 1;
    #endif

    #ifdef GSHARE
    assert(checkPowerofTwo(globalPredictorSize));
    branch_predictor->global_predictor_size = globalPredictorSize;
   
    // Initialize global counters
    branch_predictor->global_counters = (Sat_Counter *)malloc(globalPredictorSize * sizeof(Sat_Counter));
    for (int i = 0; i < globalPredictorSize; i++)
        initSatCounter(&(branch_predictor->global_counters[i]), globalCounterBits);
    branch_predictor->global_history_mask = globalPredictorSize - 1;

    // global history register
    branch_predictor->global_history = 0;
    #endif

    #ifdef PERCEPTRON
    assert(checkPowerofTwo(globalPredictorSize));
    branch_predictor->global_predictor_size = globalPredictorSize;
   
    // Initialize global counters
    branch_predictor->global_counters = (Sat_Counter *)malloc(globalPredictorSize * sizeof(Sat_Counter));
    for (int i = 0; i < globalPredictorSize; i++)
        initSatCounter(&(branch_predictor->global_counters[i]), globalCounterBits);
    branch_predictor->global_history_mask = globalPredictorSize - 1;

    // global history register
    branch_predictor->global_history = 0;

    for(int i = 0; i < 2048; i++)
	for(int j = 0; j <= 1; j++)
	    branch_predictor->perceptrons[i][j] = 0;
    branch_predictor->threshold = 1;
    branch_predictor->perceptron_mask = 2048 - 1;
    #endif
    
    return branch_predictor;
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

// Branch Predictor functions
bool predict(Branch_Predictor *branch_predictor, Instruction *instr)
{
    uint64_t branch_address = instr->PC;

    #ifdef TWO_BIT_LOCAL    
    // Step one, get prediction
    unsigned local_index = getIndex(branch_address, 
                                    branch_predictor->index_mask);

    bool prediction = getPrediction(&(branch_predictor->local_counters[local_index]));

    // Step two, update counter
    if (instr->taken)
    {
        // printf("Correct: %u -> ", branch_predictor->local_counters[local_index].counter);
        incrementCounter(&(branch_predictor->local_counters[local_index]));
        // printf("%u\n", branch_predictor->local_counters[local_index].counter);
    }
    else
    {
        // printf("Incorrect: %u -> ", branch_predictor->local_counters[local_index].counter);
        decrementCounter(&(branch_predictor->local_counters[local_index]));
        // printf("%u\n", branch_predictor->local_counters[local_index].counter);
    }

    return prediction == instr->taken;
    #endif

    #ifdef TOURNAMENT
    // Step one, get local prediction.
    unsigned local_history_table_idx = getIndex(branch_address,
                                           branch_predictor->local_history_table_mask);
    
    unsigned local_predictor_idx = 
        branch_predictor->local_history_table[local_history_table_idx] & 
        branch_predictor->local_predictor_mask;

    bool local_prediction = 
        getPrediction(&(branch_predictor->local_counters[local_predictor_idx]));

    // Step two, get global prediction.
    unsigned global_predictor_idx = 
        branch_predictor->global_history & branch_predictor->global_history_mask;

    bool global_prediction = 
        getPrediction(&(branch_predictor->global_counters[global_predictor_idx]));

    // Step three, get choice prediction.
    unsigned choice_predictor_idx = 
        branch_predictor->global_history & branch_predictor->choice_history_mask;

    bool choice_prediction = 
        getPrediction(&(branch_predictor->choice_counters[choice_predictor_idx]));


    // Step four, final prediction.
    bool final_prediction;
    if (choice_prediction)
    {
        final_prediction = global_prediction;
    }
    else
    {
        final_prediction = local_prediction;
    }

    bool prediction_correct = final_prediction == instr->taken;
    // Step five, update counters
    if (local_prediction != global_prediction)
    {
        if (local_prediction == instr->taken)
        {
            // Should be more favorable towards local predictor.
            decrementCounter(&(branch_predictor->choice_counters[choice_predictor_idx]));
        }
        else if (global_prediction == instr->taken)
        {
            // Should be more favorable towards global predictor.
            incrementCounter(&(branch_predictor->choice_counters[choice_predictor_idx]));
        }
    }

    if (instr->taken)
    {
        incrementCounter(&(branch_predictor->global_counters[global_predictor_idx]));
        incrementCounter(&(branch_predictor->local_counters[local_predictor_idx]));
    }
    else
    {
        decrementCounter(&(branch_predictor->global_counters[global_predictor_idx]));
        decrementCounter(&(branch_predictor->local_counters[local_predictor_idx]));
    }

    // Step six, update global history register
    branch_predictor->global_history = branch_predictor->global_history << 1 | instr->taken;
    // exit(0);
    //
    return prediction_correct;
    #endif

    #ifdef GSHARE
    // Step one, get global prediction.
    unsigned global_predictor_idx = (branch_predictor->global_history & branch_predictor->global_history_mask) ^ (branch_address & branch_predictor->global_history_mask);
    bool global_prediction = getPrediction(&(branch_predictor->global_counters[global_predictor_idx]));
    
    bool prediction_correct = global_prediction == instr->taken;
    // Step five, update counters
    if (instr->taken)
        incrementCounter(&(branch_predictor->global_counters[global_predictor_idx]));
    else
        decrementCounter(&(branch_predictor->global_counters[global_predictor_idx]));

    // Step six, update global history register
    branch_predictor->global_history = branch_predictor->global_history << 1 | instr->taken;
    return prediction_correct;
    #endif

    #ifdef PERCEPTRON
    // Step one, get global prediction.
    unsigned global_predictor_idx = (branch_predictor->global_history & branch_predictor->global_history_mask) ^ (branch_address & branch_predictor->global_history_mask);
    
    // Step five, update counters
    if (instr->taken)
        incrementCounter(&(branch_predictor->global_counters[global_predictor_idx]));
    else
        decrementCounter(&(branch_predictor->global_counters[global_predictor_idx]));

    // Step six, update global history register
    branch_predictor->global_history = branch_predictor->global_history << 1 | instr->taken;

    unsigned perceptron_idx = branch_predictor->perceptron_mask & branch_address;
    int8_t y = computePerceptron(branch_predictor->perceptrons[perceptron_idx], &(branch_predictor->global_counters[global_predictor_idx]));
    train(branch_predictor->perceptrons[perceptron_idx], branch_predictor->threshold, &(branch_predictor->global_counters[global_predictor_idx]), instr->taken, y);
    bool prediction = (y > 0);
    return prediction == instr->taken;
    #endif
}

inline unsigned getIndex(uint64_t branch_addr, unsigned index_mask)
{
    return (branch_addr >> instShiftAmt) & index_mask;
}

inline bool getPrediction(Sat_Counter *sat_counter)
{
    uint8_t counter = sat_counter->counter;
    unsigned counter_bits = sat_counter->counter_bits;

    // MSB determins the direction
    return (counter >> (counter_bits - 1));
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

bool computePerceptron(int8_t *perceptrons, Sat_Counter *sat_counter)
{
    int8_t y = perceptrons[0];
    for(int i = 1; i <= sat_counter->counter_bits; i++)
    {
	int8_t temp = (sat_counter->counter & (1 << i));
	int8_t x = 1;
	if(temp < 0)
	    x = -1;
	y += x * perceptrons[i];
    }
}

bool train(int8_t *perceptrons, int8_t threshold, Sat_Counter *sat_counter, bool is_taken, int8_t y)
{
    if((y < 0) == is_taken || (y > 0) == is_taken || y <= threshold)
	for(int i = 0; i <= sat_counter->counter_bits; i++)
	{
	    int8_t temp = (sat_counter->counter & (1 << i));
	    int8_t x = 1;
	    if(temp < 0)
		x = -1;
	    perceptrons[i] = perceptrons[i] + (is_taken * x);

	}
}







