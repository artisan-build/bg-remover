#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>

// Check if opencv_contrib is available
#if CV_VERSION_MAJOR >= 3 && __has_include(<opencv2/ximgproc.hpp>)
#define HAVE_OPENCV_CONTRIB
#include <opencv2/ximgproc.hpp>
#endif

#ifdef WITH_ML
#include <onnxruntime/onnxruntime_cxx_api.h>
#endif

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
    bool useML = false;
    string modelPath = "";
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

// Load image from file or stdin
Mat loadImage(const string& path) {
    if (path == "-") {
        // Read from stdin
        #ifdef _WIN32
        _setmode(_fileno(stdin), _O_BINARY);
        #endif

        // Read entire stdin into buffer
        vector<uchar> buffer(istreambuf_iterator<char>(cin), {});

        if (buffer.empty()) {
            cerr << "Error: No data received from stdin" << endl;
            exit(1);
        }

        // Decode image from buffer
        Mat img = imdecode(buffer, IMREAD_COLOR);

        if (img.empty()) {
            cerr << "Error: Could not decode image from stdin" << endl;
            exit(1);
        }

        return img;
    } else {
        // Read from file
        Mat img = imread(path, IMREAD_COLOR);

        if (img.empty()) {
            cerr << "Error: Could not open or find the image: " << path << endl;
            exit(1);
        }

        return img;
    }
}

#ifdef WITH_ML
// Run ML-based segmentation using ONNX model
Mat runMLSegmentation(const Mat& image, const string& modelPath, bool verbose) {
    try {
        // Initialize ONNX Runtime
        Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "bg-remover");
        Ort::SessionOptions session_options;
        session_options.SetIntraOpNumThreads(1);
        session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);

        if (verbose) {
            cerr << "Loading ML model: " << modelPath << endl;
        }

        // Load model
        Ort::Session session(env, modelPath.c_str(), session_options);

        // Get input shape (assuming model expects 320x320 for U2-Net-lite)
        int input_height = 320;
        int input_width = 320;

        // Preprocess: resize and normalize
        Mat resized;
        resize(image, resized, Size(input_width, input_height));
        resized.convertTo(resized, CV_32FC3, 1.0 / 255.0);

        // Convert to tensor format (NCHW: batch, channels, height, width)
        vector<float> input_tensor_values(1 * 3 * input_height * input_width);
        for (int c = 0; c < 3; c++) {
            for (int h = 0; h < input_height; h++) {
                for (int w = 0; w < input_width; w++) {
                    input_tensor_values[c * input_height * input_width + h * input_width + w] =
                        resized.at<Vec3f>(h, w)[c];
                }
            }
        }

        // Setup input tensor
        vector<int64_t> input_shape = {1, 3, input_height, input_width};
        auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
            memory_info, input_tensor_values.data(), input_tensor_values.size(),
            input_shape.data(), input_shape.size());

        // Get input/output names
        Ort::AllocatorWithDefaultOptions allocator;
        const char* input_names[] = {"input"};
        const char* output_names[] = {"output"};

        if (verbose) {
            cerr << "Running ML inference..." << endl;
        }

        // Run inference
        auto output_tensors = session.Run(Ort::RunOptions{nullptr}, input_names, &input_tensor, 1,
                                         output_names, 1);

        // Get output tensor
        float* output_data = output_tensors[0].GetTensorMutableData<float>();
        auto output_shape = output_tensors[0].GetTensorTypeAndShapeInfo().GetShape();

        // Convert output to Mat (assuming output is HxW mask)
        Mat mask(input_height, input_width, CV_32F, output_data);

        // Resize mask back to original size
        Mat result_mask;
        resize(mask, result_mask, image.size());

        // Convert to 8-bit (0-255)
        result_mask.convertTo(result_mask, CV_8UC1, 255.0);

        if (verbose) {
            cerr << "ML inference completed" << endl;
        }

        return result_mask;

    } catch (const Ort::Exception& e) {
        cerr << "ONNX Runtime error: " << e.what() << endl;
        exit(1);
    }
}
#endif

// Save image to file or stdout
void saveImage(const string& path, const Mat& img) {
    vector<int> compression_params;
    compression_params.push_back(IMWRITE_PNG_COMPRESSION);
    compression_params.push_back(9);

    if (path == "-") {
        // Write to stdout
        #ifdef _WIN32
        _setmode(_fileno(stdout), _O_BINARY);
        #endif

        // Encode image to PNG buffer
        vector<uchar> buffer;
        if (!imencode(".png", img, buffer, compression_params)) {
            cerr << "Error: Could not encode image to PNG" << endl;
            exit(1);
        }

        // Write buffer to stdout
        cout.write(reinterpret_cast<char*>(buffer.data()), buffer.size());
        cout.flush();
    } else {
        // Write to file
        if (!imwrite(path, img, compression_params)) {
            cerr << "Error: Could not save output image: " << path << endl;
            exit(1);
        }
    }
}

