#include <stdio.h>
#include "qpn_port.h"
#include "elevator.h"

/* QHsmTst class -----------------------------------------------------------*/
typedef struct QHsmTstTag {
	QHsm super;             /* extend class QHsm */
                                /* extended state variables... */
	int floor_req_curr[5];  /* instantaneous request list */
	int floor_pen[5];       /* floors to stop at list */
	int curr_dir;           /* instantaneous direction of travel */
	int curr_floor;         /* location (updates on arrival event) */
	int stop_time;          /* counts time to stop on a floor */
	int move_time;          /* counts time between floors */

	int floor_curr_call_time[5]; /* measures when request is made */
	int floor_calls[5];          /* keeps track of how many requests were made to each floor */
	double floor_total_time[5];  /* keeps track of cumulative time to service each request */
    
} QHsmTst;

void   QHsmTst_ctor(void);                               /* the ctor */
static QState QHsmTst_initial(QHsmTst *me);  /* initial pseudostate-handler */
static QState QHsmTst_elevator    (QHsmTst *me);           /* state-handler */
static QState QHsmTst_stopped   (QHsmTst *me);             /* state-handler */
static QState QHsmTst_moving  (QHsmTst *me);               /* state-handler */

/* global objects ----------------------------------------------------------*/
QHsmTst HSM_QHsmTst;           /* the sole instance of the QHsmTst HSM */

int simTime=0;                 /* keeps global time */
int STOP_TIME_F = 10;          /* sets the time to stay stopped */
int MOVE_TIME_F = 5;           /* sets the time to move between floors */
void updatePending(int floor); /* updates only calls to floor i */
void updatePending_all(void);  /* updates all pending floor calls */
int checkPending(void);        /* checks if floors are pending  */
void findDirection(void);      /* determines which direction to go next */
void printData(void);	       /* prints average service time and number of calls */

/*..........................................................................*/
void QHsmTst_ctor(void) {
	int x=0;
	QHsm_ctor(&HSM_QHsmTst.super, (QStateHandler)&QHsmTst_initial);
	HSM_QHsmTst.curr_dir = 1;   /* initialize extended state variable */
	HSM_QHsmTst.curr_floor = 0;
	HSM_QHsmTst.stop_time = 0;
	HSM_QHsmTst.move_time = 0;
	for (x=0; x<5; x++){
		HSM_QHsmTst.floor_req_curr[x]=0;
		HSM_QHsmTst.floor_pen[x]=0;
		HSM_QHsmTst.floor_calls[x]=0;
		HSM_QHsmTst.floor_curr_call_time[x]=-1;
		HSM_QHsmTst.floor_total_time[x]=0;
		}
}

