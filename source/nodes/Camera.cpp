///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright (c) 2016 Dimo Markov <team@langulus.com>                        
/// Part of the Langulus framework, see https://langulus.com                  
///                                                                           
/// SPDX-License-Identifier: GPL-3.0-or-later                                 
///                                                                           
#include "Camera.hpp"
#include <Langulus/Graphics.hpp>
#include <Math/Matrix.hpp>

using namespace Nodes;


/// Camera node descriptor-constructor                                        
///   @param desc - the camera node descriptor                                
Camera::Camera(Describe descriptor)
   : Resolvable {this}
   , Node       {*descriptor} { }

/// Generate the camera code                                                  
///   @return the output symbol                                               
auto Camera::Generate() -> const Symbol& {
   // Generate children first                                           
   Descend();
   AddDefine("CameraResult", CameraResult);

   // Check traits in descriptor to figure out what kind of camera we   
   // are creating                                                      
   bool explicitCamera = false;
   mDescriptor.ForEachDeep([&](const Traits::View& view) {
      (void) view;

      // Projection based on camera view transformation                 
      if (mRate == Rate::Pixel) {
         auto symView = GetSymbol<Traits::View,       Mat4>(Rate::Level);
         auto symFov  = GetSymbol<Traits::FOV,        Real>(Rate::Camera);
         auto symProj = GetSymbol<Traits::Projection, Mat4>(Rate::Level);
         auto symRes  = GetSymbol<Traits::Size,       Vec2>(Rate::Tick);

         // Combine pixel position with the view matrix to from         
         // the projection per pixel. This allows for optically         
         // realistic rendering (near and far planes are not flat)      
         AddDefine("Camera",
            Text::TemplateRt(CameraFuncPerPixel, *symRes, *symView, *symFov, *symProj));
         explicitCamera = true;
      }
      else if (mRate == Rate::Vertex) {
         auto symView = GetSymbol<Traits::View,  Mat4>(Rate::Level);
         auto symPos  = GetSymbol<Traits::Place, Vec4>(Rate::Vertex);

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
      mRate = Rate::Pixel;
      auto symRes = GetSymbol<Traits::Size, Vec2>(Rate::Tick);
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
