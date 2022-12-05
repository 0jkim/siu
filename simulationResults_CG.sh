#! /bin/bash
echo "Hello World"
# This document can be used to simulate the four types of schedulers implemented in [1] based on configured grant or dynamic scheduling. In the case of OFDMA access mode, two new techniques have been implemented in addition to the existing 5G-LENA OFDMA. These two techniques are Sym-OFDMA and RB-OFMDA [1].
# [1] A. Larrañaga et al. "An open-source implmentation and validation of 5G NR Configured-Grant for URLLC in ns-3 5G-LENA: a scheduling case study in Industry 4.0 scenario", Pre-print SSRN Electonics


export 'NS_LOG=ConfiguredGrant=level_all|prefix_func|prefix_time:NrUePhy=level_all|prefix_func|prefix_time:NrUeMac=level_all|prefix_func|prefix_time:NrMacSchedulerNs3=level_all|prefix_func|prefix_time:LteRlcUm=level_all|prefix_func|prefix_time:NrGnbPhy=level_all|prefix_func|prefix_time:NrGnbMac=level_all|prefix_func|prefix_time:NrMacSchedulerOfdma=level_all|prefix_func|prefix_time'


## Initialization
declare -A sch # sch type 0 = 5G-TDMA, 1 = 5G-OFDMA, 2 = Sym-OFDMA, 3 = RB-OFDMA
MAXCOUNT=3
count=0
declare -A aux 
count_aux=1

# Data for Figure 7-8-9 # BW
count_value=10000000 #Hz -> 10e6
MAX_value=40000000
MIN_value=10000000

# Figure 10 # Packet
#count_value=10
#MAX_value=10
#MIN_value=15



mkdir RESULTS
cd RESULTS
mkdir BW
cd ..

while [ "$count" -le $MAXCOUNT ];
do
 echo " Selected scheduling Type: $count" 
 sch[$count]=$count

 let "count_aux = 1"
 while [ "$count_value" -le $MAX_value ];
 do
   aux[$count_aux]=$count_value
   echo "${aux[$count_aux]}"
   let "count_value += MIN_value"
   let "count_aux += 1"
 done

 cd RESULTS/BW/
 mkdir ${sch[$count]}
 cd ../..
 
 for j in "${!aux[@]}";
 do
   echo "========Scheduling Types: $count (0 = 5G-TDMA, 1 = 5G-OFDMA, 2 = Sym-OFDMA, 3 = RB-OFDMA) and BW ${aux[$j]} [Hz] ============" 
   echo "BW = ${aux[$j]} [Hz]"
   ./ns3 run "scratch/ConfiguredGrant_firstTest --bandwidthBand1="${aux[$j]}" --scheduler="${sch[$count]}"" > r_aux_"${aux[$j]}"_sch_"${sch[$count]}".out 2>&1 
   #./ns3 run "scratch/ConfiguredGrant_firstTest --packetSize="${aux[$j]}" --scheduler="${sch[$count]}"" > r_aux_"${aux[$j]}"_sch_"${sch[$count]}".out 2>&1 

   cd RESULTS/BW/${sch[$count]}
   mkdir ${aux[$j]}
   cd ../../..
   cp Scenario.txt RESULTS/BW/${sch[$count]}/${aux[$j]}/
   cp r_aux_${aux[$j]}_sch_${sch[$count]}.out RESULTS/BW/${sch[$count]}/${aux[$j]}/
   rm Scenario.txt
   rm r_aux_${aux[$j]}_sch_${sch[$count]}.out 
   
 done
 let "count += 1"

done

echo "${ueNum[*]}"

# You can access them using echo "${arr[0]}", "${arr[1]}" also

