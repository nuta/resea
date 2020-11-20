#!/usr/bin/env python3
import argparse
import json
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
import pandas as pd

def main():
    parser = argparse.ArgumentParser(
        description="Visualize benchmark score (reported by the METRIC macro) changes in the log.")
    parser.add_argument("--log", required=True, help="The boot log.")
    parser.add_argument("--key", required=True, help="The metric key.")
    args = parser.parse_args()

    ys = []
    for line in open(args.log).readlines():
        try:
            data = json.loads(line)
        except json.JSONDecodeError:
            continue

        if data.get("type") != "metric":
            continue
        if data.get("key") != args.key:
            continue

        ys.append(data["value"])

    data = pd.DataFrame({ "x": np.arange(len(ys)), "score": ys })
    sns.lineplot(data=data, x="x", y="score")
    plt.show()

if __name__ == "__main__":
    main()
