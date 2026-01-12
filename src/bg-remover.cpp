#include <opencv2/opencv.hpp>
#include <opencv2/ximgproc.hpp>
#include <iostream>
#include <string>

using namespace cv;
using namespace std;

// Processing options structure
struct ProcessingOptions {
    string quality = "balanced";
    int iterations = 8;
    int margin = -1;  // -1 = auto
    string edgeMode = "guided";
    bool verbose = false;
    double kernelScale = 1.0;
};

// Apply quality preset to options
void applyPreset(ProcessingOptions& opts) {
    if (opts.quality == "fast") {
        opts.iterations = 5;
        opts.edgeMode = "blur";
        opts.kernelScale = 0.5;
    } else if (opts.quality == "quality") {
        opts.iterations = 12;
        opts.edgeMode = "guided";
        opts.kernelScale = 1.5;
    }
    // balanced is default (iterations=8, edgeMode="guided", kernelScale=1.0)
}

void removeBackground(const string& inputPath, const string& outputPath, const ProcessingOptions& opts) {
    // Read input image
    Mat image = imread(inputPath, IMREAD_COLOR);
    
    if (image.empty()) {
        cerr << "Error: Could not open or find the image: " << inputPath << endl;
        exit(1);
    }

    if (opts.verbose) {
        cout << "Image loaded: " << image.cols << "x" << image.rows << endl;
        cout << "Processing options:" << endl;
        cout << "  Quality: " << opts.quality << endl;
        cout << "  Iterations: " << opts.iterations << endl;
        cout << "  Edge mode: " << opts.edgeMode << endl;
        cout << "  Kernel scale: " << opts.kernelScale << endl;
    }
    
    // Create mask for background removal
    Mat mask = Mat::zeros(image.size(), CV_8UC1);
    
    // Use GrabCut algorithm for background removal
    int inset_x, inset_y;
    if (opts.margin >= 0) {
        // Use user-specified margin
        inset_x = opts.margin;
        inset_y = opts.margin;
    } else {
        // Proportional inset: 2% of dimensions or minimum 5px
        inset_x = max(5, image.cols / 50);
        inset_y = max(5, image.rows / 50);
    }
    Rect rectangle(inset_x, inset_y, image.cols - 2*inset_x, image.rows - 2*inset_y);
    Mat bgModel, fgModel;

    if (opts.verbose) {
        cout << "Processing image with GrabCut algorithm..." << endl;
    }

    // Run GrabCut
    grabCut(image, mask, rectangle, bgModel, fgModel, opts.iterations, GC_INIT_WITH_RECT);
    
    // Create binary mask (0 = background, 255 = foreground)
    Mat mask2 = (mask == GC_FGD) | (mask == GC_PR_FGD);
    mask2.convertTo(mask2, CV_8UC1, 255);
    
    // Apply morphological operations to clean up mask
    // Calculate kernel size based on image dimensions and scale factor
    int base_dim = min(image.cols, image.rows);
    int kernel_size = max(3, min(15, static_cast<int>((base_dim / 150) * opts.kernelScale)));
    if (kernel_size % 2 == 0) kernel_size++;  // Must be odd

    Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(kernel_size, kernel_size));
    morphologyEx(mask2, mask2, MORPH_CLOSE, kernel);
    morphologyEx(mask2, mask2, MORPH_OPEN, kernel);
    
    // Apply edge refinement based on selected mode
    if (opts.edgeMode == "guided") {
        // Edge-preserving guided filter for best boundary quality
        Mat mask_float, image_gray, image_gray_float;
        mask2.convertTo(mask_float, CV_32F, 1.0/255.0);

        cvtColor(image, image_gray, COLOR_BGR2GRAY);
        image_gray.convertTo(image_gray_float, CV_32F, 1.0/255.0);

        int guide_radius = max(4, kernel_size);
        double eps = 0.01;

        Mat refined;
        ximgproc::guidedFilter(image_gray_float, mask_float, refined, guide_radius, eps);
        refined.convertTo(mask2, CV_8UC1, 255.0);
    } else if (opts.edgeMode == "bilateral") {
        // Bilateral filter for edge-preserving smoothing
        Mat mask_float;
        mask2.convertTo(mask_float, CV_32F);
        Mat filtered;
        bilateralFilter(mask_float, filtered, 9, 75, 75);
        filtered.convertTo(mask2, CV_8UC1);
    } else {
        // Simple Gaussian blur (fast mode)
        int blur_size = max(5, kernel_size * 2 + 1);
        if (blur_size % 2 == 0) blur_size++;
        double sigma = blur_size / 4.0;
        GaussianBlur(mask2, mask2, Size(blur_size, blur_size), sigma);
    }
    
    // Create output image with alpha channel
    Mat result;
    vector<Mat> channels;
    split(image, channels);
    channels.push_back(mask2); // Add alpha channel
    merge(channels, result);
    
    // Save output as PNG with transparency
    vector<int> compression_params;
    compression_params.push_back(IMWRITE_PNG_COMPRESSION);
    compression_params.push_back(9);
    
    if (imwrite(outputPath, result, compression_params)) {
        cout << "✅ Background removed successfully → " << outputPath << endl;
    } else {
        cerr << "Error: Could not save output image: " << outputPath << endl;
        exit(1);
    }
}

