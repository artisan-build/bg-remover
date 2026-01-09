#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>

using namespace cv;
using namespace std;

void removeBackground(const string& inputPath, const string& outputPath) {
    // Read input image
    Mat image = imread(inputPath, IMREAD_COLOR);
    
    if (image.empty()) {
        cerr << "Error: Could not open or find the image: " << inputPath << endl;
        exit(1);
    }
    
    cout << "Image loaded: " << image.cols << "x" << image.rows << endl;
    
    // Create mask for background removal
    Mat mask = Mat::zeros(image.size(), CV_8UC1);
    
    // Use GrabCut algorithm for background removal
    Rect rectangle(10, 10, image.cols - 20, image.rows - 20);
    Mat bgModel, fgModel;
    
    cout << "Processing image with GrabCut algorithm..." << endl;
    
    // Run GrabCut
    grabCut(image, mask, rectangle, bgModel, fgModel, 5, GC_INIT_WITH_RECT);
    
    // Create binary mask (0 = background, 255 = foreground)
    Mat mask2 = (mask == GC_FGD) | (mask == GC_PR_FGD);
    mask2.convertTo(mask2, CV_8UC1, 255);
    
    // Apply morphological operations to clean up mask
    Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(5, 5));
    morphologyEx(mask2, mask2, MORPH_CLOSE, kernel);
    morphologyEx(mask2, mask2, MORPH_OPEN, kernel);
    
    // Apply Gaussian blur to smooth edges
    GaussianBlur(mask2, mask2, Size(5, 5), 0);
    
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
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if ((arg == "-i" || arg == "--input") && i + 1 < argc) {
            inputPath = argv[++i];
        } else if ((arg == "-o" || arg == "--output") && i + 1 < argc) {
            outputPath = argv[++i];
        } else if (arg == "-h" || arg == "--help") {
            cout << "Background Remover CLI" << endl;
            cout << "Usage: bg-remover -i <input> -o <output>" << endl;
            cout << "  -i, --input   Input image file path" << endl;
            cout << "  -o, --output  Output image file path" << endl;
            return 0;
        }
    }
    
    if (inputPath.empty() || outputPath.empty()) {
        cerr << "Error: Both input and output paths are required." << endl;
        cerr << "Usage: bg-remover -i <input> -o <output>" << endl;
        return 1;
    }
    
    try {
        removeBackground(inputPath, outputPath);
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
    
    return 0;
}