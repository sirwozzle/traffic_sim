#include <stdio.h>

//Grid size and content params
//===================================
//size of the grid
//int length = 250;
//int height = 17;
//int length = 23;
//int height = 9;
int length = 931;
int height = 51;

//is calc'd on init
int total_cells;

//These are used to init the grid with borders as rows, as well as starting spawners and cars
int number_of_cells_to_start_cars = 0;
int cells_to_start_cars[0] = {}; 

/*
//for 23*9
int number_of_rows_to_start_barriers = 3;
int rows_to_start_barriers[3] = {0,4,8}; 

int number_of_cells_to_start_spawners = 6;
int cells_to_start_spawners[6] = {45,68,91,115,138,161}; 
*/
/*
//for 250*17
int number_of_rows_to_start_barriers = 3;
int rows_to_start_barriers[3] = {0, 8, 16};
int number_of_cells_to_start_spawners = 14;
int cells_to_start_spawners[14] = {499, 749, 999, 1249, 1499, 1749, 1999, 2250, 2500, 2750, 3000, 3250, 3500, 3750}; 
*/
int number_of_rows_to_start_barriers = 3;
int rows_to_start_barriers[3] = {0, 25, 50};
int number_of_cells_to_start_spawners = 48;
int cells_to_start_spawners[48] = {1861, 2792, 3723, 4654, 5585, 6516, 7447, 8378, 9309, 10240, 11171, 12102, 13033, 13964, 14895, 15826, 16757, 17688, 18619, 19550, 20481, 21412, 22343, 23274, 24206, 25137, 26068, 26999, 27930, 28861, 29792, 30723, 31654, 32585, 33516, 34447, 35378, 36309, 37240, 38171, 39102, 40033, 40964, 41895, 42826, 43757, 44688, 45619}; 

int cell_size = 10;//how long/tall is each cell in ft,
// to be used for speed calc and so each cell=1 car length

//===================================


//How many runs should be done, and how should be output
//===================================

//how many cycles should be simulated
int cycles_to_do = 5000;

int do_output_grid = 1;//if set to 1, alot of .dat files will be made
//this is used for making gifs

//===================================


//vehicle parameters
//===================================
int starting_speed = 65;
int target_speed = 65;//target "moving" value to get

//NOTE in about 1 second the average breaking rate would be 4mph lost in 1 second
//so lets try to keep this rate

//speed change rates
double braking_rate = 8;//breaking rate in 1mph
double accel_rate = .4;//in 1 mph
//so to calc the new movement speed at a time step,
//future_moving (speed) = moving - (accel/braking_rate * time_step_duration)

//this can be overriden in the spawner logic, so a spawner could call init_vehcile then change its direction
int default_direction = 0;//default move to the right

//uuid is for vehicles only, set to 0 for all other cells
int uuid_counter = 1;//uuids are assigned as 1, then uuid is incremented

//===================================

//world/grid params such as time for crashes, spawning etc
//other section
//===================================

//how long does each time step act as
double time_step_duration_sec = .1;//how many seconds is each time step acting as

int spawning_threshold = 75;//number 0-100
//the higher it is, less vehicles spawn, lower = more

int lane_transfer_threshold = 101;//number 0-100
//the higher it is, the less likely a vehicle will be to transfer lanes out of desire rather than necessity


double crash_duration = .04;//time in minutes that a crash lasts for

int run_counter = 0;//how many times program has been run
//===================================



//main cell object in the grid
struct Cell{
    
    int is_populated;//is the cell populated, if so, change all the other values to make it a valid vehcivle or border
    int future_is_populated;
    
    double moving;//speed 0-100 0 for stop
    double future_moving;//next time step state of moving
    
    int neighbors[8];//neighbors 
    int future_neighbors[8];
    
    int number;//index in array current
    int future_number;//future location
    
    //directions 0 1 2 3 are right, left, up, down
    int direction;//which direction to move
    int future_direction;//which direction it will be moving
    
    int speeding_value;//20 for target = target  0 for -20 under and 40 for 20 over
    int future_speeding_value;//20 for target = target  0 for -20 under and 40 for 20 over
    
    int self_target;//on init this should be set to the golbal target +/- the speeding target
    int future_self_target;//on init this should be set to the golbal target +/- the speeding target
    
    
    
