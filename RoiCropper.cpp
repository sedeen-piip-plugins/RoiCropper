// Primary header
#include "RoiCropper.h"

// System headers
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <vector>
#include <QtGui/qimage.h>
#include <fstream>

//DPTK headres
#include "Algorithm.h"
#include "Geometry.h"
#include "Global.h"
#include "Image.h"
#include "image/tile/Compositor.h"
#include "image/formats/tiff/TIFFWriter.h"
#include "algorithm/parameter/system/DisplayArea.h"
#include "archive/Session.h"
#include "global/geometry/PointF.h"

// Poco header needed for the macros below 
#include <Poco/ClassLibrary.h>

// Declare that this object has AlgorithmBase subclasses
//  and declare each of those sub-classes
POCO_BEGIN_MANIFEST(sedeen::algorithm::AlgorithmBase)
POCO_EXPORT_CLASS(sedeen::algorithm::RoiCropper)
POCO_END_MANIFEST

namespace sedeen {
namespace algorithm {

RoiCropper::RoiCropper() 
{
}

RoiCropper::~RoiCropper() 
{
}

void RoiCropper::init(const image::ImageHandle& image) 
{
	if (image::isNull(image)) return;
	m_region = createGraphicItemParameter(*this,
										"Region of Interest",	// Widget label
										"Region to be cropped",
										true);					// Widget tooltip
	m_intermediate_result = createImageResult(*this, "ROI Image");

	m_output_text = createTextResult(*this, "text Result");
}

std::string RoiCropper::generateReport() const
{
	std::ostringstream ss;
	ss << std::left << std::setfill(' ') << std::setw(10);
	ss <<"Width " << "Height"<< std::endl;
	ss<<std::left<<std::setfill(' ')<<std::setw(10);
	ss<<m_size.width()<<std::fixed<<std::setprecision(1)<<m_size.height();
	ss<<std::endl;
	ss << std::left << std::setfill(' ') << std::setw(10);
	ss<<"xmin"<<"xmax"<<std::endl;
	ss<<std::left<<std::setfill(' ')<<std::setw(10);
	ss<<m_boundingBox.x()<<std::fixed<<std::setprecision(1)<<xMax(m_boundingBox);
	ss<<std::endl;
	ss << std::left << std::setfill(' ') << std::setw(10);
	ss<<"ymin"<<"ymax"<<std::endl;
	ss<<std::left<<std::setfill(' ')<<std::setw(10);
	ss<<m_boundingBox.y()<<std::fixed<<std::setprecision(1)<<yMax(m_boundingBox);	
	ss<<std::endl;
	return ss.str();
}
std::string RoiCropper::openFile(std::string path)
{
	OPENFILENAME ofn;
	char szFileName[MAX_PATH]="";
	//WCHAR szFileName[MAX_PATH]= L"";
	const std::string temp_str = m_path_to_root.substr(m_path_to_root.find_last_of("/\\") + 1);
	auto p = temp_str.find_last_of('.');
	std::string base_name;
	p >0 && p != std::string::npos ? base_name = temp_str.substr(0, temp_str.rfind(".")) : base_name = temp_str;
	std::copy( std::begin(base_name), std::begin(base_name) + std::min(base_name.size(), sizeof(szFileName)), 
				std::begin(szFileName));
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn); 
	ofn.hwndOwner = NULL;
	ofn.lpstrFilter = "*.tif";
	ofn.lpstrFile = (LPSTR)szFileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	//ofn.lpstrDefExt = (LPSTR)L"tif";
	ofn.lpstrInitialDir = (LPSTR) m_path_to_root.c_str();
	GetSaveFileName(&ofn);

