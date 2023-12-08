#ifndef CAMERA_CALIBRATION_HPP_
#define CAMERA_CALIBRATION_HPP_

#define CLEAR_BUFFER_FRAMES 20

#define COLLECT_INSTRUCTION "press SPACE to capture the image, ESC to break"
#define COLLECT_IMAGES_WND "Collect images - " COLLECT_INSTRUCTION
#define RESULT_IMAGES_WND "Result images"
#define WRONG_PARAMETERS_MSG "Not enough parameters!"

const std::string IMAGES_FOLDER_NAME = "imgs";
const std::string RESULT_FILE_NAME = "calibration.xml";

int CHECKERBOARD[2] = { 0, 0 };
std::string CAMERA_ADDRESS = "";

char* get_cmd_option(char** begin, char** end, const std::string& option);
bool cmd_option_exists(char** begin, char** end, const std::string& option);
int collect_images();
int calibrate();
int apply_calibration();

#endif /* CAMERA_CALIBRATION_HPP_ */
