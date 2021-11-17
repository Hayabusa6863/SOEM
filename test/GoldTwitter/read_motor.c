/**
 * @file    read_motor.c
 * @author  your name (you@domain.com)
 * @brief   IRL-Meijiのread_motorを参考に移したもの
 * @version 0.1
 * @date    2021-11-17
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>

#include <ethercattype.h>
#include <nicdrv.h>
#include <ethercatbase.h>
#include <ethercatmain.h>
#include <ethercatdc.h>
#include <ethercatcoe.h>
#include <ethercatfoe.h>
#include <ethercatconfig.h>
#include <ethercatprint.h>

#include "helper_macros.h"

#define EC_TIMEOUTMON   500
#define INITIAL_POS     0

// PDO data capacity
char IOmap[4096];

pthread_t thread1;
int expectedWKC;
boolean needlf;
volatile int wkc;
boolean inOP;
uint8 currentgroup = 0;

// The data sent from Master to Slave
struct Output{
    uint16 torque;
    uint16 status;
};

// The data sent from Slave to Master
struct Input{
    int32 position;
    int16 torque;
    uint16 status;
    int8 profile;
};



void simpletest(char *ifname){
    int oloop, iloop, chk;

    needlf = FALSE;
    inOP = FALSE;

    uint32 buf32;
    uint16 buf16;
    uint8   buf8;

    struct Input *val;  // スレーブからの応答値
    struct Output *target;  // スレーブへの目標値

    /* initialize SOEM, bind socket to ifname */
    if(ec_init(ifname)){
        printf("ec_init on %s succeeded.\n", ifname);

        /* network discovery */
        if(ec_config_init(FALSE) > 0){
            printf("%d slaves found and configured.\n", ec_slavecount);
            
            // Q. なぜ0から始めないのか？
            for(int i=1; i<=ec_slavecount; i++){
                // CA = Complete Access <- この意味は？
                printf("Slave %d has CA_ %s\n", i, ec_slave[i].CoEdetails & ECT_COEDET_SDOCA ? "true":"false");

                /* Complete Access disabled for Elmo driver */
                ec_slave[i].CoEdetails ^= ECT_COEDET_SDOCA;
            }

            ec_statecheck(0, EC_STATE_PRE_OP, EC_TIMEOUTSTATE);

            /* Set PDO Mapping */
            /** Operation Mode: 8 => Position profile **/
            // Q. このインデックス等はどこで確認できるのか？
            for(int i=1; i<=ec_slavecount; i++){
                WRITE(i, 0x6060, 0, buf8, 10, "OpMode");
                READ(i, 0x6061, 0, buf8, "OpMode display");

                READ(i, 0x1c12, 0, buf32, "rxPDO:0");
                READ(i, 0x1c13, 0, buf32, "txPDO:0");

                READ(i, 0x1c12, 1, buf32, "rxPDO:1");
                READ(i, 0x1c13, 1, buf32, "txPDO:1");
            }

            int32 ob2;
            int os;
            for(int i=1; i<=ec_slavecount; i++){
                /* Receive PDO Mapping (Outputs) */
                os = sizeof(ob2);
                ob2 = 0x16020001;   // Torque
                ec_SDOwrite(i, 0x1c12, 0, TRUE, os, &ob2, EC_TIMEOUTRXM);

                READ(i, 0x1c12, 0, buf32, "rxPDO:0");
                READ(i, 0x1c13, 0, buf32, "txPDO:0");

                READ(i, 0x1c12, 1, buf32, "rxPDO:1");
                READ(i, 0x1c13, 1, buf32, "txPDO:1");
            }

            /* if CA disable => automapping works */
            ec_config_map(&IOmap);

            // show slave info
            for(int i=1; i<=ec_slavecount; i++){
                printf("\nSlave:%d\n Name:%s\n Output size: %dbits\n Input size:%d\n State: %d\n Delay: %d[ns]\n Has DC: %d\n",
                    i, ec_slave[i].name, ec_slave[i].Obits, ec_slave[i].Ibits,
                    ec_slave[i].state, ec_slave[i].pdelay, ec_slave[i].hasdc
                );
            }

            /* Disable heartbeat alarm (verify communication) */
            for(int i=1; i<=ec_slavecount; i++){
                READ(i, 0x10F1, 2, buf32, "Heartbeat?");
                WRITE(i, 0x10F1, 2, buf32, 1, "Heartbeat");

                WRITE(i, 0x60c2, 1, buf8, 2, "Time period");
                WRITE(i, 0x2f75, 0, buf16, 2, "Interpolation timeout");
            }
            
            printf("Slaves mapped, state to SAFE_OP\n");

            // 制御周期？単位は[μs]
            int timestep = 500;

            /* Wait for all slaves to reach SAFE_OP state (blocking function) */
            ec_statecheck(0, EC_STATE_SAFE_OP, EC_TIMEOUTSTATE * 4);

            /** old SOEM code, inactive */
            oloop = ec_slave[0].Obytes;
            if ((oloop == 0) && (ec_slave[0].Obits > 0)) oloop = 1;
            if (oloop > 20) oloop = 8;
            iloop = ec_slave[0].Ibytes;
            if ((iloop == 0) && (ec_slave[0].Ibits > 0)) iloop = 1;
            if (iloop > 20) iloop = 8;

            printf("segments : %d : %d %d %d %d\n",ec_group[0].nsegments ,ec_group[0].IOsegment[0],ec_group[0].IOsegment[1],ec_group[0].IOsegment[2],ec_group[0].IOsegment[3]);

            printf("Request operational state for all slaves\n");
            expectedWKC = (ec_group[0].outputsWKC*2) + ec_group[0].inputsWKC;
            printf("Calculated workcounter %d\n", expectedWKC);

            /* going operational */
            ec_slave[0].state = EC_STATE_OPERATIONAL;

            /* send one valid process data to make outputs in slaves happy */
            ec_send_processdata();
            ec_receive_processdata(EC_TIMEOUTRET);

            for(int i=1; i<=ec_slavecount; i++){
                READ(i, 0x6083, 0, buf32, "Profile acceleration");
                READ(i, 0x6084, 0, buf32, "Profile deceleration");
                READ(i, 0x6085, 0, buf32, "Quick stop deceleration");
            }

            /* Request OP state for all slaves */
            ec_writestate(0);
            chk = 40;

            /* Wait for all slaves to reach OP state */
            do{
                ec_send_processdata();
                ec_receive_processdata(EC_TIMEOUTRET);
                ec_statecheck(0, EC_STATE_OPERATIONAL, 50000);
            }
            while(chk-- && (ec_slave[0].state != EC_STATE_OPERATIONAL));

            if(ec_slave[0].state == EC_STATE_OPERATIONAL){
                printf("Operational state reached for all slaves\n");
                inOP = TRUE;

                /* Drive state machine transitions (0 -> 6 -> 7 -> 15) */
                for(int i=1; i<=ec_slavecount; i++){
                    READ(i, 0x6041, 0, buf16, "*status word*");
                    if(buf16 == 0x218){
                        WRITE(i, 0x6040, 0, buf16, 128, "*control word*");  usleep(100000);
                        READ(i, 0x6041, 0, buf16, "*status word*");
                    }

                    WRITE(i, 0x6040, 0, buf16, 0, "*control word*");    usleep(100000);
                    READ(i, 0x6041, 0, buf16, "*status word*");

                    WRITE(i, 0x6040, 0, buf16, 6, "*control word*");    usleep(100000);
                    READ(i, 0x6041, 0, buf16, "*status word*");

                    WRITE(i, 0x6040, 0, buf16, 7, "*control word*");    usleep(100000);
                    READ(i, 0x6041, 0, buf16, "*status word*");

                    WRITE(i, 0x6040, 0, buf16, 15, "*control word*");    usleep(100000);
                    READ(i, 0x6041, 0, buf16, "*status word*");
                
                    CHECKERROR(i);
                    READ(i, 0x1a0b, 0, buf8, "OpMode Display");

                    READ(i, 0x1001, 0, buf8, "Error");
                }
                int reachedInitial = 0;

                /* cyclic loop for a slave */
                target = (struct Output *)(ec_slave[1].outputs);
                val = (struct Input *)(ec_slave[1].inputs);
                int i=0;
                
                for(;;){
                    /* PDO I/O Refresh */
                    ec_send_processdata();
                    wkc = ec_receive_processdata(EC_TIMEOUTRET);
                    if(wkc >= expectedWKC){
                        printf("Processdata cycle %4d, WKC %d", i, wkc);

                        /* 角度とトルクのデジタル値を表示 */
                        printf("pos: %8d, tor: %8d , stat: 0x%x, mode: 0x%x\r", val->position, val->torque, val->status, val->profile);
                        i++;

                        /* If in fault or in the way to normal status, update the state machine */
                        switch(target->status){
                            case 0:
                                target->status = 6;
                                break;
                            case 6:
                                target->status = 7;
                                break;
                            case 7:
                                target->status = 15;
                                break;
                            case 128:
                                target->status = 0;
                                break;
                            default:
                                if(val->status >> 3 & 0x01){
                                    // READ(1, 0x1001, 0, buf8, "Error");
                                    target->status = 128;
                                }
                        }

                        /* We wait to be in ready-to-run mdoe and with initial value reached */
                        if(reachedInitial == 0 && (val->status & 0x0FFF) == 0x0237){
                            reachedInitial = 1;
                        }

                        if((val->status & 0x0FFF) == 0x0237 && reachedInitial){
                            // target->torque = (in16) 80;
                        }

                        // printf("target value => tor: %5d, control: 0x%x\n\n", target->torque, target->status);
                        printf("\r");
                        needlf = TRUE;
                    }
                    usleep(timestep);
                }
                inOP = FALSE;
            }
            else{
                printf("Not all slaves reached operational state\n");
                ec_readstate();

                for(int i=1; i<=ec_slavecount; i++){
                    if(ec_slave[i].state != EC_STATE_OPERATIONAL){
                        printf("Slave %d State=0x%2.2x StatusCode=0x%4.4x : %s\n",
                            i, ec_slave[i].state, ec_slave[i].ALstatuscode, ec_ALstatuscode2string(ec_slave[i].ALstatuscode));
                    }
                }
            }

            printf("\nRequest init state for all slaves\n");
            for(int i=1; i<=ec_slavecount; i++){
                WRITE(i, 0x10F1, 2, buf32, 0, "Heartbeat");
            }

            ec_slave[0].state = EC_STATE_INIT;
            /* Request INIT state for all slaves */
            ec_writestate(0);
        }
        else{
            printf("No slaves found!\n");
        }
        printf("End simple test, close socket\n");

        /* stop SOEM, close socket */
        ec_close();
    }
    else{
        printf("No socket connection on %s\nExecute as root\n", ifname);
    }
}



