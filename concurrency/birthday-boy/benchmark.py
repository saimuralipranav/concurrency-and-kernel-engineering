#!/usr/bin/env python3
import subprocess
import time
import random
import os
import math

def generate_connected_graph(V, E):
    """Generates a random connected graph with V vertices and E edges."""
    if E < V - 1:
        E = V - 1
    max_edges = V * (V - 1) // 2
    if E > max_edges:
        E = max_edges

    edges = set()
    # Spanning tree to ensure connectedness
    nodes = list(range(V))
    random.shuffle(nodes)
    for i in range(1, V):
        u = nodes[i]
        v = nodes[random.randint(0, i - 1)]
        if u > v:
            u, v = v, u
        edges.add((u, v))

    # Add remaining random edges
    while len(edges) < E:
        u = random.randint(0, V - 1)
        v = random.randint(0, V - 1)
        if u != v:
            if u > v:
                u, v = v, u
            edges.add((u, v))

    edge_list = list(edges)
    return edge_list

def run_bellman_ford(binary_path, V, E, N, T, edge_list):
    """Runs the bellman_ford binary and measures execution time in milliseconds."""
    lines = [f"{V} {E} {N} {T}"]
    for u, v in edge_list:
        lines.append(f"{u} {v}")
    input_data = "\n".join(lines) + "\n"

    start_time = time.perf_counter()
    res = subprocess.run([binary_path], input=input_data, text=True,
                         stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    end_time = time.perf_counter()

    if res.returncode != 0:
        raise RuntimeError(f"bellman_ford failed: {res.stderr}")

    duration_ms = (end_time - start_time) * 1000.0
    return duration_ms

def generate_svg_plot(title, x_label, y_label, series_list, filename, width=700, height=450):
    """Generates a clean SVG plot without external dependencies."""
    margin_l = 80
    margin_r = 40
    margin_t = 60
    margin_b = 60

    plot_w = width - margin_l - margin_r
    plot_h = height - margin_t - margin_b

    all_x = []
    all_y = []
    for s in series_list:
        for x, y in s['data']:
            all_x.append(x)
            all_y.append(y)

    min_x = min(all_x) if all_x else 0
    max_x = max(all_x) if all_x else 1
    min_y = 0
    max_y = max(all_y) * 1.15 if all_y else 1

    if max_x == min_x:
        max_x += 1
    if max_y == min_y:
        max_y += 1

    def to_screen(x, y):
        sx = margin_l + (x - min_x) / (max_x - min_x) * plot_w
        sy = margin_t + (1 - (y - min_y) / (max_y - min_y)) * plot_h
        return sx, sy

    svg = []
    svg.append(f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" style="background:#ffffff; font-family: -apple-system, BlinkMacSystemFont, Segoe UI, Roboto, Helvetica, Arial, sans-serif;">')

    # Title
    svg.append(f'<text x="{width/2}" y="32" text-anchor="middle" font-size="18" font-weight="bold" fill="#1f2937">{title}</text>')

    # Grid & Axes
    num_ticks_y = 6
    for i in range(num_ticks_y + 1):
        y_val = min_y + i * (max_y - min_y) / num_ticks_y
        _, sy = to_screen(min_x, y_val)
        svg.append(f'<line x1="{margin_l}" y1="{sy}" x2="{width - margin_r}" y2="{sy}" stroke="#e5e7eb" stroke-width="1" />')
        svg.append(f'<text x="{margin_l - 10}" y="{sy + 4}" text-anchor="end" font-size="11" fill="#6b7280">{y_val:.2f}</text>')

    num_ticks_x = 6
    for i in range(num_ticks_x + 1):
        x_val = min_x + i * (max_x - min_x) / num_ticks_x
        sx, _ = to_screen(x_val, min_y)
        svg.append(f'<line x1="{sx}" y1="{margin_t}" x2="{sx}" y2="{height - margin_b}" stroke="#e5e7eb" stroke-width="1" />')
        svg.append(f'<text x="{sx}" y="{height - margin_b + 20}" text-anchor="middle" font-size="11" fill="#6b7280">{int(x_val) if x_val.is_integer() else f"{x_val:.1f}"}</text>')

    # Axis labels
    svg.append(f'<text x="{margin_l + plot_w/2}" y="{height - 15}" text-anchor="middle" font-size="13" font-weight="600" fill="#374151">{x_label}</text>')
    svg.append(f'<text x="25" y="{margin_t + plot_h/2}" text-anchor="middle" font-size="13" font-weight="600" fill="#374151" transform="rotate(-90, 25, {margin_t + plot_h/2})">{y_label}</text>')

    # Series
    colors = ['#2563eb', '#dc2626', '#059669', '#d97706']
    legend_y = margin_t + 15
    for s_idx, s in enumerate(series_list):
        col = s.get('color', colors[s_idx % len(colors)])
        pts = s['data']
        if len(pts) >= 2:
            path_d = []
            for i, (x, y) in enumerate(pts):
                sx, sy = to_screen(x, y)
                cmd = 'M' if i == 0 else 'L'
                path_d.append(f'{cmd} {sx:.2f} {sy:.2f}')
            dash = ' stroke-dasharray="6,4"' if s.get('dashed') else ''
            svg.append(f'<path d="{" ".join(path_d)}" fill="none" stroke="{col}" stroke-width="2.5"{dash} />')

        # Draw markers
        if not s.get('dashed'):
            for x, y in pts:
                sx, sy = to_screen(x, y)
                svg.append(f'<circle cx="{sx:.2f}" cy="{sy:.2f}" r="4" fill="{col}" stroke="#ffffff" stroke-width="1.5" />')

        # Legend entry
        lx = width - margin_r - 180
        ly = legend_y + s_idx * 20
        dash = ' stroke-dasharray="4,2"' if s.get('dashed') else ''
        svg.append(f'<line x1="{lx}" y1="{ly}" x2="{lx + 20}" y2="{ly}" stroke="{col}" stroke-width="2.5"{dash} />')
        svg.append(f'<text x="{lx + 28}" y="{ly + 4}" font-size="11" fill="#374151">{s["name"]}</text>')

    svg.append('</svg>')
    with open(filename, 'w') as f:
        f.write("\n".join(svg))
    print(f"Generated plot: {filename}")

def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    binary_path = os.path.join(script_dir, "bellman_ford")

    if not os.path.exists(binary_path):
        print("Compiling bellman_ford...")
        subprocess.run(["make", "-C", script_dir], check=True)

    print("=== Running Experiment 1: Runtime vs V (Fixed Cores T=4) ===")
    V_values = [100, 200, 400, 600, 800, 1000]
    fixed_T = 4
    num_runs = 5

    exp1_results = []
    for V in V_values:
        E = 4 * V
        times = []
        for _ in range(num_runs):
            edges = generate_connected_graph(V, E)
            t = run_bellman_ford(binary_path, V, E, 0, fixed_T, edges)
            times.append(t)
        avg_time = sum(times) / len(times)
        exp1_results.append((V, E, avg_time))
        print(f"V={V:4d}, E={E:5d}, Avg Runtime = {avg_time:.3f} ms")

    # Fit k * V * E to experimental values
    # Minimize sum( (avg_time - k * V * E)^2 )
    # k = sum(avg_time * V * E) / sum((V * E)^2)
    sum_num = sum(t * (V * E) for V, E, t in exp1_results)
    sum_den = sum((V * E) ** 2 for V, E, _ in exp1_results)
    k_val = sum_num / sum_den if sum_den > 0 else 0
    print(f"\nFitted Constant k = {k_val:.6e} ms / (V*E)")

    exp_data = [(V, t) for V, _, t in exp1_results]
    theo_data = [(V, k_val * V * (4 * V)) for V, _, _ in exp1_results]

    plot1_path = os.path.join(script_dir, "plot_runtime_vs_v.svg")
    generate_svg_plot(
        "Bellman-Ford Runtime vs Vertices (V) [Cores = 4]",
        "Number of Vertices (V)",
        "Runtime (ms)",
        [
            {"name": "Experimental Runtime", "data": exp_data, "color": "#2563eb"},
            {"name": f"Theoretical O(VE) (k={k_val:.2e})", "data": theo_data, "color": "#dc2626", "dashed": True}
        ],
        plot1_path
    )

    print("\n=== Running Experiment 2: Runtime vs Threads (Fixed V=500, E=2000) ===")
    fixed_V = 500
    fixed_E = 2000
    T_values = [1, 2, 4, 8, 16]

    exp2_results = []
    for T in T_values:
        times = []
        for _ in range(num_runs):
            edges = generate_connected_graph(fixed_V, fixed_E)
            t = run_bellman_ford(binary_path, fixed_V, fixed_E, 0, T, edges)
            times.append(t)
        avg_time = sum(times) / len(times)
        exp2_results.append((T, avg_time))
        print(f"Threads T={T:2d}, Avg Runtime = {avg_time:.3f} ms")

    plot2_path = os.path.join(script_dir, "plot_runtime_vs_threads.svg")
    generate_svg_plot(
        "Bellman-Ford Runtime vs Thread Count (V=500, E=2000)",
        "Thread Count (T)",
        "Runtime (ms)",
        [
            {"name": "Experimental Runtime", "data": exp2_results, "color": "#059669"}
        ],
        plot2_path
    )

    print("\nBenchmark completed successfully! Plots generated.")

if __name__ == "__main__":
    main()
