///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "Camera.hpp"
#include <Math/Matrix.hpp>

using namespace Nodes;


/// Camera node descriptor-constructor                                        
///   @param desc - the camera node descriptor                                
Camera::Camera(const Neat& desc)
   : Node {MetaOf<Camera>(), desc} { }

/// Generate the camera code                                                  
///   @return the output symbol                                               
const Symbol& Camera::Generate() {
   // Generate children first                                           
   Descend();
   AddDefine("CameraResult", CameraResult);

   // Check traits in descriptor to figure out what kind of camera we   
   // are creating                                                      
   bool explicitCamera = false;
   mDescriptor.ForEachTrait([&](const Traits::View& view) {
      (void)view;

      // Projection based on camera view transformation                 
      if (mRate == PerPixel) {
         auto symView = GetSymbol<Traits::View, Mat4>(PerLevel);
         auto symFov  = GetSymbol<Traits::FOV, Real>(PerCamera);
         auto symProj = GetSymbol<Traits::Projection, Mat4>(PerLevel);
         auto symRes  = GetSymbol<Traits::Size, Vec2>(PerTick);

         // Combine pixel position with the view matrix to from         
         // the projection per pixel. This allows for optically         
         // realistic rendering (near and far planes are not flat)      
         AddDefine("Camera",
            Text::TemplateRt(CameraFuncPerPixel, *symRes, *symView, *symFov, *symProj));
         explicitCamera = true;
      }
      else if (mRate == PerVertex) {
         auto symView = GetSymbol<Traits::View, Mat4>(PerLevel);
         auto symPos  = GetSymbol<Traits::Place, Vec4>(PerVertex);

         // Combine vertex position with the view matrix to from        
         // the projection per vertex                                   
         AddDefine("Camera",
            Text::TemplateRt(CameraFuncPerVertex, *symView, *symPos));
         explicitCamera = true;
      }
      else TODO();
   });

   if (not explicitCamera) {
      // By default it simply projects 2D based on the pixel position   
      Logger::Warning("No explicit camera defined - using default 2D screen projection");
      mRate = PerPixel;
      auto symRes = GetSymbol<Traits::Size, Vec2>(PerTick);
      AddDefine("Camera", Text::TemplateRt(CameraFuncDefault, *symRes));
   }

   // Expose the results to the rest of the nodes                       
   return ExposeData<Camera>("Camera()");

   /*Expose<Traits::Place, Vec2>("camResult.mFragment");
   Expose<Traits::Sampler, Vec2>("camResult.mScreenUV");
   Expose<Traits::Place, Vec3>("camResult.mOrigin");
   Expose<Traits::Projection, Mat4>("camResult.mProjectedView");
   Expose<Traits::Aim, Vec3>("camResult.mDirection");*/
}
