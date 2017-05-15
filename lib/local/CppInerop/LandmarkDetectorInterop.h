///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2017, Carnegie Mellon University and University of Cambridge,
// all rights reserved.
//
// ACADEMIC OR NON-PROFIT ORGANIZATION NONCOMMERCIAL RESEARCH USE ONLY
//
// BY USING OR DOWNLOADING THE SOFTWARE, YOU ARE AGREEING TO THE TERMS OF THIS LICENSE AGREEMENT.  
// IF YOU DO NOT AGREE WITH THESE TERMS, YOU MAY NOT USE OR DOWNLOAD THE SOFTWARE.
//
// License can be found in OpenFace-license.txt

//     * Any publications arising from the use of this software, including but
//       not limited to academic journal and conference publications, technical
//       reports and manuals, must cite at least one of the following works:
//
//       OpenFace: an open source facial behavior analysis toolkit
//       Tadas Baltru�aitis, Peter Robinson, and Louis-Philippe Morency
//       in IEEE Winter Conference on Applications of Computer Vision, 2016  
//
//       Rendering of Eyes for Eye-Shape Registration and Gaze Estimation
//       Erroll Wood, Tadas Baltru�aitis, Xucong Zhang, Yusuke Sugano, Peter Robinson, and Andreas Bulling 
//       in IEEE International. Conference on Computer Vision (ICCV),  2015 
//
//       Cross-dataset learning and person-speci?c normalisation for automatic Action Unit detection
//       Tadas Baltru�aitis, Marwa Mahmoud, and Peter Robinson 
//       in Facial Expression Recognition and Analysis Challenge, 
//       IEEE International Conference on Automatic Face and Gesture Recognition, 2015 
//
//       Constrained Local Neural Fields for robust facial landmark detection in the wild.
//       Tadas Baltru�aitis, Peter Robinson, and Louis-Philippe Morency. 
//       in IEEE Int. Conference on Computer Vision Workshops, 300 Faces in-the-Wild Challenge, 2013.    
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __LANDMARK_DETECTOR_INTEROP_h_
#define __LANDMARK_DETECTOR_INTEROP_h_

#pragma once

#pragma managed
#include <msclr\marshal.h>
#include <msclr\marshal_cppstd.h>

#pragma unmanaged

// Include all the unmanaged things we need.

#include <opencv2/core/core.hpp>
#include "opencv2/objdetect.hpp"
#include "opencv2/calib3d.hpp"
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <OpenCVWrappers.h>

#include <LandmarkCoreIncludes.h>

#include <Face_utils.h>
#include <FaceAnalyser.h>

#pragma managed

namespace CppInterop {
	
	namespace LandmarkDetector {

		public ref class FaceModelParameters
		{
		public:
			::LandmarkDetector::FaceModelParameters* params;

		public:

			// Initialise the parameters
			FaceModelParameters(System::String^ root, bool demo)
			{
				std::string root_std = msclr::interop::marshal_as<std::string>(root);
				vector<std::string> args;
				args.push_back(root_std);

				params = new ::LandmarkDetector::FaceModelParameters(args);

				if(demo)
				{
					params->model_location = "model/main_clnf_demos.txt";
				}

				params->track_gaze = true;
			}

			// TODO this could have optimize for demo mode (also could appropriately update sigma, reg_factor as well)
			void optimiseForVideo()
			{
				params->window_sizes_small = vector<int>(4);
				params->window_sizes_init = vector<int>(4);

				// For fast tracking
				params->window_sizes_small[0] = 0;
				params->window_sizes_small[1] = 9;
				params->window_sizes_small[2] = 7;
				params->window_sizes_small[3] = 5;

				// Just for initialisation
				params->window_sizes_init.at(0) = 11;
				params->window_sizes_init.at(1) = 9;
				params->window_sizes_init.at(2) = 7;
				params->window_sizes_init.at(3) = 5;

				// For first frame use the initialisation
				params->window_sizes_current = params->window_sizes_init;

				params->multi_view = false;
				params->num_optimisation_iteration = 5;

				params->sigma = 1.5;
				params->reg_factor = 25;
				params->weight_factor = 0;
			}			

			void optimiseForImages()
			{
				params->window_sizes_init = vector<int>(4);
				params->window_sizes_init[0] = 15;
				params->window_sizes_init[1] = 13; 
				params->window_sizes_init[2] = 11; 
				params->window_sizes_init[3] = 9;

				params->multi_view = true;

				params->sigma = 1.25;
				params->reg_factor = 35;
				params->weight_factor = 2.5;
				params->num_optimisation_iteration = 10;
			}			

			::LandmarkDetector::FaceModelParameters* getParams() {
				return params;
			}

			~FaceModelParameters()
			{
				delete params;
			}

		};