void removeBackground(const string& inputPath, const string& outputPath, const ProcessingOptions& opts) {
    // Read input image (from file or stdin)
    Mat image = loadImage(inputPath);

    // Suppress verbose output when writing to stdout to avoid corrupting image data
    bool showVerbose = opts.verbose && (outputPath != "-");

    if (showVerbose) {
        cout << "Image loaded: " << image.cols << "x" << image.rows << endl;
        cout << "Processing options:" << endl;
        cout << "  Mode: " << (opts.useML ? "ML" : "GrabCut") << endl;
        if (!opts.useML) {
            cout << "  Quality: " << opts.quality << endl;
            cout << "  Iterations: " << opts.iterations << endl;
            cout << "  Edge mode: " << opts.edgeMode << endl;
            cout << "  Kernel scale: " << opts.kernelScale << endl;
        }
    }

    Mat mask2;

#ifdef WITH_ML
    if (opts.useML) {
        // Use ML-based segmentation
        if (opts.modelPath.empty()) {
            cerr << "Error: ML mode (default) requires --model <path> to specify model file" << endl;
            cerr << "       Use --grabcut to use the traditional GrabCut algorithm instead" << endl;
            exit(1);
        }
        mask2 = runMLSegmentation(image, opts.modelPath, showVerbose);
    } else
#else
    if (opts.useML) {
        cerr << "Error: ML mode not available. Binary was compiled without WITH_ML flag." << endl;
        cerr << "To use ML mode, rebuild with: make ML=1" << endl;
        exit(1);
    }
#endif
    {
        // Use traditional GrabCut algorithm for background removal
        Mat mask = Mat::zeros(image.size(), CV_8UC1);

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

        if (showVerbose) {
            cout << "Processing image with GrabCut algorithm..." << endl;
        }

        // Run GrabCut
        grabCut(image, mask, rectangle, bgModel, fgModel, opts.iterations, GC_INIT_WITH_RECT);

        // Create binary mask (0 = background, 255 = foreground)
        mask2 = (mask == GC_FGD) | (mask == GC_PR_FGD);
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
#ifdef HAVE_OPENCV_CONTRIB
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
#else
            // Fallback to bilateral filter if opencv_contrib not available
            Mat mask_float;
            mask2.convertTo(mask_float, CV_32F);
            Mat filtered;
            bilateralFilter(mask_float, filtered, 9, 75, 75);
            filtered.convertTo(mask2, CV_8UC1);
#endif
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
    }
    
    // Create output image with alpha channel
    Mat result;
    vector<Mat> channels;
    split(image, channels);
    channels.push_back(mask2); // Add alpha channel
    merge(channels, result);

    // Save output (to file or stdout)
    saveImage(outputPath, result);

    // Success message (skip for stdout to avoid mixing with image data)
    if (outputPath != "-") {
        cout << "✅ Background removed successfully → " << outputPath << endl;
    }
}

int main(int argc, char** argv) {
    string inputPath, outputPath;
    ProcessingOptions opts;

#ifdef WITH_ML
    // ML mode is default when compiled with ML support
    opts.useML = true;
#endif

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
        } else if (arg == "--ml") {
            opts.useML = true;
        } else if (arg == "--grabcut") {
            opts.useML = false;
        } else if (arg == "--model" && i + 1 < argc) {
            opts.modelPath = argv[++i];
        } else if (arg == "-h" || arg == "--help") {
            cout << "Background Remover CLI" << endl;
            cout << "Usage: bg-remover -i <input> -o <output> [options]" << endl;
            cout << endl;
            cout << "Required:" << endl;
            cout << "  -i, --input <path>       Input image file path (use '-' for stdin)" << endl;
            cout << "  -o, --output <path>      Output image file path (use '-' for stdout)" << endl;
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
#ifdef WITH_ML
            cout << "ML Options (ML enabled by default):" << endl;
            cout << "  --model <path>           Path to ONNX model file (U2-Net, RMBG, etc.)" << endl;
            cout << "  --grabcut                Use GrabCut algorithm instead of ML" << endl;
            cout << "  --ml                     Force ML mode on (already default)" << endl;
            cout << endl;
#endif
            cout << "Quality Presets:" << endl;
            cout << "  fast      - Quick processing (5 iterations, blur)" << endl;
            cout << "  balanced  - Good quality and speed (8 iterations, guided)" << endl;
            cout << "  quality   - Best results (12 iterations, guided, 1.5x kernel)" << endl;
            cout << endl;
            cout << "Examples:" << endl;
            cout << "  bg-remover -i photo.jpg -o output.png" << endl;
            cout << "  bg-remover -i photo.jpg -o output.png -q quality" << endl;
            cout << "  bg-remover -i photo.jpg -o output.png -n 15 -e guided -v" << endl;
            cout << endl;
            cout << "Piping workflows:" << endl;
            cout << "  cat photo.jpg | bg-remover -i - -o output.png" << endl;
            cout << "  bg-remover -i photo.jpg -o - > output.png" << endl;
            cout << "  cat photo.jpg | bg-remover -i - -o - > output.png" << endl;
            cout << "  curl https://example.com/photo.jpg | bg-remover -i - -o -" << endl;
#ifdef WITH_ML
            cout << endl;
            cout << "ML mode examples (ML is default, just specify model):" << endl;
            cout << "  bg-remover -i photo.jpg -o output.png --model u2net.onnx" << endl;
            cout << "  bg-remover -i photo.jpg -o output.png --model rmbg-1.4.onnx" << endl;
            cout << "  bg-remover -i photo.jpg -o output.png --grabcut  # Use GrabCut instead" << endl;
#endif
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