#include <osgpcl/common.h>


#include <osgpcl/point_cloud_reader.h>
#include <osgpcl/outofcore_octree_reader.h>
#include <osgpcl/surfel.h>
#include <osgpcl/utility_point_types.h>

#include <osgViewer/Viewer>
#include <osgDB/ReadFile>

#include <iostream>
#include <pcl/console/print.h>

#include <boost/program_options.hpp>
namespace po=boost::program_options;


int main(int argc, char** argv){
  po::options_description desc("./cloud_viewer [options] input ... ");

  std::vector<std::string> infiles;

  desc.add_options()
    ("help", "produce help message")
    ("input,i",po::value<std::vector<std::string> >(&infiles)->multitoken()->required(), "input point cloud ")
    ("sampling_rate,s", po::value<float>(), "randomly subsample the input cloud")
    ("color,C",  "Render point cloud using RGB field")
    ("range,r", po::value<std::string>(), "Render point cloud using field range.  specify the field")
    ("disk,D", po::value<float>(), "Render point cloud a surfel disk of radius X")
    ("surfel,S", "Render surfel point cloud ")
    ;

  po::positional_options_description p;
  p.add("input",-1);

  po::variables_map vm;
 try{
  po::store(po::command_line_parser(argc, argv).
  options(desc).positional(p).run(), vm);
  po::notify(vm);
 }
 catch( const std::exception& e)
 {
     std::cerr << e.what() << std::endl;
     std::cout << desc << std::endl;
     return 1;
 }

 osg::ref_ptr< osgpcl::CloudReaderOptions>  options = new osgpcl::CloudReaderOptions;

 if( vm.count("sampling_rate")){
   options->setSamplingRate(vm["sampling_rate"].as<float>() );
 }
 if (vm.count("color")){
   options->setFactory( new osgpcl::PointCloudRGBFactory<pcl::PointXYZ, pcl::RGB>());
   pcl::console::print_info("Using RGB Field for Rendering...\n");
 }
 if (vm.count("range")){
	 std::string field = vm["range"].as<std::string>();

   options->setFactory( new osgpcl::PointCloudCRangeFactory<pcl::PointXYZ, pcl::PointXYZ>(field));
   pcl::console::print_info("Using %s for Rendering...\n", field.c_str());
 }
 if (vm.count("disk")){
	 float radius = vm["disk"].as<float>();
   options->setFactory( new osgpcl::SurfelFactory<pcl::PointXYZ, pcl::Normal>(radius));
   pcl::console::print_info("Using SurfelFactory %f m radius for Rendering...\n", radius);
 }
 if (vm.count("surfel")){
   options->setFactory( new osgpcl::SurfelFactoryFF<pcl::PointXYZ, pcl::Normal, osgpcl::RadiusPointT>());
   pcl::console::print_info("Using SurfelFactoryFF   for Rendering...\n");
 }



osgViewer::Viewer viewer;
viewer.setUpViewInWindow(0,0,500,500,0);

osgDB::Registry::instance()->addReaderWriter(new osgpcl::PointCloudReader);
osgDB::Registry::instance()->addReaderWriter(new osgpcl::OutofCoreOctreeReader);

osg::Group * group = new osg::Group;
for(int i=0; i<infiles.size(); i++) {
  pcl::console::print_info("Loading :  %s \n", infiles[i].c_str());
  group->addChild(osgDB::readNodeFile(infiles[i], options));
}

//Shader source for normal image rendering
const char vertex_shader_src[] ="#version 130\n"
 "varying vec3 normal;\n"
 "void main(void) {\n"
 "  gl_Position = gl_ModelViewProjectionMatrix* gl_Vertex;\n"
 "  normal = normalize(gl_NormalMatrix*gl_Normal);} \n";

 const char fragment_shader_src[] =  "#version 130\n"
     "varying vec3 normal;\n"
     "void main(void){\n"
     "\n gl_FragColor  = vec4(normal/2.0+0.5,1);}\n";
/*
 const char fragment_shader_src[] =  "#version 130\n"
      "varying vec3 normal;\n"
      "void main(void){\n"
      "float th= acos( normal[1] )/3.14;\n "
      "float del= atan(normal[2], normal[0])/6.28 +0.5;\n;"
      "int di= int(gl_FragCoord.z)*32;\n"
      "float dl=( di & 0x00ff)/255;\n"
      "float du=( ( di & 0xff00)>>8 )/255;\n"
      "\n gl_FragColor  = vec4(th  ,del , dl, du);}\n";
*/

 osg::Program* pgm = new osg::Program;
 pgm->setName( "NormalRendering" );

 pgm->addShader( new osg::Shader( osg::Shader::VERTEX,   vertex_shader_src ) );
 pgm->addShader( new osg::Shader( osg::Shader::FRAGMENT, fragment_shader_src ) );

 osg::StateSet* normal_ss_ = new osg::StateSet;
 normal_ss_->setAttribute(pgm);

 group->setStateSet( normal_ss_);

viewer.setSceneData(group);
viewer.getCamera()->setClearColor( osg::Vec4(0,0,0,1));
return viewer.run();

return 0;
}
