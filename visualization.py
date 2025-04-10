
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns

def plot_combined_results(csv_file, output_image):
    # Load the CSV data.
    df = pd.read_csv(csv_file)
    
    # Ensure that 'Universe' is treated as a categorical variable for better styling.
    df['Universe'] = df['Universe'].astype(str)
    
    # Create a 2x2 grid of subplots.
    fig, axs = plt.subplots(2, 2, figsize=(12, 10))
    fig.suptitle('Elias--Fano vs. Golomb Delta: Performance Comparison', fontsize=16)

    # Subplot 1: Encoded Size (bits)
    sns.lineplot(data=df, x='Density', y='EncodedSizeBits', hue='Method', style='Universe', markers=True, ax=axs[0, 0])
    axs[0, 0].set_title('Encoded Size (bits)')
    axs[0, 0].set_ylabel('Encoded Size (bits)')
    axs[0, 0].grid(True)

    # Subplot 2: Encoding Time (µs)
    sns.lineplot(data=df, x='Density', y='EncodingTimeUs', hue='Method', style='Universe', markers=True, ax=axs[0, 1])
    axs[0, 1].set_title('Encoding Time (µs)')
    axs[0, 1].set_ylabel('Encoding Time (µs)')
    axs[0, 1].grid(True)

    # Subplot 3: Random Access Time (µs)
    sns.lineplot(data=df, x='Density', y='RandomAccessTimeUs', hue='Method', style='Universe', markers=True, ax=axs[1, 0])
    axs[1, 0].set_title('Random Access Time (µs)')
    axs[1, 0].set_ylabel('Access Time (µs)')
    axs[1, 0].grid(True)

    # Subplot 4: Compression Ratio (%)
    sns.lineplot(data=df, x='Density', y='CompressionRatio', hue='Method', style='Universe', markers=True, ax=axs[1, 1])
    axs[1, 1].set_title('Compression Ratio (\%)')
    axs[1, 1].set_ylabel('Compression Ratio (\%)')
    axs[1, 1].grid(True)

    # Adjust legends and layout.
    for ax in axs.flat:
        ax.legend(loc='best')
    plt.tight_layout(rect=[0, 0, 1, 0.95])
    plt.savefig(output_image)
    plt.show()

if __name__ == '__main__':
    # Adjust the filename accordingly.
    csv_filename = 'results.csv'
    output_img = 'combined_figure.png'
    plot_combined_results(csv_filename, output_img)