    //how far has it traveled through this cell
    double percent_through_current_cell;
    double future_percent_through_current_cell;
    
    double time_until_moving_again;//if crashed, how long in minutes until earased
    
    int id;//uniquie vehchle id
    int future_id;//for when a cell moves
    
    
    //This section doesnt need a future, as these values are either non-vechile or non-moving cells
    
    int todo;//is cell processed yet

    int is_road_border;//to tell if the cell is a road boarder, if so, then set moving to -1 and not process this cell
    int is_spawn_cell;
    int spawn_direction;//the target of a spawner to spawn new vehcicle
    
    int modified_by_count;//how many other cells are looking to occupy in the future
    
};

//used to populate vehcile and spawner arrays on init
//ONLY USE ON SMALL-ish ARRAYS
int is_value_in_array(int val, int *arr, int size){
    int i;
    for (i=0; i < size; i++) {
        if (arr[i] == val)
            return 1;
    }
    return 0;
}


/* Output of the grid to file or console */
//===============================================
//only needed for gif making
int output_grid(struct Cell grid[]){
    
    
    char s[5];
    sprintf(s,"%i",run_counter);//get the run counter as string
    
    char file_name[256];
    
    //open a file called #.grid
    snprintf(file_name, sizeof file_name, "%s%s", s, ".grid");
    FILE *dot_grid = fopen(file_name, "w");
    if (dot_grid == NULL)
    {
        printf("Error opening file!\n");
        exit(1);
    }
    //and a file called #.speed
    snprintf(file_name, sizeof file_name, "%s%s", s, ".speed");
    FILE *dot_speed = fopen(file_name, "w");
    if (dot_speed == NULL)
    {
        printf("Error opening file!\n");
        exit(1);
    }
    
    //for all cells, output their status/type to the grid file and their speed to the speed file
        
    for(int i=0;i<total_cells;i++){
        int status = grid[i].is_populated + grid[i].is_road_border + grid[i].is_spawn_cell;
        if(grid[i].is_spawn_cell){
                status++;
        }
        int speed;
        if(status==1){
            speed = grid[i].moving;
        }else{
            speed=0;
        }
        //printf("status %i\n",status);
        //0 for empty, 1 for vehicle, 2 for border, 3 for spawner
        fprintf(dot_grid,"%i",status);
        fprintf(dot_speed,"%i\n",speed);
        
        //grid is a single line of ints, and speed is a bunch of speed an d\n's 
    }
    //close files
    fclose(dot_grid);
    fclose(dot_speed);
    
    return 0;
    
}


//print neighbors of cell by index
int print_neighbors(struct Cell grid[],int cell)
{   
    printf("neighbors: ");
    for(int i = 0; i < 8;i++){
        printf("%i ",grid[cell].neighbors[i]);
    }
    printf("\n");
}

//print cell info by index all info minus neighbors
int print_cell_info(struct Cell grid[],int cell)
{
    
    
 printf("direction %i \nfuture_direction %i \nfuture_id %i \nfuture_is_populated %i \nfuture_moving %f \nfuture_number %i \nfuture_percent_through_current_cell %f \nfuture_self_target %i \nfuture_speeding_value %i \nid %i \nis_populated %i \nis_road_border %i \nis_spawn_cell %i \nmodified_by_count %i \nmoving %f \nnumber %i \npercent_through_current_cell %f \nself_target %i \nspawn_direction %i \nspeeding_value %i \ntime_until_moving_again %f \ntodo %i\n",
 
    grid[cell].direction
    ,grid[cell].future_direction
    ,grid[cell].future_id
    ,grid[cell].future_is_populated
    ,grid[cell].future_moving
    ,grid[cell].future_number
    ,grid[cell].future_percent_through_current_cell
    ,grid[cell].future_self_target
    ,grid[cell].future_speeding_value
    ,grid[cell].id
    ,grid[cell].is_populated
    ,grid[cell].is_road_border
    ,grid[cell].is_spawn_cell
    ,grid[cell].modified_by_count
    ,grid[cell].moving
    ,grid[cell].number
    ,grid[cell].percent_through_current_cell
    ,grid[cell].self_target
    ,grid[cell].spawn_direction
    ,grid[cell].speeding_value
    ,grid[cell].time_until_moving_again
    ,grid[cell].todo
    );
    print_neighbors(grid,cell);
    return 0;
}

