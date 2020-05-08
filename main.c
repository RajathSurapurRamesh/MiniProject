#include <iostream>
#include <stdio.h>
#include <vector>
#include <string>
#include <algorithm>
#include <array>
#include <random>
#include <cmath>
#include <unordered_map>
#include <iomanip>
using namespace std;

#define BOARD_COLS 4
#define BOARD_ROWS 3
#define exp_rate 0.3 //exploration rate = 1/3
#define lr 0.2   //learning rate

const int LOSE[2] = {1,3};
const int WIN[2] ={0,3};
const int START [2] ={2,0};
const int actions[] = {0,1,2,3};

#ifdef __APPLE__
    #include <OpenCL/opencl.h>
#else
    #include <CL/cl.h>
#endif

#ifdef AOCL
    #include "CL/opencl.h"
    #include "AOCLUtils/aocl_utils.h"

    using namespace aocl_utils;
    void cleanup();
#endif

#define MAX_SOURCE_SIZE (0x100000)
#define DEVICE_NAME_LEN 128

void errorCheck(cl_int ret, char *check);


static char dev_name[DEVICE_NAME_LEN];

int main()
{
    cl_uint platformCount;
    cl_platform_id* platforms;
    cl_device_id device_id;
    cl_uint ret_num_devices;
    cl_int ret = 0;
    cl_context context = NULL;
    cl_command_queue command_queue = NULL;
    cl_program program = NULL;
    cl_kernel kernel = NULL;

    cl_uint num_comp_units;
    size_t global_size;
    size_t local_size;

    FILE *fp;
    char fileName[] = "./mykernel.cl";
    char *source_str;
    size_t source_size;

    #ifdef __APPLE__
        /* Get Platform and Device Info */
        clGetPlatformIDs(1, NULL, &platformCount);
        platforms = (cl_platform_id*) malloc(sizeof(cl_platform_id) * platformCount);
        clGetPlatformIDs(platformCount, platforms, NULL);
        // we only use platform 0, even if there are more plantforms
        // Query the available OpenCL device.
        ret = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_DEFAULT, 1, &device_id, &ret_num_devices);
        ret = clGetDeviceInfo(device_id, CL_DEVICE_NAME, DEVICE_NAME_LEN, dev_name, NULL);
        printf("device name= %s\n", dev_name);
    #else

        #ifdef AOCL  /* Altera FPGA */
            // get all platforms
            clGetPlatformIDs(0, NULL, &platformCount);
            platforms = (cl_platform_id*) malloc(sizeof(cl_platform_id) * platformCount);
            // Get the OpenCL platform.
            platforms[0] = findPlatform("Intel(R) FPGA");
            if(platforms[0] == NULL) {
              printf("ERROR: Unable to find Intel(R) FPGA OpenCL platform.\n");
              return false;
            }
            // Query the available OpenCL device.
            getDevices(platforms[0], CL_DEVICE_TYPE_ALL, &ret_num_devices);
            printf("Platform: %s\n", getPlatformName(platforms[0]).c_str());
            printf("Using one out of %d device(s)\n", ret_num_devices);
            ret = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_DEFAULT, 1, &device_id, &ret_num_devices);
            printf("device name=  %s\n", getDeviceName(device_id).c_str());
        #else
            #error "unknown OpenCL SDK environment"
        #endif
    #endif

    /* Determine global size and local size */
    clGetDeviceInfo(device_id, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(num_comp_units), &num_comp_units, NULL);
    printf("num_comp_units=%u\n", num_comp_units);

    #ifdef __APPLE__
        clGetDeviceInfo(device_id, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(local_size), &local_size, NULL);
    #endif
	
    /* local size reported Altera FPGA is incorrect */
    #ifdef AOCL
        printf("Local size is static 16 for AOCL \n");
        local_size = 16;
    #endif
	
    global_size = num_comp_units * local_size;
    printf("global_size=%lu, local_size=%lu\n", global_size, local_size);

    /* Create OpenCL context */
    context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &ret);
    errorCheck(ret, "Create Context");
    
    /* Create Command Queue */
    command_queue = clCreateCommandQueue(context, device_id, 0, &ret);
    errorCheck(ret, "Create Command Queue");
    
    #ifdef __APPLE__
        /* Load the source code containing the kernel*/
        fp = fopen(fileName, "r");
        if (!fp) 
        {
            fprintf(stderr, "Failed to load kernel.\n");
            exit(1);
        }
        
source_str = (char*)malloc(MAX_SOURCE_SIZE);
        source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
        fclose(fp);

        /* Create Kernel Program from the source */
        program = clCreateProgramWithSource(context, 1, (const char **)&source_str, (const size_t *)&source_size, &ret);
        errorCheck(ret, "Create Program With Source");
    #else

        #ifdef AOCL  /* on FPGA we need to create kernel from binary */
           /* Create Kernel Program from the binary */
           std::string binary_file = getBoardBinaryFile("mykernel", device_id);
           printf("Using AOCX: %s\n", binary_file.c_str());
           program = createProgramFromBinary(context, binary_file.c_str(), &device_id, 1);
        #else
            #error "unknown OpenCL SDK environment"
        #endif

    #endif

    /* Build Kernel Program */
    ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
    errorCheck(ret, "Build Program");

