__kernel void play(int rounds, bool &isEnd, vector<int> &state, unordered_map<pair<int, int>, float, hash_pair> &state_values, vector<vector<int> > &states){
    //unordered_map<vector<int>, float> state_values;
   __private  int i = 0;
   __private float reward = 0.0;
   __private  int action;
    while(i < rounds){
        if(isEnd){
            reward = giveReward(state);      //only has 1, -1, 0
            cout<<reward<<endl;
            for(auto it : state_values){
                pair<int, int>pair_tmp = make_pair(state[0], state[1]);
                if(it.first==pair_tmp){
                    state_values[it.first] = reward;
                }
            }
            //cout<<"GAME END REWARD "<<round3(reward)<<endl;
            vector<int> path(2,0);
            reverse(states.begin(), states.end());
            for(auto s : states){
                pair<int, int>pair_tmp = make_pair(s[0], s[1]);
                reward = state_values[pair_tmp] + lr *(reward - state_values[pair_tmp]);
                state_values[pair_tmp] = round3(reward);
            }
            reset(states, state, isEnd);
            i+=1;
        }
        else{
            action = chooseAction(state_values, state);
            states.push_back(nxtPosition(action, state));
            state = takeAction(action,state);

            /* printf("action : %d, state : %d, %d",action,state[0],state[1]);*/

            isEnd = isEndFunc(state);
        }
    }

}
