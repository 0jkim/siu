#! /bin/bash
echo "Hello World"

export 'NS_LOG=ConfiguredGrant=level_all|prefix_func|prefix_time:NrUePhy=level_all|prefix_func|prefix_time:NrUeMac=level_all|prefix_func|prefix_time:NrMacSchedulerNs3=level_all|prefix_func|prefix_time:LteRlcUm=level_all|prefix_func|prefix_time:NrGnbPhy=level_all|prefix_func|prefix_time:NrGnbMac=level_all|prefix_func|prefix_time:NrMacSchedulerOfdma=level_all|prefix_func|prefix_time'


## Initialization
declare -A sch # sch type
MAXCOUNT=1
count=1
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
 echo "$count" 
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
   echo "========Scheduling Type: $count (0 = 5G-TDMA, 1 = 5G-OFDMA, 2 = Sym-OFDMA, 3 = RB-OFDMA) and BW ${aux[$j]}============" 
   echo "${aux[$j]}"
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