/*..........................................................................*/
QState QHsmTst_initial(QHsmTst *me) {
	BSP_display("Initializing Elevator Controller!\n");
	return Q_TRAN(&QHsmTst_stopped);
}
/*..........................................................................*/
QState QHsmTst_elevator(QHsmTst *me) {
	switch (Q_SIG(me)) {
		case Q_ENTRY_SIG: {
			BSP_display("elevator-ENTRY\n");
			return Q_HANDLED();
			}
		case Q_EXIT_SIG: {
			BSP_display("elevator-EXIT\n");
			return Q_HANDLED();
			}
		case Q_INIT_SIG: {
			BSP_display("elevator-INIT\n");
			return Q_TRAN(&QHsmTst_stopped);
			}
		case TICK_SIG: {
			BSP_display("elevator-TICK\n");
			return Q_TRAN(&QHsmTst_stopped);
			}
		case F1_SIG:
		case F2_SIG:
		case F3_SIG:
		case F4_SIG:
		case F5_SIG: {                   
			BSP_display("elevator-Floor_Call\n");
			return Q_TRAN(&QHsmTst_stopped);
			}
		case TERMINATE_SIG: {
			BSP_exit();
			return Q_HANDLED();
			}
		case PRINT_SIG: {
			printData();
			}
	
		return Q_HANDLED();
		}
	return Q_SUPER(&QHsm_top);
}
/*..........................................................................*/
QState QHsmTst_stopped(QHsmTst *me) {
	switch (Q_SIG(me)) {
		case Q_ENTRY_SIG: {
            		BSP_display("stopped-ENTRY\n");
	    		HSM_QHsmTst.stop_time = 0;
	    		updatePending_all(); /*Floor call requests made when the elevator was moving are updated to pending here*/
            		return Q_HANDLED();
        		}
        	case Q_EXIT_SIG: {
			BSP_display("stopped-EXIT\n");
			return Q_HANDLED();
        		}
        	case TICK_SIG: {
            		BSP_display("stopped-TICK\n");
	    
	    		if (checkPending() == -1){ /*If the current floor is pending, stop for STOP_TIME_F*/
		    		if (HSM_QHsmTst.stop_time < STOP_TIME_F-1) HSM_QHsmTst.stop_time++;
		    		else {
			    		HSM_QHsmTst.stop_time = 0;
			    		HSM_QHsmTst.floor_pen[HSM_QHsmTst.curr_floor] = 0; /*Clear that the floor is pending*/ 
			    		if (checkPending() == 1) return Q_TRAN(&QHsmTst_moving); /*If any other floor is pending, switch to the moving state in the same tick*/
			    	}
			}
	   		
			else if (checkPending() == 1) return Q_TRAN(&QHsmTst_moving); /*If any other floor becomes pending later, switch to the moving state*/
		
	   		return Q_HANDLED();
		}
        	case F1_SIG: {
            		BSP_display("stopped-F1\n");
	    		if (HSM_QHsmTst.floor_pen[0] != 1){ /*If the floor is not pending already, then record its arrival*/
				HSM_QHsmTst.floor_req_curr[0] = 1;  /*Mark that it is requested*/
				updatePending(0);                   /*Mark that it is pending*/ 
				HSM_QHsmTst.floor_curr_call_time[0] = simTime; /*Store the time when it is requested*/
				HSM_QHsmTst.floor_calls[0]++;                  /*Increment the number of calls to that floor*/
			}
            		return Q_HANDLED();
        	}
       		 case F2_SIG: {
            		BSP_display("stopped-F2\n");
	    		if (HSM_QHsmTst.floor_pen[1] != 1){
				HSM_QHsmTst.floor_req_curr[1] = 1;
				updatePending(1);
				HSM_QHsmTst.floor_curr_call_time[1] = simTime;
				HSM_QHsmTst.floor_calls[1]++;
			}
            		return Q_HANDLED();
        	}
        	case F3_SIG: {
            		BSP_display("stopped-F3\n");
	    		if (HSM_QHsmTst.floor_pen[2] != 1){
				HSM_QHsmTst.floor_req_curr[2] = 1;
				updatePending(2);
				HSM_QHsmTst.floor_curr_call_time[2] = simTime;
				HSM_QHsmTst.floor_calls[2]++;
			}
            		return Q_HANDLED();
        	}
        	case F4_SIG: {
            		BSP_display("stopped-F4\n");
	    		if (HSM_QHsmTst.floor_pen[3] != 1){
				HSM_QHsmTst.floor_req_curr[3] = 1;
				updatePending(3);
				HSM_QHsmTst.floor_curr_call_time[3] = simTime;
				HSM_QHsmTst.floor_calls[3]++;
			}
            		return Q_HANDLED();
        	}
        	case F5_SIG: {
            		BSP_display("stopped-F5\n");
	    		if (HSM_QHsmTst.floor_pen[4] != 1){
				HSM_QHsmTst.floor_req_curr[4] = 1;
				updatePending(4);
				HSM_QHsmTst.floor_curr_call_time[4] = simTime;
				HSM_QHsmTst.floor_calls[4]++;
			}
            		return Q_HANDLED();
        	}
	}
	return Q_SUPER(&QHsmTst_elevator);
}


/*..........................................................................*/
QState QHsmTst_moving(QHsmTst *me) {
	switch (Q_SIG(me)) {
		case Q_ENTRY_SIG: {
			BSP_display("moving-ENTRY\n");
			HSM_QHsmTst.move_time = 0;
			findDirection();    /*Find the direction of movement*/
            		return Q_HANDLED();
        	}
        	case Q_EXIT_SIG: {
            		BSP_display("moving-EXIT\n");
	    		HSM_QHsmTst.floor_total_time[HSM_QHsmTst.curr_floor] += (simTime - HSM_QHsmTst.floor_curr_call_time[HSM_QHsmTst.curr_floor]); //Keep cumulative sum of service times
	    		HSM_QHsmTst.floor_curr_call_time[HSM_QHsmTst.curr_floor] = -1; /*Reset the call time*/
            		return Q_HANDLED();
        	}
        	case TICK_SIG: {
            		BSP_display("moving-TICK\n");
	    		if (HSM_QHsmTst.move_time < MOVE_TIME_F-1) HSM_QHsmTst.move_time++;
	    		else {
				HSM_QHsmTst.move_time = 0;
				HSM_QHsmTst.curr_floor = HSM_QHsmTst.curr_floor + HSM_QHsmTst.curr_dir; /*Update the current floor*/
				if (checkPending() == -1) return Q_TRAN(&QHsmTst_stopped); /*Switch to stopped if the current floor is pending*/ 
				updatePending_all(); /*Updating pending floors here makes sure that only calls (to the current floor) that were made more than 1 floor away make the elevator stop*/
		 	} 
            		return Q_HANDLED();
        	}
        	case F1_SIG: {
            		BSP_display("moving-F1\n");
	    		if (HSM_QHsmTst.floor_pen[0] != 1 && HSM_QHsmTst.floor_req_curr[0] != 1){ /*If the floor is not already pending or requested, then record its arrival*/
				HSM_QHsmTst.floor_req_curr[0] = 1; /*Mark that it is requested*/
				HSM_QHsmTst.floor_curr_call_time[0] = simTime; /*Store the time when it is requested*/
				HSM_QHsmTst.floor_calls[0]++;                  /*Increment the number of calls to that floor*/
			}
            		return Q_HANDLED();
        	}
        	case F2_SIG: {
            		BSP_display("moving-F2\n");
	    		if (HSM_QHsmTst.floor_pen[1] != 1 && HSM_QHsmTst.floor_req_curr[1] != 1){
				HSM_QHsmTst.floor_req_curr[1] = 1;
				HSM_QHsmTst.floor_curr_call_time[1] = simTime;
				HSM_QHsmTst.floor_calls[1]++;
			}
            		return Q_HANDLED();
        	}
        	case F3_SIG: {
            		BSP_display("moving-F3\n");
	    		if (HSM_QHsmTst.floor_pen[2] != 1 && HSM_QHsmTst.floor_req_curr[2] != 1){
				HSM_QHsmTst.floor_req_curr[2] = 1;
				HSM_QHsmTst.floor_curr_call_time[2] = simTime;
				HSM_QHsmTst.floor_calls[2]++;
			}
            		return Q_HANDLED();
        	}
        	case F4_SIG: {
            		BSP_display("moving-F4\n");
	    		if (HSM_QHsmTst.floor_pen[3] != 1 && HSM_QHsmTst.floor_req_curr[3] != 1){
				HSM_QHsmTst.floor_req_curr[3] = 1;
				HSM_QHsmTst.floor_curr_call_time[3] = simTime;
				HSM_QHsmTst.floor_calls[3]++;
			}
            		return Q_HANDLED();
        	}
        	case F5_SIG: {
            		BSP_display("moving-F5\n");
	    		if (HSM_QHsmTst.floor_pen[4] != 1 && HSM_QHsmTst.floor_req_curr[4] != 1){
				HSM_QHsmTst.floor_req_curr[4] = 1;
				HSM_QHsmTst.floor_curr_call_time[4] = simTime;
				HSM_QHsmTst.floor_calls[4]++;
			}
            		return Q_HANDLED();
        	}
	}
	return Q_SUPER(&QHsmTst_elevator);
}


