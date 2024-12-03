#include "opencv2/opencv.hpp"
#include <iostream>
#include <thread>
#include <atomic>
#include <filesystem>
#include "Server.h"

namespace fs = std::filesystem;

using namespace cv;

// Biến trạng thái toàn cục để kiểm soát việc quay
// std::atomic<bool> isRecording(false);
// std::thread recordingThread;  // Luồng quay video
// cv::VideoCapture capture;
// cv::VideoWriter writer;

// Lớp Webcam quản lý quá trình ghi hình
class Webcam {
public:
    bool StartWebcamRecording(const std::string& filename);
    bool StopWebcamRecording();
    void Record();
};