int main(int argc, char** argv) {
    string inputPath, outputPath;
    ProcessingOptions opts;

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if ((arg == "-i" || arg == "--input") && i + 1 < argc) {
            inputPath = argv[++i];
        } else if ((arg == "-o" || arg == "--output") && i + 1 < argc) {
            outputPath = argv[++i];
        } else if ((arg == "-q" || arg == "--quality") && i + 1 < argc) {
            opts.quality = argv[++i];
            if (opts.quality != "fast" && opts.quality != "balanced" && opts.quality != "quality") {
                cerr << "Error: Invalid quality preset. Use: fast, balanced, or quality" << endl;
                return 1;
            }
        } else if ((arg == "-n" || arg == "--iterations") && i + 1 < argc) {
            opts.iterations = stoi(argv[++i]);
            if (opts.iterations < 1 || opts.iterations > 20) {
                cerr << "Error: Iterations must be between 1 and 20" << endl;
                return 1;
            }
        } else if ((arg == "-m" || arg == "--margin") && i + 1 < argc) {
            opts.margin = stoi(argv[++i]);
            if (opts.margin < 0) {
                cerr << "Error: Margin must be >= 0" << endl;
                return 1;
            }
        } else if ((arg == "-e" || arg == "--edge-mode") && i + 1 < argc) {
            opts.edgeMode = argv[++i];
            if (opts.edgeMode != "blur" && opts.edgeMode != "bilateral" && opts.edgeMode != "guided") {
                cerr << "Error: Invalid edge mode. Use: blur, bilateral, or guided" << endl;
                return 1;
            }
        } else if (arg == "-v" || arg == "--verbose") {
            opts.verbose = true;
        } else if (arg == "-h" || arg == "--help") {
            cout << "Background Remover CLI" << endl;
            cout << "Usage: bg-remover -i <input> -o <output> [options]" << endl;
            cout << endl;
            cout << "Required:" << endl;
            cout << "  -i, --input <path>       Input image file path" << endl;
            cout << "  -o, --output <path>      Output image file path" << endl;
            cout << endl;
            cout << "Options:" << endl;
            cout << "  -q, --quality <preset>   Quality preset: fast, balanced, quality" << endl;
            cout << "                           (default: balanced)" << endl;
            cout << "  -n, --iterations <n>     GrabCut iterations (1-20, default: 8)" << endl;
            cout << "  -m, --margin <pixels>    Edge margin/inset in pixels (default: auto)" << endl;
            cout << "  -e, --edge-mode <mode>   Edge refinement: blur, bilateral, guided" << endl;
            cout << "                           (default: guided)" << endl;
            cout << "  -v, --verbose            Show detailed processing information" << endl;
            cout << "  -h, --help               Show this help message" << endl;
            cout << endl;
            cout << "Quality Presets:" << endl;
            cout << "  fast      - Quick processing (5 iterations, blur)" << endl;
            cout << "  balanced  - Good quality and speed (8 iterations, guided)" << endl;
            cout << "  quality   - Best results (12 iterations, guided, 1.5x kernel)" << endl;
            cout << endl;
            cout << "Examples:" << endl;
            cout << "  bg-remover -i photo.jpg -o output.png" << endl;
            cout << "  bg-remover -i photo.jpg -o output.png -q quality" << endl;
            cout << "  bg-remover -i photo.jpg -o output.png -n 15 -e guided -v" << endl;
            return 0;
        }
    }

    if (inputPath.empty() || outputPath.empty()) {
        cerr << "Error: Both input and output paths are required." << endl;
        cerr << "Usage: bg-remover -i <input> -o <output> [options]" << endl;
        cerr << "Run 'bg-remover --help' for more information." << endl;
        return 1;
    }

    // Apply quality preset
    applyPreset(opts);

    try {
        removeBackground(inputPath, outputPath, opts);
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}