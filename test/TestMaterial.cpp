///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright (c) 2016 Dimo Markov <team@langulus.com>                        
/// Part of the Langulus framework, see https://langulus.com                  
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "Main.hpp"
#include <Langulus/Material.hpp>
#include <catch2/catch.hpp>


/// See https://github.com/catchorg/Catch2/blob/devel/docs/tostring.md        
CATCH_TRANSLATE_EXCEPTION(::Langulus::Exception const& ex) {
   const Text serialized {ex};
   return ::std::string {Token {serialized}};
}

/// Flow code for generating a complex material                               
constexpr auto MaterialCode = R"code(
   Nodes::Scene(
      // Create an animated rectangle, that rotates/scales/moves with time
      Box2, {
         Interpolator(Cerp), Time(.Time % 5),
         Move @0   (Yaw(180), Scale(275.5, 381), Point(400, 400)),
         Move @1   (Roll(3),  Scale(551, 762),   .MousePosition),
         Move @1.5 (Roll(3),  Scale(551, 762),   .MousePosition),
         Move @3   (          Scale(551, 762),   .MousePosition),
         Move @4   (Yaw(180), Scale(275.5, 381), Point(400, 400))
      }
   ),

   // Rasterize scene. Since no scene was provided as argument, 
   // it will seek an available Nodes::Scene in the node hierarchy
   Nodes::Raster(Bilateral),

   // Texturize the scene. Since no node was provided as argument,
   // it will seek an available Nodes::Raster in the node hierarchy
   Nodes::Texture(Front, `pebbles.png`),
   Nodes::Texture(Back,  `border.png`),
   Nodes::Texture({
      Nodes::FBM(4, {
         vec2(.Sampler.x, -(.Time * 8.75 - .Sampler.y ^ 2)) rand real
      })
   }),

   // Illuminate directionally. Since no node was provided as argument,
   // it will seek an available Nodes::Raster in the node hierarchy
   Nodes::Light(Normal3(0.5))
)code";


SCENARIO("Shader generation", "[materials]") {
   static Allocator::State memoryState;

   for (int repeat = 0; repeat != 10; ++repeat) {
      GIVEN(std::string("Init and shutdown cycle #") + std::to_string(repeat)) {
         // Create root entity                                          
         auto root = Thing::Root<false>(
            "FileSystem",
            "AssetsImages",
            "AssetsMaterials"
         );
         
         WHEN("The material is created via abstractions") {
            auto producedMaterial = root.CreateUnit<A::Material>(Code(MaterialCode));

            // Update once                                              
            root.Update({});
            root.DumpHierarchy();

            REQUIRE(producedMaterial.GetCount() == 1);
            REQUIRE(producedMaterial.CastsTo<A::Material>(1));
            REQUIRE(producedMaterial.IsSparse());
            REQUIRE(root.GetUnits().GetCount() == 1);
         }

      #if LANGULUS_FEATURE(MANAGED_REFLECTION)
         WHEN("The material is created via tokens") {
            auto producedMaterial = root.CreateUnitToken("Material", Code(MaterialCode));
            
            // Update once                                              
            root.Update({});
            root.DumpHierarchy();

            REQUIRE(producedMaterial.GetCount() == 1);
            REQUIRE(producedMaterial.CastsTo<A::Material>(1));
            REQUIRE(producedMaterial.IsSparse());
            REQUIRE(root.GetUnits().GetCount() == 1);
         }
      #endif

         // Check for memory leaks after each cycle                     
         REQUIRE(memoryState.Assert());
      }
   }
}