//Print the grid with the alive values shown in the cells
int print_grid(struct Cell grid[])
{
    printf("\n");
    for(int i = 0; i < total_cells;i++){
        
        if(grid[i].is_populated){
            if(grid[i].is_road_border){
                printf("[b]");
            }else if(grid[i].is_spawn_cell){
                printf("[s]");
            }else{
                printf("[v]");
            }
        }else{
            printf("[ ]");//,grid[i].is_populated);
        }
        if(((i+1)%length)==0){
            printf("\n");
        }
    }
    return 0;
}
//=============================================

//rows start at 0
//returns row numver of a cell
int find_row_from_cell_and_row_length(int cell,int length)
{
    //printf("finding row from cell\n");
    //printf("cell %i\n",cell);
    if(cell%length==0){
        cell++;
    }
    //printf("cell %i\n",cell);
    int temp = cell;
    int row = 0;
    while(1){
        temp = temp - length;
        //printf("temp %i\n",temp);
        if(temp<0){
            break;
        }
        row++;

    }
    return row;
}


//misleading title, it gets a cells neightbors and sets them
//used at init to populate all cells neighbors, so only needed once
void get_neighbors(struct Cell grid[],int cell){

    //first grab indexs that would be neighbors
    
    //set the cell to left andright of it to be neighbors
    grid[cell].neighbors[0] = cell-1;
    grid[cell].neighbors[1] = cell+1;
    //same for the next row down below cell, and left/right of that
    grid[cell].neighbors[2] = cell+1+length;
    grid[cell].neighbors[3] = cell+length;
    grid[cell].neighbors[4] = cell-1+length;
    //and same for above
    grid[cell].neighbors[5] = cell+1-length;
    grid[cell].neighbors[6] = cell-length;
    grid[cell].neighbors[7] = cell-1-length;
    

    //then trim them to only valid neighbors
    
    
    int section = 0;
    
    //get row of target cell whos neighbors we are setting
    int t_row = find_row_from_cell_and_row_length(cell,length);
    for(int i = 0; i < 8;i++){//for all neightbors
        
        //printf("\nneighbor is %i \n",grid[cell].neighbors[i]);
        
        //make sure neighbor isnt out of bound total
        if(!(grid[cell].neighbors[i]>0 && grid[cell].neighbors[i]<=total_cells)){
            grid[cell].neighbors[i] = -1;
            continue;//once is set to -1, skip rest of loop
        }
        

        int current = grid[cell].neighbors[i];
        //printf("current %i\n",current);
        //printf("i is %i\n",i);
        //printf("current section is %i\n",section);//0 for left,right 2 for above, 1 for below
        
        //then also make sure the neighbor isnt out of row
        
        //get the row info for the current neighbors looking at
        int n_row = find_row_from_cell_and_row_length(grid[cell].neighbors[i],length);
        
        //THESE VALUES ONLY NEEDED FOR DEBUG
        //and with that row (which starts at 0, so add 1) then find the max/min values in row
        //int row_upper_end = ((n_row+1)*length) -1;
        //int row_lower_end = (((n_row+1)*length)-length+1) -1;
        
        //printf("row:%i from:%i to:%i\nbase row: %i\n",n_row,row_lower_end,row_upper_end,t_row);
        
        //default behavior is to keep neighbor
        int keep = 1;//defautl to keep neighbors
        
        //depenedng on which section (above,below,l/r
        //check if neighbor is in correct row, if not, dont keep it
        if(section==0){//left right
            if(t_row!=n_row){//for neighbors that should be left and right, remove if not in same row
                keep=0;
            }
        }else if(section==1){//below
            if(t_row+1!=n_row){//for neighbors that should be below, remove if row isnt 1 greater
                keep=0;
            }
        }else{//above
            if(t_row-1!=n_row){//for neighbors that should be below, remove if row isnt 1 less
                keep=0;
            }
        }
        
        
        if(!keep){//if we dont keep neighbor
            grid[cell].neighbors[i] = -1;//set to -1 to be ignored
        }


        //increment section counter
        if(i==1){
            section++;
        }
        if(i==4){
            section++;
        }
    }
    //printf("\n");

    return;
}