struct hash_pair { 
    template <class T1, class T2> 
    size_t operator()(const pair<T1, T2>& p) const
    { 
        auto hash1 = hash<T1>{}(p.first); 
        auto hash2 = hash<T2>{}(p.second); 
        return hash1 ^ hash2; 
    } 
}; 

int giveReward(vector<int> &state){
    if(state == WIN){
        return 1;
    }
    else if(state == LOSE){
        return -1;
    }
    else{
        return 0;
    }
}

bool isEndFunc(vector<int> &state){
    if(state == WIN || state == LOSE)
        return true;
    else return false;
}

vector<int> nxtPosition(int action,vector<int> &state){
    vector<int> nxtState; //2 spaces, initialize to 0 {0,0}
    if(true){
        switch(action){
            case 0: nxtState = {state[0]-1, state[1]}; // up
                    break;
            case 1: nxtState = {state[0]+1, state[1]}; //down
                    break;
            case 2: nxtState = {state[0], state[1]-1}; //left
                    break;
            case 3: nxtState = {state[0], state[1]+1}; //right
                    break;
            default: nxtState = {state[0]-1, state[1]};
        }
    }
    vector<int> obstacle{1,1}; //obstacle position
    if(nxtState[0] >= 0 && nxtState[0] <=2){
        if (nxtState[1] >= 0 && nxtState[1] <= 3){
            if(nxtState != obstacle){
                return nxtState;
            }
        }
    }
    return state;
}

int chooseAction(unordered_map<pair<int, int>, float, hash_pair> &state_values, vector<int> &state){
    float mx_nxt_reward = 0.0;
    float nxt_reward = 0.0;
    int action;
    float num_float = static_cast <float> (rand()) / static_cast <float> (RAND_MAX); //0.0 to 1.0
    int num_int = static_cast <float> (rand()) / static_cast <float> (RAND_MAX/4); //0 to 3
    if(num_float <= exp_rate){
        action = num_int;
    }
    else{
        //greedy
        for (int a : actions){
            vector<int> state_tmp = nxtPosition(action, state);
            pair<int, int>pair_tmp = make_pair(state_tmp[0], state_tmp[1]);
            nxt_reward = state_values[pair_tmp];
            if(nxt_reward >= mx_nxt_reward){
                action = a;
                mx_nxt_reward = nxt_reward;
            }
        }
    }
    return action;
}

void reset(vector<vector<int> > &states, vector<int> &state, bool &isEnd){
    states.clear();
    state = START;
    isEnd = false;
}

vector<int> takeAction(int action, vector<int> &state){
    vector<int> position;
    position = nxtPosition(action, state);
    state = position;
    return state;
}

float round3(float var) { 
    // use array of chars to store number as a string. 
    char str[40];  
  
    // Print in string the value of var with two decimal point 
    sprintf(str, "%.3f", var); 
  
    // scan string value in var  
    sscanf(str, "%f", &var);  
    return var;  
} 

void showValues(unordered_map<pair<int, int>, float, hash_pair> &state_values){
    float reward;
    for(int i=0;i<BOARD_ROWS;i++){
        cout<<"-----------------------------------------"<<endl;
        cout<<"| ";
        for(int j=0;j<BOARD_COLS;j++){
            pair<int, int>pair_tmp = make_pair(i,j);
            cout << std::fixed << std::showpoint;
            cout << std::setprecision(3);
            if(state_values[pair_tmp]<0) cout<<state_values[pair_tmp]<<"| ";
            else cout<<" "<<state_values[pair_tmp]<<"| ";
        }
        cout<<endl;
    }
    cout<<"-----------------------------------------"<<endl;
}


int main(int argc, char *argv[]){
    srand (static_cast <unsigned> (time(0)));
    int board[BOARD_ROWS][BOARD_COLS];
    board[1][1] = -1;
    vector<int> state(2);
    vector<vector<int> > states{};
    bool isEnd = false;
    unordered_map<pair<int, int>, float, hash_pair> state_values;

    //initialize all state values to zero
    for(int i =0; i<=BOARD_ROWS; i++ ){
        for(int j= 0; j<= BOARD_COLS; j++){
            state_values[{i,j}] = 0.0;
        } 
    }
    state = START;
    int rounds = atoi(argv[1]);

    //start playing
    play(rounds, isEnd, state, state_values, states);
    showValues(state_values);
    return 0;    
}
    clReleaseMemObject(result_buffer);
    clReleaseCommandQueue(command_queue);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseContext(context);

void errorCheck(cl_int ret, char *check)
{
  if (ret != CL_SUCCESS)
    {
      printf("ERROR: %s returned error code %d \n", check, ret);
      exit(1);
    }
  else
    {
      printf("%s SUCCESS \n", check);
    }
}

#ifdef AOCL
    // Altera OpenCL needs this callback function implemented in main.c
    // Free the resources allocated during initialization
    void cleanup() {}
#endif
