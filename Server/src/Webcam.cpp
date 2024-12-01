#include "Webcam.h"

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
    capture.open(0); // Mở webcam mặc định (0 là camera đầu tiên)
    if (!capture.isOpened()) {
        std::cerr << "Error: Unable to open the webcam." << std::endl;
        return false;
    }

    // Lấy kích thước khung hình
    cv::Size frameSize((int)capture.get(cv::CAP_PROP_FRAME_WIDTH), (int)capture.get(cv::CAP_PROP_FRAME_HEIGHT));
    std::cout << "Frame size: " << frameSize.width << "x" << frameSize.height << std::endl;

    // Kiểm tra đường dẫn lưu video
    if (!fs::exists(fs::path(filename).parent_path())) {
        std::cerr << "Error: The directory does not exist." << std::endl;
        return false;
    }

    // Khởi tạo VideoWriter để lưu video
    // Sử dụng codec 'XVID' cho video MP4
    writer.open(filename, cv::VideoWriter::fourcc('X', 'V', 'I', 'D'), 30, frameSize, true);
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
            std::cerr << "Error: Blank frame captured. Check your webcam or drivers." << std::endl;
            break;
        }

        // Ghi khung hình vào tệp video
        writer.write(frame);

        // Hiển thị khung hình
        cv::imshow("Recording", frame);

        // Kiểm tra phím để thoát (tùy chọn, nhưng không thoát nếu không nhấn Stop)
        if (cv::waitKey(10) == 27) { // Phím ESC để thoát (nếu bạn muốn dừng việc ghi video)
            std::cerr << "Warning: Esc pressed, but will not stop recording. Use Stop action instead." << std::endl;
        }
    }

    // Đóng cửa sổ hiển thị
    cv::destroyWindow("Recording");
}