//init a vehicles needed values
void init_vehicle(struct Cell grid[],int i){
    
    grid[i].is_populated = 1;//say a cell is populated, if it populated and not a border, then it must be a car
    
    grid[i].moving = starting_speed;//speed 0-100 0 for stop
    //grid[i].future_moving = grid[i].moving;//next time step state of moving
    
    //grid[i].future_number = grid[i].number+1;//future location
    grid[i].direction = default_direction;//which direction to move
    //grid[i].future_direction = default_direction;//which direction will it be moving
    
    int r = rand();
    //printf("Test Rand: %d\n", r);
    //vehicles will now set their target speed on init to be 15 less or 35 more than the speed limit
    grid[i].speeding_value = r%(40) + 1;//20 for target = target  0 for -20 under and 40 for 20 over
    grid[i].self_target = target_speed-15+grid[i].speeding_value;//on init this should be set to the golbal target +/- the speeding target
    
    
    grid[i].percent_through_current_cell = 0;
    grid[i].time_until_moving_again = 0;//if crashed, how long will it act as a barrier
    grid[i].todo = 1;
    grid[i].id = uuid_counter;
    uuid_counter++;
    
    
    //TODO make car aware of neighbors
    //populate neighbors array as array of cell numbers
}


//takes a cells current status and sets what it should be
//basically the same as init but follows rules based on prexisitng values
void set_vehicle_future(struct Cell grid[],int cell){
    //if time until moving again is >0, then its a crash, dont bother
    if(grid[cell].time_until_moving_again>0){
        return;
    }
    
    
    //tell if cell is on an edge, left or right, and based on its direction, de-populate it
    int direction_edge_modifier = grid[cell].direction;
    //at start, make it check that vehicle isnt at right most edge, if it is, set populated to 0 and be done
    int is_edge = ((cell+1-direction_edge_modifier)%length==0);
    //if not on end of row, do nromal, else kill
    if(is_edge){
        //printf("cell at edge\n");
        grid[cell].is_populated = 0;
        return;
    }else{
        
        grid[cell].future_number = grid[cell].number;
        
        
        //lane transfer logic -------------------------------
        int lane_change_val = rand()%(100) + 1;
        //movement for top lane (direction 1 movement)
        if (grid[cell].direction == 1){
            //check for obstacles in driving path
            if (grid[grid[cell].neighbors[0]].is_populated){
                //check left-hand cell and left diagonal for crash prevention
                if (!grid[grid[cell].neighbors[3]].is_populated || 
                    !grid[grid[cell].neighbors[4]].is_populated){
                        //transfer to the left
                        grid[cell].future_number = grid[cell].number + length;
                        //printf("ding_left\n");
                }
                else{
                    //check right-hand cell and right diagonal for crash prevention
                    if (!grid[grid[cell].neighbors[7]].is_populated || 
                        !grid[grid[cell].neighbors[8]].is_populated){
                            //transfer to the right
                            grid[cell].future_number = grid[cell].number - length;
                            //printf("ding_right\n");
                    }
                }
            }
            //no obstacle results in randomized, potential lane change
            else{
                //use lane change threshold to determine whether or not a transfer will occur
                if (lane_change_val > lane_transfer_threshold){
                    //check left-hand cell and left diagonal for crash prevention
                    if (!grid[grid[cell].neighbors[3]].is_populated || 
                        !grid[grid[cell].neighbors[4]].is_populated){
                            //transfer to the left
                            grid[cell].future_number = grid[cell].number + length;
                            //printf("ding_left\n");
                    }
                    else{
                        //check right-hand cell and right diagonal for crash prevention
                        if (!grid[grid[cell].neighbors[6]].is_populated || 
                            !grid[grid[cell].neighbors[7]].is_populated){
                                //transfer to the right
                                grid[cell].future_number = grid[cell].number - length;
                                //printf("ding_right\n");
                        }
                    }
                }
            }
        }
        //movement for bottom lane (direction 0 movement)
        if (grid[cell].direction == 0){
            //check for obstacles in driving path
            if (grid[grid[cell].neighbors[1]].is_populated){
                //check left-hand cell and left diagonal for crash prevention
                if (!grid[grid[cell].neighbors[2]].is_populated || 
                    !grid[grid[cell].neighbors[3]].is_populated){
                        //transfer to the left
                        grid[cell].future_number = grid[cell].number - length;
                        //printf("ding_left\n");
                }
                else{
                    //check right-hand cell and right diagonal for crash prevention
                    if (!grid[grid[cell].neighbors[5]].is_populated || 
                        !grid[grid[cell].neighbors[6]].is_populated){
                            //transfer to the right
                            grid[cell].future_number = grid[cell].number + length;
                            //printf("ding_right\n");
                    }
                }
            }
            //no obstacle results in randomized, potential lane change
            else{
                //use lane change threshold to determine whether or not a transfer will occur
                if (lane_change_val > lane_transfer_threshold){
                    //check left-hand cell and left diagonal for crash prevention
                    if (!grid[grid[cell].neighbors[2]].is_populated || 
                        !grid[grid[cell].neighbors[3]].is_populated){
                            //transfer to the left
                            grid[cell].future_number = grid[cell].number - length;
                            //printf("ding_left\n");
                    }
                    else{
                        //check right-hand cell and right diagonal for crash prevention
                        if (!grid[grid[cell].neighbors[5]].is_populated || 
                            !grid[grid[cell].neighbors[6]].is_populated){
                                //transfer to the right
                                grid[cell].future_number = grid[cell].number + length;
                                //printf("ding_right\n");
                        }
                    }
                }
            }
        }   
        //-------------------------------------------------
        
        
        
        //This is where actual future setting work goes
        grid[cell].future_direction = grid[cell].direction;
        grid[cell].future_id = grid[cell].id;
        
    
        //calculate vehicle displacement (in feet) over the time step duration
        //calculate percentage through future cell and determine the number of cells advanced during the cycle
        double distance_traveled = (time_step_duration_sec * (grid[cell].moving/3600))*5280;
        double new_percent_through_cell = (grid[cell].percent_through_current_cell*cell_size + distance_traveled)/cell_size;
        int cells_traveled = 0;
        if (new_percent_through_cell >=1){
            while (new_percent_through_cell >=1){
                new_percent_through_cell --;
                cells_traveled++;
            }
        }
        int cell_in_front;
        if(grid[cell].direction==0){
            cell_in_front = cell+1;
        }else{
            cell_in_front = cell-1;
        }
        
        //set percentage through cell and advance the vehicle accordingly
        //printf("Cells traveled %i\n",cells_traveled);
        grid[cell].future_percent_through_current_cell = new_percent_through_cell;
        grid[cell].future_number = grid[cell].future_number+cells_traveled-(2*cells_traveled*direction_edge_modifier);
        grid[cell].future_moving = grid[cell].moving; //update future moving value
        grid[cell].future_self_target = grid[cell].self_target; //update future self target value
        
        
        //THIS IS JUST PLACEHOLDER, remove for actual stuff
        //only if cell moved to next cell, change number
        //grid[cell].future_number = grid[cell].number+1-(2*direction_edge_modifier);
        
        //if 0, stay same, -1 is brake, 1 is accel
        int dir_todo = 0;
        
        //first check if cell wants to accel or brake
        
        //if wants to accel, set to 1, if brake set to 0
        if(grid[cell].moving<grid[cell].self_target){
            dir_todo = 1;
        }else if (grid[cell].moving>grid[cell].self_target+20){
            dir_todo = -1;
        }else{
            dir_todo = 0;
        }
        
        //now check if its allowed to do so
        
        //if there is space in front, it might be able to speed up
        if(!grid[cell_in_front].is_populated && dir_todo==1){
            //check if the 2 cells in front of self are open, or if cell in front is open and the nest frontmost is fast enough, then accel
            int cancle_accel = 0;
            if(grid[(cell_in_front+1-(2*grid[cell].direction))].is_populated){
                if(grid[(cell_in_front+1-(2*grid[cell].direction))].moving>10+grid[cell].moving){
                    cancle_accel=0;
                    dir_todo = 1;
                }else if(grid[(cell_in_front+1-(2*grid[cell].direction))].moving<grid[cell].moving+5){
                    cancle_accel=0;
                    dir_todo = -1;
                }else{
                    cancle_accel = 1;
                }
            }else{//if 2 cells in front is ok, check 3 in front
                if(grid[(cell_in_front+1-(2*grid[cell].direction))].is_populated){
                    if(grid[(cell_in_front+1-(2*grid[cell].direction))].moving<grid[cell].moving+10){
                        cancle_accel = 1;
                    }else if(grid[(cell_in_front+1-(2*grid[cell].direction))].moving<grid[cell].moving){
                        dir_todo = -1;
                        cancle_accel = 0;
                    }
                }else{
                    dir_todo = 1;
                }   
            }
            if(cancle_accel){//if accel is cancled then cancle
                dir_todo = 0;
            }
            
        }
        //if car in front, think about slowing down no matter what is wanted
        if(grid[cell_in_front].is_populated && grid[cell_in_front].moving<(grid[cell].moving)){
            //do decellaration
            dir_todo = -1;
        }

        //if car in front, think about slowing down no matter what is wanted
        if(grid[cell_in_front].is_populated && grid[cell_in_front].moving<(grid[cell].moving+20)){
            //do decellaration
            dir_todo = -1;
        }
        
        if(dir_todo==-1){//if decelarate
            //printf("cell %i is slowing down\n",cell);
            //print_cell_info(grid,cell);
            grid[cell].future_moving = grid[cell].moving - braking_rate;
        }else if(dir_todo==1){//if accellerate
            grid[cell].future_moving = grid[cell].moving + accel_rate;
        }else{//if stay the same
            grid[cell].future_moving = grid[cell].moving;

        }
        
        //grid[cell].future_moving = grid[cell].moving;
        //grid[cell].future_percent_through_current_cell = grid[cell].percent_through_current_cell;
        
        
    }
}

