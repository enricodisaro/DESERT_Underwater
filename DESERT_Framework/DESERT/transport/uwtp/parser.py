import numpy as np

# Carica i dati dal file: 3 colonne (float, int, int)
data = np.loadtxt("data.txt", delimiter=",", dtype=float)

# Colonne separate
pdr = data[:, 0]             # prima colonna
period = data[:, 1].astype(int) # seconda colonna
cum = data[:, 2].astype(int)
cumpar = data[:, 3].astype(int) # terza colonna

print("pdr:", pdr)
print("period :", period)
print("cum : ", cum)
print("cumpar :", cumpar)

for i in range(len(pdr)):
    if cum[i] == 0:
        cumpar[i] = 1
    elif cum[i] == 2:
        cumpar[i] = 0