/*Updates the pending list and clears the request list for a floor*/
void updatePending(int floor) { 
	if (HSM_QHsmTst.floor_req_curr[floor] == 1){
		HSM_QHsmTst.floor_pen[floor] = 1;
		HSM_QHsmTst.floor_req_curr[floor] = 0;
	}
	return;
}

/*Updates the pending list and clears the request list for all floors*/
void updatePending_all(void) {
	int x;
	for (x=0; x<5; x++) updatePending(x);
	return;
}

/*Checks if there are any pending floors. Returns -1 if the current floor is pending, 1 if any other floor is pending and 0 if no floor is pending*/ 
int checkPending(void) {
	int x;
	if (HSM_QHsmTst.floor_pen[HSM_QHsmTst.curr_floor] == 1) return (-1);
	for (x=0; x<5; x++){
		if (HSM_QHsmTst.floor_pen[x] == 1 && x != HSM_QHsmTst.curr_floor) return 1;
	}
	return 0;

}

/*Finds direction of movement based on the current floor, current direction and list of pending floors
curr_dir = 1 => upwards and curr_dir = -1 => downwards */
void findDirection(void){
	int x;
	/*If the elevator is moving upwards, first check if any floors in the upward direction are pending
	  Then check if any floors in the downward direction are pending*/ 	
	if (HSM_QHsmTst.curr_dir == 1){
		for (x=HSM_QHsmTst.curr_floor+1; x<5; x++){
			if (HSM_QHsmTst.floor_pen[x] == 1) {
				HSM_QHsmTst.curr_dir = 1;
				return;
			}
		}	
		for (x=HSM_QHsmTst.curr_floor-1; x>=0; x--){
			if (HSM_QHsmTst.floor_pen[x] == 1) {
				HSM_QHsmTst.curr_dir = -1;
				return;
			}
		}	
	}

	/*If the elevator is moving downwards, first check if any floors in the downward direction are pending
	  Then check if any floors in the upward direction are pending */
	if (HSM_QHsmTst.curr_dir == -1){
		for (x=HSM_QHsmTst.curr_floor-1; x>=0; x--){
			if (HSM_QHsmTst.floor_pen[x] == 1) {
				HSM_QHsmTst.curr_dir = -1;
				return;
			}
		}	
		for (x=HSM_QHsmTst.curr_floor+1; x<5; x++){
			if (HSM_QHsmTst.floor_pen[x] == 1) {
				HSM_QHsmTst.curr_dir = 1;
				return;
			}
		}	
	}
	return;
}

/*Prints statistics*/
void printData(void){
	int x=0;
	for (x=0; x<5; x++){
		printf("F%d calls: %d\n",x+1, HSM_QHsmTst.floor_calls[x]);
		printf("F%d average time: %f\n",x+1, HSM_QHsmTst.floor_total_time[x]/HSM_QHsmTst.floor_calls[x]);
		printf("\n");
	}
	return;
}