//takes a cells future and sets it to that
//also handles crashes if 2 cars enter same cell
void do_vehicle(struct Cell grid[],int cell){
    
    //printf("doing vehicle %i\n",cell);
    
    //first check that is not a crash
    if(grid[cell].is_populated && !(grid[cell].time_until_moving_again>0)){
        //determine what cell gets the future values assigened
        //printf("isnt crashed yet\n");
        int target_cell = grid[cell].number;
        if(grid[cell].future_number != grid[cell].number){//if the vehicle left its cell, set new cell to get target values
            target_cell = grid[cell].future_number;
            grid[cell].is_populated = 0;
            grid[target_cell].is_populated = 1;
        }
        
        //if the cell has already been modified by and isnt empty, then its a crash
        if(grid[target_cell].modified_by_count>=1 ){
            //printf("crash at cell number %i\n",target_cell);
            grid[target_cell].moving = 0;
            grid[target_cell].time_until_moving_again = crash_duration;
            
        }else{//if not crash, continue as normal
            //printf("didnt crash\n");
            grid[target_cell].direction = grid[cell].future_direction;
            grid[target_cell].self_target = grid[cell].future_self_target;
            grid[target_cell].speeding_value = grid[cell].future_speeding_value;
            grid[target_cell].id = grid[cell].future_id;
            grid[target_cell].moving = grid[cell].future_moving;
            grid[target_cell].percent_through_current_cell = grid[cell].future_percent_through_current_cell;
            grid[target_cell].future_number = target_cell;
            grid[target_cell].modified_by_count++;
        }
        
    }else{//if it is a crashed cell
        if(grid[cell].is_populated){//only populated non-edged cells can be crashed
            //printf("crashed cell %i\n",cell);
            grid[cell].moving = 0;
            //simulate passing of the time step duration on the crash timer
            grid[cell].time_until_moving_again = grid[cell].time_until_moving_again-(time_step_duration_sec/60);
            //printf("Time until moving again:%f\n",grid[cell].time_until_moving_again);
            //then tick down the time until moving again
            if(grid[cell].time_until_moving_again<=0){//if a crash is moving again
                grid[cell].is_populated=0;//clear the cell
            }
        }
    }
    
}

