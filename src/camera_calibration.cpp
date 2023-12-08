#ifdef _WIN32
	#include <windows.h>
#elif __linux__
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <unistd.h>
#endif

#include <opencv2/core.hpp>
#include <opencv2/core/utility.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/core/utils/logger.hpp>

#include "camera_calibration.hpp"

using namespace cv;
using namespace std;

int main(int argc, char* argv[])
{
	#ifdef _WIN32
		std::cout << "Windows" << std::endl;
	#elif __linux__
		std::cout << "Linux" << std::endl;
	#endif

    cv::utils::logging::setLogLevel(cv::utils::logging::LogLevel::LOG_LEVEL_SILENT);

    if (cmd_option_exists(argv, argv + argc, "-h"))
        CHECKERBOARD[0] = atoi(get_cmd_option(argv, argv + argc, "-h"));
    if (cmd_option_exists(argv, argv + argc, "-w"))
        CHECKERBOARD[1] = atoi(get_cmd_option(argv, argv + argc, "-w"));
    //
    if (cmd_option_exists(argv, argv + argc, "-v"))
    {
        char* cmd_camera_address = get_cmd_option(argv, argv + argc, "-v");
        CAMERA_ADDRESS = cmd_camera_address;
    }
    //
    if (!CHECKERBOARD[0] || !CHECKERBOARD[1] || (CAMERA_ADDRESS == ""))
    {
        std::cout << WRONG_PARAMETERS_MSG << std::endl;
        system("pause");
        return 1;
    }
    //
    std::cout << "CHECKERBOARD size = " << CHECKERBOARD[0] << " x " << CHECKERBOARD[1] << std::endl;
    std::cout << "CAMERA_ADDRESS = " << CAMERA_ADDRESS << std::endl;
    //
    if (cmd_option_exists(argv, argv + argc, "-i"))
        collect_images();       //  collect photos
    if (cmd_option_exists(argv, argv + argc, "-c"))
        calibrate();            //  find corners and calibrate
    if (cmd_option_exists(argv, argv + argc, "-a"))
        apply_calibration();    //  use calibration results
    //
    return 0;
}

char* get_cmd_option(char** begin, char** end, const std::string& option)
{
    char** itr = find(begin, end, option);
    if (itr != end && ++itr != end)
    {
        return *itr;
    }
    return 0;
}

bool cmd_option_exists(char** begin, char** end, const std::string& option)
{
    return find(begin, end, option) != end;
}

int collect_images()
{

	struct stat sb;
	if (stat(IMAGES_FOLDER_NAME.c_str(), &sb) == -1)
	{

#ifdef _WIN32
        std::wstring stemp = std::wstring(IMAGES_FOLDER_NAME.begin(), IMAGES_FOLDER_NAME.end());
        CreateDirectory(stemp.c_str(), NULL);
#elif __linux__
        mkdir(IMAGES_FOLDER_NAME.c_str(), 0700);
#endif

	}

    std::cout << "Images collecting started - " << COLLECT_INSTRUCTION << std::endl;

    VideoCapture capture = VideoCapture(CAMERA_ADDRESS);

    char buf[255];

    cv::Mat frame;

    int count = 0;
    while (true)
    {
        capture >> frame;

        imshow(COLLECT_IMAGES_WND, frame);

        int key = waitKey(1);

        if (key == 27)
            break;

        if (key == ' ')
        {
            count++;
            sprintf(buf, "%s/%d.jpg", IMAGES_FOLDER_NAME.c_str(), count);
            std::cout << buf << std::endl;
            imwrite(buf, frame);
            //
            // blink
            bitwise_not(frame, frame);
            imshow(COLLECT_IMAGES_WND, frame);
            waitKey(50);
        }
    }

    cv::destroyWindow(COLLECT_IMAGES_WND);

    std::cout << "Images collecting finished! " << count << " images collected." << std::endl;

    return 0;
}