void *ecatcheck(){
    int slave;

    while(1){
        if(inOP && ((wkc < expectedWKC) || ec_group[currentgroup].docheckstate)){
            if(needlf){
                needlf = FALSE;
                printf("\n");
            }

            /* One or more slaves are not responding */
            ec_group[currentgroup].docheckstate = FALSE;
            ec_readstate();
            for(slave = 1; slave <= ec_slavecount; slave++){
                if((ec_slave[slave].group == currentgroup) && (ec_slave[slave].state != EC_STATE_OPERATIONAL)){
                    ec_group[currentgroup].docheckstate = TRUE;
                    if(ec_slave[slave].state == (EC_STATE_SAFE_OP + EC_STATE_ERROR)){
                        printf("ERROR: slave %d is in SAFE_OP + ERROR, attempting ack.\n", slave);
                        ec_slave[slave].state = (EC_STATE_SAFE_OP + EC_STATE_ACK);
                        ec_writestate(slave);
                    }
                    else if(ec_slave[slave].state == EC_STATE_SAFE_OP){
                        printf("WARNING : slave %d is in SAFE_OP, change to OPERATIONAL.\n", slave);
                        ec_slave[slave].state = EC_STATE_OPERATIONAL;
                        ec_writestate(slave);
                    }
                    else if(ec_slave[slave].state > 0){
                        if(ec_reconfig_slave(slave, EC_TIMEOUTMON)){
                            ec_slave[slave].islost = FALSE;
                            printf("MESSAGE : slave %d reconfigured\n", slave);
                        }
                    }
                    else if(!ec_slave[slave].islost){
                        /* re-check state */
                        ec_statecheck(slave, EC_STATE_OPERATIONAL, EC_TIMEOUTRET);
                        if(!ec_slave[slave].state){
                            ec_slave[slave].islost = TRUE;
                            printf("ERROR : slave %d lost\n", slave);
                        }
                    }
                }

                if(ec_slave[slave].islost){
                    if(!ec_slave[slave].state){
                        if(ec_recover_slave(slave, EC_TIMEOUTMON)){
                            ec_slave[slave].islost = FALSE;
                            printf("MESSAGE : slave %d recovered\n", slave);
                        }
                    }
                    else{
                        ec_slave[slave].islost = FALSE;
                        printf("MESSAGE : slave %d found\n", slave);
                    }
                }
            }
            if(!ec_group[currentgroup].docheckstate)
                printf(".");
        }
        usleep(250);
    }
}



int main(int argc, char* argv[]){
    printf("SOEM (Simple Open EtherCAT Master)\nSimple test\n");

    if(argc > 1){
        /* Start cyclic part */
        simpletest(argv[1]);
    }else{
        printf("Usage: simple_test ifname1\nifname = eth0 for example\n");
    }
    printf("End Program\n");
    return 0;
}