//follows spawning rules to call init_vehcile on cells
void do_spawner(struct Cell grid[],int spawner){
    
    //printf("Doing spawner %i\n",spawner);
    //print_cell_info(grid,spawner);
    
    //get count of free spaces
    int free_spaces = 0;
    for(int i =0;i<8;i++){
        int neighbor_cell = grid[spawner].neighbors[i];
        if(neighbor_cell!=-1){
            
            //printf("neighbor_cell %i\n",neighbor_cell);
            if(!grid[neighbor_cell].is_populated){
                free_spaces++;
            }
        }
    }

    //if free spaces is none, then exit this run
    if(free_spaces==0){
        return;
    }
    
    //to pick if should spawn, pick random number 1-100 and check if there is space
    int random_n = rand()%((100+1)-1) + 1;//random num b/t 1-100
    //printf("random %i\n",random_n);
    
    //NOTE the 60 is spawn threshold, so set to higher for less spawns, lesser for more
    int can_spawn = (free_spaces>=1 && random_n>spawning_threshold);//if there is at least 1 free space, and random is >threshold
    //printf("Free spaces around %i\n",free_spaces);
    //printf("Can spawn %i\n",can_spawn);
    
    
    if(can_spawn){//if allowed to spawn, spawn in 1st possible space
        
        for(int i =0;i<8;i++){
            int neighbor_cell = grid[spawner].neighbors[i];
            if(neighbor_cell!=-1){
                if(!grid[neighbor_cell].is_populated){
                    
                    /*
                    //printf("spawning vehicle at %i\n",neighbor_cell);
                    int spawner_on_edge  = ((spawner+1)%length==0) || ((spawner)%length==0);
                    printf("spawner at %i is on edge %i\n",spawner,spawner_on_edge);
                    
                    int left_edge = ((spawner)%length==0);
                    printf("spawner at %i is on left edge %i\n",spawner,left_edge);
                    
                    */
                    init_vehicle(grid,neighbor_cell);
                    grid[neighbor_cell].direction = grid[spawner].spawn_direction;
                    /*
                    printf("spawned vehcicle at %i\n",neighbor_cell);
                    if(!left_edge){//if spawner isnt on left edge, then the spawned cars direction must be fixed
                        //TODO more in depth
                        grid[neighbor_cell].direction=1;//for now, assume its only on right edge, and direction is left
                    }
                    */
                    
                    
                    break;
                }
            }
        }
        
    }
    
}



