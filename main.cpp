#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <iostream>
#include <fstream>

using namespace std;
using namespace cv;
using namespace dnn;

int main() {
    Mat frame, left_roi, right_roi, lane_roi, lane_roi2, roi;
    VideoCapture cap;
    vector<Vec2f> lines_left, lines_right, lines_lane, lines_lane2;
    int fps, delay;
    int count = 0;
    bool isLoad = false;
    int count_load = 0;
    int max = 0;
    bool stop = false;
    int count_mov;
    bool flag = false;
    bool car, human = false;
    bool human_prev = false;
    bool car_prev = false;
    int car_max, human_max;

    if(cap.open("Project3_2.mp4") == 0) {
        cout << "no such file!" << endl;
        waitKey(0);
    }

    String modelConfiguration = "YOLO/yolov4-tiny.cfg"; 
    String modelBinary = "YOLO/yolov4-tiny.weights";

    Net net = readNetFromDarknet(modelConfiguration, modelBinary);
    
    vector<String> classNamesVec;
    ifstream classNamesFile("YOLO/coco.names");

    if (classNamesFile.is_open()) {
        string className = "";
        while (std::getline(classNamesFile, className)) classNamesVec.push_back(className);
    }

    fps = cap.get(CAP_PROP_FPS);
    delay = 1000/fps;

    while (true) {
        cap >> frame;

        if(frame.empty()) {
            break;
        }
        
        //1) Alert a warning text "Lane departure!"
        left_roi = frame(Rect(Point(100,380), Point(250,480)));
        cvtColor(left_roi, left_roi, COLOR_BGR2GRAY);
        blur(left_roi, left_roi, Size(5,5));
        Canny(left_roi, left_roi, 10, 60, 3);

        right_roi = frame(Rect(Point(450,380), Point(600,480)));
        cvtColor(right_roi, right_roi, COLOR_BGR2GRAY);
        blur(right_roi, right_roi, Size(5,5));
        Canny(right_roi, right_roi, 10, 60, 3);

        lane_roi = frame(Rect(Point(300,380), Point(400,480)));
        cvtColor(lane_roi, lane_roi, COLOR_BGR2GRAY);
        blur(lane_roi, lane_roi, Size(5,5));
        Canny(lane_roi, lane_roi, 10, 60, 3);

        lane_roi2 = frame(Rect(Point(0,380), Point(100,480)));
        cvtColor(lane_roi2, lane_roi2, COLOR_BGR2GRAY);
        blur(lane_roi2, lane_roi2, Size(5,5));
        Canny(lane_roi2, lane_roi2, 10, 60, 3);

        // Use lines whose angle is between 30 and 60 degress
        HoughLines(left_roi, lines_left, 1, CV_PI / 180, 75, 0, 0, CV_PI/6, CV_PI/3);

        // Use lines whose angle is between 120 and 150 degress
        HoughLines(right_roi, lines_right, 1, CV_PI / 180, 75, 0, 0, (CV_PI*2)/3, (CV_PI*5)/6);

        // Determine if the vehicle is in the road lane
        if(lines_right.size() != 0 && lines_left.size() != 0) {
            count_load++;
        } else {
            count_load = 0;
        }
        if(count_load > 10) {
            isLoad = true;
        }

        // If the car crosses the lane, detect the line
        HoughLines(lane_roi, lines_lane, 1, CV_PI / 180, 0, 0, 0, -(CV_PI/18), (CV_PI/18));
        HoughLines(lane_roi2, lines_lane2, 1, CV_PI / 180, 75, 0, 0, (CV_PI/18)*3, CV_PI/2);

        //Message output if sustained for more than 5 counts
        if(lines_left.size()==0 && lines_right.size()==0 && lines_lane.size()!=0 && lines_lane2.size()!=0 && isLoad) {
            count++;
        } else {
            count=0;
        }
        if(count > 5) {
            putText(frame, "Lane departure!", Point(frame.cols/2-180, 100), FONT_HERSHEY_SIMPLEX, 1.5,
            Scalar(0, 0, 255), 3);
        }

        //2)Alert a warning text "Start Moving!"
        roi = frame(Rect(Point(200,150), Point(500,480)));

        if (roi.channels() == 4) cvtColor(roi, roi, COLOR_BGRA2BGR);
        
        //Run YOLO once per 2 frames
        if(flag) {
            Mat inputBlob = blobFromImage(roi, 1 / 255.F, Size(416, 416), Scalar(), true, false); 
            net.setInput(inputBlob, "data");
            std::vector<cv::String> outNames = net.getUnconnectedOutLayersNames();
            std::vector<cv::Mat> outs;
            net.forward(outs, outNames);

            float confidenceThreshold = 0.1; 

            for (size_t k = 0; k < outs.size(); ++k) {
                Mat& detectionMat = outs[k];
                for (int i = 0; i < detectionMat.rows; i++) { 
                    const int probability_index = 5;
                    const int probability_size = detectionMat.cols - probability_index;
                    float *prob_array_ptr = &detectionMat.at<float>(i, probability_index);
                    size_t objectClass = max_element(prob_array_ptr, prob_array_ptr + probability_size) - prob_array_ptr; 

                    float confidence = detectionMat.at<float>(i, (int)objectClass + probability_index);
                
                    if (confidence > confidenceThreshold) {
                        float x_center = detectionMat.at<float>(i, 0) * roi.cols; 
                        float y_center = detectionMat.at<float>(i, 1) * roi.rows; 
                        float width = detectionMat.at<float>(i, 2) * roi.cols; 
                        float height = detectionMat.at<float>(i, 3) * roi.rows;

                        Point p1(cvRound(x_center - width / 2), cvRound(y_center - height / 2)); 
                        Point p2(cvRound(x_center + width / 2), cvRound(y_center + height / 2)); 
                        Rect object(p1, p2);
                        Scalar object_roi_color(0, 255, 0);

                        String className = objectClass < classNamesVec.size() ? classNamesVec[objectClass] : cv::format("unknown(%ld)", objectClass);

                        if(className == "car" && height > 150) {
                            if(height > max) {
                                max = height;
                            }
                        }
                    }
                }
            }
        }

        //Determine the change in maximum height value when the vehicle in front of you begins to move in a stationary state
        if(max != 0 && max < 180 && isLoad) {
            stop = true;
        }
        //The message output for 80 counts
        if(stop == true && count_mov < 80) {
            putText(frame, "Start Moving!", Point(frame.cols/2-150, 100), FONT_HERSHEY_SIMPLEX, 1.5,
            Scalar(255, 0, 0), 3);
            count_mov++;
        }
        max = 0;

        //3)Alert a warning text "Human" or "Car" "detected nearby!"
        if (frame.channels() == 4) cvtColor(frame, frame, COLOR_BGRA2BGR);

        //Run YOLO once per 2 frames
        if(flag) {
            Mat inputBlob = blobFromImage(frame, 1 / 255.F, Size(416, 416), Scalar(), true, false); 
            net.setInput(inputBlob, "data");
            std::vector<cv::String> outNames2 = net.getUnconnectedOutLayersNames();
            std::vector<cv::Mat> outs2;
            net.forward(outs2, outNames2);

            float confidenceThreshold = 0.24;

            for (size_t k = 0; k < outs2.size(); ++k) {
                Mat& detectionMat = outs2[k];
                int i = 0;
                for (i = 0; i < detectionMat.rows; i++) { 
                    const int probability_index = 5;
                    const int probability_size = detectionMat.cols - probability_index;
                    float *prob_array_ptr = &detectionMat.at<float>(i, probability_index);
                    size_t objectClass = max_element(prob_array_ptr, prob_array_ptr + probability_size) - prob_array_ptr; 
                    float confidence = detectionMat.at<float>(i, (int)objectClass + probability_index);
                 
                    if (confidence > confidenceThreshold) {
                        float x_center = detectionMat.at<float>(i, 0) * frame.cols; 
                        float y_center = detectionMat.at<float>(i, 1) * frame.rows; 
                        float width = detectionMat.at<float>(i, 2) * frame.cols; 
                        float height = detectionMat.at<float>(i, 3) * frame.rows;

                        Point p1(cvRound(x_center - width / 2), cvRound(y_center - height / 2)); 
                        Point p2(cvRound(x_center + width / 2), cvRound(y_center + height / 2)); 
                        Rect object(p1, p2);

                        //A car and a person are represented by red and green rectangle, respectively
                        String className = objectClass < classNamesVec.size() ? classNamesVec[objectClass] : cv::format("unknown(%ld)", objectClass);
                        if(className == "car") {
                            rectangle(frame, object, Scalar(0, 0, 255));
                        } else if (className == "person") {
                            rectangle(frame, object, Scalar(0, 255, 0));
                        }

                        if(className == "person" && human_max < width) {
                            human_max = width;
                        }
                        if(className == "car" && car_max < width) {
                            car_max = width;
                        }

                        //If the detected rectangle is large in size because it is close, change the flag
                        if( (className == "car" && width > 130) || (className == "car" && height > 110) ) {
                            car = true;
                            break;
                        } else if(className == "person" && height > 120) {
                            human = true;
                            break;
                        } else {
                            car = false;
                            human = false;
                        }
                    }
                }
            }
        }

        if(!human && human_max > 24 && human_prev) {
            human = true;
        }
        human_max = 0;
        if(!car && car_max > 110 && car_prev) {
            car = true;
        }
        car_max = 0;

        //Output message according to flag
        if(car) {
            putText(frame, "Car detected nearby!", Point(frame.cols/2-250, 200), FONT_HERSHEY_SIMPLEX, 1.5,
            Scalar(0, 0, 255), 3);
        }
        if(human) {
            putText(frame, "Human detected nearby!", Point(frame.cols/2-280, 200), FONT_HERSHEY_SIMPLEX, 1.5,
            Scalar(0, 255, 0), 3);
        }
        human_prev = human;
        car_prev = car;

        if(!flag) flag = true;
        else flag = false;

        imshow("Project3", frame);
        if (waitKey(delay) == 27) break;
    }
}