# parser.py
import matplotlib.pyplot as plt
from collections import defaultdict
import numpy as np

def parse_results(filename):
    # Array per ogni metrica
    pdr = []
    rtx = []
    delay = []


    with open(filename, "r") as f:
        content = f.read().strip()

    # Ogni simulazione è separata da due newline
    blocks = content.split("\n\n")

    for block in blocks:
        lines = block.strip().split("\n")
        if not lines or lines[0] != "UWTP":
            continue  # ignora blocchi strani

        for line in lines[1:]:  # salta UWTP
            key, value = line.split(":", 1)
            key = key.strip()
            value = value.strip()

            if key == "PDR":
                pdr.append(float(value))
            elif key == "mean delay":
                if float(value) > 0:
                    delay.append(float(value))
                else:
                    delay.append(0.0)
            elif key == "nack_max_rtx":
                rtx.append(int(value))

    return pdr, delay, rtx



def parse_udp(filename):
    # Array per ogni metrica
    pdr = []
    bitrate = []

    with open(filename, "r") as f:
        content = f.read().strip()

    # Ogni simulazione è separata da due newline
    blocks = content.split("\n\n")

    for block in blocks:
        lines = block.strip().split("\n")
        if not lines or lines[0] != "UWTP":
            continue  # ignora blocchi strani

        for line in lines[1:]:  # salta UWTP
            key, value = line.split(":", 1)
            key = key.strip()
            value = value.strip()

            if key == "PDR":
                pdr.append(float(value))
            elif key == "period":
                bitrate.append(float(value))

    values = []
    for i in range(len(pdr)):
        if bitrate[i] == 4:
            values.append(pdr[i])
    



    return np.mean(values)






def aplot(rtx, pdr, delay):
    # Raggruppa i dati per periodo
    data_t = defaultdict(list)
    data_d = defaultdict(list)
    for p, t, d in zip(rtx, pdr, delay):
        data_t[p].append(t)
        data_d[p].append(d)

    # Ordina i periodi
    periods_sorted = sorted(set(rtx))
    throughput_groups = [data_t[p] for p in periods_sorted]
    delay_means = [np.mean(data_d[p]) for p in periods_sorted]

    fig, ax1 = plt.subplots(figsize=(10, 6))

    # Boxplot del throughput
    bp = ax1.boxplot(
        throughput_groups,
        labels=[f"{p:.2f}" for p in periods_sorted],
        patch_artist=True,
        boxprops=dict(facecolor="skyblue"),
    )
    ax1.set_xlabel("Period")
    ax1.set_ylabel("Throughput", color="blue")
    ax1.tick_params(axis="y", labelcolor="blue")
    ax1.grid(True, axis="y", linestyle="--", alpha=0.7)

    # Secondo asse Y per il delay
    ax2 = ax1.twinx()
    ax2.plot(range(1, len(periods_sorted) + 1), delay_means, "-o", color="red", label="Mean Delay")
    ax2.set_ylabel("Delay", color="red")
    ax2.tick_params(axis="y", labelcolor="red")

    # Titolo e legenda
    fig.suptitle("Throughput (boxplot) e Delay (media) vs Period")
    ax2.legend(loc="upper left")

    plt.show()






p, d, rtx = parse_results("./network_SIM_1.out")
#p1, d1, rtx1 = parse_results("./network_UDP_SIM0.out")


aplot(rtx, p, d)