//does a whole cycle, all cars futures, spawners, set grid to future state
int do_cycle(struct Cell grid[])
{   
    
    //get 2 arrays, for shorter iteration over all vehciles, and all spawners
    
    //counters for length of arrays, and 2 temp counters
    int num_vehicles = 0;
    int num_spawners = 0;
    int v_count = 0;
    int s_count = 0;
    
    //get the counts of vehicles and spawners
    for(int i=0;i<total_cells-1;i++){
        if(grid[i].is_populated){
            if(grid[i].is_spawn_cell){
                num_spawners++;
                s_count++;
            }else if(grid[i].is_road_border==0){
                num_vehicles++;
                v_count++;
            }
        }
    }
    
    //printf("There are %i spawners and %i vehicles\n",num_spawners,num_vehicles);
    
    //make 2 arrays to hold the exact amount of vehicles and spawners to be iterated over
    int vehicles[num_vehicles];
    int spawners[num_spawners];
    
    //place each vehicle/spawner in the correct array
    for(int i=0;i<total_cells;i++){
        if(grid[i].is_populated){
            if(grid[i].is_spawn_cell){
                spawners[s_count-1] = grid[i].number;
                s_count--;
            }else if(grid[i].is_road_border==0){
                vehicles[v_count-1] = grid[i].number;
                v_count--;
            }
        }
    }
    
    /*
    //Copy paste this code to view all vehicle debug info
    printf("Printing all cell info for all vehicles\n");
    for(int i = 0;i<num_vehicles;i++){
        //printf(" i %i v[i] %i\n",i,vehicles[i]);
        print_cell_info(grid,vehicles[i]);
        printf("\n");
        
    }
    */
    
    
    //now there are 2 arrays, one of cell numbers/locations of vehciles and one for spawners
    //TODO jason and quinn, this bits yours, for each cell in vehcicles, set the future_ values
    //First for all vehicles, calc future values
    //(cell 1 has future value of 2 and such now)
        //set moving speed 1st TODO how should accel and braking work
        //then use that speed and the length of time step to calc distence traveled in that time step
    for(int i = 0;i<num_vehicles;i++){
        set_vehicle_future(grid,vehicles[i]);// <- this is where seting future stuff code goes
    }
    
    
    
    //do all spawner logic
    for(int i =0;i<num_spawners;i++){
        do_spawner(grid,spawners[i]);
    }
    
    
    
    //look at all cells future values, for all of them that have moved out of their current cells
    //then set their future cells to thier values. each cell has a modified_by counter,
    // if a cell is modified by >1 others, set it to a crash
    
    //then set all other cells to their future values if modified by >0
    
    //then reset todo on all cells and modified by to 0 for next cycle
    //this part ^^ is already done at bottom
    
    for(int i = 0;i<num_vehicles;i++){
        do_vehicle(grid,vehicles[i]);// <- this is where seting future stuff code goes
    }
    
    
    
    
    
    //set todo back to 1, as now all cells acted upon
    //and none have been modified yet by other cells
    for(int i = 0; i < total_cells;i++){
        grid[i].todo = 1;
        grid[i].modified_by_count = 0;
    }
    
    
    
    
    //output to the file, only for gif making
    //if this errors on make on mac,just comment it out
    if(do_output_grid){
        output_grid(grid);
    }
    
    
    
    run_counter++;
    
    return 0;
}


