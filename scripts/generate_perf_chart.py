#!/usr/bin/env python3
"""
Prism Performance Chart Generator
Reads benchmark_results.csv and produces docs/media/perf_chart.png using matplotlib.
"""

import sys
import os
import csv
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np

def main():
    csv_path = "benchmark_results.csv"
    if len(sys.argv) > 1:
        csv_path = sys.argv[1]

    if not os.path.exists(csv_path):
        print(f"Error: {csv_path} not found.")
        return 1

    scenes = []
    optimizations = []
    data = {} # (scene, opt) -> fps

    with open(csv_path, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            sc = row['Scene']
            opt = row['Optimization']
            fps = float(row['AvgFPS'])
            if sc not in scenes:
                scenes.append(sc)
            if opt not in optimizations:
                optimizations.append(opt)
            data[(sc, opt)] = fps

    if not scenes or not optimizations:
        print("No valid benchmark data found.")
        return 1

    # Filter key optimizations for clean presentation
    key_opts = [
        "a) Baseline",
        "b) + Frustum Culling",
        "c) + BVH Spatial Hierarchy",
        "d) + SoA Transforms",
        "f) + Tiled (4 Threads)",
        "f) + Tiled (8 Threads)",
        "f) + Tiled (12 Threads)"
    ]
    # Keep only those present
    opts_to_plot = [o for o in key_opts if any((s, o) in data for s in scenes)]
    if not opts_to_plot:
        opts_to_plot = optimizations[:6]

    x = np.arange(len(scenes))
    width = 0.85 / len(opts_to_plot)

    fig, ax = plt.subplots(figsize=(12, 6.5), dpi=150)
    fig.patch.set_facecolor('#181824')
    ax.set_facecolor('#1f1f2e')

    colors = ['#e74c3c', '#e67e22', '#f1c40f', '#2ecc71', '#3498db', '#9b59b6', '#1abc9c']

    for i, opt in enumerate(opts_to_plot):
        fps_values = [data.get((sc, opt), 0.0) for sc in scenes]
        offset = (i - len(opts_to_plot) / 2) * width + width / 2
        rects = ax.bar(x + offset, fps_values, width, label=opt, color=colors[i % len(colors)], alpha=0.9, edgecolor='white', linewidth=0.5)

    ax.set_ylabel('Average FPS (Higher is Better)', color='white', fontsize=12, fontweight='bold')
    ax.set_title('Prism 3D Software Renderer: Optimization Progression by Scene Complexity\n(1280x720 Resolution, Blinn-Phong Shading, GCC 13 -O3, 12th Gen Intel i5-1235U)',
                 color='white', fontsize=12, pad=15)
    ax.set_xticks(x)
    ax.set_xticklabels(scenes, color='white', fontsize=11, fontweight='bold')
    ax.tick_params(colors='white')
    ax.grid(axis='y', linestyle='--', alpha=0.3, color='#888888')
    ax.set_yscale('log') # Log scale handles large dynamic range across 10k to 500k
    ax.set_ylabel('Average FPS [Log Scale]', color='white', fontsize=11)

    legend = ax.legend(facecolor='#252538', edgecolor='#444455', labelcolor='white', fontsize=9, loc='upper right')
    plt.tight_layout()

    out_dir = "docs/media"
    os.makedirs(out_dir, exist_ok=True)
    out_path = os.path.join(out_dir, "perf_chart.png")
    plt.savefig(out_path, facecolor=fig.get_facecolor(), edgecolor='none')
    print(f"Chart successfully saved to {out_path}")
    return 0

if __name__ == "__main__":
    sys.exit(main())
