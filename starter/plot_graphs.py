import matplotlib.pyplot as plt
import numpy as np

# Set global style
plt.style.use('ggplot')

# Function to save plots
def save_plot(filename):
    plt.tight_layout()
    plt.savefig(filename, format='pdf', bbox_inches='tight')
    plt.close()

# 1. Impact of Graph Density
density_labels = ['8', '32', '64']
pointer_density = [9.13, 91.62, 161.97]
csr_density = [3.77, 12.61, 28.27]

x = np.arange(len(density_labels))
width = 0.35

fig, ax = plt.subplots(figsize=(6, 4))
rects1 = ax.bar(x - width/2, pointer_density, width, label='Pointer', color='#E24A33')
rects2 = ax.bar(x + width/2, csr_density, width, label='CSR', color='#348ABD')

ax.set_ylabel('Execution Time (ms)')
ax.set_xlabel('Average Degree')
ax.set_title('Impact of Graph Density on Execution Time (N=50k)')
ax.set_xticks(x)
ax.set_xticklabels(density_labels)
ax.legend()
save_plot('density_impact.pdf')

# 2. Cache Misses Comparison
labels = ['L1 Data (D1) Misses', 'Last-Level (LLd) Misses']
pointer_misses = [68.08, 16.92]
csr_misses = [51.21, 0.75]

x = np.arange(len(labels))

fig, ax = plt.subplots(figsize=(6, 4))
rects1 = ax.bar(x - width/2, pointer_misses, width, label='Pointer', color='#E24A33')
rects2 = ax.bar(x + width/2, csr_misses, width, label='CSR', color='#348ABD')

ax.set_ylabel('Misses (Millions)')
ax.set_title('Cache Misses for 50k Graph (100 Repeats)')
ax.set_xticks(x)
ax.set_xticklabels(labels)
ax.legend()
save_plot('cache_misses.pdf')

# 3. Impact of Cache Line Size
line_sizes = ['32 Bytes', '64 Bytes', '128 Bytes']
pointer_d1 = [86.24, 68.08, 57.54]
csr_d1 = [56.08, 51.21, 48.78]

fig, ax = plt.subplots(figsize=(6, 4))
ax.plot(line_sizes, pointer_d1, marker='o', label='Pointer D1 Misses', color='#E24A33', linewidth=2)
ax.plot(line_sizes, csr_d1, marker='o', label='CSR D1 Misses', color='#348ABD', linewidth=2)

ax.set_ylabel('L1 Data Misses (Millions)')
ax.set_xlabel('Cache Line Size')
ax.set_title('Impact of Cache Line Size on L1 Data Misses')
ax.legend()
save_plot('line_size_impact.pdf')

print("Graphs generated successfully.")
