#!/bin/bash

# Parametri fissi (1..9) – sostituisci con i valori reali
cumulative="1"
duration="5001"
cbr_period="5"
cum_ACK_param="100"
NACK_rtx_max="6"
NACK_rtx_period="8"
send_buffer="100"
receive_buffer="100"
rng_seed="1"


params=(2 3 4 5 6 8 10 12.5 15 17.5 20 25 30 40 50)  # valori diversi di param10



# Ciclo 100 volte (param10 = 1..100)
for rtx_period in "${params[@]}"; do
    for i in $(seq 1 100); do
        echo "Esecuzione $i con rng=$i, rtx_time=$j"
        ns test_network.tcl "$cumulative" "$duration" "$cbr_period" "$cum_ACK_param" "$NACK_rtx_max" "$rtx_period" "$send_buffer" "$receive_buffer" "$i"
    done
done