		public ref class CLNF
		{
		public:

			// A pointer to the CLNF landmark detector
			::LandmarkDetector::CLNF* clnf;	

		public:

			// Wrapper functions for the relevant CLNF functionality
			CLNF() : clnf(new ::LandmarkDetector::CLNF()) { }
			
			CLNF(FaceModelParameters^ params)
			{				
				clnf = new ::LandmarkDetector::CLNF(params->getParams()->model_location);
			}
			
			~CLNF()
			{
				delete clnf;
			}

			::LandmarkDetector::CLNF* getCLNF() {
				return clnf;
			}

			void Reset() {
				clnf->Reset();
			}

			void Reset(double x, double y) {
				clnf->Reset(x, y);
			}


			double GetConfidence()
			{
				return clnf->detection_certainty;
			}

			bool DetectLandmarksInVideo(OpenCVWrappers::RawImage^ image, FaceModelParameters^ modelParams) {
				return ::LandmarkDetector::DetectLandmarksInVideo(image->Mat, *clnf, *modelParams->getParams());
			}

			bool DetectFaceLandmarksInImage(OpenCVWrappers::RawImage^ image, FaceModelParameters^ modelParams) {
				return ::LandmarkDetector::DetectLandmarksInImage(image->Mat, *clnf, *modelParams->getParams());
			}

			System::Collections::Generic::List<System::Collections::Generic::List<System::Tuple<double,double>^>^>^ DetectMultiFaceLandmarksInImage(OpenCVWrappers::RawImage^ image, FaceModelParameters^ modelParams) {

				auto all_landmarks = gcnew System::Collections::Generic::List<System::Collections::Generic::List<System::Tuple<double,double>^>^>();

				// Detect faces in an image
				vector<cv::Rect_<double> > face_detections;
				
				vector<double> confidences;
		
				// TODO this should be pre-allocated as now it might be a bit too slow
				dlib::frontal_face_detector face_detector_hog = dlib::get_frontal_face_detector();

				::LandmarkDetector::DetectFacesHOG(face_detections, image->Mat, face_detector_hog, confidences);

				// Detect landmarks around detected faces
				int face_det = 0;
				// perform landmark detection for every face detected
				for(size_t face=0; face < face_detections.size(); ++face)
				{
					cv::Mat depth;
					// if there are multiple detections go through them
					bool success = ::LandmarkDetector::DetectLandmarksInImage(image->Mat, depth, face_detections[face], *clnf, *modelParams->getParams());

					auto landmarks_curr = gcnew System::Collections::Generic::List<System::Tuple<double,double>^>();
					if(clnf->detected_landmarks.cols == 1)
					{
						int n = clnf->detected_landmarks.rows / 2;
						for(int i = 0; i < n; ++i)
						{
							landmarks_curr->Add(gcnew System::Tuple<double,double>(clnf->detected_landmarks.at<double>(i,0), clnf->detected_landmarks.at<double>(i+n,0)));
						}
					}
					else
					{
						int n = clnf->detected_landmarks.cols / 2;
						for(int i = 0; i < clnf->detected_landmarks.cols; ++i)
						{
							landmarks_curr->Add(gcnew System::Tuple<double,double>(clnf->detected_landmarks.at<double>(0,i), clnf->detected_landmarks.at<double>(0,i+1)));
						}
					}
					all_landmarks->Add(landmarks_curr);

				}

				return all_landmarks;
			}

			void GetPoseWRTCamera(System::Collections::Generic::List<double>^ pose, double fx, double fy, double cx, double cy) {
				auto pose_vec = ::LandmarkDetector::GetPoseWRTCamera(*clnf, fx, fy, cx, cy);
				pose->Clear();
				for(int i = 0; i < 6; ++i)
				{
					pose->Add(pose_vec[i]);
				}
			}

			void GetPose(System::Collections::Generic::List<double>^ pose, double fx, double fy, double cx, double cy) {
				auto pose_vec = ::LandmarkDetector::GetPose(*clnf, fx, fy, cx, cy);
				pose->Clear();
				for(int i = 0; i < 6; ++i)
				{
					pose->Add(pose_vec[i]);
				}
			}
	
			System::Collections::Generic::List<System::Tuple<double,double>^>^ CalculateLandmarks() {
				vector<cv::Point2d> vecLandmarks = ::LandmarkDetector::CalculateLandmarks(*clnf);
				
				auto landmarks = gcnew System::Collections::Generic::List<System::Tuple<double,double>^>();
				for(cv::Point2d p : vecLandmarks) {
					landmarks->Add(gcnew System::Tuple<double,double>(p.x, p.y));
				}

				return landmarks;
			}

			System::Collections::Generic::List<System::Tuple<double, double>^>^ CalculateEyeLandmarks() {
				vector<cv::Point2d> vecLandmarks = ::LandmarkDetector::CalculateEyeLandmarks(*clnf);

				auto landmarks = gcnew System::Collections::Generic::List<System::Tuple<double, double>^>();
				for (cv::Point2d p : vecLandmarks) {
					landmarks->Add(gcnew System::Tuple<double, double>(p.x, p.y));
				}

				return landmarks;
			}

			System::Collections::Generic::List<System::Windows::Media::Media3D::Point3D>^ Calculate3DLandmarks(double fx, double fy, double cx, double cy) {
				
				cv::Mat_<double> shape3D = clnf->GetShape(fx, fy, cx, cy);
				
				auto landmarks_3D = gcnew System::Collections::Generic::List<System::Windows::Media::Media3D::Point3D>();
				
				for(int i = 0; i < shape3D.cols; ++i) 
				{
					landmarks_3D->Add(System::Windows::Media::Media3D::Point3D(shape3D.at<double>(0, i), shape3D.at<double>(1, i), shape3D.at<double>(2, i)));
				}

				return landmarks_3D;
			}


			// Static functions from the LandmarkDetector namespace.
			void DrawLandmarks(OpenCVWrappers::RawImage^ img, System::Collections::Generic::List<System::Windows::Point>^ landmarks) {

				vector<cv::Point> vecLandmarks;

				for(int i = 0; i < landmarks->Count; i++) {
					System::Windows::Point p = landmarks[i];
					vecLandmarks.push_back(cv::Point(p.X, p.Y));
				}

				::LandmarkDetector::DrawLandmarks(img->Mat, vecLandmarks);
			}


			System::Collections::Generic::List<System::Tuple<System::Windows::Point, System::Windows::Point>^>^ CalculateBox(float fx, float fy, float cx, float cy) {

				cv::Vec6d pose = ::LandmarkDetector::GetPose(*clnf, fx,fy, cx, cy);

				vector<pair<cv::Point2d, cv::Point2d>> vecLines = ::LandmarkDetector::CalculateBox(pose, fx, fy, cx, cy);

				auto lines = gcnew System::Collections::Generic::List<System::Tuple<System::Windows::Point,System::Windows::Point>^>();

				for(pair<cv::Point2d, cv::Point2d> line : vecLines) {
					lines->Add(gcnew System::Tuple<System::Windows::Point, System::Windows::Point>(System::Windows::Point(line.first.x, line.first.y), System::Windows::Point(line.second.x, line.second.y)));
				}

				return lines;
			}

			void DrawBox(System::Collections::Generic::List<System::Tuple<System::Windows::Point, System::Windows::Point>^>^ lines, OpenCVWrappers::RawImage^ image, double r, double g, double b, int thickness) {
				cv::Scalar color = cv::Scalar(r,g,b,1);

				vector<pair<cv::Point, cv::Point>> vecLines;

				for(int i = 0; i < lines->Count; i++) {
					System::Tuple<System::Windows::Point, System::Windows::Point>^ points = lines[i];
					vecLines.push_back(pair<cv::Point, cv::Point>(cv::Point(points->Item1.X, points->Item1.Y), cv::Point(points->Item2.X, points->Item2.Y)));
				}

				::LandmarkDetector::DrawBox(vecLines, image->Mat, color, thickness);
			}

			int GetNumPoints()
			{
				return clnf->pdm.NumberOfPoints();
			}

			int GetNumModes()
			{
				return clnf->pdm.NumberOfModes();
			}

			// Getting the non-rigid shape parameters describing the facial expression
			System::Collections::Generic::List<double>^ GetNonRigidParams()
			{
				auto non_rigid_params = gcnew System::Collections::Generic::List<double>();

				for (int i = 0; i < clnf->params_local.rows; ++i)
				{
					non_rigid_params->Add(clnf->params_local.at<double>(i));
				}

				return non_rigid_params;
			}

			// Getting the rigid shape parameters describing face scale rotation and translation (scale,rotx,roty,rotz,tx,ty)
			System::Collections::Generic::List<double>^ GetRigidParams()
			{
				auto rigid_params = gcnew System::Collections::Generic::List<double>();

				for (size_t i = 0; i < 6; ++i)
				{
					rigid_params->Add(clnf->params_global[i]);
				}
				return rigid_params;
			}

			// Rigid params followed by non-rigid ones
			System::Collections::Generic::List<double>^ GetParams()
			{
				auto all_params = GetRigidParams();
				all_params->AddRange(GetNonRigidParams());
				return all_params;
			}

		};

	}

}

#endif