/* init section, called on grid creation */
//=============================================

//creates cell to be spawner
void init_spawner(struct Cell grid[],int i){
    grid[i].is_populated = 1;    
    grid[i].is_spawn_cell = 1;
    grid[i].spawn_direction = 0;
    
    //int spawner_on_edge  = ((spawner+1)%length==0) || ((spawner)%length==0);
    
    int left_edge = ((i)%length==0);
    
    if(!left_edge){//if spawner isnt on left edge, then the spawned cars direction must be fixed
        //TODO more in depth
        grid[i].spawn_direction=1;//for now, assume its only on right edge, and direction is left
    }
    

}

//sets cell to barrier
void init_barrier(struct Cell grid[],int i){
    grid[i].is_populated = 1;
    grid[i].is_road_border = 1;
}

//sets grid to be populated with initial spawners and barriers as defined at top
int init_grid(struct Cell grid[],int total_cells,int cells_to_start_cars[])
{
    int row_num = 0;
    int row_num_counter = 0;
    //init the grid cells to their starting state
    for(int i = 0; i < total_cells;i++){
        if (row_num_counter == length){
            row_num++;
            row_num_counter = 0;
        }
        
        //set the cells number
        grid[i].number = i;
        //set the grid to be empt witn nothing
        grid[i].id = 0;
        grid[i].is_populated = 0;
        grid[i].is_road_border = 0;
        grid[i].is_spawn_cell = 0;
        grid[i].time_until_moving_again = 0;
        
        
        //populate the nighbors of all cells
        get_neighbors(grid,i);
        

        //set all start cars
        if(is_value_in_array(i,cells_to_start_cars,number_of_cells_to_start_cars))
        {
            //if value is a car
            init_vehicle(grid,i);
            //grid[i].is_populated = 1;
            
        }
        else if(is_value_in_array(i,cells_to_start_spawners,number_of_cells_to_start_spawners))
        {
            init_spawner(grid,i);
        }
        else if(is_value_in_array(row_num,rows_to_start_barriers,number_of_rows_to_start_barriers))//TODO make way to make rows into barriers
        {
            init_barrier(grid,i);        
        }
        else
        {
            grid[i].is_populated = 0;
        }
        
        
        row_num_counter++;
    }
    return 0;
}

//=============================================

//main function that inits the grid then runs for n cycles
int main()
{
    //make rand work
    srand(time(NULL));
    
    //make  a grid based on l x w size and populate with cells
    total_cells = length*height;
    struct Cell grid[total_cells];
    printf("Creating grid of %i cells\n",total_cells);
    //n_random_cells_to_start_cars();
    init_grid(grid,total_cells,cells_to_start_cars);
    
    if(total_cells<2000){//if grid is small enough
        //Print the grid as numbers for reference
        for(int i = 0; i < total_cells;i++){
            if(i<9){
                printf("[00%i]",i);
            }else if(i<100) {
                printf("[0%i]",i);
            }else{
                printf("[%i]",i);
            }
            if(((i+1)%length)==0){
                printf("\n");
            }
        }
    
    
        print_grid(grid);
        printf("\n\n\n");
    }
    printf("Grid made, running %i cycles\n",cycles_to_do);
    while(1){
        
        //char str[20];
        //printf("\nPush Enter to cycle (ctrl+c to quit)");
        //gets(str);
        do_cycle(grid);
        //print_grid(grid);
        //printf("\n===========end===========\n");
        //print_cell_info(grid,21);
        if(cycles_to_do==0){
            break;
        }
        cycles_to_do--;

    }
    printf("Finished with all cycles\n");
    
    
    
    return 0;
}




//other 





