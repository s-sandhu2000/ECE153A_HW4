#ifndef elevator_h
#define elevator_h

enum QHsmTstSignals {
    F1_SIG = Q_USER_SIG,
    F2_SIG,
    F3_SIG,
    F4_SIG,
    F5_SIG,
    TICK_SIG,
    PRINT_SIG,
    TERMINATE_SIG
};

extern struct QHsmTstTag HSM_QHsmTst;       /* the sole instance of QHsmTst */

extern int simTime;
extern int STOP_TIME_F;
extern int MOVE_TIME_F;
void QHsmTst_ctor(void);              /* instantiate and initialize the HSM */

/* Board Support Package */
void BSP_display(char const *msg);
void BSP_exit(void);

#endif                                                         /* elevator_h */
