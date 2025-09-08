# parser.py
import matplotlib.pyplot as plt
from collections import defaultdict

def parse_results(filename):
    # Array per ogni metrica
    pdr = []
    throughput = []
    delay = []
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
            elif key == "mean throughput":
                throughput.append(float(value))
            elif key == "mean delay":
                if float(value) > 0:
                    delay.append(float(value))
                else:
                    delay.append(0.0)
            elif key == "period":
                bitrate.append(1000/float(value))

    return pdr, throughput, delay, bitrate




def parse_udp(filename):
    # Array per ogni metrica
    pdr = []
    throughput = []
    delay = []
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
            elif key == "mean throughput":
                throughput.append(float(value)/2)
            elif key == "mean delay":
                if float(value) > 0:
                    delay.append(float(value))
                else:
                    delay.append(0.0)
            elif key == "period":
                bitrate.append(1000/float(value))

    return pdr, throughput, delay, bitrate





import numpy as np

def plot(period, throughput, delay, period2, throughput2):
    # Raggruppa dati prima simulazione
    data_t1 = defaultdict(list)
    data_d = defaultdict(list)
    for p, t, d in zip(period, throughput, delay):
        data_t1[p].append(t)
        data_d[p].append(d)

    # Raggruppa dati seconda simulazione
    data_t2 = defaultdict(list)
    for p, t in zip(period2, throughput2):
        data_t2[p].append(t)

    # Periodi unificati e ordinati
    periods_sorted = sorted(set(period) | set(period2))
    throughput_groups1 = [data_t1.get(p, []) for p in periods_sorted]
    throughput_groups2 = [data_t2.get(p, []) for p in periods_sorted]
    delay_means = [np.mean(data_d[p]) if p in data_d else np.nan for p in periods_sorted]

    fig, ax1 = plt.subplots(figsize=(12, 6))

    # Posizioni dei boxplot
    positions1 = [i - 0.2 for i in range(1, len(periods_sorted) + 1)]
    positions2 = [i + 0.2 for i in range(1, len(periods_sorted) + 1)]

    # Boxplot simulazione 1
    ax1.boxplot(
        throughput_groups1,
        positions=positions1,
        widths=0.35,
        patch_artist=True,
        boxprops=dict(facecolor="yellow"),
    )

    # Boxplot simulazione 2
    ax1.boxplot(
        throughput_groups2,
        positions=positions2,
        widths=0.35,
        patch_artist=True,
        boxprops=dict(facecolor="blue"),
    )

    ax1.set_xlabel("Period")
    ax1.set_ylabel("Throughput", color="blue")
    ax1.tick_params(axis="y", labelcolor="blue")
    ax1.set_xticks(range(1, len(periods_sorted) + 1))
    ax1.set_xticklabels([f"{p:.2f}" for p in periods_sorted])
    ax1.grid(True, axis="y", linestyle="--", alpha=0.7)

    # Asse destro per delay medio
    ax2 = ax1.twinx()
    ax2.plot(range(1, len(periods_sorted) + 1), delay_means, "-o", color="red", label="Mean Delay")
    ax2.set_ylabel("Delay", color="red")
    ax2.tick_params(axis="y", labelcolor="red")

    # Titolo e legenda
    fig.suptitle("Confronto Throughput (2 simulazioni) + Mean Delay")
    ax1.legend(["Simulazione 1", "Simulazione 2"], loc="upper left")
    ax2.legend(loc="upper right")

    plt.show()





p, t, d, br = parse_results("./network_SIM_0.out")
p1, t1, d1, br1 = parse_udp("./network_UDP_SIM0.out")


plot(br, t, d, br1, t1)