int calibrate()
{
    std::cout << "Calibration started!" << std::endl;

    // Creating vector to store vectors of 3D points for each checkerboard image
    std::vector<std::vector<cv::Point3f> > objpoints;

    // Creating vector to store vectors of 2D points for each checkerboard image
    std::vector<std::vector<cv::Point2f> > imgpoints;

    // Defining the world coordinates for 3D points
    std::vector<cv::Point3f> objp;
    for (int i{ 0 }; i < CHECKERBOARD[1]; i++)
    {
        for (int j{ 0 }; j < CHECKERBOARD[0]; j++)
            objp.push_back(cv::Point3f(j, i, 0));
    }

    // Extracting path of individual image stored in a given directory
    std::vector<cv::String> images;
    // Path of the folder containing checkerboard images
    std::string path = IMAGES_FOLDER_NAME + "/*.jpg";

    cv::glob(path, images);

    cv::Mat frame, gray;
    // vector to store the pixel coordinates of detected checker board corners
    std::vector<cv::Point2f> corner_pts;
    bool success;

    std::cout << "Image processing started!" << std::endl;

    // Looping over all the images in the directory
    for (size_t i{ 0 }; i < images.size(); i++)
    {
        std::cout << "Processing image " << (i + 1) << " of " << images.size() << " ... ";

        frame = cv::imread(images[i]);
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

        // Finding checker board corners
        // If desired number of corners are found in the image then success = true
        success = cv::findChessboardCorners(gray, cv::Size(CHECKERBOARD[0], CHECKERBOARD[1]), corner_pts,
            cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_FAST_CHECK | cv::CALIB_CB_NORMALIZE_IMAGE);

        /*
         * If desired number of corner are detected,
         * we refine the pixel coordinates and display
         * them on the images of checker board
        */
        if (success)
        {
            std::cout << "successfully!" << std::endl;

            cv::TermCriteria criteria(cv::TermCriteria::EPS | cv::TermCriteria::MAX_ITER, 30, 0.001);

            // refining pixel coordinates for given 2d points.
            cv::cornerSubPix(gray, corner_pts, cv::Size(11, 11), cv::Size(-1, -1), criteria);

            // Displaying the detected corner points on the checker board
            cv::drawChessboardCorners(frame, cv::Size(CHECKERBOARD[0], CHECKERBOARD[1]), corner_pts, success);

            objpoints.push_back(objp);
            imgpoints.push_back(corner_pts);
        } else {
            std::cout << "corners NOT found!" << std::endl;
        }

        cv::putText(frame, to_string(i + 1), cv::Point2f(10, 30),
            cv::FONT_HERSHEY_DUPLEX, 1, cv::Scalar(0x00, 0x00, 0xFF));

        cv::imshow(RESULT_IMAGES_WND, frame);
        cv::waitKey(0);
    }

    cv::destroyWindow(RESULT_IMAGES_WND);

    std::cout << "Image processing finished!" << std::endl;

    cv::Mat cameraMatrix, distCoeffs, R, T;

    /*
     * Performing camera calibration by
     * passing the value of known 3D points (objpoints)
     * and corresponding pixel coordinates of the
     * detected corners (imgpoints)
    */
    std::cout << "Camera calibration started!" << std::endl;
    cv::calibrateCamera(objpoints, imgpoints, cv::Size(gray.rows, gray.cols), cameraMatrix, distCoeffs, R, T);
    std::cout << "Camera calibration finished!" << std::endl;

    std::cout << "cameraMatrix : " << cameraMatrix << std::endl;
    std::cout << "distCoeffs : " << distCoeffs << std::endl;
    std::cout << "Rotation vector : " << R << std::endl;
    std::cout << "Translation vector : " << T << std::endl;

    cv::FileStorage fs(RESULT_FILE_NAME, cv::FileStorage::WRITE);
    // Save calibration parameters
    fs << "cameraMatrix" << cameraMatrix;
    fs << "distCoeffs" << distCoeffs;
    fs.release();

    std::cout << "Calibration finshed!" << std::endl;
    std::cout << "Calibration results successfully saved to file \"" << RESULT_FILE_NAME << "\"" << std::endl;

    return 0;
}

int apply_calibration()
{
    Mat frame, frame_undistoted;
    Mat cameraMatrix, distCoeffs;

    cv::FileStorage fs(RESULT_FILE_NAME, cv::FileStorage::READ);
    fs["cameraMatrix"] >> cameraMatrix;
    fs["distCoeffs"] >> distCoeffs;
    fs.release();

    VideoCapture capture = VideoCapture(CAMERA_ADDRESS);

    for (int i = 0; i < CLEAR_BUFFER_FRAMES; i++)
        capture >> frame;

    while (1)
    {
        std::vector<cv::Mat> frames;
        Mat mergedFrames;

        capture >> frame;
        //
        bool err = false;
        try {
            undistort(frame, frame_undistoted, cameraMatrix, distCoeffs);
        }
        catch (...) {
            std::cout << "Undistortion error!" << std::endl;
            err = true;
            //
            capture.release();
            capture.open(CAMERA_ADDRESS);
        }
        if (err) continue;
        //
        frames.push_back(frame);
        frames.push_back(frame_undistoted);
        cv::hconcat(frames, mergedFrames);
        //
        imshow("Applied calibration", mergedFrames);

        int key = waitKey(1);
        if (key == 27)
        	break;
    }

    capture.release();

    return 0;
}
