#ifndef WEBCAM_H_
#define WEBCAM_H_

#include "opencv2/opencv.hpp"
#include <iostream>
#include <thread>
#include <atomic>

using namespace cv;

// Biến trạng thái toàn cục để kiểm soát việc quay
extern std::atomic<bool> isRecording;
extern std::thread recordingThread;  // Luồng quay video
extern cv::VideoCapture capture;
extern cv::VideoWriter writer;

// Lớp Webcam quản lý quá trình ghi hình
class Webcam {
public:
    bool StartWebcamRecording(const std::string& filename);
    bool StopWebcamRecording();
    void Record();
};

#endif // WEBCAM_H_