	return ofn.lpstrFile;
}
bool RoiCropper::buildPipeline() 
{
	using namespace image::tile;
	bool pipelineChanged = false;
	if (m_region.isUserDefined()) 
	{
		// Cropping selected ROI
		std::shared_ptr<GraphicItemBase> region = m_region;
		m_boundingBox = containingRect(region->graphic());
		auto xmin = m_boundingBox.x();
		auto ymin = m_boundingBox.y();
		auto xmax = xMax(m_boundingBox);
		auto ymax = yMax(m_boundingBox);
		m_size.setWidth(xmax-xmin);
		m_size.setHeight(ymax-ymin);
		// Reading xml file
		std::string path_to_image = 
			image()->getMetaData()->get(image::StringTags::SOURCE_DESCRIPTION,0);
		Session s(path_to_image);

		auto b = s.loadFromFile();
		auto p = s.imagePath();
		auto graphics = s.getGraphics();
		//Session roi;
		//roi.loadFromFile("C:/Users/Shiva/Documents/images/sedeen/99797cropped.session.xml");
		//roi.setPixelSize(s.getPixelSize());
		//auto trans = s.getTransform();
		//PointF center;
		//center.setX(m_size.width()/2);
		//center.setY(m_size.height()/2);
		//trans.setCenter(center);
		//roi.setTransform(trans);
		//roi.setDimension(m_size);
		std::vector<GraphicDescription> roi_graphics;
		bool flag = false;
		for(int i = 0; i<graphics.size(); i++)
		{
			auto points = graphics[i].getPoints();
			auto rgb = graphics[i].getStyle().pen().color(); 
			flag = false;
			for(int j = 0; j<points[0].size();j++)
			{

				if ((static_cast<int>(points[0][j].getX())==xmin && static_cast<int>(points[0][j].getY())==ymin)
					|| (static_cast<int>(points[0][j].getX())==xmin && static_cast<int>(points[0][j].getY())==ymax)
					|| (static_cast<int>(points[0][j].getX())==xmax && static_cast<int>(points[0][j].getY())==ymin)
					|| (static_cast<int>(points[0][j].getX())==xmax && static_cast<int>(points[0][j].getY())==ymax))
				{
					m_roi = graphics[i].getName();
				}
				else if (points[0][j].getX()>xmin && points[0][j].getX()<xmax &&
					points[0][j].getY()>ymin && points[0][j].getY()<ymax)
				{
					flag = true;
					points[0][j].setX(points[0][j].getX()-xmin);
					points[0][j].setY(points[0][j].getY()-ymin);
					m_pointsInRegion.push_back(points[0][j]);
					m_colorsInRegion.push_back(qRgb(rgb.red(),rgb.green(),rgb.blue())); 
					red = rgb.red();
					green = rgb.green();
					blue = rgb.blue();
				}
			}
			if (flag)
			{
				GraphicDescription region;
				region.setStyle(graphics[i].getStyle());
				region.setDescription(graphics[i].getDescription());
				region.setGeometry(graphics[i].getGeometry());
				region.setName(graphics[i].getName());
				region.setPoints(points);
				roi_graphics.push_back(region);
				Info roi_info;
				roi_info.red = red;
				roi_info.green = green;
				roi_info.blue = blue;
				roi_info.region = graphics[i].getName();
				roi_info.style = graphics[i].getStyle();
				roi_info.description = graphics[i].getDescription();
				roi_info.type = graphics[i].getGeometry();
				auto points = graphics[i].getPoints();
				std::copy(points[0].begin(),points[0].end(),back_inserter(roi_info.points));
				m_sub_regions.push_back(roi_info);
			}

		}
		//roi.setGraphics(roi_graphics);
		// Creating mask based on the corresponding annotations within selected ROI
		if (m_pointsInRegion.size()!=0)
		{
			Size size;
			size.setWidth(static_cast<int>(xmax-xmin));
			size.setHeight(static_cast<int>(ymax-ymin));
			QImage mask(size.width(),size.height(),QImage::Format_RGB888);
			mask.fill(qRgb(255,255,255));

			std::vector<PointF> pointsInRegion;
			std::copy(m_pointsInRegion.begin(),m_pointsInRegion.end(),back_inserter(pointsInRegion));
			while(!pointsInRegion.empty())
			{
				auto points = pointsInRegion.back();
				auto colors = m_colorsInRegion.back(); 
				mask.setPixel(static_cast<int>(points.getX()),
							static_cast<int>(points.getY()),
							colors/*qRgb(255,255,255)*/);
				pointsInRegion.pop_back();
				m_colorsInRegion.pop_back(); 
			}
			auto found = path_to_image.find_last_of(".");
			m_path_to_root = path_to_image.substr(0,found);
			m_roi_file_name = openFile(m_path_to_root);
			if (m_sub_regions.size()>0)
			{
				std::reverse(m_sub_regions.begin(),m_sub_regions.end());
				if (m_roi.empty())
				{
					m_roi = m_sub_regions.back().region;
				}
			}
			double x_cord = (double)m_boundingBox.x() + 0.5* (double)m_boundingBox.width();
			double y_cord = (double)m_boundingBox.y() + 0.5* (double)m_boundingBox.height();
			mask.save(QString::fromStdString(m_roi_file_name+"_"+m_roi+ "_" +
					std::to_string((int)(x_cord))+ "_" + std::to_string((int)(y_cord)) + "_mask.tif"));
		}
		pipelineChanged = true;
		//roi.saveToFile();
	}
	return pipelineChanged;
}

void RoiCropper::run()
{
	using namespace image::tile;

	auto pipeline_changed = buildPipeline();
	if (pipeline_changed)
	{
		updateIntermediateResult();
		auto source_factory = image()->getFactory();
		auto compositor = 
			std::unique_ptr<Compositor>(new Compositor(source_factory));
		image::RawImage raw_image = compositor->getImage(m_boundingBox, m_size);

		//Save the ROI
		double x_cord = (double)m_boundingBox.x() + 0.5* (double)m_boundingBox.width();
		double y_cord = (double)m_boundingBox.y() + 0.5* (double)m_boundingBox.height();
		raw_image.save(m_roi_file_name+"_"+m_roi+"_" + std::to_string((int)(x_cord))+
						"_"+ std::to_string((int)(y_cord)) +"_crop.tif");


		std::ofstream txtfile;
		txtfile.open(m_roi_file_name+"_"+m_roi+"_" + std::to_string((int)(x_cord))+
						"_"+ std::to_string((int)(y_cord)) +"_key.xml");
		txtfile<<"<?xml version=\"1.0\"?>\n";
		txtfile<<"<session software=\"PathCore Session Printer\" version=\"0.1.0\">\n";
		txtfile<<"    <image identifier=\""<<m_roi_file_name+"_"+m_roi+ std::to_string((int)(x_cord))+
						"_"+ std::to_string((int)(y_cord)) +"_crop.tif\">\n";
		//txtfile<<image()->getMetaData()->get(image::StringTags::SOURCE_DESCRIPTION,0)+"\n";
		txtfile<<"        <overlays>\n";
		while(!m_sub_regions.empty())
		{
			auto region_info = m_sub_regions.back();
			txtfile<<"            <graphic type=\""
				<<region_info.type<<"\" name=\"";
			txtfile<<region_info.region<<"\" description=\""
				<<region_info.description<<"\">\n";
			txtfile<<"                <pen color=\""
				<<colorToString(region_info.style.pen().color())
				<<"\" width=\""<<region_info.style.pen().width()
				<<"\" style=\""
				<<styleToString(region_info.style.pen().style())
				<<"\"/>\n";
			txtfile<<"                <font>"<<toString(region_info.style.font())<<"</font>\n";
			txtfile<<"                <point-list>\n";
			for (int i = 0; i<region_info.points.size(); i++)
			if (region_info.points[i].getX()>m_boundingBox.x() && 
				region_info.points[i].getX()<xMax(m_boundingBox) &&
				region_info.points[i].getY()>m_boundingBox.y() && 
				region_info.points[i].getY()<yMax(m_boundingBox))
			{
				txtfile<<"                    <point>"
					<<static_cast<int>(region_info.points[i].getX())-m_boundingBox.x()
					<<","
					<<static_cast<int>(region_info.points[i].getY())-m_boundingBox.y()
					<<"</point>\n";
			}
			/*txtfile<<region_info.region<<", ("<<region_info.red<<","
				<<region_info.green<<","<<region_info.blue<<"), "<<
				region_info.description<<"\n";*/
			m_sub_regions.pop_back();
			txtfile<<"                </point-list>\n";
			txtfile<<"            </graphic>\n";
		}
		txtfile<<"        </overlays>\n";
		txtfile<<"</session>";
		txtfile.close();
		m_roi.clear();
	}
	auto report = generateReport();
	m_output_text.sendText(report);
}

void RoiCropper::updateIntermediateResult()
{
	auto update_result = [&](const std::shared_ptr<image::tile::Factory> &factory) 
	{
		// Create a compositor
		auto compositor = std::unique_ptr<image::tile::Compositor>
			(new image::tile::Compositor(factory));

		DisplayRegion region = DisplayRegion(m_boundingBox,m_size);
		auto outputImage = 
			compositor->getImage(region.source_region, region.output_size);

		// Update UI
		m_intermediate_result.update(outputImage, region.source_region);
	};

}
} // namespace algorithm
} // namespace sedeen