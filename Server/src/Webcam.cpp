#include "Webcam.h"

// Biến trạng thái toàn cục để kiểm soát việc quay
std::atomic<bool> isRecording(false);
std::thread recordingThread;  // Luồng quay video
cv::VideoCapture capture;
cv::VideoWriter writer;

bool Webcam::StartWebcamRecording(const std::string& filename) {
    if (isRecording) {
        std::cerr << "Error: Recording is already in progress." << std::endl;
        return false;
    }

    // Mở webcam
    capture.open(0);
    if (!capture.isOpened()) {
        std::cerr << "Error: Unable to open the webcam." << std::endl;
        return false;
    }

    // Lấy kích thước khung hình
    cv::Size frameSize((int)capture.get(cv::CAP_PROP_FRAME_WIDTH), (int)capture.get(cv::CAP_PROP_FRAME_HEIGHT));
    std::cout << "Frame size: " << frameSize.width << "x" << frameSize.height << std::endl;

    // Khởi tạo VideoWriter để lưu video
    writer.open(filename, cv::VideoWriter::fourcc('m', 'p', '4', 'v'), 30, frameSize);
    if (!writer.isOpened()) {
        std::cerr << "Error: Unable to open video writer." << std::endl;
        capture.release();
        return false;
    }

    // Bắt đầu quay
    isRecording = true;
    recordingThread = std::thread(&Webcam::Record, this);

    return true;
}

bool Webcam::StopWebcamRecording() {
    if (!isRecording) {
        std::cerr << "Error: No recording is in progress." << std::endl;
        return false;
    }

    // Dừng quay
    isRecording = false;

    // Chờ luồng kết thúc
    if (recordingThread.joinable()) {
        recordingThread.join();
    }

    // Giải phóng tài nguyên
    capture.release();
    writer.release();
    std::cout << "Recording stopped and saved." << std::endl;

    return true;
}

void Webcam::Record() {
    cv::Mat frame;

    while (isRecording) {
        // Lấy khung hình từ webcam
        capture >> frame;
        if (frame.empty()) {
            std::cerr << "Error: Blank frame captured." << std::endl;
            break;
        }

        // Ghi khung hình vào tệp video
        writer.write(frame);

        // Hiển thị khung hình
        cv::imshow("Recording", frame);

        // Kiểm tra phím để thoát (tùy chọn, nhưng không thoát nếu không nhấn Stop)
        if (cv::waitKey(10) == 27) {
            std::cerr << "Warning: Esc pressed, but will not stop recording. Use Stop action instead." << std::endl;
        }
    }

    // Đóng cửa sổ hiển thị
    cv::destroyWindow("Recording");
}