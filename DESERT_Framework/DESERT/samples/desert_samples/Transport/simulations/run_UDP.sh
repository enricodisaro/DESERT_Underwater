#!/bin/bash

# Parametri fissi (1..9) – sostituisci con i valori reali
duration="5001"
cbr_period="2"
rng_seed="1"

# Ciclo 100 volte (param10 = 1..100)
for i in $(seq 1 100); do
    echo "Esecuzione $i con rng=$i"
    ns test_network_UDP.tcl "$duration" "$cbr_period" "$i"
done
