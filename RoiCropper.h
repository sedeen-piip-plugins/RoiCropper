/*=============================================================================
 *
 *  Copyright (c) 2019 Sunnybrook Research Institute
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 *
 *=============================================================================*/

#ifndef SEDEEN_SRC_PLUGINS_ROICROPPER_ROICROPPER_H
#define SEDEEN_SRC_PLUGINS_ROICROPPER_ROICROPPER_H

// System headers
#include <memory>
#include <QtGui/qimage.h>
#include <Windows.h>

// DPTK headers - a minimal set 
#include "algorithm/AlgorithmBase.h"
#include "algorithm/Parameters.h"
#include "algorithm/Results.h"
#include "algorithm/RegionListParameter.h"
#include "archive/Session.h"

namespace sedeen {
class Point;
namespace image {
class RawImage;

} //namespace image

namespace algorithm {

/// Algorithm for cropping the delineated region of interest
//
/// This algorithm crops an image with the size of bounding box arround the  
///	selected delineated region and saves the regin in a jpeg image.
///
class RoiCropper: public AlgorithmBase {
public:
	RoiCropper();
	virtual ~RoiCropper();
private:
	/// TRUE if the pipeline has changed since the call to this function, FALSE
	///  otherwise
	virtual void run();
	virtual void init(const image::ImageHandle& image);
	void cropBoundingBox(image::RawImage update_image, const Size& output_size);
	bool buildPipeline();
	std::string generateReport() const;
	void updateIntermediateResult();
	std::string openFile(std::string path);

	/// User region of interest
	algorithm::GraphicItemParameter m_region;
	ImageResult m_intermediate_result;
	Size m_size;
	Rect m_boundingBox;

	/// The output result
	algorithm::TextResult m_output_text;
	std::vector<PointF> m_pointsInRegion;
	std::string m_path_to_root;
	std::string m_path_to_image;
	std::string m_roi_file_name;
	std::string m_roi;
	int red,green,blue; //not used yet
	std::vector<QRgb> m_colorsInRegion; //not used yet
	struct Info{
		int red;
		int green;
		int blue;
		std::string region;
		std::string description;
		std::string type;
		std::vector<PointF> points;
		GraphicStyle style;
	};
	std::vector<Info> m_sub_regions;
};

} //namespace algorithm
} //namespace sedeen

#endif // ifndef SEDEEN_SRC_PLUGINS_ROICROPPER_ROICROPPER_H